//#define SFS_DEBUG

#ifdef SFS_DEBUG
#define debug_print(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define debug_print(fmt, ...) do {;} while (0);
#endif

#ifndef SFS_API_H
#define SFS_API_H


// 16 with '\0'
#define MAXFILENAME 32
#define FILENAMEBYTES (MAXFILENAME + 1)
#define DISKNAME "file.sfs"

#define BLOCKSIZE 1024

#define SUPERBLOCKCOUNT 1
#define DATABLOCKCOUNT 1024
#define INODEBLOCKCOUNT 128
#define BITMAPBLOCKCOUNT 10 // MUST BE GREATER THAN DATABLOCKCOUNT / (BLOCKSIZE / 4)

#define TOTALBLOCKCOUNT SUPERBLOCKCOUNT + DATABLOCKCOUNT + INODEBLOCKCOUNT + BITMAPBLOCKCOUNT

#define ROOT_INODE_NUMBER 0
#define FIRSTDATABLOCKADDRESS INODEBLOCKCOUNT + 1
#define FIRSTBITMAPBLOCKADDRESS SUPERBLOCKCOUNT + INODEBLOCKCOUNT + DATABLOCKCOUNT

struct bmap_entry{
    int block_address;
    int available;
}bmap_entry;

struct fd_entry{
    int inode_number;
    int filepointer;
    int available;
} fd_entry;

struct dir_entry{
    char* filename;
    int inode_number;
    int available;
} dir_entry;

struct inode{
    int size;
    int direct_addresses[12];
    int indirect_address;
    int available;
} inode;

int inode_count;

void mksfs(int);

int sfs_getnextfilename(char*);

int sfs_getfilesize(const char*);

int sfs_fopen(char*);

int sfs_fclose(int);

int sfs_fwrite(int, const char*, int);

int sfs_fread(int, char*, int);

int sfs_fseek(int, int);

int sfs_remove(char*);

#endif
