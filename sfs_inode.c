#include "sfs_inode.h"
#include "sfs_api.h"
#include "disk_emu.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sfs_free_manager.h"


int get_block_address(int inode_number, int* filepointer, int* blockpointer, int length);
int write_block_at_byte(int block_address, void* buffer, int blockpointer, int length);
int read_block_at_byte(int block_address, void* buffer, int blockpointer, int length);
void initialize_inode(struct inode* node);
void create_inode(int inode_number);
void load_inode(int inode_number);
void setup_root_inode();
void allocate_direct_blocks(int inode_number, int index);
void allocate_indirect_data_blocks(int inode_number, int blockpointer);
void allocate_indirect_block(int inode_number);

void update_inode_entry(int inode_number);
void read_inode_entry(int inode_number, int* buffer);

struct inode* inode_table;
int entry_int_size = 14;

void update_inode_entry(int inode_number){
    debug_print("%s\n", "/////////////////////////////");
    debug_print("Updating i-node %d in disk\n", inode_number);

    int first_inode_block = SUPERBLOCKCOUNT;
    int inode_per_block = BLOCKSIZE / sizeof(struct inode);
    int inode_block_index = inode_number / inode_per_block;

    int blockpointer = (inode_number - inode_block_index * inode_per_block) * sizeof(struct inode);
    int block_address = first_inode_block + inode_block_index;

    debug_print("i-node block pointer: %d\tblock address: %d\twith %d i-nodes per block\n", blockpointer, block_address, inode_per_block);

    struct inode* entry = get_inode(inode_number);
    int buffer[entry_int_size];
    buffer[0] = entry->size;
    for(int i = 0; i < 12; i++){
        buffer[1 + i] = entry->direct_addresses[i];  
    }
    buffer[13] = entry->indirect_address;
    buffer[14] = entry->available;
    write_block_at_byte(block_address, buffer, blockpointer, sizeof(struct inode));
    debug_print("i-node %d has been updated in the disk\n", inode_number);
    debug_print("%s\n", "/////////////////////////////");
}

void load_all_inodes(){
    inode_table = (struct inode*) malloc(sizeof(struct inode) * inode_count);
    for(int i = 0; i < inode_count; i++){
        load_inode(i);
    }
}

void load_inode(int inode_number){
    struct inode* entry = (struct inode*)malloc(sizeof(struct inode));
    int buffer[entry_int_size];
    read_inode_entry(inode_number, buffer);
    entry->size = buffer[0];
    for(int i = 0; i < 12; i++){
        entry->direct_addresses[i] = buffer[1 + i]; 
    }
    entry->indirect_address = buffer[13];
    entry->available = buffer[14];
    debug_print("i-node %d availability status %d.\n", inode_number, entry->available);
    if(!entry->available){
        debug_print("i-node %d has been loaded, current size %d.\n", inode_number, entry->size);
    }
    inode_table[inode_number] = *entry;
}

void read_inode_entry(int inode_number, int* buffer){
    debug_print("Loading i-node %d from the disk.\n", inode_number);
    int first_inode_block = SUPERBLOCKCOUNT;
    int inode_per_block = BLOCKSIZE / sizeof(struct inode);
    int inode_block_index = inode_number / inode_per_block;

    int blockpointer = (inode_number - inode_block_index * inode_per_block) * sizeof(struct inode);
    int block_address = first_inode_block + inode_block_index;
    read_block_at_byte(block_address, buffer, blockpointer, sizeof(struct inode));
}

void initialize_all_inodes(){
    inode_table = (struct inode*) malloc(sizeof(struct inode) * inode_count);
    
    for(int i = 0; i < inode_count; i++){
        create_inode(i);
    }
    setup_root_inode();
}

void setup_root_inode(){
    struct inode* root_inode = get_inode(ROOT_INODE_NUMBER);
    root_inode->direct_addresses[0] = FIRSTDATABLOCKADDRESS;
    root_inode->available = 0;
    update_inode_entry(ROOT_INODE_NUMBER);
}

void create_inode(int inode_number){
    struct inode* new_node = (struct inode*)malloc(sizeof(struct inode));
    initialize_inode(new_node);
    inode_table[inode_number] = *new_node;
    update_inode_entry(inode_number);
}

void initialize_inode(struct inode* node){
    node->size = 0;
    for(int i = 0; i < 12; i++){
        node->direct_addresses[i] = -1;
    }
    node->indirect_address = -1;
    node->available = 1;
}

struct inode* get_inode(int number){
    return &inode_table[number];
}

