/* 
 * (Ficheiro alterado)
 * Grupo 27:
 * 93230 Catarina Bento
 * 94179 Luis Freire D'Andrade
*/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/stat.h>
#include "fs/operations.h"

#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100

/* Sockets */
#define INDIM 30
#define OUTDIM 512

/* Variables that store the arguments from the server call. */
int numberThreads;
FILE * inputFile;
FILE * outputFile;

/* Variables needed for the program. */
char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numCommands, produceIndex, consumeIndex, eof;

/* Server socket. */
char *socketPath;
int sockfd;
struct sockaddr_un server_addr;
socklen_t addrlen;

/* Mutexes. */
pthread_mutex_t lock_commands, lock_numCommands, lock_consumeIndex;

/* Locks the respective lock.
 */
void lockCommands(pthread_mutex_t * lock) {
    if (pthread_mutex_lock(lock) != 0) {
		fprintf(stderr, "Error: readwrite lock failed\n");
		exit(EXIT_FAILURE);
    }
}

/* Unlocks the respective lock.
 */
void unlockCommands(pthread_mutex_t * lock) {
    if (pthread_mutex_unlock(lock) != 0) {
		fprintf(stderr, "Error: readwrite lock failed\n");
		exit(EXIT_FAILURE);
    }
}

/* Processes the arguments from server call.
 * Input:
 *  - argc: number of arguments.
 *  - argv: the arguments.
 */
void processArgs(int argc, char * argv[]) {
    /* Verifies number of arguments. */
    if (argc != 3) {
        fprintf(stderr, "Error: invalid number of arguments\n");
        exit(EXIT_FAILURE);
    }
    
    /* Verifies number of threads */
    if ((numberThreads = atoi(argv[1])) < 1) {
        fprintf(stderr, "Error: invalid number of threads\n");
        exit(EXIT_FAILURE);
    }

    /* Define the socket's path */
    socketPath = strdup(argv[2]);
}

/* Inserts a valid command in inputCommands, and returns if it was successful.
 * Input:
 *  - data: line from the input file.
 * Returns:
 *  - Integer that represents the success of the function call.
 */
int insertCommand(char* data) {
    lockCommands(&lock_numCommands);

    strcpy(inputCommands[produceIndex], data);
    produceIndex = (produceIndex + 1) % MAX_COMMANDS;
    numCommands++;

    unlockCommands(&lock_numCommands);

    return 1;
}

/* Returns a boolean that represents if the global variable numberCommands is 
 * equal to a number. 
 * Input:
 *  - comparee: the number comparing to.
 */
int numCommandsEquals(int comparee) {
    lockCommands(&lock_numCommands);
    int result = (numCommands == comparee);
    unlockCommands(&lock_numCommands);

    return result;
}

/* Removes a command from inputCommands.
 * Returns:x«    
 *  - The command that was removed.
 */
char* removeCommand() {
    if (numCommandsEquals(0)) {
        return NULL;
    }

    lockCommands(&lock_numCommands);
    lockCommands(&lock_consumeIndex);

    int head = consumeIndex;
    consumeIndex = (consumeIndex + 1) % MAX_COMMANDS;
    numCommands--;
    /* Copy of the command */
    char *command = (char *) malloc(sizeof(char) * (strlen(inputCommands[head]) + 1));
    strcpy(command, inputCommands[head]);

    unlockCommands(&lock_numCommands);
    unlockCommands(&lock_consumeIndex);

    return command;
}

/* Throws an error when a command is invalid.
 */
