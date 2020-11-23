#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

int sockfd;
char *servname;
struct sockaddr_un serv_addr, client_addr;
socklen_t servlen, clilen;

int setSockAddrUn(char *path, struct sockaddr_un *addr) {
  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}

/* int tfsCreate(char *filename, char nodeType); */
int tfsCreate(char * message) {
  if (sendto(sockfd, message, strlen(message) + 1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    return -1;
  }

  return 0;
}

int tfsDelete(char *path) {
  return -1;
}

int tfsMove(char *from, char *to) {
  return -1;
}

int tfsLookup(char *path) {
  return -1;
}

int tfsMount(char * sockPath) {  
  servname = strdup(sockPath);

  if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0) ) < 0) {
    perror("client: can't open socket");
    return -1;
  }

  unlink(SOCKETCLIENTE);
  clilen = setSockAddrUn (SOCKETCLIENTE, &client_addr);
  if (bind(sockfd, (struct sockaddr *) &client_addr, clilen) < 0) {
    perror("client: bind error");
    return -1;
  }  

  servlen = setSockAddrUn(sockPath, &serv_addr);

  return 0;
}

int tfsUnmount() {
  close(sockfd);
  free(servname);
  unlink(SOCKETCLIENTE);

  return 0;
}
