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
#define MUTEX 0 /* Used to access the FS lock, when its a mutex (locks and unlocks). */
#define RWLOCK 1 /* Used to access the FS lock, when its a readwrite lock (unlocks). */
#define READ 2 /* Used to access the FS lock, when its a readwrite lock, and you want to read (locks). */
#define WRITE 3 /* Used to access the FS lock, when its a readwrite lock, and you want to write (locks). */
#define COMMANDS 4 /* Used to access the lock of the variables associated with the commands (locks and unlocks). */
#define NOSYNC 5 /* Used when theres only one thread, and no need for locks. */

/* Synch strategy chosen (MUTEX, RWLOCK or NOSYNC). */
int synchstrategy;

void init_fs();
void destroy_fs();
void initLocks();
void destroyLocks();
void lock(int flag);
void unlock(int flag);
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType);
int delete(char *name);
int lookup(char *name);
void print_tecnicofs_tree(FILE *fp);

#endif /* FS_H */
