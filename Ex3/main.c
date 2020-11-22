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
#include "fs/operations.h"

#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100

/* Variables that store the arguments from the server call. */
int numberThreads;
FILE * inputFile;
FILE * outputFile;

/* Variables needed for the program. */
char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numCommands, produceIndex, consumeIndex, eof;

/* Mutexes. */
pthread_mutex_t lock_commands, lock_numCommands, lock_consumeIndex;

/* Conditional variables. */
pthread_cond_t consume, produce;

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
    if (argc != 4) {
        fprintf(stderr, "Error: invalid number of arguments\n");
        exit(EXIT_FAILURE);
    }

    /* Opens input file. */
    if (!(inputFile = fopen(argv[1], "r"))) {
        fprintf(stderr, "Error: couldn't open input file\n");
        exit(EXIT_FAILURE);
    }

    /* Opens output file. */
    if (!(outputFile = fopen(argv[2], "w"))) {
        fprintf(stderr, "Error: couldn't open output file\n");
        exit(EXIT_FAILURE);
    }
    
    /* Verifies number of threads */
    if ((numberThreads = atoi(argv[3])) < 1) {
        fprintf(stderr, "Error: invalid number of threads\n");
        exit(EXIT_FAILURE);
    }
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

/* Processes the input and inserts all the possible valid commands in 
 * inputCommands.
 */
void processInput() {
    /* Shared variables dont need protection since threads will not be involved. */
    char line[MAX_INPUT_SIZE];

    /* break loop with ^Z or ^D. */
    while (fgets(line, sizeof(line)/sizeof(char), inputFile)) {
        /* Conditional variable */
        lockCommands(&lock_commands);
        while(numCommandsEquals(MAX_COMMANDS)) {
            if (pthread_cond_wait(&produce, &lock_commands) != 0) {
                fprintf(stderr, "Error: conditional variable wait failed\n");
                exit(EXIT_FAILURE);
            }
        }
        
        char token, type;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        /* perform minimal validation. */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;

            case 'm':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case '#':
                break;
            
            default:{ /* error. */
                errorParse();
            }
        }

        /* Conditional variable signal */
        if (pthread_cond_signal(&consume) != 0) {
            fprintf(stderr, "Error: conditional variable signal failed\n");
            exit(EXIT_FAILURE);
        }
        unlockCommands(&lock_commands);
    }

    eof = 1;
    if (pthread_cond_broadcast(&consume) != 0) {
        fprintf(stderr, "Error: conditional variable broadcast failed\n");
        exit(EXIT_FAILURE);
    }

    fclose(inputFile);
}

/* Applies the commands in inputCommands.
 * Input:
 *  - ptr: pointer to arguments (needed because this function is used by threads).
 * Returns:
 *  - Pointer to results (needed because this function is used by threads).
 */
void * applyCommands(void * ptr) {
    while (!eof || !numCommandsEquals(0)) {
        /* Conditional variable */
        lockCommands(&lock_commands);
        while (!eof && numCommandsEquals(0)) {
            if (pthread_cond_wait(&consume, &lock_commands) != 0) {
                fprintf(stderr, "Error: conditional variable wait failed\n");
                exit(EXIT_FAILURE);
            }
        }
        
        /* No more commands to be applied */
        if (eof && numCommandsEquals(0)) {
            unlockCommands(&lock_commands);
            break;
        }
        
        char* command = removeCommand();
        /* Dynamically allocated memory for command */

        /* Conditional variable signal */
        if (pthread_cond_signal(&produce) != 0) {
            fprintf(stderr, "Error: conditional variable signal failed\n");
            exit(EXIT_FAILURE);
        }
        unlockCommands(&lock_commands);
        
        if (command == NULL) {
            unlockCommands(&lock_commands);
            continue;
        }
        
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
                        create(name, T_FILE);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
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
                delete(name);
                break;
            case 'm':            
                printf("Move: %s To: %s\n", name, thirdArg);
                while ((commandResult = move(name, thirdArg)) == GIVEUP);
                if(commandResult == SUCCESS)
                    printf("Move: %s To: %s successful.\n", name, thirdArg);
                else
                    printf("Move: %s To: %s failed.\n", name, thirdArg);
                break;
            default:{ /* error. */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    return NULL;
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
    
    /* Conditional Variables */
    if (pthread_cond_init(&consume, NULL) != 0) {
        fprintf(stderr, "Error: mutex lock initialization failed\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_init(&produce, NULL) != 0) {
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

    /* Conditional Variables */
    if (pthread_cond_destroy(&consume) != 0) {
        fprintf(stderr, "Error: mutex lock destruction failed\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_destroy(&produce) != 0) {
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
        if (pthread_create(&tid[i], NULL, applyCommands, NULL) != 0) {
            fprintf(stderr, "Error: couldnt create thread\n");
            exit(EXIT_FAILURE);
        }
    }
    
    /* Processes input. */
    processInput();

    for (i = 0; i < numberThreads; i++) {
        if (pthread_join(tid[i], NULL) != 0) {
            fprintf(stderr, "Error: couldnt join threads\n");
            exit(EXIT_FAILURE);
        }
    }

    destroySynch();
}

/* Main.
 */
int main(int argc, char* argv[]) {
    struct timeval tick, tock;
    double time;

    /* Initializes global variables. */
    numCommands = 0;
    produceIndex = 0;
    consumeIndex = 0;
    eof = 0;

    /* Initializes filesystem. */
    init_fs();

    /* Processes argurments from server call. */
    processArgs(argc, argv);

    /* Starts counting the time. */
    gettimeofday(&tick, NULL);

    

    parallelization();

    /* End of counting the time and print of the result. */
    gettimeofday(&tock, NULL);
    time = (tock.tv_sec - tick.tv_sec); 
    time = ((time * 1e6) + (tock.tv_usec - tick.tv_usec)) * 1e-6;
    printf("TecnicoFS completed in %.4f seconds.\n", time);

    /* Prints tree to the output file. */
    print_tecnicofs_tree(outputFile);
    fclose(outputFile);

    /* Releases allocated memory. */
    destroy_fs();
    exit(EXIT_SUCCESS);
}
