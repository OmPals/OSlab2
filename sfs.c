#include "sfs.h"
#include<string.h>
#include<stdbool.h>

// checks for the existence of file in the current directory. returns inode number if found else returns -1
int check_valid(char *fname, int curr_dir_inode) {
	char itype;
	int blocks[3];

	_directory_entry _directory_entries[4];		
	
	int i,j,k;
	int e_inode;

	// read inode entry for current directory
	// in SFS, an inode can point to three blocks at the most		
	itype = _inode_table[curr_dir_inode].TT[0];
	blocks[0] = stoi(_inode_table[curr_dir_inode].XX,2);
	blocks[1] = stoi(_inode_table[curr_dir_inode].YY,2);
	blocks[2] = stoi(_inode_table[curr_dir_inode].ZZ,2);

	// its a directory; so the following should never happen		
	if (itype=='F') {
		printf("Error: This is a file and NOT a directory\n");
		exit(1);
	}
	
	// lets traverse the directory entries in all three blocks
	for (i=0; i<3; i++) {
		if (blocks[i]==0) continue;	// 0 means pointing at nothing
		
		readSFS(blocks[i],(char *)_directory_entries);	// lets read a directory entry; notice the cast
		
		// so, we got four possible directory entries now		
		for (j=0; j<4; j++) {
			if (_directory_entries[j].F=='0') continue;	// means unused entry

			e_inode = stoi(_directory_entries[j].MMM,3);	// this is the inode that has more info about this entry
			
			if(strcmp(fname, _directory_entries[j].fname) == 0) // found the file/directory being searched for
				return e_inode;	// we send the inode and let the function decide what it is
		}
	}

	return -1;
}

// Task-1
void display_file(char *fname){
	int i, f_blocks[3];
	char buffer[1024];
	
	int e_inode = check_valid(fname, CD_INODE_ENTRY);
	
	if(_inode_table[e_inode].TT[0]=='F'){ // that is given inode corresponds to a file
		int check = 0;
		f_blocks[0] = stoi(_inode_table[e_inode].XX, 2);
		f_blocks[1] = stoi(_inode_table[e_inode].YY, 2);
		f_blocks[2] = stoi(_inode_table[e_inode].ZZ, 2);

		for(i=0; i<3; i++){
			if((f_blocks[i] != 0) && (readSFS(f_blocks[i], buffer))){
				printf("%s", buffer);
			}
		}
		printf("\n");
	}else{
		printf("Error: File not found\n"); 
	}

	return;
}

