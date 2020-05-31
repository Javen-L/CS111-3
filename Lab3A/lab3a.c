// NAME: Anh Mac,William Chen
// EMAIL: anhmvc@gmail.com,billy.lj.chen@gmail.com
// UID: 905111606,405131881

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h> // for open(3)
#include <fcntl.h> // for open(3)
#include <string.h>
#include "ext2_fs.h"

#define SBLOCK_OFFSET 1024

int mount = -1;
int numGroups = 1; // according to spec, there is only 1 single group
struct ext2_super_block superblock;
struct ext2_group_desc* group_desc_table;
__u32 block_size;

void exit_with_error(char* message) {
    fprintf(stderr, "%s with error: %s\n", message, strerror(errno));
    exit(2);
}

__u32 group_offset(int groupNum) {
    return block_size + SBLOCK_OFFSET + groupNum * sizeof(struct ext2_group_desc);
}

void printSB() {
    if (pread(mount, &superblock, sizeof(struct ext2_super_block),SBLOCK_OFFSET) < 0)
        exit_with_error("Failed to read superblock");

    if (superblock.s_magic != EXT2_SUPER_MAGIC) // verify superblock
        exit_with_error("Failed to verify superblock");

    block_size = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;
    fprintf(stdout, "SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n",
        superblock.s_blocks_count,
        superblock.s_inodes_count,
        block_size,
        superblock.s_inode_size,
        superblock.s_blocks_per_group,
        superblock.s_inodes_per_group,
        superblock.s_first_ino);
}

void printGroups() {
    int i;
    struct ext2_group_desc group;
    for (i = 0; i < numGroups; i++) {
        if (pread(mount, &group_desc_table[i], sizeof(struct ext2_group_desc), group_offset(i)) < 0)
            exit_with_error("Failed to read Block Group Descriptor Table");

        group = group_desc_table[i];
        fprintf(stdout, "GROUP,%u,%u,%u,%u,%u,%u,%u,%u\n",
            i,
            superblock.s_blocks_count,
            superblock.s_inodes_count,
            group.bg_free_blocks_count,
            group.bg_free_inodes_count,
            group.bg_block_bitmap,
            group.bg_inode_bitmap,
            group.bg_inode_table
        );
    }
}

int main(int argc, char *argv[]){
    if (argc != 2) {
        fprintf(stderr, "Incorrect number of arguments. Correct usage: ./lab3a [filesystem image]");
        exit(1);
    }
        
    mount = open(argv[1], O_RDONLY);
    if (mount < 0)
        exit_with_error("Failed to open given image");

    printSB();
    group_desc_table = malloc(numGroups * sizeof(struct ext2_group_desc));
    printGroups();
    free(group_desc_table);
    exit(0);
}