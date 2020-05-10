// NAME: Anh Mac
// EMAIL: anhmvc@gmail.com
// UID: 905111606

#include <stdlib.h> // for atexit(3), atoi(3)
#include <stdio.h> // for fprintf(3), atoi(3), fopen(3)
#include <errno.h> // for errno(3)
#include <getopt.h> // for getopt_long(3)
#include <termios.h> // for termios(3), tcgetattr(3), tcsetattr(3)
#include <unistd.h> // for termios(3), tcgetattr(3), tcsetattr(3), fork(2), read(2), write(2), exec(3), pipe(2), getopt_long(3), close(2)
#include <string.h> // for strerror(3), atoi(3), memset(3)
#include <signal.h> // for kill(3)
#include <poll.h> // for poll(2)
#include <sys/socket.h> // for socket(7), connect(2), shutdown(2)
#include <sys/types.h> // for connect(2), creat(2)
#include <sys/stat.h> // for creat(2)
#include <fcntl.h> // for creat(2)
#include <netdb.h> // for gethostbyname(3)
#include <arpa/inet.h> // for htons(3)
#include <ulimit.h> // for ulimit(3)
#include <zlib.h> // for compression
#include <sys/wait.h> // for waitpid(2)

#define EOT '\004' // ^D
#define KILL '\003' // ^C
#define CR '\r'
#define LF '\n'

int toShell[2];
int fromShell[2];
pid_t pid;
int ready_to_exit = 0;

// initialize compression streams
z_stream ostream_toClient;
z_stream istream_fromClient;

void create_pipe(int* fd);
void signal_handler(int signum);

void do_compress(int sock_fd, char* buffer) {
    char compress_buf[256];
    
    ostream_toClient.avail_in = 1;
    ostream_toClient.next_in = (unsigned char*) buffer;
    ostream_toClient.avail_out = 256;
    ostream_toClient.next_out = (unsigned char*) compress_buf;
    
    do {
        deflate(&ostream_toClient, Z_SYNC_FLUSH);
    } while (ostream_toClient.avail_in > 0);
    
    // write compressed content to socket
    int cnt = 256 - ostream_toClient.avail_out;
    write(sock_fd, compress_buf, cnt);
    deflateEnd(&ostream_toClient);
}

void do_decompress(char* buffer, char* decompressed_buf, int count) {
    istream_fromClient.avail_in = count;
    istream_fromClient.next_in = (unsigned char*) buffer;
    istream_fromClient.avail_out = 1024;
    istream_fromClient.next_out = (unsigned char*) decompressed_buf;
    
    do {
        inflate(&istream_fromClient, Z_SYNC_FLUSH);
    } while (istream_fromClient.avail_out > 0);
    
    inflateEnd(&istream_fromClient);
}

