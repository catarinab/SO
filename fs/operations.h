/* 
 * (Ficheiro alterado)
 * Grupo 27:
 * 93230 Catarina Bento
 * 94179 Luis Freire D'Andrade
*/



#ifndef FS_H
#define FS_H

#include "state.h"

/* Flags for the functions that involve locks. */
#define READ 0 /* Used to access the FS lock, when its a readwrite lock, and you want to read (locks). */
#define WRITE 1 /* Used to access the FS lock, when its a readwrite lock, and you want to write (locks). */

/*
 * Contains the inumbers of the inodes locked
 */
typedef struct lockedInodes {
	int *inumbers;
	int size;
} LockedInodes;

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType);
int delete(char *name);
int lookup(char *name);
void print_tecnicofs_tree(FILE *fp);

#endif /* FS_H */
