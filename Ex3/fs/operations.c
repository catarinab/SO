/* 
 * (Ficheiro alterado)
 * Grupo 27:
 * 93230 Catarina Bento
 * 94179 Luis Freire D'Andrade
*/

#include "operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

/* Given a structure of locked inodes' inumbers, adds another inumber.
 * Input:
 *  - locked_inodes: the structure with the inumbers.
 *  - inumber: the inumber to add.
 * 
 * Note: Not included in operations.h because its not supposed to be used outside 
 * this file.
 */
void addLockedInode(LockedInodes * locked_inodes, int inumber) {
	locked_inodes->inumbers = (int *) realloc(locked_inodes->inumbers, 
		(locked_inodes->size + 1) * sizeof(int));
	locked_inodes->inumbers[locked_inodes->size] = inumber;
	locked_inodes->size += 1;
}

/* Given a structure of locked inodes' inumbers, unlocks every inode.
 * Input:
 *  - locked_inodes: the structure with the inumbers.
 * 
 * Note: Not included in operations.h because its not supposed to be used outside 
 * this file.
 */
void unlockLockedInodes(LockedInodes * locked_inodes) {
	for(int i = 0; i < locked_inodes->size; i++) {
		unlock(locked_inodes->inumbers[i]);
	}
	free(locked_inodes->inumbers);
}

/* Given a structure of locked inodes' inumbers and an inumber, checks if the 
 * inumber is locked.
 * Input:
 *  - locked_inodes: the structure with the inumbers.
 *  - inumber: the inumber to check.
 * Returns: SUCCESS or FAIL
 * 
 * Note: Not included in operations.h because its not supposed to be used outside 
 * this file.
 */
int lockedInode(LockedInodes * locked_inodes, int inumber) {
	for (int i = 0; i < locked_inodes->size; i++) {
		if (locked_inodes->inumbers[i] == inumber) {
			return SUCCESS;
		}
	}
	return FAIL;
}

/* Given a path, fills pointers with strings for the parent path and child
 * file name
 * Input:
 *  - path: the path to split. ATENTION: the function may alter this parameter
 *  - parent: reference to a char*, to store parent path
 *  - child: reference to a char*, to store child file name
 */
void split_parent_child_from_path(char * path, char ** parent, char ** child) {
	int n_slashes = 0, last_slash_location = 0;
	int len = strlen(path);

	// deal with trailing slash ( a/x vs a/x/ )
	if (path[len-1] == '/') {
		path[len-1] = '\0';
	}

	for (int i=0; i < len; ++i) {
		if (path[i] == '/' && path[i+1] != '\0') {
			last_slash_location = i;
			n_slashes++;
		}
	}

	if (n_slashes == 0) { // root directory
		*parent = "";
		*child = path;
		return;
	}

	path[last_slash_location] = '\0';
	*parent = path;
	*child = path + last_slash_location + 1;
}


/*
 * Initializes tecnicofs and creates root node.
 */
void init_fs() {
	inode_table_init();
	
	/* create root inode */
	int root = inode_create(T_DIRECTORY);
	
	if (root != FS_ROOT) {
		printf("failed to create node for tecnicofs root\n");
		exit(EXIT_FAILURE);
	}

	/* inode_create locks the node created */
	unlock(root);
}


/*
 * Destroy tecnicofs and inode table.
 */
void destroy_fs() {
	inode_table_destroy();
}


/*
 * Checks if content of directory is not empty.
 * Input:
 *  - entries: entries of directory
 * Returns: SUCCESS or FAIL
 */

int is_dir_empty(DirEntry *dirEntries) {
	if (dirEntries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		if (dirEntries[i].inumber != FREE_INODE) {
			return FAIL;
		}
	}
	return SUCCESS;
}


/*
 * Looks for node in directory entry from name.
 * Input:
 *  - name: path of node
 *  - entries: entries of directory
 * Returns:
 *  - inumber: found node's inumber
 *  - FAIL: if not found
 */
int lookup_sub_node(char *name, DirEntry *entries) {
	if (entries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].inumber != FREE_INODE && strcmp(entries[i].name, name) == 0) {
            return entries[i].inumber;
        }
    }
	return FAIL;
}

/* Not included in operations.h because its not supposed to be used outside this
file. */
int lookupAux(char *name, LockedInodes *locked_inodes, int lockMode) {
	char full_path[MAX_FILE_NAME];
	char delim[] = "/";
	char *saveptr;

	strcpy(full_path, name);

	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;

	char *path = strtok_r(full_path, delim, &saveptr);

	if (path == NULL && lockMode == MODIFY) {
		lock(current_inumber, WR);
	}
	else {
		/* get root inode data */
		lock(current_inumber, RD);
		inode_get(current_inumber, &nType, &data);
	}
	addLockedInode(locked_inodes, current_inumber);

	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		path = strtok_r(NULL, delim, &saveptr); 

		if (path == NULL && lockMode == MODIFY) {
			lock(current_inumber, WR);
		}
		else {
			lock(current_inumber, RD);
		}
		addLockedInode(locked_inodes, current_inumber);

		inode_get(current_inumber, &nType, &data);
	}

	return current_inumber;
}

