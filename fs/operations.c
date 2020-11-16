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
	locked_inodes->inumbers = (int *) realloc(locked_inodes->inumbers, (locked_inodes->size + 1) * sizeof(int));
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
	char *parent_name, *child_name_orig, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType;
	union Data pdata;

	LockedInodes locked_inodes;
	locked_inodes.inumbers = NULL;
	locked_inodes.size = 0;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name_orig);

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

	if (lookup_sub_node(child_name_orig, pdata.dirEntries) != FAIL) {
		printf("failed to create %s, already exists in dir %s\n",
		       child_name_orig, parent_name);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	/* create node and add entry to folder that contains new node */
	child_inumber = inode_create(nodeType);
	if (child_inumber == FAIL) {
		printf("failed to create %s in  %s, couldn't allocate inode\n",
		        child_name_orig, parent_name);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}
	/* inode_create locks the node created */
	addLockedInode(&locked_inodes, child_inumber);

	if (dir_add_entry(parent_inumber, child_inumber, child_name_orig) == FAIL) {
		printf("could not add entry %s in dir %s\n",
		       child_name_orig, parent_name);
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
	char *parent_name, *child_name_orig, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	LockedInodes locked_inodes;
	locked_inodes.inumbers = NULL;
	locked_inodes.size = 0;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name_orig);

	parent_inumber = lookupAux(parent_name, &locked_inodes, MODIFY);
	if (parent_inumber == FAIL) {
		printf("failed to delete %s, invalid parent dir %s\n",
		        child_name_orig, parent_name);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}
	
	inode_get(parent_inumber, &pType, &pdata);
	if(pType != T_DIRECTORY) {
		printf("failed to delete %s, parent %s is not a dir\n",
		        child_name_orig, parent_name);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	child_inumber = lookup_sub_node(child_name_orig, pdata.dirEntries);
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
		       child_name_orig, parent_name);
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
 * Checks if a given path is in the array given.
 * Input:
 *  - table: array of paths.
 *  - path: path to check.
 * Returns: SUCCESS or FAIL
 */
int included(char **table, char *path) {
	for (int i = 0; i < 4; i++){
		if (strcmp(table[i], path) == 0) {
			return SUCCESS;
		}
	}
	return FAIL;
}

/*
 * Moves a node given an origin path and a destiny path.
 * Input:
 *  - origin: path of node
 *  - destiny: future path of node
 * Returns: SUCCESS or FAIL
 */
int move(char *origin, char *destiny) {
	/* esta operação só deve ser executada caso se verifiquem duas condições no 
	 * momento em que é invocada: existe um ficheiro/diretoria com o pathname
	 * atual e não existe nenhum ficheiro/diretoria com o novo pathname.
	 * 1. guardar os dois trincos que são para leitura no writeMode.
	 * 2. verificar que existe um ficheiro/diretoria com o pathname do primeiro 
	 * path. depois fazer os locks e se ele passar pelo trinco de escrita do outro 
	 * path, dar lock.
	 * 3. verificar que não existe nenhum ficheiro/diretoria com o novo pathname.
	 * fazer os locks com trylock. Se falhar porque o diretorio já está nos locked 
	 * inodes, continuar. se não. então largar os locks, fazer sleep, e depois 
	 * tentar de novo.
	 */

	char *parent_name = NULL, *child_name_orig = NULL, *child_name_dest = NULL, *waste = NULL;
	char origin_copy1[MAX_FILE_NAME], destiny_copy1[MAX_FILE_NAME];
	char origin_copy2[MAX_FILE_NAME], destiny_copy2[MAX_FILE_NAME];
	int current_inumber, parent_inumber_orig, parent_inumber_dest, child_inumber;

	/* use for strtok_r */
	char delim[] = "/";
	char *saveptr_orig = NULL, *saveptr_dest = NULL, *origin_path = NULL, *destiny_path = NULL;

	/* use for copy */
	type pType;
	union Data pdata;

	/* array of directories/files that have to be locked for writing */
	char *writeMode[4];

	/* locked inodes */
	LockedInodes locked_inodes;
	locked_inodes.inumbers = NULL;
	locked_inodes.size = 0;

	/* stores in writeMode the nodes that have to be locked for writing */
	strcpy(origin_copy1, origin);
	strcpy(destiny_copy1, destiny);
	split_parent_child_from_path(origin_copy1, &parent_name, &writeMode[0]);
	split_parent_child_from_path(parent_name, &waste, &writeMode[1]);
	split_parent_child_from_path(destiny_copy1, &parent_name, &writeMode[2]);
	split_parent_child_from_path(parent_name, &waste, &writeMode[3]);

	current_inumber = FS_ROOT;
	if (included(writeMode, "") == SUCCESS) {
		lock(current_inumber, WR);
	}
	else {
		lock(current_inumber, RD);
	}
	addLockedInode(&locked_inodes, current_inumber);
	/* get root inode data */
	inode_get(current_inumber, &pType, &pdata);

	/* locks origin path */
	strcpy(origin_copy2, origin);
	split_parent_child_from_path(origin_copy2, &origin_path, &child_name_orig);
	origin_path = strtok_r(origin_path, delim, &saveptr_orig);
	while (origin_path != NULL && (current_inumber = lookup_sub_node(origin_path, pdata.dirEntries)) != FAIL) {
		if (included(writeMode, origin_path) == SUCCESS) {
			lock(current_inumber, WR);
		}
		else {
			lock(current_inumber, RD);
		}
		addLockedInode(&locked_inodes, current_inumber);
		inode_get(current_inumber, &pType, &pdata);
		origin_path = strtok_r(NULL, delim, &saveptr_orig); 
	}
	if ((parent_inumber_orig = current_inumber) == FAIL || (child_inumber = lookup_sub_node(child_name_orig, pdata.dirEntries)) == FAIL) {
		printf("failed to move, invalid origin path %s\n", origin);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	current_inumber = FS_ROOT;
	/* get root inode data */
	inode_get(current_inumber, &pType, &pdata);

	/* locks destiny path */
	strcpy(destiny_copy2, destiny);
	split_parent_child_from_path(destiny_copy2, &destiny_path, &child_name_dest);
	destiny_path = strtok_r(destiny_path, delim, &saveptr_dest);
	while (destiny_path != NULL && (current_inumber = lookup_sub_node(destiny_path, pdata.dirEntries)) != FAIL) {
		if (lockedInode(&locked_inodes, current_inumber) != SUCCESS) {
			if (included(writeMode, destiny_path) == SUCCESS) {
				if (trylock(current_inumber, WR) == FAIL) {
					unlockLockedInodes(&locked_inodes);
					sleep((double)rand() / (double)RAND_MAX);
					return GIVEUP;
				}
			}
			else {
				if (trylock(current_inumber, RD) == FAIL) {
					unlockLockedInodes(&locked_inodes);
					sleep((double)rand() / (double)RAND_MAX);
					return GIVEUP;
				}
			}
			addLockedInode(&locked_inodes, current_inumber);
		}
		inode_get(current_inumber, &pType, &pdata);
		destiny_path = strtok_r(NULL, delim, &saveptr_dest);
	}
	if ((parent_inumber_dest = current_inumber) == FAIL || (lookup_sub_node(child_name_dest, pdata.dirEntries)) != FAIL) {
		printf("failed to move, invalid destiny path %s\n", destiny);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	/* checks if move makes sense */
	if (lockedInode(&locked_inodes, child_inumber) == SUCCESS) {
		printf("failed to move %s to %s\n", origin, destiny);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	/* deletes file/directory from the origin path */
	if (dir_reset_entry(parent_inumber_orig, child_inumber) == FAIL) {
		printf("failed to move %s to %s\n", origin, destiny);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	/* adds file/directory to the destiny path */
	if (dir_add_entry(parent_inumber_dest, child_inumber, child_name_dest) == FAIL) {
		dir_add_entry(parent_inumber_orig, child_inumber, child_name_orig);
		printf("failed to move %s to %s\n", origin, destiny);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	unlockLockedInodes(&locked_inodes);
	
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