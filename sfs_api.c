#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sfs_api.h"
#include "disk_emu.h"
#include "sfs_inode.h"
#include "sfs_dir.h"
#include "sfs_fd.h"
#include "sfs_free_manager.h"


void initialize_super_block(){
    
    int file_system_size = TOTALBLOCKCOUNT * BLOCKSIZE;
    int superblock[5];
    superblock[0] = 0xACBD0005;
    superblock[1] = BLOCKSIZE;
    superblock[2] = file_system_size;
    superblock[3] = inode_count;
    superblock[4] = ROOT_INODE_NUMBER;

    int superblock_address = 0;

    write_block_at_byte(superblock_address, superblock, 0, 5*sizeof(int));
}

void make_new_disk(){
    init_fresh_disk(DISKNAME, BLOCKSIZE, TOTALBLOCKCOUNT);
    initialize_super_block();
    new_free_bmap();
    initialize_all_inodes();
    initialize_new_directory();
    initialize_fd_table();
}

void load_old_disk(){
    //TODO: Technically, I believe this should be loaded from the super block,
    // but I'm not sure how to do that since init_disk is the one which opens the file for reading...
    init_disk(DISKNAME, BLOCKSIZE, TOTALBLOCKCOUNT);
    load_free_bmap();
    load_all_inodes();
    load_directory();
    initialize_fd_table();
}

/*
*   PROVIDED INTERFACE
*/
void mksfs(int new_disk_flag){
    debug_print("%s\n", "Initializing Disk...");
    inode_count = (INODEBLOCKCOUNT * BLOCKSIZE) / sizeof(struct inode);
    if(new_disk_flag){
        make_new_disk();
}else{
        load_old_disk();
    }
    debug_print("%s\n\n\n", "Disk has been initialized.");
}

int sfs_getnextfilename(char* filename){
    return get_next_file_in_dir(filename);
}

int sfs_getfilesize(const char* filename){
    char* name = (char*) malloc(strlen(filename));
    strcpy(name, filename);
    int inode_number = get_inode_number(name, 0);
    if(inode_number < 0){
        return -1;
    }
    int size = get_inode(inode_number)->size;
    return size;
}

int sfs_fopen(char* filename){
    debug_print("Opening file %s\n", filename);
    if(strlen(filename) > MAXFILENAME){
        printf("WARNING: The file name '%s' is %d character long. The max character count is %d\n", filename, strlen(filename), MAXFILENAME);
        return -1;
    }
    int inode_number = get_inode_number(filename, 1);
    if(inode_number == -1){
        printf("WARNING: not enough i-nodes: %d, allocate more space for i-node blocks and try again.\n", inode_count);
    }
    debug_print("File '%s' has been opened i-node: %d\n", filename, inode_number);
    int fd = get_fd(inode_number);
    debug_print("File has been opened %s\n\n\n", filename);
    return fd;
}

int sfs_fclose(int fd){
    if(fd < 0){
        printf("WARNING: Invalid file descriptor %d, check the i-node count\n", fd);
        return -1;
    }
    debug_print("\n\nClosing fd_entry: %d\n", fd);
    struct fd_entry* entry = get_fd_entry(fd);
    if(entry->available){
        debug_print("%s\n", "WARNING: Entry is not open.");
        return -1;
    }
    entry->inode_number = -1;
    entry->filepointer = -1;
    entry->available = 1;

    debug_print("The file descriptor %d has been closed, available status %d.\n", fd, entry->available);
    return 0;
}

int sfs_fwrite(int fd, const char* buffer, int length){
     if(fd < 0){
        printf("WARNING: Invalid file descriptor %d, check the i-node count\n", fd);
        return -1;
    }
    debug_print("\n\nWriting to file with fd %d\n", fd);

    debug_print("%s\n", "Fetching fd entry...");
    struct fd_entry* entry = get_fd_entry(fd);
    debug_print("%s\n", "Entry has been fetched.");
    if(entry->available){
        debug_print("%s\n", "WARNING: Entry has not been opened.");
        return -1;
    }
    
    int inode_number = entry->inode_number;

    int* filepointer = (int*) malloc(sizeof(int));
    *filepointer = entry->filepointer;
    debug_print("Writing to inode numbered %d\n", inode_number);
    int bytes_written = write_to_inode(inode_number, buffer, filepointer, length);
    debug_print("Write complete %d, %d bytes have been written.\n", inode_number, bytes_written);
    debug_print("Updating filepointer from %d to %d\n", entry->filepointer, *filepointer);
    entry->filepointer = *filepointer;
    free(filepointer);
    return bytes_written;
}

int sfs_fread(int fd, char* buffer, int length){
    if(fd < 0){
        printf("WARNING: Invalid file descriptor %d, check the i-node count\n", fd);
        return -1;
    }
    debug_print("\n\nReading from file with fd %d\n", fd);
    struct fd_entry* entry = get_fd_entry(fd);
    if(entry->available){
        debug_print("%s\n", "WARNING: Entry has not been opened.");
        return -1;
    }

    int inode_number = entry->inode_number;
    int* filepointer = (int*) malloc(sizeof(int));
    *filepointer = entry->filepointer;
    debug_print("Reading from inode numbered %d\n", inode_number);
    int bytes_read = read_from_inode(inode_number, buffer, filepointer, length);
    debug_print("Updating filepointer from %d to %d\n", entry->filepointer, *filepointer);
    entry->filepointer = *filepointer;
    free(filepointer);

    return bytes_read;
}

int sfs_fseek(int fd, int loc){
    if(fd < 0){
        printf("WARNING: Invalid file descriptor %d, check the i-node count\n", fd);
        return -1;
    }
    debug_print("Fseek on fd %d to location %d\n", fd, loc);
    struct fd_entry* entry = get_fd_entry(fd);
    if(entry->available){
        debug_print("%s\n", "WARNING: Entry has not been opened.");
        return -1;
    }
    entry->filepointer = loc;
    debug_print("Fseek completed succesffuly, new location of fd_entry is %d.\n", entry->filepointer);
    return 0;
}

int sfs_remove(char* filename){
    debug_print("\n\nRemoving file '%s'.\n", filename);
    int inode_number = get_inode_number(filename, 0);
    debug_print("File associated with inode_number: %d.\n", inode_number);
    if(inode_number < 0){
        debug_print("File '%s' does not exist, returning -1.\n", filename);
        return -1;
    }
    if(remove_fd(inode_number) == -1){
        debug_print("WARNING: The file '%s' is currently open.\n", filename);
        return -1;
    }
    remove_dir_entry(filename);
    return 0;
}