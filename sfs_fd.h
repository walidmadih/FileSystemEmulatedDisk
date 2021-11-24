#include "sfs_api.h"

void initialize_fd_table();
int get_fd(int inode_number);
int remove_fd(int inode_number);
struct fd_entry* get_fd_entry(int index);