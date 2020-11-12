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
 */
void addLockedInode(LockedInodes * locked_inodes, int inumber){
	printf("Adicionar locked inumber: %d\n", inumber);
	locked_inodes->inumbers = (int *) realloc(locked_inodes->inumbers, (locked_inodes->size + 1) * sizeof(int));
	locked_inodes->inumbers[locked_inodes->size] = inumber;
	locked_inodes->size += 1;
}

/* Given a structure of locked inodes' inumbers, unlocks every inode.
 * Input:
 *  - locked_inodes: the structure with the inumbers.
 */
void unlockLockedInodes(LockedInodes * locked_inodes){
	printf("unlockedInodes\n");
	for(int i = 0; i < locked_inodes->size; i++) {
		printf("unlock LockedInodes: %d\n", i);
		unlock(locked_inodes->inumbers[i]);
	}
	printf("free LockedInodes->inumbers\n");
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
file, since it doesnt have any locks. */
int lookupAux(char *name, LockedInodes *locked_inodes, int flag) {
	char full_path[MAX_FILE_NAME];
	char delim[] = "/";

	strcpy(full_path, name);

	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;

	char *path = strtok(full_path, delim);
	
	if (path == NULL && flag == MODIFY) {
		printf("Lock WRITE no Inode: %d\n", current_inumber);
		lock(current_inumber, WRITE);
	}
	else {
		/* get root inode data */
		printf("Lock Read no Inode: %d\n", current_inumber);
		lock(current_inumber, READ);
		inode_get(current_inumber, &nType, &data);
	}
	addLockedInode(locked_inodes, current_inumber);

	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		path = strtok(NULL, delim);
		printf("2 lock no inumber: %d (lookupAux)\n", current_inumber);
		if (path == NULL && flag == MODIFY) {
			printf("Lock Write no Inode: %d\n", current_inumber);
			lock(current_inumber, WRITE);
		}
		else {
			printf("Lock Read no Inode: %d\n", current_inumber);
			lock(current_inumber, READ);
		}
		addLockedInode(locked_inodes, current_inumber);
		inode_get(current_inumber, &nType, &data);
	}
	printf("retornar inumber: %d\n", current_inumber);
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
	printf("entrou no lookup\n");
	LockedInodes locked_inodes;
	locked_inodes.size = 0;

	int searchResult = lookupAux(name, &locked_inodes, LOOKUP);
	unlockLockedInodes(&locked_inodes);

	printf("saiu no lookup\n");

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
	locked_inodes.size = 0;


	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

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

	printf("lock no inumber: %d (create)\n", child_inumber);
	lock(child_inumber, WRITE);
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
	locked_inodes.size = 0;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookupAux(parent_name, &locked_inodes, MODIFY);

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

	printf("lock no inumber: %d (create)\n", child_inumber);
	lock(child_inumber, WRITE);
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
 * Prints tecnicofs tree.
 * Input:
 *  - fp: pointer to output file
 */
void print_tecnicofs_tree(FILE *fp){
	inode_print_tree(fp, FS_ROOT, "");
}
