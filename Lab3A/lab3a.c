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
#include <time.h>
#include "ext2_fs.h"

#define SBLOCK_OFFSET 1024
#define FILE 0x8000
#define DIRECTORY 0x4000
#define LINK 0xA000

int mount = -1;
int numGroups = 1; // according to spec, there is only 1 single group
struct ext2_super_block superblock;
struct ext2_group_desc* group_desc_table;
__u32 block_size;

void exit_with_error(char* message) {
    fprintf(stderr, "%s with error: %s\n", message, strerror(errno));
    exit(2);
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

void printGroups(int num) {
    struct ext2_group_desc group = group_desc_table[num];
    fprintf(stdout, "GROUP,%u,%u,%u,%u,%u,%u,%u,%u\n",
        num,
        superblock.s_blocks_count,
        superblock.s_inodes_count,
        group.bg_free_blocks_count,
        group.bg_free_inodes_count,
        group.bg_block_bitmap,
        group.bg_inode_bitmap,
        group.bg_inode_table);
}

int block_offset(int num) {
    return (num - 1) * (int) block_size + SBLOCK_OFFSET;
}

void printFreeBlocks(int num) {
    struct ext2_group_desc group = group_desc_table[num];
    char* bytes = malloc(block_size * sizeof(char));
    if (pread(mount, bytes, block_size, block_offset(group.bg_block_bitmap)) < 0)
        exit_with_error("Failed to read Block Bitmap");

    int i,j;
    int block = 1; // block number starts from 1 (EXT2 docs)
    for (i = 0; i < (int) block_size; i++) {
        char byte = bytes[i];
        for (j = 0; j < 8; j++) { // read each bit of the byte
            int free = !(byte & 1); // 1 means "used", 0 means "free/available"
            if (free)
                fprintf(stdout, "BFREE,%d\n", block);
            byte = byte >> 1;
            block++;
        }
    }
    free(bytes);
}

int inode_offset(int num, int i) {
    return block_offset(num) + i * sizeof(struct ext2_inode);
}

char formatFiletype(__u32 type) {
    int mask = 0xF000;
    if ((type&mask) == FILE)
        return 'f';
    if ((type&mask) == DIRECTORY)
        return 'd';
    if ((type&mask) == LINK)
        return 's';
    else
        return '?';
}

void formatTime(__u32 time, char* buffer) {
    time_t rawtime = time;
    struct tm *info;
    info = gmtime(&rawtime);
    strftime(buffer, 20, "%m/%d/%y %H:%M:%S", info);
}

void printDirectoryEntries(__u32 num, struct ext2_inode inode) {
    int i;
    for (i = 0; i < 12; i++) { // 12 directory blocks
        if (inode.i_block[i] != 0) {
            unsigned int byte_offset = 0;
            while (byte_offset < block_size) {
                struct ext2_dir_entry dir;
                if (pread(mount, &dir, sizeof(dir), block_offset(inode.i_block[i]) + byte_offset) < 0)
                    exit_with_error("Failed to read Directory Entry");

                if (dir.inode != 0)
                    fprintf(stdout,"DIRENT,%d,%d,%d,%d,%d,'%s'\n",
                        num, // parent inode number
                        byte_offset, // logical byte offset within the directory
                        dir.inode,// inode number of referenced file
                        dir.rec_len, // entry length
                        dir.name_len, // name length
                        dir.name // name
                        );
                byte_offset += dir.rec_len;
            }
        }
    }
    //for indirect entries
    //level 1
    if(inode.i_block[12] > 0){
        printIndrectBlock(num, 1, 12, 12);
    }
    //level 2
    if(inode.i_block[13] > 0){
        printIndrectBlock(num, 2, 13, (256+12));
    }
    //level 3
    if(inode.i_block[14] > 0){
        printIndrectBlock(num, 3, 14, (65536+256+12));
    }
}

void printIndirectBlock(int num, int level, int blockN, int levelOffset){
    __u32 *block = malloc(block_size*sizeof(__u32));
    __u32 block_number = block_size/4;
    int addOffset = 1;
    if(level == 2)
        addOffset = 256;
    else if(level == 3)
        addOffset = 65536;
    int inDirectBlockOffset = block_size*blockN;
    if (pread(mount, block, block_size, inDirectBlockOffset) < 0)
        exit_with_error("Failed to read Directory Entry for Indirect Entry");
    i = 0;
    for(; i < block_number; i++){
        if(block[i] != 0){
            fprintf(stdout,"INDIRECT,%d,%d,%d,%d,%u\n",
                num,
                level,
                levelOffset,
                blockN, 
                block[i],
                );
            if(level != 1){
                level--;
                printIndirectBlock(element[i], num, levelOffset, level);
            }
        }
        levelOffset = levelOffset+addOffset;
    }
}

void printInode(__u32 inode_num, __u32 inodeTable) {      
    struct ext2_inode inode;
    if (pread(mount, &inode, sizeof(inode), inode_offset(inodeTable, inode_num-1)) < 0)
        exit_with_error("Failed to read Inode");
    
    if (inode.i_mode != 0 && inode.i_links_count != 0) {
        char filetype = formatFiletype(inode.i_mode);
        char atime[20], ctime[20], mtime[20];
        formatTime(inode.i_atime, atime);
        formatTime(inode.i_ctime, ctime);
        formatTime(inode.i_mtime, mtime);

        fprintf(stdout, "INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d",
            inode_num, // inode number
            filetype, // file type
            inode.i_mode & 0x0FFF, // mode, lower 12-bits
            inode.i_uid, // owner
            inode.i_gid, // group
            inode.i_links_count, // link count
            atime, // time of last inode change
            ctime, // modification time
            mtime, // time of last access
            inode.i_size, // file size
            inode.i_blocks // number of blocks of disk space
            );  
        
        // for ordinary files ('f' and 'd') and symbolic links w length greater than 60, next 15 fields are block addresses
        if (filetype == 'f' || filetype == 'd' || (filetype == 's' && inode.i_size > 60)) {
            int i;
            for (i = 0; i < 15; i++) {
                fprintf(stdout, ",%d", inode.i_block[i]);
            }
        }
        fprintf(stdout, "\n");

        if (filetype == 'd')
            printDirectoryEntries(inode_num, inode);
    }
}

void printInodesSummary(int num) {
    struct ext2_group_desc group = group_desc_table[num];
    __u32 inodeTable = group.bg_inode_table;
    int numBytes = superblock.s_inodes_per_group / 8;
    char* bytes = malloc(numBytes * sizeof(char));
    if (pread(mount, bytes, block_size, block_offset(group.bg_inode_bitmap)) < 0)
        exit_with_error("Failed to read Inode Bitmap");

    int i,j;
    __u32 inode_num = 1; // inode number starts from 1 (EXT2 docs)
    for (i = 0; i < numBytes; i++) {
        char byte = bytes[i];
        for (j = 0; j < 8; j++) { // read each bit of the byte
            int free = !(byte & 1); // 1 means "used", 0 means "free/available"
            if (free)
                fprintf(stdout, "IFREE,%d\n", inode_num);
            else // allocated
                printInode(inode_num, inodeTable);
                
            byte = byte >> 1;
            inode_num++;
        }
    }
    free(bytes);
}

int group_offset(int num) {
    return (int) block_size + SBLOCK_OFFSET + num * sizeof(struct ext2_group_desc);
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
    
    int i;
    for (i = 0; i < numGroups; i++) {
        if (pread(mount, &group_desc_table[i], sizeof(struct ext2_group_desc), group_offset(i)) < 0)
            exit_with_error("Failed to read Block Group Descriptor Table");

        printGroups(i);
        printFreeBlocks(i);
        printInodesSummary(i);
    }
    free(group_desc_table);
    exit(0);
}