void errorParse() {
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

/* Applies the commands in inputCommands.
 * Input:
 *  - ptr: pointer to arguments (needed because this function is used by threads).
 * Returns:
 *  - Pointer to results (needed because this function is used by threads).
 */
int applyCommands(char * command) {
    char token;
    char name[MAX_INPUT_SIZE], thirdArg[MAX_INPUT_SIZE];
    int numTokens = sscanf(command, "%c %s %s", &token, name, thirdArg);
    free(command);
    
    if (numTokens < 2) {
        fprintf(stderr, "Error: invalid command in Queue\n");
        exit(EXIT_FAILURE);
    }

    int commandResult;
    switch (token) {
        case 'c':
            switch (thirdArg[0]) {
                case 'f':
                    printf("Create file: %s\n", name);
                    commandResult = create(name, T_FILE);
                    break;
                case 'd':
                    printf("Create directory: %s\n", name);
                    commandResult = create(name, T_DIRECTORY);
                    break;
                default:
                    fprintf(stderr, "Error: invalid node type\n");
                    exit(EXIT_FAILURE);
            }
            break;
        case 'l':
            commandResult = lookup(name);
            if (commandResult >= 0)
                printf("Search: %s found\n", name);
            else
                printf("Search: %s not found\n", name);
            break;
        case 'd':
            printf("Delete: %s\n", name);
            commandResult = delete(name);
            break;
        case 'm':            
            printf("Move: %s To: %s\n", name, thirdArg);
            while ((commandResult = move(name, thirdArg)) == GIVEUP);
            if (commandResult == SUCCESS)
                printf("Move: %s To: %s successful.\n", name, thirdArg);
            else
                printf("Move: %s To: %s failed.\n", name, thirdArg);
            break;
        default:{ /* error. */
            fprintf(stderr, "Error: command to apply\n");
            exit(EXIT_FAILURE);
        }
    }
    return commandResult;
}

/* Initializes all the locks and conditional variables needed.
 */
void initSynch() {
    /* command's lock. */
    if (pthread_mutex_init(&lock_commands, NULL) != 0) {
        fprintf(stderr, "Error: mutex lock initialization failed\n");
        exit(EXIT_FAILURE);
    }
    /* numCommand's lock. */
    if (pthread_mutex_init(&lock_numCommands, NULL) != 0) {
        fprintf(stderr, "Error: mutex lock initialization failed\n");
        exit(EXIT_FAILURE);
    }
    /* consumeIndex's lock. */
    if (pthread_mutex_init(&lock_consumeIndex, NULL) != 0) {
        fprintf(stderr, "Error: mutex lock initialization failed\n");
        exit(EXIT_FAILURE);
    }
}

/* Destroys all the locks and conditional variables created.
 */
void destroySynch() {
    /* Commands lock. */
    if (pthread_mutex_destroy(&lock_commands)!= 0) {
        fprintf(stderr, "Error: mutex lock destruction failed\n");
        exit(EXIT_FAILURE);  
    }
    /* numCommand's lock. */
    if (pthread_mutex_destroy(&lock_numCommands) != 0) {
        fprintf(stderr, "Error: mutex lock destruction failed\n");
        exit(EXIT_FAILURE);
    }
    /* consumeIndex's lock. */
    if (pthread_mutex_destroy(&lock_consumeIndex) != 0) {
        fprintf(stderr, "Error: mutex lock destruction failed\n");
        exit(EXIT_FAILURE);
    }

}

/* Initializes the locks and condition variablrs, and creates the threads. Then, 
 * it waits for the threads to join and destroys the locks.
 */
void parallelization() {
    int i;
    pthread_t tid[numberThreads];

    initSynch();

    for (i = 0; i < numberThreads; i++) {
        if (pthread_create(&tid[i], NULL, receiveInput, NULL) != 0) {
            fprintf(stderr, "Error: couldnt create thread\n");
            exit(EXIT_FAILURE);
        }
    }
    
    for (i = 0; i < numberThreads; i++) {
        if (pthread_join(tid[i], NULL) != 0) {
            fprintf(stderr, "Error: couldnt join threads\n");
            exit(EXIT_FAILURE);
        }
    }

    destroySynch();
}

/* Sets up socket.
 * Input:
 *  - path: path of the socket.
 *  - addr: socket address.
 * Returns:
 *  - Lenght of the socket address.
 */
int setSockAddrUn(char *path, struct sockaddr_un *addr) {
    if (addr == NULL)
        return 0;

    bzero((char *)addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, path);

    return SUN_LEN(addr);
}

void * receiveInput(void *args){
    while (1) {
        struct sockaddr_un client_addr;
        char in_buffer[INDIM], out_buffer[OUTDIM];
        int c, commandResult;

        addrlen = sizeof(struct sockaddr_un);
        c = recvfrom(sockfd, in_buffer, sizeof(in_buffer)-1, 0,
            (struct sockaddr *)&client_addr, &addrlen);
        if (c <= 0) continue;
        //Preventivo, caso o cliente nao tenha terminado a mensagem em '\0', 
        in_buffer[c]='\0';
        
        printf("Recebeu mensagem de %s: %s.\n", client_addr.sun_path, in_buffer);

        commandResult = applyCommands(in_buffer);
    
        sendto(sockfd, &commandResult, sizeof(int), 0, (struct sockaddr *)&client_addr, addrlen);
    }
    return NULL;
}

/* Initializes the sockets, and works with them.
 */
void socketInit() {
    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("server: can't open socket");
        exit(EXIT_FAILURE);
    }

    unlink(socketPath);

    addrlen = setSockAddrUn(socketPath, &server_addr);
    if (bind(sockfd, (struct sockaddr *) &server_addr, addrlen) < 0) {
        perror("server: bind error");
        exit(EXIT_FAILURE);
    }
}


/* Main.
 */
int main(int argc, char* argv[]) {;
    /* Initializes filesystem. */
    init_fs();

    /* Processes argurments from server call. */
    processArgs(argc, argv);

    /* Initializes the*/
    socketInit();

    parallelization();

    /* Prints tree to the output file. */
    print_tecnicofs_tree(outputFile);
    fclose(outputFile);

    /* Fechar e apagar o nome do socket, apesar deste programa nunca chegar a este ponto */
    close(sockfd);
    unlink(socketPath);
    free(socketPath);

    /* Releases allocated memory. */
    destroy_fs(); 
    exit(EXIT_SUCCESS);
}
