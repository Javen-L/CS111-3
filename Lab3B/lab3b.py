#!/usr/bin/env python3
# NAME: Anh Mac,William Chen
# EMAIL: anhmvc@gmail.com,billy.lj.chen@gmail.com
# UID: 905111606,405131881

import sys, csv

superblock = None
group = None
inode = []
dirent = []
indirect = []
freeBlock = []
freeInode = []
blocktoInodes = {}
duplicates = set()

class breakSuperBlock:
    def __init__(self,row):
        self.numBlocks =int(row[1])
        self.numInodes =int(row[2])
        self.blockSize =int(row[3])
        self.inodeSize =int(row[4])
        self.blocksPerGroup =int(row[5])
        self.inodesPerGroup =int(row[6])
        self.firstInode =int(row[7])

class breakGroup:
    def __init__(self,row):
        self.inodeTable = int(row[-1])
        self.groupNum = int(row[1])
        self.numBlocks = int(row[2])
        self.numInodes = int(row[3])
        self.numFreeBlocks = int(row[4])
        self.numFreeInodes = int(row[5])
        self.bitmapBlockNum = int(row[6])
        self.imapBlockNum = int(row[7])
        self.firstBlockNum = int(row[8])
        
class breakInode:
    def __init__(self,row):
        self.inodeNum = int(row[1])
        self.fileType = row[2]
        self.mode = int(row[3])
        self.owner = int(row[4])
        self.group = int(row[5])
        self.numLinks = int(row[6])
        self.ctime = row[7]
        self.mtime = row[8]
        self.atime = row[9]
        self.fileSize = int(row[10])
        self.numBlocks = int(row[11])
        self.dirent =[int(i) for i in row[12:24]]
        self.indirect =[int(i) for i in row[24:27]]

class breakDirent:
    def __init__(self,row):
        self.parentInodeNum = int(row[1])
        self.byteOffset = int(row[2])
        self.inodeNum = int(row[3])
        self.entryLength = int(row[4])
        self.nameLength = int(row[5])
        self.name = row[6]

class breakIndirect:
    def __init__(self,row):
        self.inodeNum =int(row[1])
        self.level =int(row[2])
        self.offset =int(row[3])
        self.indirectBlockNum =int(row[4])
        self.referenceBlockNum =int(row[5])

def main():
    if len(sys.argv) != 2:
        sys.exit("Invalid arguments")

    # read csv file
    file = sys.argv[1]
    with open(file, newline='') as csvfile:
        reader = csv.reader(csvfile, delimiter=',')
        for row in reader:
            if row[0] == "SUPERBLOCK":
                superblock = breakSuperBlock(row)
            elif row[0] == "GROUP":
                group = breakGroup(row)
            elif row[0] == "INODE":
                i = doInode(row)
                inode.append(i)
            elif row[0] == "DIRENT":
                d = doDirent(row)
                dirent.append(d)
            elif row[0] == "INDIRECT":
                i = doIndirect(row)
                indirect.append(i)
            elif row[0] == "BFREE":
                freeBlock.append(int(row[1]))
            elif row[0] == "IFREE":
                freeInode.append(int(row[1]))