/* 
 * (Ficheiro alterado)
 * Grupo 27:
 * 93230 Catarina Bento
 * 94179 Luis Freire D'Andrade
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tecnicofs-client-api.h"
#include "../tecnicofs-api-constants.h"

FILE* inputFile;
char* serverName;

static void displayUsage (const char* appName) {
    printf("Usage: %s inputfile server_socket_name\n", appName);
    exit(EXIT_FAILURE);
}

static void parseArgs (long argc, char* const argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage(argv[0]);
    }

    if ((inputFile = fopen(argv[1], "r"))== NULL) {
        fprintf(stderr, "Error: cannot open input file\n");
        exit(EXIT_FAILURE);
    }

    serverName = strdup(argv[2]);

}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void *processInput() {
    char line[MAX_INPUT_SIZE];

    while (fgets(line, sizeof(line)/sizeof(char), inputFile)) {
        char op;
        char arg1[MAX_INPUT_SIZE], arg2[MAX_INPUT_SIZE];
        int res;

        int numTokens = sscanf(line, "%c %s %s", &op, arg1, arg2);

        if (numTokens < 1) {
            continue;
        }
        switch (op) {
            case 'c':
                if(numTokens != 3) {
                    errorParse();
                    break;
                }
                switch (arg2[0]) {
                    case 'f':
                        res = tfsCreate(arg1, 'f');
                        if (!res)
                          printf("Created file: %s\n", arg1);
                        else
                          printf("Unable to create file: %s\n", arg1);
                        break;
                    case 'd':
                        res = tfsCreate(arg1, 'd');
                        if (!res)
                          printf("Created directory: %s\n", arg1);
                        else
                          printf("Unable to create directory: %s\n", arg1);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                }
                break;
            case 'l':
                if(numTokens != 2)
                    errorParse();
                res = tfsLookup(arg1);
                if (res >= 0)
                    printf("Search: %s found\n", arg1);
                else
                    printf("Search: %s not found\n", arg1);
                break;
            case 'd':
                if(numTokens != 2)
                    errorParse();
                res = tfsDelete(arg1);
                if (!res)
                  printf("Deleted: %s\n", arg1);
                else
                  printf("Unable to delete: %s\n", arg1);
                break;
            case 'm':
                if(numTokens != 3)
                    errorParse();
                res = tfsMove(arg1, arg2);
                if (!res)
                  printf("Moved: %s to %s\n", arg1, arg2);
                else
                  printf("Unable to move: %s to %s\n", arg1, arg2);
                break;
            case 'p':
                if(numTokens != 2)
                    errorParse();
                res = tfsPrint(arg1);
                if (!res)
                  printf("Print Tecnico FS Tree successful.\n");
                else
                  printf("Print Tecnico FS Tree Failed.\n");
                break;
            case '#':
                break;
            default: {
                errorParse();
            }
        }
    }
    fclose(inputFile);
    return NULL;
}

int main(int argc, char* argv[]) {
    parseArgs(argc, argv);

    if (tfsMount(serverName) == 0) {
        printf("Mounted! (socket = %s)\n", serverName);
    }
    else {
        fprintf(stderr, "Unable to mount socket: %s\n", serverName);
        exit(EXIT_FAILURE);
    }

    processInput();

    tfsUnmount();

    free(serverName);

    exit(EXIT_SUCCESS);
}
