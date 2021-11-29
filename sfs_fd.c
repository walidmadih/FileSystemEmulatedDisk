#include "sfs_api.h"
#include <stdlib.h>
#include <stdio.h>

struct fd_entry* fd_table;


struct fd_entry* get_fd_entry(int index);
int create_new_fd_entry(int inode_number);
int initialize_fd_entry(int index, int inode_number);

void initialize_fd_table(){
    debug_print("Initializing file descriptor table with %d entries.\n", inode_count);
    fd_table = (struct fd_entry*) malloc(sizeof(struct fd_entry) * inode_count);

    for(int i = 0; i < inode_count; i++){
        struct fd_entry entry = {-1, -1, 1};
        fd_table[i] = entry;
    }
    debug_print("%s\n", "File descriptor table has been initialized.");
}

int get_fd(int inode_number){
    debug_print("Looking for a file descriptor mapped to i-node: %d...\n", inode_number);
    struct fd_entry* entry;
    for(int i = 0; i < inode_count; i++){
        entry = get_fd_entry(i);
        if((!(entry->available)) && (entry->inode_number == inode_number)){
            debug_print("File descriptor found, the fd is %d for i-node: %d.\n", i, inode_number);
            return i;
        }
    }
    return create_new_fd_entry(inode_number);
}

int remove_fd(int inode_number){
     struct fd_entry* entry;
    for(int i = 0; i < inode_count; i++){
        entry = get_fd_entry(i);
        if(entry->inode_number == inode_number){
            if(entry->available){
                return 0;
            }else{
                return -1;
            }
        }
    }
    return 0;
}


int create_new_fd_entry(int inode_number){
    struct fd_entry* entry;
    for(int i = 0; i < inode_count; i++){
        entry = get_fd_entry(i);
        if(entry->available){
            int fd = initialize_fd_entry(i, inode_number);
            debug_print("File descriptor not found, creating fd  %d for i-node: %d.\n", i, inode_number);
            return fd;
        }
    }
    debug_print("%s\n", "No fd entries available, exiting...");
    exit(1);
    return -1;
}

int initialize_fd_entry(int index, int inode_number){
    struct fd_entry* entry = get_fd_entry(index);
    entry->inode_number = inode_number;
    entry->filepointer = 0;
    entry->available = 0;
    return index;
}

struct fd_entry* get_fd_entry(int index){
    return &fd_table[index];
}