int get_block_address(int inode_number, int* filepointer, int* blockpointer, int length){
    debug_print("Retrieve block address for i-node %d at byte %d.\n", inode_number, *filepointer);
    struct inode* node = get_inode(inode_number);
    int block_address;
    if (*filepointer < BLOCKSIZE * 12){
        //Direct
        int index = *filepointer / BLOCKSIZE;

        allocate_direct_blocks(inode_number, index);

        block_address = node->direct_addresses[index];
        *blockpointer = *filepointer - (BLOCKSIZE * index);
        debug_print("Block pointer has been set to byte %d of the direct block at index %d.\n", *blockpointer, index);
    }else{


        if(node->indirect_address < 0){
            allocate_indirect_block(inode_number);
        }

        int indirect_file_pointer = *filepointer - BLOCKSIZE * 12;
        int indirect_index = indirect_file_pointer / BLOCKSIZE;
        int indirect_blockpointer = indirect_index * sizeof(int);


        if(indirect_blockpointer  >= BLOCKSIZE){
            return -1;
        }
        // THE POINTER GOT OVERWRITTEN 100%
        // first one came, claimed block 143, wrote some bits, 
        // and didnt write enough such that the next one's length wont satistfy this
        if(*filepointer + length > node->size){

            int stored_address;
            read_block_at_byte(node->indirect_address, &stored_address, indirect_blockpointer, sizeof(int));
            if(stored_address == 0){
                allocate_indirect_data_blocks(inode_number, indirect_blockpointer);
            }
        }

        read_block_at_byte(node->indirect_address, &block_address, indirect_blockpointer, sizeof(int));
        
        *blockpointer = indirect_file_pointer - indirect_index * BLOCKSIZE;
    }
    debug_print("Block address has been retrieved: %d.\n", block_address);
    return block_address;
}

int read_block_at_byte(int block_address, void* buffer, int blockpointer, int length){
    debug_print("Loading block at address %d to memory buffer.\n", block_address);
    // Load block
    char* block_data = malloc(BLOCKSIZE);
    // TODO: Returns read error... we may want to handle this
    read_blocks(block_address, 1, block_data);
    debug_print("Block at address %d has been loaded to memory.\n", block_address);

    debug_print("Moving pointer to block pointer %d.\n", blockpointer);
    // Point to file pointer
    void* start_reading_at = &(block_data[blockpointer]);
    debug_print("%s\n", "Block pointer has been moved.");
    

    
    int read_bytes = length;
    int bytes_left_in_block = BLOCKSIZE - blockpointer;
    debug_print("There are %d bytes left in block address %d, attempting to read %d bytes.\n", bytes_left_in_block, block_address, length);
    if(length > bytes_left_in_block){
        read_bytes = bytes_left_in_block;
        length -= bytes_left_in_block;
    }
    debug_print("Reading %d bytes from the block to the buffer.\n", read_bytes);
    memcpy(buffer, start_reading_at, read_bytes);
    free(block_data);
    return read_bytes;
}

int read_from_inode(int inode_number, char* buffer, int* filepointer, int length){
    debug_print("%s\n", "''''''''''''''''''''''''''''''''''''");
    debug_print("Reading from i-node %d at byte %d.\n", inode_number, *filepointer);
    int* blockpointer = malloc(sizeof(int));
    int block_address;
    int read_bytes;
    int max_length = get_inode(inode_number)->size - *filepointer;
    if(length > max_length){
        length = max_length;
    }

    int read_so_far = 0;
    do{
        block_address = get_block_address(inode_number, filepointer, blockpointer, length);
        // Max File Size Reached
        if(block_address < 0){
            return read_so_far;
        }
        read_bytes = read_block_at_byte(block_address, &buffer[read_so_far], *blockpointer, length);
        *filepointer += read_bytes;
        length -= read_bytes;
        read_so_far += read_bytes;
        debug_print("%d bytes left to read\n", length);
        debug_print("%s\n", "''''''''''''''''''''''''''''''''''''");
    }
    while(length > 0);
    debug_print("Finished reading from i-node %d with %d bytes left to read, a total of %d bytes have been read. File pointer has been moved to byte %d.\n", inode_number, length, read_so_far, *filepointer);
    return read_so_far;
}

int write_block_at_byte(int block_address, void* buffer, int blockpointer, int length){

    debug_print("Loading block at address %d to memory buffer.\n", block_address);
    // Load block
    char* block_data = malloc(BLOCKSIZE);
    // TODO: Returns read error... we may want to handle this
    read_blocks(block_address, 1, block_data);
    debug_print("Block at address %d has been loaded to memory.\n", block_address);

    debug_print("Moving pointer to block pointer %d.\n", blockpointer);
    // Point to file pointer
    void* start_writing_at = &(block_data[blockpointer]);
    debug_print("%s\n", "Block pointer has been moved.");
    

    
    int written_bytes = length;
    int bytes_left_in_block = BLOCKSIZE - blockpointer;
    debug_print("There are %d bytes left in block address %d, attempting to write %d bytes.\n", bytes_left_in_block, block_address, length);
    if(length > bytes_left_in_block){
        written_bytes = bytes_left_in_block;
    }

    debug_print("Copying %d bytes from the buffer into the in-memory block.\n", written_bytes);
    memcpy(start_writing_at, buffer, written_bytes);
    debug_print("%s\n", "Buffer content has been copied");

    // TODO: Returns write error... we may want to handle this
    debug_print("%s\n", "Writting the in-memory block back to the disk.");
    write_blocks(block_address, 1, block_data);
    debug_print("%s\n", "The in-memory block has been written to the disk.");
    free(block_data);
    return written_bytes;

}

