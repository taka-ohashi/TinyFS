#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "libDisk.h"
#include "TinyFS_errno.h"
#include "tinyFS.h"

int openDisk(char *filename, int nBytes){
	int file;
	int floor, allocate, x;
	if (filename == NULL){
		return NOFILENAME;
	}
	else if (nBytes == 0){
		file = open(filename, O_RDWR);
      if(file <= 0) {
         return FOPENERR_WR;
      }
		return file;
	}
	else if (nBytes < BLOCKSIZE){
		return BYTENUM_ERR;
	}
	else{ /*nBytes > BLOCKSIZE*/
		floor = nBytes / BLOCKSIZE;
		allocate = floor * BLOCKSIZE;
		file = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
      if(file < 0) {
         return FILECRT_ERR;
      }
		for (x = 0; x < allocate; x++){
			if( write(file, "\0", 1) < 0) {
            return WRITEDISK_ERR;
         }
		}
	}
	return file;
}

void closeDisk(int disk){
	close(disk);
}

int readBlock(int disk, int bNum, void *block){
	int factor;
	if (bNum < 0){
		return BYTENUM_ERR;
	}
	else if (fcntl(disk, F_GETFD) == -1){
		return -2;
	}
	else{ /* bNum == n */
		factor = bNum * BLOCKSIZE;
		if (lseek(disk, factor, SEEK_SET) < 0){
			return READSEEK_ERR;
		};
		if (read(disk, block, BLOCKSIZE) == -1){
			return READDISK_ERR;
		};
		return 0;
	}
	return 0;
}


int writeBlock(int disk, int bNum, void *block){
	int factor;
	if (bNum < 0){
		return BYTENUM_ERR;
	}
	else if (fcntl(disk, F_GETFD) == -1){
		return -3;
	}
	else{ /* bNum == n */
		factor = bNum * BLOCKSIZE;
		if (lseek(disk, factor, SEEK_SET) < 0){
			return WRITESEEK_ERR;
		};
		if (write(disk, block, BLOCKSIZE) == -1){
			return WRITEDISK_ERR;
		};
		return 0;
	}
	return 0;
}