void create_file(char *fname) {
	char itype;
	int blocks[3], file_blocks[3] = {-1};
	_directory_entry _directory_entries[4];		
	char buffer[1024];
	
	int i,j;
	
	int empty_dblock=-1,empty_dentry=-1;
	int empty_ientry;
	
	// non-empty name
	if (strlen(fname)==0) {
		printf("Usage: create <file name>\n");
		return;
	}
	
	// do we have free inodes
	if (free_inode_entries == 0) {
		printf("Error: Inode table is full.\n");
		return;
	}
	
	// read inode entry for current directory
	// in SFS, an inode can point to three blocks at the most
	itype = _inode_table[CD_INODE_ENTRY].TT[0];
	blocks[0] = stoi(_inode_table[CD_INODE_ENTRY].XX,2);
	blocks[1] = stoi(_inode_table[CD_INODE_ENTRY].YY,2);
	blocks[2] = stoi(_inode_table[CD_INODE_ENTRY].ZZ,2);
	
	// its a directory; so the following should never happen
	if (itype=='F') {
		printf("Fatal Error! Aborting.\n");
		exit(1);
	}
	
	// now lets try to see if the name already exists
	for (i=0; i<3; i++) {
		if (blocks[i]==0) { 	// 0 means pointing at nothing
			if (empty_dblock==-1) empty_dblock=i; // we can later add a block if needed
			continue;
		}	
		
		readSFS(blocks[i],(char *)_directory_entries); // lets read a directory entry; notice the cast
		
		// so, we got four possible directory entries now
		for (j=0; j<4; j++) {
			if (_directory_entries[j].F=='0') { // means unused entry
				if (empty_dentry==-1) { empty_dentry=j; empty_dblock=i; } // lets keep a note of it, just in case we have to create the new directory
				continue;
			}
			
			if (strncmp(fname,_directory_entries[j].fname, 252) == 0) { // compare with user given name
					printf("%.252s: Already exists.\n",fname); 
					return;
			}
		}
		
	}
	// so directory name is new	
	
	// if we did not find an empty directory entry and all three blocks are in use; then no new directory can be made
	if (empty_dentry==-1 && empty_dblock==-1) {
		printf("Error: Maximum directory entries reached.\n");
		return;
	}
	else { // otherwise
		bool asked_for_a_block = false;
		if (empty_dentry == -1) { // Great! didn't find an empty entry but not all three blocks have been used
			empty_dentry=0;
			
			if ((blocks[empty_dblock] = getBlock())==-1) {  // first get a new block using the block bitmap
				printf("Error: Disk is full.\n");
				return;
			}						
			
			asked_for_a_block = true;
			writeSFS(blocks[empty_dblock],NULL);	// write all zeros to the block (there may be junk from the past!)
			
			switch(empty_dblock) {	// update the inode entry of current dir to reflect that we are using a new block
				case 0: itos(_inode_table[CD_INODE_ENTRY].XX,blocks[empty_dblock],2); break;
				case 1: itos(_inode_table[CD_INODE_ENTRY].YY,blocks[empty_dblock],2); break;
				case 2: itos(_inode_table[CD_INODE_ENTRY].ZZ,blocks[empty_dblock],2); break;
			}
		}

		int word_count = 0;
		char c;
		bool err_flag = false;
		memset(buffer, 0, 1024);

		printf("(Max 3072 characters : hit ESC to end)\n"); 
		c = getchar(); // skipping the new line
		for(i=0,j=0; !err_flag; j++, word_count++){
			if((c = getchar()) == 13) c = 10; // carraige return problem
			if(c == 27){ // user pressed the escape character
				file_blocks[i] = getBlock(); 
				writeSFS(file_blocks[i], buffer); // writing the buffer to the disk
				break;
			}
			if(j < 1024)	buffer[j] = c;
			else if( i+1 > free_disk_blocks || i == 2 ){
				err_flag = true; // raise an error
			}else{
				file_blocks[i] = getBlock();
				writeSFS(file_blocks[i], buffer); // write the current block to the disk first
				memset(buffer, 0, 1024); // reset the buffer
				
				i++; j=0; // now we can use an extra block! Go on!
				buffer[j] = c;
			}
		}

		if(err_flag){
			int ch;
			while ((ch = getchar()) != '\n' && ch != EOF);

			printf("\nError: Not enough space on the disk.\n");
			switch(empty_dblock) {	//clean up the inode table
				case 0: itos(_inode_table[CD_INODE_ENTRY].XX,00,2); break;
				case 1: itos(_inode_table[CD_INODE_ENTRY].YY,00,2); break;
				case 2: itos(_inode_table[CD_INODE_ENTRY].ZZ,00,2); break;
			}
			// we don't need to clean the blocks explicitly as they will be cleaned while creating a new file/directory
			// just returning the blocks we asked for
			for(i=0; i<3; i++)	if(file_blocks[i] != -1) returnBlock(file_blocks[i]);
			if(asked_for_a_block) returnBlock(blocks[empty_dblock]);
			
			return;
		}
		// NOTE: all error checkings have already been done at this point. 
		printf("\n%d bytes saved.\n", word_count);

		empty_ientry = getInode();	// get an empty place in the inode table which will store info about blocks for this new directory
		readSFS(blocks[empty_dblock],(char *)_directory_entries);	// read block of current directory where info on this new directory will be written
		_directory_entries[empty_dentry].F = '1';			// remember we found which directory entry is unused; well, set it to used now
		strncpy(_directory_entries[empty_dentry].fname,fname,252);	// put the name in there
		itos(_directory_entries[empty_dentry].MMM,empty_ientry,3);	// and the index of the inode that will hold info inside this directory
		writeSFS(blocks[empty_dblock],(char *)_directory_entries);	// now write this block back to the disk

		strncpy(_inode_table[empty_ientry].TT,"FI",2);		// create the inode entry...first, its a file, so FI
		if(file_blocks[0] != -1)	itos(_inode_table[empty_ientry].XX,file_blocks[0],2);	// block used
		if(file_blocks[1] != -1)	itos(_inode_table[empty_ientry].YY,file_blocks[1],2);	// block used
		if(file_blocks[2] != -1)	itos(_inode_table[empty_ientry].ZZ,file_blocks[2],2);	// block used
		
		writeSFS(BLOCK_INODE_TABLE, (char *)_inode_table);	// phew!! write the inode table back to the disk
	}				
}

