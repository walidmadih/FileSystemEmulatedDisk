#include "sfs_api.h"
#include "sfs_inode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct dir_entry* directory;
struct inode* root;


int next_file_index = 0;
int dir_filepointer = 0;

int get_size();
struct dir_entry* get_directory_entry(int index);
int create_new_entry(char* filename);
int initialize_entry(int index, char* filename);
void update_directory_entry(int index);


void initialize_new_directory(){
    debug_print("%s\n", "Initializing a new directory");
    root = get_inode(ROOT_INODE_NUMBER);
    directory = (struct dir_entry*) malloc(sizeof(struct dir_entry) * inode_count);

    for(int i = 0; i < inode_count; i++){
        struct dir_entry entry = {"", -1, 1};
        directory[i] = entry;
    }
    debug_print("Directory has been initialized, maximum file count: %d\n", inode_count);
}

void load_directory(){
    debug_print("%s\n", "Loading the old directory.");
    root = get_inode(ROOT_INODE_NUMBER);
    directory = (struct dir_entry*) malloc(sizeof(struct dir_entry) * inode_count);

    for(int i = 0; i < inode_count; i++){
        struct dir_entry entry = {"", -1, 1};
        directory[i] = entry;
    }

    int files_found = 0;
    for(int i = 0; i < get_size(); i ++){

        char* buffer = (char*) malloc(FILENAMEBYTES + 2*sizeof(int));
        read_from_inode(ROOT_INODE_NUMBER, buffer, &dir_filepointer, FILENAMEBYTES + 2*sizeof(int));
        char* name = (char*) malloc(FILENAMEBYTES);
        int inode_number;
        int available;
        debug_print("The directory loaded buffer is: '%s'\n.", buffer);
        memcpy(name, buffer, FILENAMEBYTES);
        memcpy(&inode_number, &buffer[FILENAMEBYTES], sizeof(int));
        memcpy(&available, &buffer[FILENAMEBYTES + sizeof(int)], sizeof(int));
        free(buffer);

        struct dir_entry* entry = get_directory_entry(i);
        entry->filename = name;
        entry->inode_number = inode_number;
        entry->available = available;
        if(!entry->available){
            files_found += 1;
            debug_print("A directory entry has been found: index: %d\t name: '%s'\ti-node number: %d\t available: %d\n", i, entry->filename, entry->inode_number, entry->available);
            debug_print("The associated dir_index: %d\ti-node: %d\t size: %d.\n", i, entry->inode_number, get_inode(inode_number)->size);
            debug_print("Loaded file '%s' with i-node number %d\n", entry->filename, entry->inode_number);
        }
    }
    debug_print("A total of %d files have been recovered.\n", files_found);
}

void update_directory_entry(int index){
    debug_print("Updating directory entry in disk, directory entry index %d.\n", index);
    struct dir_entry* entry = get_directory_entry(index);
    int filepointer = index * (FILENAMEBYTES + 2*sizeof(int));
    debug_print("The filpointer is %d.\n", filepointer);

    char* buffer = malloc(FILENAMEBYTES + 2 * sizeof(int));
    memcpy(buffer,entry->filename, FILENAMEBYTES);
    int int_buff[2];
    int_buff[0] = entry->inode_number;
    int_buff[1] = entry->available;
    memcpy(&buffer[FILENAMEBYTES], int_buff, 2*sizeof(int));

    debug_print("The buffer has been setup %s.\n", buffer);
    write_to_inode(ROOT_INODE_NUMBER, buffer, &filepointer, FILENAMEBYTES + 2*sizeof(int));
    free(buffer);
    debug_print("Directory entry index %d has been updated in the disk.\n", index);
}

// TODO: Ensure the size matches whatever we are writing to the disk
int get_size(){
    int size = (root->size)/(FILENAMEBYTES + sizeof(int) * 2);
    return size;
}

struct dir_entry* get_directory_entry(int index){
    return &directory[index];
}

// Get i_node number
int get_inode_number(char* filename, int create_new_flag){
    debug_print("Retrieving inode number for file %s. Current directory size %d.\n", filename, get_size());
    for(int i = 0; i < get_size(); i++){
        struct dir_entry* entry = get_directory_entry(i);
        if(!entry->available && strcmp(entry->filename, filename) == 0){
            debug_print("File %s exists in the directory, the associated i-node number is %d.\n", filename, entry->inode_number);
            return entry->inode_number;
        }
    }
    if(create_new_flag){
        return create_new_entry(filename);
    }else{
        return -1;
    }
}

int create_new_entry(char* filename){
    debug_print("File %s not found in the directory, creating a directory entry...\n", filename);
    struct dir_entry* entry;
    for(int i = 0; i < get_size(); i++){
        entry = get_directory_entry(i);
        if(entry->available){
            debug_print("Found a usable dir_entry slot for file %s. Initializing a directory entry...\n", filename);
            return initialize_entry(i, filename);
        }
    }
    debug_print("No usable dir_entry slot found for file %s. Initializing a new directory entry...\n", filename);
    debug_print("Current directory size %d\n", get_size());
    int inode_number = initialize_entry(get_size(), filename);
    debug_print("New directory size %d\n", get_size());

    return inode_number;

}

int initialize_entry(int index, char * filename){
    debug_print("Initialzing a directory entry for file %s at directory index %d...\n", filename, index);
    char* name =  (char*) malloc(FILENAMEBYTES);
    strcpy(name, filename);

    struct dir_entry* entry = get_directory_entry(index);
    entry->inode_number = get_free_inode_number();
    if(entry->inode_number == -1){
        return -1;
    }
    entry->filename =  name;
    entry->available = 0;

    debug_print("%s\n", "Writing the initialized entry to the disk...");

    update_directory_entry(index);
    /*
    char* buffer = (char*) malloc(FILENAMEBYTES);
    read_from_inode(ROOT_INODE_NUMBER, buffer, &initial_dir_filepointer, FILENAMEBYTES);
    debug_print("TEST: The filename read is ---> '%s'\n", buffer);
    free(buffer);
    int buffer2;
    read_from_inode(ROOT_INODE_NUMBER, &buffer2, &initial_dir_filepointer, sizeof(int));
    debug_print("TEST: The inode-number read is ---> '%d'\n", buffer2);
    */
    return entry->inode_number;
}

//TODO: This is most likely not right, we a new file could have been added at a previous index. It will do for now.
int get_next_file_in_dir(char* name){
    for(next_file_index; next_file_index < get_size(); next_file_index++){
        struct dir_entry* entry = get_directory_entry(next_file_index);
        if(!entry->available){
            debug_print("Next file in the dir is '%s', directory index: %d\tdirectory size: %d.\n", entry->filename, next_file_index, get_size());
            strcpy(name, entry->filename);
            next_file_index++;
            return 1;
        }
    }
    next_file_index = next_file_index % get_size();
    name =  (char*) malloc(FILENAMEBYTES);
    strcpy(name, "\n");
    debug_print("%s\n", "End of directory.");
    return 0;
}

// TODO: the root might no longer be using an allocated data blocks, we may way to free those up, but its not 100% necessary
void remove_dir_entry(char* filename){
    struct dir_entry* entry;
    for(int i = 0; i < get_size(); i++){
        entry = get_directory_entry(i);
        if(strcmp(entry->filename, filename) == 0){
            int inode_number = entry->inode_number;
            remove_inode(inode_number);
            entry->available = 1;
            entry->filename = "";
            entry->inode_number = -1;
            update_directory_entry(i);
            return;
        }
    }
}
