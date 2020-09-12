#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <time.h>


#define MAX_FILES 64
//Block codes
#define SUPERBlOCK_CODE 1
#define INODE_CODE 2
#define FILEEXTENT_CODE 3
#define FREE_CODE 4

#define MAGIC_NUMBER 0x44
#define NAMESIZE 8

//Inode indexes
#define FILESIZE_INDEX 2
#define FILEEXTENT_INDEX 3
#define FILENAME_INDEX 4
#define CREATE_TIME_INDEX 20
#define MODIFY_TIME_INDEX 50
#define ACCESS_TIME_INDEX 80
//Generic Block Indexes
#define BLOCKTYPE_INDEX 0
#define MAGICNUMBER_INDEX 1

//Superblock indexes
#define FIRSTFREEBLOCK_INDEX 2
#define NUMFILES_ONDISK_INDEX 3
#define INODES_START_INDEX 4

//Free Block indexes
#define NEXTFREEBLOCK_INDEX 2
#define DATASTART_INDEX 4
#define DATAPERBLOCK 252

//File Extent indexes
#define POINTER_INDEX 2
#define FILE_CONTENT 4

#define ZER0_NBYTE 0

#define AVAILABLE 1
#define NOT_AVAILABLE 0

#include "libDisk.h"
#include "libTinyFS.h"
#include "tinyFS.h"
#include "TinyFS_errno.h"

/* * * * * Global Variable initialization * * * * */
fileDescriptor disk;
char * diskName;
int diskSize;
fileDescriptor mountdisk = -1;
int firstOpenCall = 1;

int openFileNum = 0;
OpenFile resourceTable[MAX_FILES];                                 //number should match the number of inodes in superblock


/* * * * * Helper functions * * * * */
int fd_exists(fileDescriptor FD){
	int idx;
	for (idx = 0; idx <= MAX_FILES; idx++){
		if (FD == resourceTable[idx].fd){
			return idx;
		}
	}
	return -1;
}

int find_empty_entry(){
	int idx;
	for (idx = 0; idx < MAX_FILES; idx++){
		if (0 == resourceTable[idx].fd){
			return idx;
		}
	}
	return -1;
}

int count = 0;
uint8_t getFreeBlock(uint8_t firstFreeBlock, uint8_t prev){
	count ++;
   uint8_t  freeBlock[BLOCKSIZE];
   uint8_t freeAddr = (uint8_t)firstFreeBlock;
   if( readBlock(disk, freeAddr, freeBlock) <= -1) {
      return READBLOCK_ERR;
   }
	 if(freeBlock[NEXTFREEBLOCK_INDEX] == 0) {
		 if (readBlock(disk, prev, freeBlock) < 0){
			 return READBLOCK_ERR;
		 }
		 freeBlock[NEXTFREEBLOCK_INDEX] = 0;
		 if (writeBlock(disk, prev, freeBlock) < 0){
			 return WRITEBLOCK_ERR;
		 }
		 return firstFreeBlock;
   }
   return getFreeBlock(freeBlock[NEXTFREEBLOCK_INDEX], firstFreeBlock);
}

int  addToFreeBlockList(uint8_t soonTobFree){
	uint8_t superBlock[BLOCKSIZE];
	uint8_t firstFreeBlock[BLOCKSIZE];
	uint8_t freeBlock_address;
   uint8_t blockToFree[BLOCKSIZE];

	if (readBlock(disk, ZER0_NBYTE, superBlock) < 0){
		return READBLOCK_ERR;
	}

	//add to freeBlock list
	freeBlock_address = superBlock[FIRSTFREEBLOCK_INDEX];
	if (readBlock(disk, freeBlock_address, firstFreeBlock) < 0){
		return READBLOCK_ERR;
	}
	while (firstFreeBlock[NEXTFREEBLOCK_INDEX] != 0){
		freeBlock_address =  firstFreeBlock[NEXTFREEBLOCK_INDEX];
		if (readBlock(disk, freeBlock_address, firstFreeBlock) < 0){
			return READBLOCK_ERR;
		}
	}
	//at the end of list
	firstFreeBlock[NEXTFREEBLOCK_INDEX] = soonTobFree;
   if (writeBlock(disk, freeBlock_address, firstFreeBlock) < 0){
		return WRITEBLOCK_ERR;
	}
	//depopulate soonTobFree
   if(readBlock(disk, soonTobFree, blockToFree) < 0) {
      return READBLOCK_ERR;
   }

	blockToFree[BLOCKTYPE_INDEX] = FREE_CODE;
	blockToFree[POINTER_INDEX] = 0;
	if (writeBlock(disk, soonTobFree, blockToFree) < 0){
		return WRITEBLOCK_ERR;
	}
   return 1;
}

