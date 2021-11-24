#include "sfs_api.h"
#include "sfs_inode.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct bmap_entry* free_blocks;

struct bmap_entry* get_bmap_entry(int index){
    return &free_blocks[index];
}

int bmap_block_address_from_index(int index){
    int bytes_till_index = index * sizeof(int);
    int block_index = bytes_till_index / BLOCKSIZE;
    return FIRSTBITMAPBLOCKADDRESS + block_index;
}

int bmap_blockpointer_from_index(int index){
    int bytes_till_index = index * sizeof(int);
    return bytes_till_index % BLOCKSIZE;
}

void init_array(){
    if(DATABLOCKCOUNT * 4 > BITMAPBLOCKCOUNT * BLOCKSIZE){
        debug_print("%s\n", "Not enough blocks allocated for the bitmap.");
        exit(1);
    }

    free_blocks = (struct bmap_entry*) malloc(sizeof(bmap_entry) * DATABLOCKCOUNT);
    struct bmap_entry table[DATABLOCKCOUNT];
    free_blocks = &(table[0]);

    for(int i = 0; i < DATABLOCKCOUNT; i++){
        struct bmap_entry entry = {-1, 0};
        free_blocks[i] = entry;
    }

}

void update_bmap_entry(int index, int available){
    debug_print("Updating bmap entry at index %d with available status: %d.\n", index, available);
    struct bmap_entry* entry = get_bmap_entry(index);
    entry->available = available;

    debug_print("%s\n", "Creating bmap entry buffer...");
    char* buffer = malloc(sizeof(int));
    memcpy(buffer, &entry->available, sizeof(int));
    debug_print("%s\n", "Buffer has been setup.");

    debug_print("%s\n", "Updating bmap entry on the disk...");
    int block_address = bmap_block_address_from_index(index) ;
    int blockpointer = bmap_blockpointer_from_index(index);
    if(blockpointer < 0){
        debug_print("\n\n\nInvalid block pointer %d with address $%d for index %d, exiting...\n\n\n", blockpointer, block_address, index);
        exit(1);
    }
    write_block_at_byte(block_address, buffer, blockpointer, sizeof(int));
    debug_print("%s\n", "The bmap entry update has been written to the disk.");
}

void new_free_bmap(){
    debug_print("%s\n", "Initializing a new free bmap.");
    init_array();
    for(int i = 0; i < DATABLOCKCOUNT; i++){
        struct bmap_entry* entry = get_bmap_entry(i);
        entry->block_address = FIRSTDATABLOCKADDRESS + i;
        update_bmap_entry(i, 1);
    }
    update_bmap_entry(ROOT_INODE_NUMBER, 0);
}

int get_free_data_block(){
    debug_print("\n%s\n\n", ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
    debug_print("%s\n", "Getting a free data block...");
    for(int i = 0; i < DATABLOCKCOUNT; i++){
        struct bmap_entry* entry = get_bmap_entry(i);
        if(entry->available){
            update_bmap_entry(i, 0);
            debug_print("\n%s\n\n", ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
            return entry->block_address;
        }
    }
    debug_print("\n\n\n%s\n\n\n", " ERROR: RAN OUT OF DATA BLOCK SPACE.");
    exit(1);
    return -1;
}


void free_data_block(int block_address){
    debug_print("\n\n\nFreeing block_address %d from bmap.\n\n\n", block_address);
    int offset =FIRSTBITMAPBLOCKADDRESS;
    int blocks_filled = block_address - offset;
    int bytes_till_index = blocks_filled * BLOCKSIZE;
    int index = bytes_till_index / sizeof(int);
    
    // Hard delete, might not be necessary, but I don't like seeing the written data when I use 'strings disk_name.sfs'
    char buffer[BLOCKSIZE] = { 0 };
    write_block_at_byte(block_address, buffer, 0, BLOCKSIZE);

    debug_print("The corresponding index for block_address %d is %d in the bmap.", block_address, index);
    update_bmap_entry(index, 1);
    debug_print("The block at address %d has been freed.", block_address);
}

void load_free_bmap(){
    init_array();
    for(int i = 0; i < DATABLOCKCOUNT; i++){
        int available;
        read_block_at_byte(bmap_block_address_from_index(i), &available, bmap_blockpointer_from_index(i), sizeof(int));
        get_bmap_entry(i)->available = available;
        get_bmap_entry(i)->block_address = bmap_block_address_from_index(i);
    }
}