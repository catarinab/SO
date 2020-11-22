#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

int tfsCreate(char *filename, char nodeType) {
  return -1;
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
  return -1;
}

int tfsUnmount() {
  return -1;
}
