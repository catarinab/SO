#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "state.h"
#include "../tecnicofs-api-constants.h"

inode_t inode_table[INODE_TABLE_SIZE];

/* Given a flag, locks the respective lock.
 * Input:
 *  - inumber: the number of the inode that is going to be locked.
 *  - flag: the flag that indicates which lock to lock (flag descriptions in 
 * operations.h). 
 */
void lock(int inumber, int flag) {
    if ((inumber < 0) || (inumber > INODE_TABLE_SIZE) || (inode_table[inumber].nodeType == T_NONE)) {
        printf("lock: invalid inumber\n");
        exit(EXIT_FAILURE);
    }

	if (flag == READ) {
		if (pthread_rwlock_rdlock(&inode_table[inumber].lock) != 0) {
			fprintf(stderr, "Error: readwrite lock failed\n");
			exit(EXIT_FAILURE);
		}
	}
	else if (flag == WRITE) {
		if( pthread_rwlock_wrlock(&inode_table[inumber].lock) != 0) {
			fprintf(stderr, "Error: readwrite lock failed\n");
			exit(EXIT_FAILURE);
		}
	}
}

/* Given a flag, unlocks the respective lock.
 * Input:
 *  - inumber: the number of the inode that is going to be locked.
 *  - flag: the flag that indicates which lock to unlock (flag descriptions in 
 * operations.h).
 */
void unlock(int inumber) {
    if ((inumber < 0) || (inumber > INODE_TABLE_SIZE) || (inode_table[inumber].nodeType == T_NONE)) {
        printf("lock: invalid inumber\n");
        exit(EXIT_FAILURE);
    }

	if (pthread_rwlock_unlock(&inode_table[inumber].lock) != 0) {
		fprintf(stderr, "Error: readwrite unlock failed\n");
		exit(EXIT_FAILURE);
	}
}

/*
 * Sleeps for synchronization testing.
 */
void insert_delay(int cycles) {
    for (int i = 0; i < cycles; i++) {}
}


/*
 * Initializes the i-nodes table.
 */
void inode_table_init() {
    for (int i = 0; i < INODE_TABLE_SIZE; i++) {
        inode_table[i].nodeType = T_NONE;
        inode_table[i].data.dirEntries = NULL;
        inode_table[i].data.fileContents = NULL;
    }
}

/*
 * Releases the allocated memory for the i-nodes tables.
 */

void inode_table_destroy() {
    for (int i = 0; i < INODE_TABLE_SIZE; i++) {
        if (inode_table[i].nodeType != T_NONE) {
            /* as data is an union, the same pointer is used for both dirEntries and fileContents */
            /* just release one of them */
	  if (inode_table[i].data.dirEntries)
            free(inode_table[i].data.dirEntries);
        }
    }
}

/*
 * Creates a new i-node in the table with the given information.
 * Input:
 *  - nType: the type of the node (file or directory)
 * Returns:
 *  inumber: identifier of the new i-node, if successfully created
 *     FAIL: if an error occurs
 */
int inode_create(type nType) {
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);

    for (int inumber = 0; inumber < INODE_TABLE_SIZE; inumber++) {
        if (inode_table[inumber].nodeType == T_NONE) {
            inode_table[inumber].nodeType = nType;

            if (nType == T_DIRECTORY) {
                /* Initializes entry table */
                inode_table[inumber].data.dirEntries = malloc(sizeof(DirEntry) * MAX_DIR_ENTRIES);
                
                for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
                    inode_table[inumber].data.dirEntries[i].inumber = FREE_INODE;
                }
            }
            else {
                inode_table[inumber].data.fileContents = NULL;
            }
            return inumber;
        }
    }
    return FAIL;
}

/*
 * Deletes the i-node.
 * Input:
 *  - inumber: identifier of the i-node
 * Returns: SUCCESS or FAIL
 */
int inode_delete(int inumber) {
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);

    if ((inumber < 0) || (inumber > INODE_TABLE_SIZE) || (inode_table[inumber].nodeType == T_NONE)) {
        printf("inode_delete: invalid inumber\n");
        return FAIL;
    } 

    inode_table[inumber].nodeType = T_NONE;
    /* see inode_table_destroy function */
    if (inode_table[inumber].data.dirEntries)
        free(inode_table[inumber].data.dirEntries);
