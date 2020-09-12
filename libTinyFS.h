#define BLOCKSIZE 256
#define DEFUALT_DISK_SIZE 10240
#define DEFUALT_DISK_NAME "tinyFSDisk"
#define MAGICNUMBER 0x44
#include "tinyFS.h"
typedef struct OpenFile{
   fileDescriptor fd;
   //int offset;
   int inodeAddr;
   int count;
}OpenFile;

/* * * * * Nine key API functions * * * * */
int tfs_mkfs(char *filename, int nBytes);
int tfs_mount(char *diskname);
int tfs_unmount(void);
fileDescriptor tfs_openFile(char *name);
int tfs_closeFile(fileDescriptor FD);
int tfs_writeFile(fileDescriptor FD,char *buffer, int size);
int tfs_deleteFile(fileDescriptor FD);
int tfs_readByte(fileDescriptor FD, char *buffer);
int tfs_seek(fileDescriptor FD, int offset);
char * return_creation_time(fileDescriptor FD);
