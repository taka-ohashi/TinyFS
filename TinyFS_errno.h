//
//
//libdisk error macros
#define NOFILENAME -1 //No file name provided
#define FOPENERR_WR -2 //existing file name cannot be opened for read/write
#define BYTENUM_ERR -3 //Number of bytes provied less then blocksize, less than 0, or larger than  than max disk size supported
#define FILECRT_ERR -4 //File cannot be created with read/write permissions
#define WRITEDISK_ERR -5 //write to disk not successful
#define READSEEK_ERR -6 //Seek to file spot on read unsucessful
#define READDISK_ERR -7 //Error when reading the disk
#define WRITESEEK_ERR -8 //Seek to file spon on write unsucessful

//libTinyFS error macors
#define OPENDISK_ERR -9 //Error when opening disk
#define WRITEBLOCK_ERR -10 //Error when writing to disk
#define READBLOCK_ERR -11 //Error when reading to disk
#define FILERESOURCE_ERR -12 //FD not in resource table
#define DISK_ALREADY_MOUNTED -13 //A disk is already mounted
#define INVALID_MAGIC_NUMBER -14 //Magic number not valid
#define INVALID_BLOCK_CODE -15 //Block code does not match intended code
#define FILE_MAX_ERR -16 //Max number of files, no more allowed
#define INVALID_FILE_NAME -17 //Invalid file name provided
#define OPEN_FILE_ERR -18 //Error when opening a file
#define CLOSE_FILE_ERR -19 //Error when closing a file
#define NO_FILE_MOUNTED -20 //no file is mounted
#define CURSOR_END_FILE -21 //Cursor is at end of file
#define NO_FILE_ERR -22 //NO file when trying to read byte
