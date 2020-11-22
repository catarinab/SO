#ifndef API_H
#define API_H

#include "tecnicofs-api-constants.h"

#define SOCKETCLIENTE "datagramCliente"

int tfsCreate(char * message);
int tfsDelete(char *path);
int tfsLookup(char *path);
int tfsMove(char *from, char *to);
int tfsMount(char* serverName);
int tfsUnmount();

#endif /* CLIENT_H */