char * getCurrentTime(){
	time_t clk = time(NULL);
	return ctime(&clk);
}

/* * * * * Nine key API functions * * * * */
int tfs_mkfs(char *filename, int nBytes){
	char superblock[BLOCKSIZE];
	int file, i;
	//open
	file = openDisk(filename, nBytes);
	if (file < 0){
		return OPENDISK_ERR;
	}
   for(i = 0; i < BLOCKSIZE; i++) {//set all bytes in superblock to 0
      superblock[i] = 0x00;
   }
	//initiallize all data to be 0x00
	superblock[BLOCKTYPE_INDEX] = 0x01/*SUPERBlOCK_CODE*/;
	//setting magic numbers
	superblock[MAGICNUMBER_INDEX] = MAGICNUMBER;

	// initialize and write thte superblock and inodes
	superblock[FIRSTFREEBLOCK_INDEX] = 0x01; //Write this differently
	int count, count2, numBlocks;
   numBlocks = nBytes / BLOCKSIZE;
	for (count = 1; count < numBlocks; count++){
		uint8_t free_block[BLOCKSIZE];
		free_block[BLOCKTYPE_INDEX] = FREE_CODE;
		free_block[MAGICNUMBER_INDEX] = MAGICNUMBER;
		free_block[NEXTFREEBLOCK_INDEX] = count + 1;
		if (count == numBlocks - 1){
			free_block[NEXTFREEBLOCK_INDEX] = 0;
		}
		for (count2 = 3; count2 < BLOCKSIZE; count2++){
			free_block[count2] = 0x00;
		}
		if (writeBlock(file, count, free_block) < 0){
			return WRITEBLOCK_ERR;
		}
	}
   if( writeBlock(file, 0, superblock) < 0) {
      return WRITEBLOCK_ERR;
   }
	return file;
}


int tfs_mount(char *diskname){
	if (mountdisk != -1){
		return DISK_ALREADY_MOUNTED;
	}
   if((disk = openDisk(diskname, 0))< 0) {
      return OPENDISK_ERR;
   }
	uint8_t block[BLOCKSIZE];
	if (readBlock(disk, 0, block) <  0){
		return READBLOCK_ERR;
	}
	if (block[1] != 0x44){
		return INVALID_MAGIC_NUMBER;
	}
	mountdisk = 1;
	return 1;
}

fileDescriptor tfs_openFile(char *name) {
	uint8_t superblock[BLOCKSIZE];
	void * inode;
	char * dst;
   int i, filesOnDisk;
	fileDescriptor idx;
   if(readBlock(disk, 0, superblock) < 0) {
      return READBLOCK_ERR;
   }
   if(superblock[BLOCKTYPE_INDEX] != SUPERBlOCK_CODE) {
      return INVALID_BLOCK_CODE;
   }
   if(superblock[MAGICNUMBER_INDEX] != MAGIC_NUMBER) {
      return INVALID_MAGIC_NUMBER;
   }
   filesOnDisk = superblock[NUMFILES_ONDISK_INDEX];
   for(i = 0; i < filesOnDisk; i++) {
		 uint8_t inode_addr = superblock[INODES_START_INDEX + i];
		 if (readBlock(disk, inode_addr, inode) != 0){
			 return READBLOCK_ERR;
		 }
		 strncpy(dst, inode + 3, NAMESIZE);
		 if (strncmp(name, dst, NAMESIZE) == 0){
			 idx = find_empty_entry();
			 if (idx < 0){
				 return FILE_MAX_ERR;
			 }
			 resourceTable[idx].fd = idx;
			 resourceTable[idx].count = 0;
			 resourceTable[idx].inodeAddr = inode_addr;
			 return idx;
		 }
   }
   //If here: file does not exist, create new file
   //Format the Inode block, write to disk
   int j;
   size_t nameLen;
   uint8_t  inode_block[BLOCKSIZE];
	 uint8_t trash = 0xFF;
   uint8_t inodeAddr = getFreeBlock(superblock[FIRSTFREEBLOCK_INDEX], trash);
	 for (i = 0; i < BLOCKSIZE; i++){
		 inode_block[i] = 0x00;
	 }
   inode_block[BLOCKTYPE_INDEX] = INODE_CODE;
   inode_block[MAGICNUMBER_INDEX] = MAGICNUMBER;
   inode_block[FILESIZE_INDEX] = 0;
   nameLen = strlen(name);
   if (nameLen > 8 || nameLen < 1) {
      return INVALID_FILE_NAME;
   }
   memcpy(&inode_block[FILENAME_INDEX], name, nameLen);


	 //create time
	 char * current_time;
	 current_time = getCurrentTime();
	 memcpy(&inode_block[CREATE_TIME_INDEX], current_time, 24);

   if (writeBlock(disk, inodeAddr, inode_block) < 0){
      return WRITEBLOCK_ERR;
   }

	 if (firstOpenCall == 1){
		 for (i = 0; i < MAX_FILES; i++){
			 resourceTable[i].fd = 0;
		 }
		 firstOpenCall = 0;
	 }

   //Set Inode address in Superblock, write supeblock back
	 for (i = INODES_START_INDEX; i < MAX_FILES; i++){
       if(superblock[i] == 0x00) {
          superblock[i] = inodeAddr;
          if(writeBlock(disk, 0, superblock) < 0) {
             return WRITEBLOCK_ERR;
          }
          idx = find_empty_entry();
			 if (idx < 0){
				 return FILE_MAX_ERR;
			 }
			 resourceTable[idx].fd = idx;
			 resourceTable[idx].count = 0;
			 resourceTable[idx].inodeAddr = inodeAddr;
          return idx;
       }
	 }
	 return OPEN_FILE_ERR;
}

