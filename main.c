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

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

/* Variables that store the arguments from the server call. */
int numberThreads;
FILE * inputFile;
FILE * outputFile;
pthread_mutex_t lockinho;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

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
    /* Shared variables dont need protection since threads will not be involved. */
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

/* Returns number of Commands while protecting with a lock the global variable numberCommands.
 */
int getNumberCommands() {
    pthread_mutex_lock(&lockinho);
    int nCommands = numberCommands;
    pthread_mutex_unlock(&lockinho);
    return nCommands;
}

/* Removes a command from inputCommands.
 * Returns:
 *  - The command that was removed.
 */
char* removeCommand() {
    pthread_mutex_lock(&lockinho);
    if (numberCommands > 0) {
        numberCommands--;
        int head = headQueue;
        headQueue++;
        pthread_mutex_unlock(&lockinho);
        return inputCommands[head];  
    }
    pthread_mutex_unlock(&lockinho);
    return NULL;
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
            
            case '#':
                break;
            
            default:{ /* error. */
                errorParse();
            }
        }
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
    while (getNumberCommands() > 0) {
        const char* command = removeCommand();
        if (command == NULL) {
            continue;
        }

        char token, type;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        switch (token) {
            case 'c':
                switch (type) {
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
                searchResult = lookup(name);
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                break;
            case 'd':
                printf("Delete: %s\n", name);
                delete(name);
                break;
            default:{ /* error. */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    return NULL;
}

/* Initializes the locks and creates the threads. Then, it waits for the threads 
 * to join and destroys the locks.
 */
void parallelization() {
    int i;
    pthread_t tid[numberThreads];

    /* Creation of the threads. */
    for (i = 0; i < numberThreads; i++) {
        if (pthread_create(&tid[i], NULL, applyCommands, NULL) != 0) {
            fprintf(stderr, "Error: couldnt create thread\n");
            exit(EXIT_FAILURE);
        }
    }

    /* Waits for the threads to finish. */
    for (i = 0; i < numberThreads; i++) {
        if (pthread_join(tid[i], NULL) != 0) {
            fprintf(stderr, "Error: couldnt join threads\n");
            exit(EXIT_FAILURE);
        }
    }
}

/* Main.
 */
int main(int argc, char* argv[]) {
    printf("mutex init\n");
    struct timeval tick, tock;
    double time;
    printf("mutex init\n");
    pthread_mutex_init(&lockinho, NULL);
    printf("done\n");

    /* Initializes filesystem. */
    init_fs();

    /* Processes argurments from server call. */
    processArgs(argc, argv);

    /* Processes input. */
    processInput();

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
    pthread_mutex_destroy(&lockinho);
    exit(EXIT_SUCCESS);
}