int write_to_inode(int inode_number, char* buffer, int* filepointer, int length){
    debug_print("%s\n", "--------------------------------------");
    debug_print("Writing '%s' to i-node %d at byte %d. Attempting to write %d bytes.\n", (char*) buffer, inode_number, *filepointer, length);

    int* blockpointer = malloc(sizeof(int));
    int block_address;
    int written_bytes;

    int written_so_far = 0;

    do{
        block_address = get_block_address(inode_number, filepointer, blockpointer, length);
        // Max File Size Reached
        if(block_address < 0){
            return written_so_far;
        }
        written_bytes = write_block_at_byte(block_address, &buffer[written_so_far], *blockpointer, length);
        *filepointer += written_bytes;
        length -= written_bytes;
        written_so_far += written_bytes;
        debug_print("%d bytes left to write\n", length);
        debug_print("%s\n", "--------------------------------------");
    } while(length > 0);

    if(*filepointer > get_inode(inode_number)->size){
        get_inode(inode_number)->size = *filepointer;
        update_inode_entry(inode_number);
    }
    debug_print("Finished writing to i-node %d with %d bytes left to write, wrote a total of %d bytes. File pointer has been moved to byte %d.\n", inode_number, length, written_so_far, *filepointer);
    return written_so_far;
}

void allocate_direct_blocks(int inode_number, int index){
    struct inode* node = get_inode(inode_number);
    for(int i = 0; i <= index; i++){
        if(node->direct_addresses[i] == -1){
            debug_print("%s\n", "**********************");
            debug_print("Allocating a free data block to direct block address index %d of inode %d.\n", index, inode_number);
            node->direct_addresses[i] = get_free_data_block();
            update_inode_entry(inode_number);
            debug_print("Direct block address index %d of inode %d has been assigned the free data block %d.\n", i, inode_number, node->direct_addresses[index]);
            debug_print("%s\n", "**********************");
        }
    }
}

void allocate_indirect_block(int inode_number){
    struct inode* node = get_inode(inode_number);
    node->indirect_address = get_free_data_block();
    char buffer[BLOCKSIZE] = {0};
    write_block_at_byte(node->indirect_address, buffer, 0, BLOCKSIZE);
    update_inode_entry(inode_number);
}

void allocate_indirect_data_blocks(int inode_number, int blockpointer){
    debug_print("\n\n%s\n\n", "))))))))))))))))))))))))))))))))))");
    debug_print("\n\nAllocating a free datablock via indirect address for i-node %d at indirect block pointer %d\n\n", inode_number, blockpointer);
     
     // Allocate indirect block
    struct inode* node = get_inode(inode_number);
    int new_data_block = get_free_data_block();
    write_block_at_byte(node->indirect_address, &new_data_block, blockpointer, sizeof(int));
    
    debug_print("\n\nIndirect data block has been allocated for i-node %d.\n\n", inode_number);
    debug_print("\n\n%s\n\n", "))))))))))))))))))))))))))))))))))");
}

int get_free_inode_number(){
    for(int i = 0; i < inode_count; i++){
        struct inode* node = get_inode(i);
        if(node->available){
            node->available = 0;
            update_inode_entry(i);
            return i;
        }
    }
    return -1;
}

void remove_inode(int inode_number){
    struct inode* node = get_inode(inode_number);
    for(int i = 0; i < 12; i++){
        int block_address = node->direct_addresses[i];
        //we dont want to free the super block... therefore not >=
        if(block_address > 0){
            free_data_block(block_address);
        }
    }
    if(node->indirect_address > 0){
        int entry_count = BLOCKSIZE/sizeof(int);
        int buffer[entry_count];
        read_block_at_byte(node->indirect_address, buffer, 0, BLOCKSIZE);
        for(int i = 0; i < entry_count; i++){
            int indirect_data_block_to_free = buffer[i];
            if(indirect_data_block_to_free > 0){
                free_data_block(indirect_data_block_to_free);
            }
        }
        free_data_block(node->indirect_address);
    }
    initialize_inode(node);
    update_inode_entry(inode_number);
}