int tfs_unmount(void){
	if (mountdisk == -1){
		return NO_FILE_MOUNTED;
	}
	closeDisk(mountdisk);
	mountdisk = -1;
	return 1;
}

int tfs_closeFile(fileDescriptor FD){
	int location = fd_exists(FD);
	if (location == -1){
		return FILERESOURCE_ERR;
	}
	resourceTable[location].fd = 0;
	return 1;
}


int tfs_writeFile(fileDescriptor FD,char *buffer, int size){

	//first check file table/structure if fd exists
   uint8_t inodeBlock[BLOCKSIZE];
	uint8_t free_block;
	uint8_t firstFreeBlock;
	uint8_t block;
	uint8_t superBlock[BLOCKSIZE];
	uint8_t fileExtentBlock[BLOCKSIZE];
	uint8_t fileextent_addr;
	int location = fd_exists(FD);
	if (location == -1){
		return FILERESOURCE_ERR;
	}
   if(readBlock(disk, resourceTable[location].inodeAddr, inodeBlock) < 0 ) {
      return READBLOCK_ERR;
   }
	if (readBlock(disk, ZER0_NBYTE, superBlock) < 0){
	    return READBLOCK_ERR;
	}
	firstFreeBlock = superBlock[FIRSTFREEBLOCK_INDEX];

	if (inodeBlock[FILESIZE_INDEX] != 0){
		 //address of the next file extent block
		 fileextent_addr = inodeBlock[FILEEXTENT_INDEX];
		 //read block
		 if (readBlock(disk, fileextent_addr, fileExtentBlock) < 0){
			 return READBLOCK_ERR;
		 }
		 while (fileExtentBlock[POINTER_INDEX] != 0){
			 fileextent_addr = fileExtentBlock[POINTER_INDEX];
			 if (readBlock(disk, fileextent_addr, fileExtentBlock) < 0){
				 return READBLOCK_ERR;
			 }
			 //call addToFreeBlockList
			 addToFreeBlockList(fileextent_addr);
		 }
	 }
	 int count = (size + DATAPERBLOCK - 1 ) / DATAPERBLOCK;
	 int i;
	 int prev_address;
	 int remaining_size;
	 for(i= 0; i < count; i++) {
		if (i == 0){
         free_block = getFreeBlock(firstFreeBlock, 0xFF);
         fileExtentBlock[BLOCKTYPE_INDEX] = FILEEXTENT_CODE;
         fileExtentBlock[MAGICNUMBER_INDEX] = MAGIC_NUMBER;
         inodeBlock[FILEEXTENT_INDEX] = free_block;
         memcpy(&fileExtentBlock[FILE_CONTENT], buffer + (DATAPERBLOCK * i), DATAPERBLOCK);
		}
		else if (i > 0 && i < count -1){
         prev_address = free_block;
         free_block = getFreeBlock(firstFreeBlock, 0xFF);
         if(writeBlock(disk, prev_address, fileExtentBlock) < 0) {
            return WRITEBLOCK_ERR;
         }
         fileExtentBlock[BLOCKTYPE_INDEX] = FILEEXTENT_CODE;
         fileExtentBlock[MAGICNUMBER_INDEX] = MAGIC_NUMBER;
         fileExtentBlock[POINTER_INDEX] = free_block;
         memcpy(&fileExtentBlock[FILE_CONTENT], buffer + (DATAPERBLOCK * i), DATAPERBLOCK);
      }
      else{
         remaining_size = size - i * DATAPERBLOCK;
         prev_address = free_block;
         free_block = getFreeBlock(firstFreeBlock, 0xFF);
         if( writeBlock(disk, prev_address, fileExtentBlock) < 0) {
            return WRITEBLOCK_ERR;
         }
         fileExtentBlock[BLOCKTYPE_INDEX] = FILEEXTENT_CODE;
         fileExtentBlock[MAGICNUMBER_INDEX] = MAGIC_NUMBER;
         fileExtentBlock[POINTER_INDEX] = 0;
         memcpy(&fileExtentBlock[FILE_CONTENT], buffer + (DATAPERBLOCK * i), remaining_size);
         //writeBlock(disk, free_block, fileExtentBlock);
      }
      if( writeBlock(disk, free_block, fileExtentBlock) < 0) {
         return WRITEBLOCK_ERR;
      }
	 }
		char * current_time;
		current_time = getCurrentTime();
		memcpy(&inodeBlock[MODIFY_TIME_INDEX], current_time, 24);
		memcpy(&inodeBlock[ACCESS_TIME_INDEX], current_time, 24);
    if(writeBlock(disk, resourceTable[location].inodeAddr, inodeBlock) < 0 ) {
       return WRITEBLOCK_ERR;
    }
	//Set the file pointer to 0
	resourceTable[location].count = 0;
	//success code
	return 1;
}


