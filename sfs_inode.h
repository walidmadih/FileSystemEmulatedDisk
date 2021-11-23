#ifndef SFS_INODE_H
#define SFS_INODE_H

void initialize_all_inodes();
void load_all_inodes();

struct inode* get_inode(int number);
int write_to_inode(int inode_number, char* buffer, int* filepointer, int length);
int write_block_at_byte(int block_address, void* buffer, int blockpointer, int length);
int read_from_inode(int inode_number, char* buffer, int* filepointer, int length);
int get_free_inode_number();

void remove_inode(int inode_number);

#endif