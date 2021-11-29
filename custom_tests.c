#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sfs_api.h"
#include "disk_emu.h"

void file_descriptor_test();
void file_descriptor_consistency_test();
void direct_block_test();
void test_write_byte_count();
void test_write_and_read();
void test_overwrite();
void indirect_block_test();

int main() {
    mksfs(1);
    
    //file_descriptor_test();
    
    //file_descriptor_consistency_test();
    //indirect_block_test();
    //direct_block_test();
    //test_write_byte_count();
    //test_write_and_read();
    //test_overwrite();

    //close_disk();
    //mksfs(0);
    exit(0);
}

void file_descriptor_test(){
    printf("Testing file descriptors.\n");
    int fd0 = sfs_fopen("test0.txt");
    int fd1 = sfs_fopen("test1.txt");
    int fd2 = sfs_fopen("test2.txt");
    int fd3 = sfs_fopen("test3.txt");
    printf("The file descriptors are: %d %d %d %d\n", fd0, fd1, fd2, fd3);
    if (fd0 == 0 && fd1 == 1 && fd2 == 2 && fd3 ==3){
        printf("File Descriptor Test Passed\n\n\n\n");
    }else{
        printf("%d %d %d %d\n.", fd0, fd1, fd2, fd3);
        printf("File Descriptor Test Failed\n\n\n\n");
        exit(0);
    }
}

void file_descriptor_consistency_test(){
    printf("Testing same file descriptor for same file name.\n");
    int fd0 = sfs_fopen("same.txt");
    int fd1 = sfs_fopen("same.txt");
    printf("The file descriptors are: %d %d\n", fd0, fd1);
    if(fd0 == fd1){
        printf("File descriptor consistency test passed.\n\n\n\n");
    }else{
        printf("File descriptor consistency test failed.\n\n\n\n");
        exit(1);
    }
}

void direct_block_test(){
    printf("Testing root i-node direct blocks.\n");
    int files_per_block = (BLOCKSIZE / (FILENAMEBYTES + 2*sizeof(int)));
    // -4 because of test file_descriptor_test();
    int total_direct_files = files_per_block * 12 - 4;
    
    printf("Writing %d files.\n", total_direct_files);
    for(int i = 0; i < total_direct_files; i++){
        char* buffer = (char*) malloc(FILENAMEBYTES);
        printf("%d/%d files\n", i + 1, total_direct_files);
        sprintf(buffer, "%d.txt", i);
        sfs_fopen(buffer);
        free(buffer);
    }
    printf("Test complete.\n");
}

void indirect_block_test(){
    printf("Testing max file creation.\n");
    printf("Writing %d files.\n", inode_count + 10);
    for(int i = 0; i < inode_count + 10; i++){
        char* buffer = (char*) malloc(FILENAMEBYTES);
        printf("%d/%d files\n", i + 1, inode_count);
        sprintf(buffer, "%d.txt", i);
        sfs_fopen(buffer);
        free(buffer);
    }
    printf("Test complete.\n");
}

void test_write_byte_count(){
    printf("Testing sfs_write byte count return.\n");
    int f = sfs_fopen("test_write.txt");
    char my_data[] = "The quick brown fox jumps over the lazy dog";
    int bytes_written = sfs_fwrite(f, my_data, strlen(my_data));
    if(bytes_written != strlen(my_data)){
        printf("Test failed, %d bytes were written, but we were expecting %d bytes.", bytes_written, (int) strlen(my_data));
        exit(1);
    }
}

void test_write_and_read(){
    printf("Testing a write followed by a seek & read.\n");
    int f = sfs_fopen("test_write_and_read.txt");
    char my_data[] = "12345678";
    char out_data[1024];
    sfs_fwrite(f, my_data, strlen(my_data) + 1);
    sfs_fseek(f, 0);
    sfs_fread(f, out_data, strlen(my_data) + 1);
    printf("The written string is: %s\n", my_data);
    printf("The read string is: %s\n", out_data);
    if(strcmp(my_data, out_data) == 0){
        printf("Test read and write passed.\n\n\n\n");
    }else{
        printf("Test read and write failed.\n\n\n\n");
        exit(1);
    }
}

void test_overwrite(){
    printf("Testing a write followed by a seek & write.\n");
    printf("WARNING: This test depends on sfs_fread.\n");

    int f = sfs_fopen("test_overwrite.txt");
    char my_data[] = "12345678";
    char my_data2[] = "abcd";
    char out_data[1024];
    sfs_fwrite(f, my_data, strlen(my_data) + 1);
    sfs_fseek(f, 4);
    sfs_fwrite(f, my_data2, strlen(my_data2) + 1);
    sfs_fseek(f, 0);
    sfs_fread(f, out_data, strlen(my_data) + 1);

    char expected_data[] = "1234abcd";

    printf("The written string should be: %s\n", expected_data);
    printf("The read string is: %s\n", out_data);

    if(strcmp(expected_data, out_data) == 0){
        printf("Test read and write passed.\n\n\n\n");
    }else{
        printf("Test read and write failed.\n\n\n\n");
        exit(1);
    }

}
