// NAME: Anh Mac/William Chen
// EMAIL: anhmvc@gmail.com/billy.lj.chen@gmail.com
// UID: 905111606/405131881

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>

int mount = -1;

void exit_with_error(char* message) {
    fprintf(stderr, "%s with error: %s\n", message, strerror(errno));
    exit(1);
}

int main(int argc, char *argv[]){
    if (argc != 2) 
        exit_with_error("Incorrect number of arguments. Correct usage: ./lab3a [filesystem image]");

    mount = open(argv[1], O_RDONLY);
    if (mount < 0)
        exit_with_error("Failed to open given image");

    exit(0);
}