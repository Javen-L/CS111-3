// NAME: Anh Mac
// EMAIL: anhmvc@gmail.com
// ID: 905111606

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>


// Signal handler reference: https://www.tutorialspoint.com/c_standard_library/c_function_signal.htm
void handle_sigsegv()
{
    fprintf(stderr, "Caught SIGSEGV signal!!\n");
    exit(4); // caught and received SIGSEGV
}

void segfault()
{
    char* ptr = NULL;
    *ptr = 'a';
}

void redirectInput(char *filename)
{
    // I/O redirection referenced from File Descriptor Management file from spec
    // input redirection to filename as passed
    int ifd = open(optarg, O_RDONLY);
    
    if (ifd >= 0)
    {
        close(0);
        dup(ifd);
        close(ifd);
    }
    else
    {
        fprintf(stderr, "--input ERROR: Unable to open the specified input file '%s'\n%s\n", filename, strerror(errno));
        exit(2); // unable to open input file
    }
}

void redirectOutput(char *filename)
{
    // I/O redirection referenced from File Descriptor Management file from spec
    // output redirection to newfile
    int ofd = creat(optarg, 0666);
    
    if (ofd >= 0)
    {
        close(1);
        dup(ofd);
        close(ofd);
    }
    
    else
    {
        fprintf(stderr, "--output ERROR: Unable to create specified output file '%s'\n%s\n", filename, strerror(errno));
        exit(3); // unable to open output file
    }
}

int main(int argc, char *argv[])
{
    
    int c;
    int seg_flag;
        
    // getopt_long(3) function reference: https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html and TA slides
    while (1)
    {
        int option_index = 0;
        
        static struct option long_options[] =
        {
            {"input",   required_argument,  0,  'i'},
            {"output",  required_argument,  0,  'o'},
            {"segfault",no_argument,         0,  's'},
            {"catch",   no_argument,         0,  'c'},
            {0,0,0,0}
        };
        
        c = getopt_long(argc, argv, "i:o:sc", long_options, &option_index);
        
        if (c == -1)
            break;
        
        switch (c)
        {
            case 'i':
                redirectInput(optarg);
                break;
                
            case 'o':
                redirectOutput(optarg);
                break;
                
            case 's':
                seg_flag = 1;
                break;
                
            case 'c':
                signal(SIGSEGV, handle_sigsegv);
                break;
                
            default:
                fprintf(stderr, "ERROR: Unrecognized argument.\n");
                exit(1); // unrecognized argument
        }
    }
    
    if (seg_flag)
        segfault();
    
    // allocate memory for buffer for copy
    char *buf = (char *) malloc(sizeof(char));
    
    // read from standard input
    ssize_t rd = read(0, buf, 1);
    
    while (rd > 0)
    {
        if (rd == -1)
        {
            fprintf(stderr, "ERROR reading input file..\n%s\n", strerror(errno));
            exit(2); // unable to read input file
        }
        
        // write to standard output
        ssize_t wd = write(1, buf, 1);
        
        if (wd <= 0)
        {
            fprintf(stderr, "ERROR writing output file..\n%s\n", strerror(errno));
            exit(3); // unable to write output file
        }
        
        rd = read(0, buf, 1);
    }
    
    // free memory
    free(buf);
    
    exit(0); // copy successful
}