int main(int argc, char *argv[])
{
    int compress_flag = 0;
    int shell_flag = 0;
    int PORT = -1;
    char *shell;
    
    struct sockaddr_in server_address, client_address;
    

    
    static const struct option long_options[] =
    {
        {"port",        required_argument,  0, 'p'},
        {"shell",       required_argument,  0 ,'s'},
        {"compress",    no_argument,        0, 'c'},
        {0,0,0,0}
    };
    
    char opt;
    while (1)
    {
        int option_index = 0;
        opt = getopt_long(argc, argv, "p:s:c", long_options, &option_index);
        
        if (opt == -1)
            break;
        
        switch (opt)
        {
            case 'p':
                PORT = atoi(optarg); // convert string to integer, then from host byte order to network byte order
                break;
            case 's':
                shell = optarg;
                shell_flag = 1;
                break;
            case 'c':
                compress_flag = 1;
                break;
            default:
                fprintf(stderr, "UNRECOGNIZED ARGUMENT. Correct usage: lab1b-client '--port=[PORT#]' '--shell=[program]' '--compress'\n");
                exit(1);
        }
    }
    
    if (PORT == -1) {
        fprintf(stderr, "ERROR: No port number specified!\n");
        exit(1);
    }
    
    if (!shell_flag) {
        fprintf(stderr, "ERROR: A shell program has to be specified!\n");
        exit(1);
    }
    
    if (compress_flag) {
        // OUT_STREAM
        ostream_toClient.zalloc = Z_NULL;
        ostream_toClient.zfree = Z_NULL;
        ostream_toClient.opaque = Z_NULL;
        
        int deflate_return = deflateInit(&ostream_toClient, Z_DEFAULT_COMPRESSION);
        if (deflate_return != Z_OK) {
            fprintf(stderr, "Unable to create compression stream with error %s\n", strerror(errno));
            exit(1);
        }
        
        // IN_STREAM
        istream_fromClient.zalloc = Z_NULL;
        istream_fromClient.zfree = Z_NULL;
        istream_fromClient.opaque = Z_NULL;
        
        int inflate_return = inflateInit(&istream_fromClient);
        if (inflate_return != Z_OK) {
            fprintf(stderr, "Unable to create decompression stream with error %s\n", strerror(errno));
            exit(1);
        }
    }
    
    // create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "ERROR: Failed to create socket with error %s\n", strerror(errno));
        exit(1);
    }
    
    // initialize address struct
    memset(&server_address, '0', sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY; // maps IP address to 127.0.0.1
    server_address.sin_port = htons(PORT); // convert to netwwork byte order
    
    // binding socket
    if (bind(sockfd, (struct sockaddr *) &server_address, sizeof(server_address))) {
        fprintf(stderr, "ERROR: Failed to bind socket with error %s\n", strerror(errno));
        exit(1);
    }
    
    // listening for requests
    if (listen(sockfd, 5) < 0) {
        fprintf(stderr, "ERROR: Failed to listen for requests with error %s\n", strerror(errno));
        exit(1);
    }
    
    // Accept Requests, fill client's sockaddr_in structure
    int client_addrlen = sizeof(client_address);
    int new_socket = accept(sockfd, (struct sockaddr *) &client_address, (socklen_t *) &client_addrlen);
    if (new_socket < 0) {
        fprintf(stderr, "ERROR: Failed to accept requests with error %s\n", strerror(errno));
        exit(1);
    }
    
    shutdown(sockfd, SHUT_RDWR); // no longer need the listening socket
    
    char *arg[2];
    
    // create pipes
    create_pipe(toShell);
    create_pipe(fromShell);
    
    signal(SIGPIPE, signal_handler);
    
    pid = fork();
    
    if (pid < 0) {
        fprintf(stderr, "ERROR: Failure in forking! with error %s\n", strerror(errno));
        exit(1);
    }
    
    else if (pid == 0) { // Child process [SHELL]
        // child process closes input in toTerminal and only allow to send output (write) to Terminal
        close(fromShell[0]);
        // child process closes output toShell and only allow to receive input (read) from Terminal
        close(toShell[1]);
            
        close(STDIN_FILENO);
        if (dup(toShell[0]) == -1) {
            fprintf(stderr, "ERROR: Failed to duplicate stdin in child process with error %s\n", strerror(errno));
            exit(1);
        }
        close(toShell[0]);
        
        close(STDOUT_FILENO);
        if (dup(fromShell[1]) == -1) {
            fprintf(stderr, "ERROR: Failed to duplicate stdout in child process with error %s\n", strerror(errno));
            exit(1);
        }
        close(STDERR_FILENO);
        
        if (dup(fromShell[1]) == -1) {
            fprintf(stderr, "ERROR: Failed to duplicate stderr in child process with error %s\n", strerror(errno));
            exit(1);
        }
        close(fromShell[1]);
            
        // exec a shell
        arg[0] = shell;
        arg[1] = NULL;
            
        if (execvp(shell, arg) == -1) {
            fprintf(stderr, "ERROR: Failed to exec shell with error %s\n", strerror(errno));
            exit(1);
        }
    }
        
    else { // PARENT PROCESS [TERMINAL]
        close(fromShell[1]);  // parent process closes output in fromTerminal and only read from Shell
        close(toShell[0]); // parent process closes input toShell and only write to Shell
        ready_to_exit = 0;
        int exitstatus;
        
        // create an array of 2 pollfd structures
        struct pollfd fds[2] = {
            {new_socket, POLLIN|POLLHUP|POLLERR, 0},
            {fromShell[0], POLLIN|POLLHUP|POLLERR, 0}
        };
        
            
        while (1) {
            int ret = poll(fds, 2, 0);
            if (ret == -1) {
                fprintf(stderr, "ERROR: Error in poll process with error %s\n", strerror(errno));
                exit(1);
            }
                
            short revents_socket = fds[0].revents;
            short revents_shell = fds[1].revents;
                
            // pending input from socket
            if (revents_socket & POLLIN) {
                char buf[1024];
                
                int rd = read(new_socket, buf, 1024);
                if (rd < 0) {
                    fprintf(stderr, "ERROR: FAiled to read into buffer from socket with error %s\n", strerror(errno));
                    exit(1);
                }
                 
                // decompress content
                if (compress_flag) {
                    char decompressed_buf[1024];
                    memset(decompressed_buf, 0, 1024);
                    do_decompress(buf, decompressed_buf, rd);
                    memset(buf, 0, 1024); // clear compressed content from the buffer
                    memcpy(buf, decompressed_buf, 1024); // set the buffer to the new decompressed content
                }
                
                // process the I/O content
                int i;
                for (i = 0; i < rd; i++)
                {
                    char ch = buf[i];
                    char temp[2];
                    if (ch == CR) {
                        temp[0] = LF;
                        if (write(toShell[1], &temp, 1) < 0) {
                                fprintf(stderr, "ERROR: Failed to write from client socket to shell with error %s\n", strerror(errno));
                                exit(1);
                        }
                    }
                    else if (ch == EOT) {
                        close(toShell[1]);
                        ready_to_exit = 1;
                    }
                    else if (ch == KILL) {
                        if (kill(pid, SIGINT) < 0) {
                            fprintf(stderr, "ERROR: Failed to send SIGINT signal to shell.\n");
                            exit(1);
                        }
                    }
                    else {
                        if (write(toShell[1], &ch, 1) < 0) {
                            fprintf(stderr, "ERROR: Failed to write from client socket to shell with error %s\n", strerror(errno));
                            exit(1);
                        }
                    }
                }
            }
            
            // pending input from shell
            if (revents_shell & POLLIN) {
                int MAXSIZE = 256;
                char buf[MAXSIZE];
                
                int rd = read(fromShell[0], buf, MAXSIZE);
                if (rd < 0) {
                    fprintf(stderr, "ERROR: Failed to read in from shell with error %s\n", strerror(errno));
                    exit(1);
                }
                
                if (rd == 0) {
                    ready_to_exit = 1;
                    waitpid(pid, &exitstatus, 0);
                    int SIGNAL = 0x007f & exitstatus;
                    int STATUS = exitstatus>>8;
                        
                    fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", SIGNAL, STATUS);
                    break;
                }
                
                // processing input from shell
                int i;
                for (i = 0; i < rd; i++) {
                    char ch = buf[i];
                    char specialchar[2];
                    if (ch == LF) {
                        specialchar[0] = CR;
                        specialchar[1] = LF;
                        if (compress_flag)
                            do_compress(new_socket, specialchar); // compress and write to socket
                        else
                            write(new_socket, &specialchar, 2);
                    }
                    else if (ch == EOT) // ^D
                        ready_to_exit = 1;
                    else {
                        if (compress_flag)
                            do_compress(new_socket, &ch); // compress and write to socket
                        else
                            write(new_socket, &ch, 1);
                    }
                }
            }
            
            
            if (revents_socket & (POLLHUP|POLLERR))
            {
                fprintf(stderr, "STDIN hangup/error.\n");
                ready_to_exit = 1;
            }
            
            if (revents_shell & (POLLHUP|POLLERR))
            {
                fprintf(stderr, "Shell hangup/error.\n");
                ready_to_exit = 1;
            }
                
            if (ready_to_exit)
                break;
                
        } // exit poll
        
        close(toShell[1]); // closing write pipe to shell
        close(fromShell[0]);
        
        if (waitpid(pid, &exitstatus, 0) < 0)
        {
            fprintf(stderr, "ERROR: Waiting child process with error %s\n", strerror(errno));
            exit(1);
        }
                
        // output shell's exit status
                
        int SIGNAL = 0x007f & exitstatus;
        int STATUS = exitstatus>>8;
            
        fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", SIGNAL, STATUS);
    }
    
    shutdown(new_socket, SHUT_RDWR);
    
    exit(0);
}

void create_pipe(int* fd) {
    int pd = pipe(fd);
    if (pd != 0) {
        fprintf(stderr, "ERROR: Failure in creating pipes with error %s\n", strerror(errno));
        exit(1);
    }
}

void signal_handler(int signum) {
    if (signum == SIGPIPE) {
        fprintf(stderr, "SIGPIPE SIGNAL RECEIVED!");
        ready_to_exit = 1;
    }
}
