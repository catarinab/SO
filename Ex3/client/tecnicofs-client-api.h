/* 
 * (Ficheiro alterado)
 * Grupo 27:
 * 93230 Catarina Bento
 * 94179 Luis Freire D'Andrade
*/

#ifndef API_H
#define API_H

#include "tecnicofs-api-constants.h"

#define SOCKETCLIENTE "/tmp/client"
#define SUCCESS 0
#define FAIL -1

int tfsCreate(char *filename, char nodeType);
int tfsDelete(char *path);
int tfsLookup(char *path);
int tfsMove(char *from, char *to);
int tfsPrint(char * fileName);
int tfsMount(char* serverName);
int tfsUnmount();

#endif /* CLIENT_H */
