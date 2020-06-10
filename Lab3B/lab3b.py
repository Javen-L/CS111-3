#!/usr/bin/env python3
# NAME: Anh Mac,William Chen
# EMAIL: anhmvc@gmail.com,billy.lj.chen@gmail.com
# UID: 905111606,405131881

import sys
import csv
import numpy
import math
#TODO: find alternative to numpy, actually run program, and remove double bracket comments

superblock = None
group = None
inode = []
dirent = []
indirect = []
freeBlock = []
freeInode = []
blocktoInodes = {}
duplicates = set()

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

class breakSuperBlock:
    def __init__(self,row):
        self.numBlocks =int(row[1])
        self.numInodes =int(row[2])
        self.blockSize =int(row[3])
        self.inodeSize =int(row[4])
        self.blocksPerGroup =int(row[5])
        self.inodesPerGroup =int(row[6])
        self.firstInode =int(row[7])
        

def main():
    if len(sys.argv) != 2:
        sys.exit("Invalid number of arguments")

    # read csv file
    try:
        file = open(sys.argv[1], 'r')
    except (OSError, IOError) as e:
        sys.exit("File not found")
    with file as csvfile:
        reader = csv.reader(csvfile, delimiter=',')
        for row in reader:
            if row[0] == "INODE":
                i = breakInode(row)
                inode.append(i)
            elif row[0] == "DIRENT":
                d = breakDirent(row)
                dirent.append(d)
            elif row[0] == "INDIRECT":
                i = breakIndirect(row)
                indirect.append(i)
            elif row[0] == "GROUP":
                group = breakGroup(row)
            elif row[0] == "SUPERBLOCK":
                superblock = breakSuperBlock(row)
            elif row[0] == "BFREE":
                freeBlock.append(int(row[1]))
            elif row[0] == "IFREE":
                freeInode.append(int(row[1]))
            else:
                sys.exit("Invalid Data in CSV file")

    #BLOCK CONSISTENCY AUDIT
    table = int(group.inodeTable)
    high = int(math.ceil(group.numInodes * superblock.inodeSize / superblock.blockSize))
    firstBlockIndex = table + high
    blockSize = superblock.numBlocks
    inodeSize = superblock.numInodes

    for i in range(len(inode)):
        if inode[i].fileType != 's' or inode[i].fileSize > blockSize:
            offset = 0
            for block in range(0, len(inode[i].dirent)):
                bNum = inode[i].dirent[block]
                inodeN = inode[i].inodeNum
                if bNum > 0 and bNum < firstBlockIndex:
                    print("RESERVED BLOCK " + str(bNum) + " IN INODE " + str(inodeN) + " AT OFFSET " + str(offset))
                elif bNum >= blockSize or bNum < 0:
                    print("INVALID BLOCK " + str(bNum) + " IN INODE " + str(inodeN) + " AT OFFSET " + str(offset))
                elif bNum != 0:
                    if bNum in blocktoInodes:
                        duplicates.add(bNum)
                        blocktoInodes[bNum].append([inodeN, offset, 0])
                    else:
                        blocktoInodes[bNum] = [[inodeN, offset, 0]]
                
                offset = offset + 1

            for indirectN in range(0,3):
                bNum = inode[i].indirect[indirectN]
                inodeN = inode[i].inodeNum
                off = 0
                indirectStr = ""
                if indirectN == 0:
                    off = 12
                    indirectStr = ""
                elif indirectN == 1:
                    off = 268
                    indirectStr = "DOUBLE "
                elif indirectN == 2:
                    off = 65804
                    indirectStr = "TRIPLE "

                if bNum < firstBlockIndex and bNum > 0:
                    print("RESERVED " + indirectStr + "INDIRECT BLOCK " + str(bNum) + " IN INODE " + str(inodeN) + " AT OFFSET " + str(off))
                elif bNum >= blockSize or bNum < 0:
                    print("INVALID " + indirectStr + "INDIRECT BLOCK " + str(bNum) + " IN INODE " + str(inodeN) + " AT OFFSET " + str(off))
                elif bNum != 0:
                    if bNum in blocktoInodes:
                        duplicates.add(bNum)
                        blocktoInodes[bNum].append([inodeN, off, indirectN + 1])
                    else:
                        blocktoInodes[bNum] = [[inodeN, off, indirectN + 1]]
    
    for i in range(len(indirect)):
        inodeN = indirect[i].inodeNum
        referenceBlock = indirect[i].referenceBlockNum
        off = indirect[i].offset
        if indirect[i].level == 1:
            if referenceBlock < firstBlockIndex and referenceBlock > 0:
                print("RESERVED INDIRECT BLOCK " + str(referenceBlock) + " IN INODE " + str(inodeN) + " AT OFFSET " + str(off))
            elif referenceBlock >= blockSize or referenceBlock < 0:
                print("INVALID INDIRECT BLOCK " + str(referenceBlock) + " IN INODE " + str(inodeN) + " AT OFFSET " + str(off))
            elif referenceBlock != 0:
                if referenceBlock in blocktoInodes:
                    blocktoInodes[referenceBlock].append([inodeN, off, 1])
                    duplicates.add(referenceBlock)
                else:
                    blocktoInodes[referenceBlock] = [[inodeN, off, 1]] 
        elif indirect[i].level == 2:
            if referenceBlock < firstBlockIndex and referenceBlock > 0:
                print("RESERVED DOUBLE INDIRECT BLOCK " + str(referenceBlock) + " IN INODE " + str(inodeN) + " AT OFFSET " + str(off))
            elif referenceBlock >= blockSize or referenceBlock < 0:
                print("INVALID DOUBLE INDIRECT BLOCK " + str(referenceBlock) + " IN INODE " + str(inodeN) + " AT OFFSET " + str(off))
            elif referenceBlock != 0:
                if referenceBlock in blocktoInodes:
                    blocktoInodes[referenceBlock].append([inodeN, off, 2])
                    duplicates.add(referenceBlock)
                else:
                    blocktoInodes[referenceBlock] = [[inodeN, off, 2]]
        elif indirect[i].level == 3:
            if referenceBlock < firstBlockIndex and referenceBlock > 0:
                print("RESERVED TRIPLE INDIRECT BLOCK " + str(referenceBlock) + " IN INODE " + str(inodeN) + " AT OFFSET " + str(off))
            elif referenceBlock >= blockSize or referenceBlock < 0:
                print("INVALID TRIPLE INDIRECT BLOCK " + str(referenceBlock) + " IN INODE " + str(inodeN) + " AT OFFSET " + str(off))
            elif referenceBlock != 0:
                if referenceBlock in blocktoInodes:
                    blocktoInodes[referenceBlock].append([inodeN, off, 3])
                    duplicates.add(referenceBlock)
                else:
                    blocktoInodes[referenceBlock] = [[inodeN, off, 3]]
    
    for i in range(firstBlockIndex, blockSize):
        if i in blocktoInodes and i in freeBlock:
            print("ALLOCATED BLOCK " + str(i) + " ON FREELIST")
        if i not in blocktoInodes and i not in freeBlock:
            print("UNREFERENCED BLOCK " + str(i))
    

    for i in duplicates:
        for x in blocktoInodes[i]:
            if x[-1] == 3:
                print("DUPLICATE TRIPLE INDIRECT BLOCK " + str(i) + " IN INODE " + str(x[0]) + " AT OFFSET " + str(x[1]))
            elif x[-1] == 2:
                print("DUPLICATE DOUBLE INDIRECT BLOCK " + str(i) + " IN INODE " + str(x[0]) + " AT OFFSET " + str(x[1]))
            elif x[-1] == 1:
                print("DUPLICATE INDIRECT BLOCK " + str(i) + " IN INODE " + str(x[0]) + " AT OFFSET " + str(x[1]))
            else:
                print("DUPLICATE BLOCK " + str(i) + " IN INODE " + str(x[0]) + " AT OFFSET " + str(x[1]))

    
    #I-node Allocation Audits
    used = []
    for i in range(0, len(inode)):
        inodeN = inode[i].inodeNum
        if inodeN != 0:
            if inodeN in freeInode:
                print("ALLOCATED INODE " + str(inodeN) + " ON FREELIST")
            used.append(inodeN)
        if inodeN not in freeInode and inode[i].fileType == '0':
            print("UNALLOCATED INODE " + str(inodeN) + " NOT ON FREELIST")
    
    for i in range(superblock.firstInode, inodeSize):
        if i not in freeInode and i not in used:
            print("UNALLOCATED INODE " + str(i) + " NOT ON FREELIST")

    #Directory Consistency Audits
    linkCount = numpy.zeros(superblock.firstInode + superblock.numInodes)
    for i in range(0, len(dirent)):
        parentInode = dirent[i].parentInodeNum
        inodeN = dirent[i].inodeNum
        if inodeN <= 0 or inodeN > superblock.numInodes:
            print("DIRECTORY INODE " + str(parentInode) + " NAME " + dirent[i].name + " INVALID INODE " + str(inodeN))
        elif inodeN not in used:
            print("DIRECTORY INODE " + str(parentInode) + " NAME " + dirent[i].name + " UNALLOCATED INODE " + str(inodeN))
        else:
            linkCount[inodeN] = linkCount[inodeN] + 1
    
    for i in range(len(inode)):
        inodeN = inode[i].inodeNum
        nLink = inode[i].numLinks
        if linkCount[inodeN] != nLink:
            print("INODE " + str(inodeN) + " HAS " + str(int(linkCount[inodeN])) + " LINKS BUT LINKCOUNT IS " + str(int(nLink)))
            
    parents = numpy.zeros(superblock.firstInode + superblock.numInodes)
    parents[2] = 2
    for i in range(len(dirent)):
        inodeN = dirent[i].inodeNum
        parentInode = dirent[i].parentInodeNum
        name = dirent[i].name
        if name != "'.'" and name != "'..'":
            parents[inodeN] = parentInode
    
    for i in range(len(dirent)):
        inodeN = dirent[i].inodeNum
        parentInode = dirent[i].parentInodeNum
        name = dirent[i].name
        if inodeN != parents[parentInode] and name == "'..'":
            print("DIRECTORY INODE " + str(parentInode) + " NAME '..' LINK TO INODE " + str(inodeN) + " SHOULD BE " + str(int(parents[parentInode])))
        if parentInode != inodeN and name == "'.'":
            print("DIRECTORY INODE " + str(parentInode) + " NAME '.' LINK TO INODE " + str(inodeN) + " SHOULD BE " + str(parentInode))

if __name__ == '__main__':
    main()