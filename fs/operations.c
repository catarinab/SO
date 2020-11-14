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

/* Given a structure of locked inodes' inumbers, adds another inumber.
 * Input:
 *  - locked_inodes: the structure with the inumbers.
 *  - inumber: the inumber to add.
 * 
 * Note: Not included in operations.h because its not supposed to be used outside 
 * this file.
 */
void addLockedInode(LockedInodes * locked_inodes, int inumber){
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
void unlockLockedInodes(LockedInodes * locked_inodes){
	for(int i = 0; i < locked_inodes->size; i++) {
		unlock(locked_inodes->inumbers[i]);
	}
	free(locked_inodes->inumbers);
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
int lookupAux(char *name, LockedInodes *locked_inodes, int lockMode, int op) {
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
		if (op == TRY)
			if (trylock(current_inumber, WR) == FAIL)
				return GIVEUP;
		else
			lock(current_inumber, WR);
	}
	else {
		/* get root inode data */
		if (op == TRY)
			if (trylock(current_inumber, RD) == FAIL)
				return GIVEUP;
		else
			lock(current_inumber, RD);
		inode_get(current_inumber, &nType, &data);
	}
	addLockedInode(locked_inodes, current_inumber);

	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		path = strtok_r(NULL, delim, &saveptr); 

		if (path == NULL && lockMode == MODIFY) {
			if (op == TRY) 
				if (trylock(current_inumber, WR) == FAIL)
					return GIVEUP;
			else
				lock(current_inumber, WR);
		}
		else {
			if (op == TRY)
				if (trylock(current_inumber, RD) == FAIL) 
					return GIVEUP;
			else
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

	int searchResult = lookupAux(name, &locked_inodes, LOOKUP, LOCK);

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
int create(char *name, type nodeType){
	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType;
	union Data pdata;

	LockedInodes locked_inodes;
	locked_inodes.inumbers = NULL;
	locked_inodes.size = 0;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookupAux(parent_name, &locked_inodes, MODIFY, LOCK);
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

	if (lookup_sub_node(child_name, pdata.dirEntries) != FAIL) {
		printf("failed to create %s, already exists in dir %s\n",
		       child_name, parent_name);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	/* create node and add entry to folder that contains new node */
	child_inumber = inode_create(nodeType);
	if (child_inumber == FAIL) {
		printf("failed to create %s in  %s, couldn't allocate inode\n",
		        child_name, parent_name);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}
	/* inode_create locks the node created */
	addLockedInode(&locked_inodes, child_inumber);

	if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL) {
		printf("could not add entry %s in dir %s\n",
		       child_name, parent_name);
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
int delete(char *name){
	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	LockedInodes locked_inodes;
	locked_inodes.inumbers = NULL;
	locked_inodes.size = 0;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookupAux(parent_name, &locked_inodes, MODIFY, LOCK);
	if (parent_inumber == FAIL) {
		printf("failed to delete %s, invalid parent dir %s\n",
		        child_name, parent_name);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}
	
	inode_get(parent_inumber, &pType, &pdata);
	if(pType != T_DIRECTORY) {
		printf("failed to delete %s, parent %s is not a dir\n",
		        child_name, parent_name);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);
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
		       child_name, parent_name);
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
 * Moves a node given an origin path and a destiny path.
 * Input:
 *  - origin: path of node
 *  - destiny: future path of node
 * Returns: SUCCESS or FAIL
 */
int move(char *origin, char *destiny){
	/* esta operação só deve ser executada caso se verifiquem duas condições no 
	 * momento em que é invocada: existe um ficheiro/diretoria com o pathname
	 * atual e não existe nenhum ficheiro/diretoria com o novo pathname.
	 */
	
	int parent_inumber_orig, child_inumber_orig;
	int parent_inumber_dest, child_inumber_dest;
	char *parent_name_orig, *child_name_orig, origin_copy[MAX_FILE_NAME];
	char *parent_name_dest, *child_name_dest, destiny_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	LockedInodes locked_inodes;
	locked_inodes.inumbers = NULL;
	locked_inodes.size = 0;

	strcpy(origin_copy, origin);
	split_parent_child_from_path(origin_copy, &parent_name_orig, &child_name_orig);
	strcpy(destiny_copy, destiny);
	split_parent_child_from_path(destiny_copy, &parent_name_dest, &child_name_dest);

	parent_inumber_orig = lookupAux(parent_name_orig, &locked_inodes, MODIFY, LOCK);
	if (parent_inumber_orig == FAIL) {
		printf("failed to move %s, invalid parent dir %s\n",
		        child_name_orig, parent_name_orig);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}
	
	inode_get(parent_inumber_orig, &pType, &pdata);
	if(pType != T_DIRECTORY) {
		printf("failed to move %s, parent %s is not a dir\n",
		        child_name_orig, parent_name_orig);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	child_inumber_orig = lookup_sub_node(child_name_orig, pdata.dirEntries);
	if (child_inumber_orig == FAIL) {
		printf("could not move %s, does not exist in dir %s\n", child_name_orig,
				parent_name_orig);
		unlockLockedInodes(&locked_inodes);
		return FAIL;
	}

	lock(child_inumber_orig, WR);
	addLockedInode(&locked_inodes, child_inumber);
}

/*
 * Prints tecnicofs tree.
 * Input:
 *  - fp: pointer to output file
 */
void print_tecnicofs_tree(FILE *fp){
	inode_print_tree(fp, FS_ROOT, "");
}