void erase_entry(char *fname, int curr_dir_inode){
	_directory_entry _directory_entries[4];		
	int blocks[3];

	int i, j;

	blocks[0] = stoi(_inode_table[curr_dir_inode].XX,2);
	blocks[1] = stoi(_inode_table[curr_dir_inode].YY,2);
	blocks[2] = stoi(_inode_table[curr_dir_inode].ZZ,2);

	int block_entries = 0;
	bool cleaned = false;
	for (i=0; i<3; i++, block_entries=0) {
		if (blocks[i]==0) continue;	// 0 means pointing at nothing
		
		readSFS(blocks[i],(char *)_directory_entries);	// lets read a directory entry; notice the cast
		
		for (j=0; j<4; j++) {
			if(_directory_entries[j].F=='0') continue;	// means unused entry
			if(strcmp(fname, _directory_entries[j].fname) == 0){ // found the entry being searched for
				_directory_entries[j].F = '0';
				writeSFS(blocks[i], (char *)_directory_entries);
				cleaned = true;
			}else {
				block_entries++; // we have come across an entry which is not the one being looked for
			}
		}

		if(cleaned){ // we've done what we came here for
			if(block_entries == 0){ // this block has nothing, return it for usage
				switch(i) {	//clean up the inode table
					case 0: itos(_inode_table[curr_dir_inode].XX,00,2); break;
					case 1: itos(_inode_table[curr_dir_inode].YY,00,2); break;
					case 2: itos(_inode_table[curr_dir_inode].ZZ,00,2); break;
				}	
				returnBlock(blocks[i]);
			}
			return;
		}
	}
}

void rm_file(char *fname, int curr_dir_inode){
	int e_inode, i, j;
	int blocks[3];

	_directory_entry _directory_entries[4];		

	e_inode = check_valid(fname, curr_dir_inode);
	if(e_inode == -1){
		printf("Error: %s not found.\n", fname);
		return;
	}
	
	blocks[0] = stoi(_inode_table[e_inode].XX, 2);
	blocks[1] = stoi(_inode_table[e_inode].YY, 2);
	blocks[2] = stoi(_inode_table[e_inode].ZZ, 2);

	if(_inode_table[e_inode].TT[0]=='D'){ // a directory is to be deleted, then remove sub directory
		
		for (i=0; i<3; i++) {
			if (blocks[i]==0) continue;	
			readSFS(blocks[i],(char *)_directory_entries);	// lets read a directory entry; notice the cast

			for (j=0; j<4; j++) {
				if(_directory_entries[j].F=='0') continue;	// means unused entry
				rm_file(_directory_entries[j].fname, e_inode); // remove the file/directory used recursively
			}
		}
		// by this point every entry in the subdirectory has been made '0'
	}

	for(i=0; i<3; i++)	returnBlock(blocks[i]); // returning the block to the system
		
	// making all the blocks point towards 00
	itos(_inode_table[e_inode].XX,00,2);
	itos(_inode_table[e_inode].YY,00,2);
	itos(_inode_table[e_inode].ZZ,00,2);
	writeSFS(BLOCK_INODE_TABLE, (char *)_inode_table); // and then writing the inode table

	returnInode(e_inode); // returning the inode
	erase_entry(fname, curr_dir_inode); // erase entry from the current directory

	return;
}

int builtin_command(char *command){
	char arg[252];

	if(strcmp(command, "exit")==0){
		exit(0);
	}

	if(strcmp(command, "ls")==0){
		ls();
		return 1;
	}
	if(strcmp(command, "rd")==0){
		rd();
		return 1;
	}
	if(strcmp(command, "cd")==0){
		scanf("%s", arg); 
		cd(arg);
		return 1; 
	}
	if(strcmp(command, "md")==0){
		scanf("%s", arg); 
		md(arg);
		return 1; 
	}
	if(strcmp(command, "stats")==0){
		stats();
		return 1;
	}
	return 0;
}

int main(int argc, char* argv[]){
	mountSFS();
	char command[16], fname[252]; 

	while(1){
		printPrompt();
		scanf("%s", command);

		if(builtin_command(command)==1)
			continue;

		if(strcmp(command, "display") == 0){
			scanf("%s", fname); // take the input file name
			display_file(fname);

		}else if(strcmp(command, "create") == 0){
			scanf("%s", fname); // take the input file name
			create_file(fname);

		}else if(strcmp(command, "rm") == 0){
			scanf("%s", fname); // take the input file name
			rm_file(fname, CD_INODE_ENTRY);

		}else{
			printf("Invalid command. $$$$ %s $$$$\n", command);

		}	

		// reset all the input containers
 		memset(command, 0, 16);
		memset(fname, 0, 252);
		fflush(stdout);
	}
	return 0;
}