/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup(char *name) {
	LockedInodes locked_inodes;
	locked_inodes.inumbers = NULL;
	locked_inodes.size = 0;

	int searchResult = lookupAux(name, &locked_inodes, LOOKUP);

	unlockLockedInodes(&locked_inodes);

	return searchResult;
}

/*
 * Creates a new node given a path.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 * Returns: SUCCESS or FAIL
 */
int create(char *name, type nodeType) {
	int parent_inumber, child_inumber;
	char *parent_name, *child_name_src, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType;
	union Data pdata;

	LockedInodes locked_inodes;
	locked_inodes.inumbers = NULL;
	locked_inodes.size = 0;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name_src);

	parent_inumber = lookupAux(parent_name, &locked_inodes, MODIFY);
	if (parent_inumber == FAIL) {
		printf("failed to create %s, invalid parent dir %s\n",
		        name, parent_name);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);
	if(pType != T_DIRECTORY) {
		printf("failed to create %s, parent %s is not a dir\n",
		        name, parent_name);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	if (lookup_sub_node(child_name_src, pdata.dirEntries) != FAIL) {
		printf("failed to create %s, already exists in dir %s\n",
		       child_name_src, parent_name);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	/* create node and add entry to folder that contains new node */
	child_inumber = inode_create(nodeType);
	if (child_inumber == FAIL) {
		printf("failed to create %s in  %s, couldn't allocate inode\n",
		        child_name_src, parent_name);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}
	/* inode_create locks the node created */
	addLockedInode(&locked_inodes, child_inumber);

	if (dir_add_entry(parent_inumber, child_inumber, child_name_src) == FAIL) {
		printf("could not add entry %s in dir %s\n",
		       child_name_src, parent_name);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}
	unlockLockedInodes(&locked_inodes);

	return SUCCESS;
}

/*
 * Deletes a node given a path.
 * Input:
 *  - name: path of node
 * Returns: SUCCESS or FAIL
 */
int delete(char *name) {
	int parent_inumber, child_inumber;
	char *parent_name, *child_name_src, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	LockedInodes locked_inodes;
	locked_inodes.inumbers = NULL;
	locked_inodes.size = 0;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name_src);

	parent_inumber = lookupAux(parent_name, &locked_inodes, MODIFY);
	if (parent_inumber == FAIL) {
		printf("failed to delete %s, invalid parent dir %s\n",
		        child_name_src, parent_name);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}
	
	inode_get(parent_inumber, &pType, &pdata);
	if(pType != T_DIRECTORY) {
		printf("failed to delete %s, parent %s is not a dir\n",
		        child_name_src, parent_name);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	child_inumber = lookup_sub_node(child_name_src, pdata.dirEntries);
	if (child_inumber == FAIL) {
		printf("could not delete %s, does not exist in dir %s\n",
		       name, parent_name);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	lock(child_inumber, WR);
	addLockedInode(&locked_inodes, child_inumber);
	inode_get(child_inumber, &cType, &cdata);
	if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
		printf("could not delete %s: is a directory and not empty\n",
		       name);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	/* remove entry from folder that contained deleted node */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to delete %s from dir %s\n",
		       child_name_src, parent_name);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	if (inode_delete(child_inumber) == FAIL) {
		printf("could not delete inode number %d from dir %s\n",
		       child_inumber, parent_name);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	unlockLockedInodes(&locked_inodes);

	return SUCCESS;
}

/*
 * Moves a node given a source path and a destiny path.
 * Input:
 *  - source: path of the node to move.
 *  - destiny: future path of the node.
 * Returns: SUCCESS or FAIL
 */
int move(char *source, char *destiny) {
	/* esta operação só deve ser executada caso se verifiquem duas condições no 
	 * momento em que é invocada: existe um ficheiro/diretoria com o pathname
	 * atual e não existe nenhum ficheiro/diretoria com o novo pathname.
	 * 1. guardar os dois trincos que são para escrita no writeMode.
	 * 2. verificar que existe um ficheiro/diretoria com o pathname do primeiro 
	 * path. depois fazer os locks e se ele passar pelo trinco de escrita do outro 
	 * path, dar lock.
	 * 3. verificar que não existe nenhum ficheiro/diretoria com o novo pathname.
	 * fazer os locks com trylock. Se falhar porque o diretorio já está nos locked 
	 * inodes, continuar. se não. então largar os locks, fazer sleep, e depois 
	 * tentar de novo.
	 */

	char *buffer1 = NULL, *buffer2 = NULL, *buffer3 = NULL;
	char *child_name_src = NULL, *child_name_dest = NULL;
	char source_copy[MAX_FILE_NAME], destiny_copy[MAX_FILE_NAME];
	int current_inumber, parent_inumber_src, parent_inumber_dest, child_inumber;

	/* use for strtok_r */
	char delim[] = "/";
	char *saveptr = NULL, *path = NULL;

	/* use for copy */
	type pType;
	union Data pdata;

	/* array of directories/files that have to be locked for writing */
	char *writeMode[2];

	/* locked inodes */
	LockedInodes locked_inodes;
	locked_inodes.inumbers = NULL;
	locked_inodes.size = 0;

	/* stores in writeMode the nodes that have to be locked for writing */
	strcpy(source_copy, source);
	split_parent_child_from_path(source_copy, &buffer1, &buffer2);
	split_parent_child_from_path(buffer1, &buffer2, &buffer3);
	writeMode[0] = strdup(buffer3);

	strcpy(destiny_copy, destiny);
	split_parent_child_from_path(destiny_copy, &buffer1, &buffer2);
	split_parent_child_from_path(buffer1, &buffer2, &buffer3);
	writeMode[1] = strdup(buffer3);

	/* locks the root */
	current_inumber = FS_ROOT;
	if (strcmp("", writeMode[0]) == 0 || strcmp("", writeMode[1]) == 0) {
		lock(current_inumber, WR);
	}
	else {
		lock(current_inumber, RD);
	}
	addLockedInode(&locked_inodes, current_inumber);
	/* get root inode data */
	inode_get(current_inumber, &pType, &pdata);

	/* locks source path */
	strcpy(source_copy, source);
	split_parent_child_from_path(source_copy, &path, &child_name_src);
	path = strtok_r(path, delim, &saveptr);
	while (path != NULL && (current_inumber = lookup_sub_node(path, pdata.dirEntries)) != FAIL) {
		if (strcmp(path, writeMode[0]) == 0 || strcmp(path, writeMode[1]) == 0) {
			lock(current_inumber, WR);
		}
		else {
			lock(current_inumber, RD);
		}
		addLockedInode(&locked_inodes, current_inumber);
		inode_get(current_inumber, &pType, &pdata);
		path = strtok_r(NULL, delim, &saveptr); 
	}
	if ((parent_inumber_src = current_inumber) == FAIL || 
		(child_inumber = lookup_sub_node(child_name_src, pdata.dirEntries)) == FAIL) {
		printf("failed to move, invalid source path %s\n", source);
		free(writeMode[0]);
		free(writeMode[1]);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	current_inumber = FS_ROOT;
	/* get root inode data */
	inode_get(current_inumber, &pType, &pdata);

	/* locks destiny path */
	strcpy(destiny_copy, destiny);
	split_parent_child_from_path(destiny_copy, &path, &child_name_dest);
	path = strtok_r(path, delim, &saveptr);
	while (path != NULL && (current_inumber = lookup_sub_node(path, pdata.dirEntries)) != FAIL) {
		if (lockedInode(&locked_inodes, current_inumber) != SUCCESS) {
			if (strcmp(path, writeMode[0]) == 0 || strcmp(path, writeMode[1]) == 0) {
				if (trylock(current_inumber, WR) == FAIL) {
					free(writeMode[0]);
					free(writeMode[1]);
					unlockLockedInodes(&locked_inodes);
					sleep((double)rand() / (double)RAND_MAX);
					return GIVEUP;
				}
			}
			else {
				if (trylock(current_inumber, RD) == FAIL) {
					free(writeMode[0]);
					free(writeMode[1]);
					unlockLockedInodes(&locked_inodes);
					sleep((double)rand() / (double)RAND_MAX);
					return GIVEUP;
				}
			}
			addLockedInode(&locked_inodes, current_inumber);
		}
		inode_get(current_inumber, &pType, &pdata);
		path = strtok_r(NULL, delim, &saveptr);
	}
	if ((parent_inumber_dest = current_inumber) == FAIL || 
		(lookup_sub_node(child_name_dest, pdata.dirEntries)) != FAIL) {
		printf("failed to move, invalid destiny path %s\n", destiny);
		free(writeMode[0]);
		free(writeMode[1]);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	free(writeMode[0]);
	free(writeMode[1]);

	/* checks if move makes sense */
	if (lockedInode(&locked_inodes, child_inumber) == SUCCESS) {
		printf("failed to move %s to %s\n", source, destiny);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	/* deletes file/directory from the source path */
	if (dir_reset_entry(parent_inumber_src, child_inumber) == FAIL) {
		printf("failed to move %s to %s\n", source, destiny);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	/* adds file/directory to the destiny path */
	if (dir_add_entry(parent_inumber_dest, child_inumber, child_name_dest) == FAIL) {
		dir_add_entry(parent_inumber_src, child_inumber, child_name_src);
		printf("failed to move %s to %s\n", source, destiny);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	unlockLockedInodes(&locked_inodes);
	
	return SUCCESS;
}

/*
 * Executes the "p" command, prints tecnicofs tree.
 * Input:
 *  - outputFile: file where the tree is printed.
 * Returns: SUCCESS or FAIL
 */
int printOperations(FILE * outputFile) {
	lock(FS_ROOT, WR);
	print_tecnicofs_tree(outputFile);
	unlock(FS_ROOT);

	return SUCCESS;
}

/*
 * Prints tecnicofs tree.
 * Input:
 *  - fp: pointer to output file
 */
void print_tecnicofs_tree(FILE *fp) {
	inode_print_tree(fp, FS_ROOT, "");
}