int tfs_deleteFile(fileDescriptor FD){
	//first check file table/structure if fd exists
	uint8_t inodeBlock[BLOCKSIZE];
	uint8_t fileExtentBlock[BLOCKSIZE];
	int location = fd_exists(FD);
	uint8_t fileextent_addr;
	if (location == -1){
		//fd doesn't exist error code
		FILERESOURCE_ERR;
	}
	if (tfs_closeFile(FD) == -1){
		//fd doesn't exist error code
		return CLOSE_FILE_ERR;
	}
	if (readBlock(disk, resourceTable[location].inodeAddr, inodeBlock) < 0){
		return READBLOCK_ERR;
	}
	fileextent_addr = inodeBlock[FILEEXTENT_INDEX];
   if(fileextent_addr != 0) {
      if(readBlock(disk, fileextent_addr, fileExtentBlock) < 0) {
         return READBLOCK_ERR;
      }
      while (fileExtentBlock[POINTER_INDEX] != 0){
         fileextent_addr = fileExtentBlock[POINTER_INDEX];
         if (readBlock(disk, fileextent_addr, fileExtentBlock) < 0){
            return READBLOCK_ERR;
         }
         //call addToFreeBlockList
         addToFreeBlockList(fileextent_addr);
      }
   }
	return 1;
}


int tfs_readByte(fileDescriptor FD, char *buffer){
	uint8_t inodeBlock[BLOCKSIZE];
	uint8_t fileExtentBlock[BLOCKSIZE];
	uint8_t fileextent_addr;
   int i;
	//first check file table/structure if fd exists
	int location = fd_exists(FD);
	if (location == -1){
		//fd doesn't exist error code
		return FILERESOURCE_ERR;
	}
   if (readBlock(disk, resourceTable[location].inodeAddr, inodeBlock) < 0){
      return READBLOCK_ERR;
   }
   if (inodeBlock[FILESIZE_INDEX] <= resourceTable[location].count){
      //pointer is already at the end of file error
      return CURSOR_END_FILE;
   }
   if(inodeBlock[FILESIZE_INDEX] == 0) {
      return NO_FILE_ERR;
   }
   int count = (resourceTable[location].count + DATAPERBLOCK - 1 ) / DATAPERBLOCK;
   fileextent_addr  = inodeBlock[FILEEXTENT_INDEX];
   for (i = 0; i < count; i++){
      if (readBlock(disk, fileextent_addr, fileExtentBlock) < 0){
         return READBLOCK_ERR;
      }
      fileextent_addr = fileExtentBlock[POINTER_INDEX];
   }
   strncpy(buffer,&fileExtentBlock[resourceTable[location].count - DATAPERBLOCK * count], 1);
   //increment file pointer
   resourceTable[location].count += 1;
	 char * current_time;
	 current_time = getCurrentTime();
	 memcpy(&inodeBlock[ACCESS_TIME_INDEX], current_time, 24);
	 if (writeBlock(disk, resourceTable[location].inodeAddr, inodeBlock) < 0){
		 return WRITEBLOCK_ERR;
	 }
	return 1; //success tfs code
}


int tfs_seek(fileDescriptor FD, int offset){
	int location = fd_exists(FD);
	if (location == -1){
		return FILERESOURCE_ERR;
	}
	else{
		resourceTable[location].count = offset;
	}
	return 1;
}


char * return_creation_time(fileDescriptor FD){
	char * output;
	int location = fd_exists(FD);
	uint8_t inodeaddr = resourceTable[location].inodeAddr;
	uint8_t inodeBlock[BLOCKSIZE];
	readBlock(disk, inodeaddr, inodeBlock);
	printf("here 2");
	memcpy(output, &inodeBlock[CREATE_TIME_INDEX], 24);
	printf("%s\n", output);
	return output;
}
