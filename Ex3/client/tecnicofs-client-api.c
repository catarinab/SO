/* 
 * (Ficheiro alterado)
 * Grupo 27:
 * 93230 Catarina Bento
 * 94179 Luis Freire D'Andrade
*/

#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

int sockfd;
char *clientName;
struct sockaddr_un serv_addr, client_addr;
socklen_t servlen, clilen;

/* Sets up a socket.
 * Input:
 *  - path: path of the socket.
 *  - addr: socket address.
 * Returns:
 *  - Lenght of the socket address.
 */
int setSockAddrUn(char *path, struct sockaddr_un * addr) {
  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}

/* Sends a command to the server's socket.
 * Input:
 *  - command: command to send.
 * Returns: SUCCESS or FAIL (of the command).
 */
int sendCommand(char * command){
  int res;
  if (sendto(sockfd, command, strlen(command) + 1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    free(command);
    return FAIL;
  }
  free(command);

  if (recvfrom(sockfd, &res, sizeof(int), 0, 0, 0) < 0) {
    perror("client: recvfrom error");
    return FAIL;
  }
  
  return res;
}

int tfsCreate(char *filename, char nodeType) {
  char * message = strdup("c ");
  strcat(message, filename);
  strcat(message, " ");
  strncat(message, &nodeType, 1);
  
  return sendCommand(message);
}

int tfsDelete(char *path) {
  char * message = strdup("d ");
  strcat(message, path);
  
  return sendCommand(message);
}

int tfsMove(char *from, char *to) {
  char * message = strdup("m ");
  strcat(message, from);
  strcat(message, " ");
  strcat(message, to);
  
  return sendCommand(message);
}

int tfsLookup(char *path) {
  char * message = strdup("l ");
  strcat(message, path);

  return sendCommand(message);
}

int tfsPrint(char * fileName){
  char * message = strdup("p ");
  strcat(message, fileName);

  return sendCommand(message);
}

int tfsMount(char * sockPath) {  
  if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0) ) < 0) {
    perror("client: can't open socket");
    return FAIL;
  }

  char pid[MAX_FILE_NAME - strlen(SOCKETCLIENTE)];
  clientName = strdup(SOCKETCLIENTE);
  sprintf(pid, "%d", getpid());
  strcat(clientName, pid);

  unlink(clientName);
  clilen = setSockAddrUn(clientName, &client_addr);
  if (bind(sockfd, (struct sockaddr *) &client_addr, clilen) < 0) {
    perror("client: bind error");
    return FAIL;
  }  

  servlen = setSockAddrUn(sockPath, &serv_addr);

  return SUCCESS;
}

int tfsUnmount() {
  if (close(sockfd) < 0) {
    perror("client: can't close socket");
    return FAIL;
  }
  unlink(clientName);
  free(clientName);

  return SUCCESS;
}
