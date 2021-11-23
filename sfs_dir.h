#include "sfs_api.h"

void initialize_new_directory();
void load_directory();

int get_inode_number(char* filename, int create_new_flag);
int get_next_file_in_dir(char* name);

void remove_dir_entry(char* filename);