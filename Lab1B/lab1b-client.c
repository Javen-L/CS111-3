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

#define EOT '\004' // ^D
#define KILL '\003' // ^C
#define CR '\r'
#define LF '\n'

// store initial terminal attributes
struct termios saved_attributes;

// initialize compression streams
z_stream ostream_toServer;
z_stream istream_fromServer;

void reset_input_mode();
void set_input_mode();

void do_compress(int socket_fd, char* buffer, int log_flag, int log_fd) {
    char compressed_buf[256];
    
    ostream_toServer.avail_in = 1;
    ostream_toServer.next_in = (unsigned char*) buffer;
    ostream_toServer.avail_out = 256;
    ostream_toServer.next_out = (unsigned char*) compressed_buf;
    
    // doing compressing
    do {
        deflate(&ostream_toServer, Z_SYNC_FLUSH);
    } while (ostream_toServer.avail_in > 0);
    
    // write to server
    write(socket_fd, compressed_buf, 256 - ostream_toServer.avail_out);
    
    // log compressed content
    if (log_flag) {
        char value[5];
        sprintf(value, "%d", (int) strlen(compressed_buf)); // write formatted output to value
        
        write(log_fd, "SENT ", strlen("SENT ")); // SENT
        write(log_fd, value, strlen(value)); // #
        write(log_fd, " bytes: ", strlen(" bytes: ")); // bytes:
        write(log_fd, compressed_buf, strlen(compressed_buf));
        write(log_fd, "\n", 1);
    }
    
    deflateEnd(&ostream_toServer);
}

void do_decompress(char* buffer, int count) {
    char decompressed_buf[1024];
    
    istream_fromServer.avail_in = count;
    istream_fromServer.next_in = (unsigned char *) buffer;
    istream_fromServer.avail_out = 1024;
    istream_fromServer.next_out = (unsigned char*) decompressed_buf;
    
    do {
        inflate(&istream_fromServer, Z_SYNC_FLUSH);
    } while (istream_fromServer.avail_in > 0);
    
    // write decompressed content to terminal
    int i;
    int num = 1024 - istream_fromServer.avail_out;
    for (i = 0; i < num; i++) {
        char ch = decompressed_buf[i];
        write(STDOUT_FILENO, &ch, 1);
    }
    
    inflateEnd(&istream_fromServer);
}

int main(int argc, char *argv[])
{
    int log_flag = 0;
    int compress_flag = 0;
    int ready_to_exit = 0;
    int PORT = -1;
    
    struct sockaddr_in address;
    int address_length = sizeof(address);
    struct hostent* serverHost;
    
    
    char *log_file;
    
    static const struct option long_options[] = {
        {"port",        required_argument,  0, 'p'},
        {"log",         required_argument,  0 ,'l'},
        {"compress",    no_argument,        0, 'c'},
        {0,0,0,0}
    };
    
    char opt;
    while (1) {
        int option_index = 0;
        opt = getopt_long(argc, argv, "p:l:c", long_options, &option_index);
        
        if (opt == -1)
            break;
        
        switch (opt) {
            case 'p':
                PORT = atoi(optarg); // convert string to integer, then from host byte order to network byte order
                break;
            case 'l':
                log_file = optarg;
                log_flag = 1;
                break;
            case 'c':
                compress_flag = 1;
                break;
            default:
                fprintf(stderr, "UNRECOGNIZED ARGUMENT. Correct usage: lab1b-client '--port=[PORT#]' '--log=[FILENAME]' '--compress'\n");
                exit(1);
        }
    }
    
    if (PORT == -1) {
        fprintf(stderr, "ERROR: No port number specified!");
        exit(1);
    }
    
    // set input mode to non-canonical terminal behavior
    set_input_mode();
    
    // initialize log file
    int log_fd;
    if (log_flag) {
        log_fd = creat(log_file, 0666);
        if (log_fd < 0) {
            fprintf(stderr, "ERROR: Failed to open log file with error %s\n", strerror(errno));
            exit(1);
        }
    }
    
    // initialize zlib streams
    if (compress_flag) {
        // OUT_STREAM
        ostream_toServer.zalloc = Z_NULL;
        ostream_toServer.zfree = Z_NULL;
        ostream_toServer.opaque = Z_NULL;
        
        int deflate_return = deflateInit(&ostream_toServer, Z_DEFAULT_COMPRESSION);
        if (deflate_return != Z_OK) {
            fprintf(stderr, "Unable to create compression stream with error %s\n", strerror(errno));
            exit(1);
        }
        
        // IN_STREAM
        istream_fromServer.zalloc = Z_NULL;
        istream_fromServer.zfree = Z_NULL;
        istream_fromServer.opaque = Z_NULL;
        
        int inflate_return = inflateInit(&istream_fromServer);
        if (inflate_return != Z_OK) {
            fprintf(stderr, "Unable to create decompression stream with error %s\n", strerror(errno));
            exit(1);
        }
    }
    
    // create socket, sockfd returns socket file descriptor
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "ERROR: Failed to create socket with error %s\n", strerror(errno));
        exit(1);
    }
        
    serverHost = gethostbyname("localhost");
    if (!serverHost) {
        fprintf(stderr, "ERROR: Failed to get host name.\n");
        exit(1);
    }

    // initialize address struct
    memset(&address, '0', address_length);
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT); // convert to netwwork byte order
    memcpy((char*) &address.sin_addr.s_addr, (char*) serverHost->h_addr, serverHost->h_length);
        
    if (connect(sockfd, (struct sockaddr *) &address, address_length) < 0) {
        fprintf(stderr, "ERROR: Failed to connect to remote host with error %s\n", strerror(errno));
        exit(1);
    }
        
    // initializing poll struct
    struct pollfd fds[2] = {
        {STDIN_FILENO,  POLLIN|POLLHUP|POLLERR, 0},
        {sockfd,        POLLIN|POLLHUP|POLLERR, 0}
    };
    int MAXSIZE = 256;
    char buf[MAXSIZE];
    
    while (1) {
        int ret = poll(fds,2,0);
        if (ret < 0) {
            fprintf(stderr, "ERROR: Error in polling process with error %s\n", strerror(errno));
            exit(1);
        }
            
        short revents_stdin = fds[0].revents;
        short revents_socket = fds[1].revents;
        
        char prefixS[] = "SENT ";
        char prefixR[] = "RECEIVED ";
        char bytes[] = " bytes: ";
        
        if (revents_stdin & POLLIN) {
            
            int rd = read(STDIN_FILENO, buf, MAXSIZE);
            if (rd < 0) {
                fprintf(stderr, "ERROR: Failed to read into buffer from keyboard with error %s\n", strerror(errno));
                exit(1);
            }
            
            // write to socket with compression
            if (compress_flag)
                do_compress(sockfd, buf, log_flag, log_fd);
            
            // write to socket without compression
            else {
                if (write(sockfd, buf, rd) < 0) {
                    fprintf(stderr, "ERROR: Failed to write from keyboard to socket with error %s\n", strerror(errno));
                    exit(1);
                }
        
                // write to log file, sending data to server
                if (log_flag) {
                    char value[5];
                    sprintf(value, "%d", rd); // write formatted output to value
            
                    // log "SENT "
                    if (write(log_fd, prefixS, strlen(prefixS)) < 0) {
                        fprintf(stderr, "ERROR: Failed to write sent data to log file.");
                        exit(1);
                    }
                    // log "#"
                    if (write(log_fd, value, strlen(value)) < 0) {
                        fprintf(stderr, "ERROR: Failed to write sent data to log file.");
                        exit(1);
                    }
                    // log " bytes: "
                    if (write(log_fd, bytes, strlen(bytes)) < 0) {
                        fprintf(stderr, "ERROR: Failed to write sent data to log file.");
                        exit(1);
                    }
                    // log data
                    if (write(log_fd, buf, rd) < 0) {
                        fprintf(stderr, "ERROR: Failed to write sent data to log file.");
                        exit(1);
                    }
                    // log "\n"
                    if (write(log_fd, "\n", 1) < 0) {
                        fprintf(stderr, "ERROR: Failed to write sent data to log file.");
                        exit(1);
                    }
                }
            }
            
            // write from keyboard to display as each character is processed
            int i;
            for (i = 0; i < rd; i++)
            {
                char ch = buf[i];
                char specialchar[2];
                    
                if (ch == CR || ch == LF) {
                    specialchar[0] = CR;
                    specialchar[1] = LF;
                    if (write(STDOUT_FILENO, &specialchar, 2) < 0) {
                        fprintf(stderr, "ERROR: Failed to write from keyboard to display with error %s\n", strerror(errno));
                        exit(1);
                    }
                }
                
                if (ch == EOT) {
                    specialchar[0] = '^';
                    specialchar[1] = 'D';
                    if (write(STDOUT_FILENO, &specialchar, 2) < 0) {
                        fprintf(stderr, "ERROR: Failed to write from keyboard to display with error %s\n", strerror(errno));
                        exit(1);
                    }
                }
                
                if (ch == KILL) {
                    specialchar[0] = '^';
                    specialchar[1] = 'C';
                    if (write(STDOUT_FILENO, &specialchar, 2) < 0) {
                        fprintf(stderr, "ERROR: Failed to write from keyboard to display with error %s\n", strerror(errno));
                        exit(1);
                    }
                }
                else {
                    if (write(STDOUT_FILENO, &ch, 1) < 0) {
                        fprintf(stderr, "ERROR: Failed to write from keyboard to display with error %s\n", strerror(errno));
                        exit(1);
                    }
                }
            }
        }
        
        // processing input from socket
        if (revents_socket & POLLIN) {
            memset(buf, 0, MAXSIZE); // clear buffer content
            int rd = read(sockfd, buf, MAXSIZE);
            if (rd < 0) {
                fprintf(stderr, "ERROR: Failed to read into buffer from socket with error %s\n", strerror(errno));
                exit(1);
            }
            if (rd == 0) // socket to the server is closed
                exit(0);
            
            // write to log file, receiving data from server
            if (log_flag) {
                int i;
                for (i = 0; i < rd; i++) {
                    char ch = buf[i];
                    // log "RECEIVED "
                    if (write(log_fd, prefixR, strlen(prefixR)) < 0) {
                        fprintf(stderr, "ERROR: Failed to write received data to log file.");
                        exit(1);
                    }
                    // log "#"
                    if (write(log_fd, "1", 1) < 0) {
                        fprintf(stderr, "ERROR: Failed to write received data to log file.");
                        exit(1);
                    }
                    // log " bytes: "
                    if (write(log_fd, bytes, strlen(bytes)) < 0) {
                        fprintf(stderr, "ERROR: Failed to write received data to log file.");
                        exit(1);
                    }
                    // log data
                    if (write(log_fd, &ch, 1) < 0) {
                        fprintf(stderr, "ERROR: Failed to write received data to log file.");
                        exit(1);
                    }
                    // log "\n"
                    if (write(log_fd, "\n", 1) < 0) {
                        fprintf(stderr, "ERROR: Failed to write received data to log file.");
                        exit(1);
                    }
                }
            }
            
            if (compress_flag)
                do_decompress(buf, rd);
            
            // write to display without decompression
            else {
                if (write(STDOUT_FILENO, buf, rd) < 0) {
                    fprintf(stderr, "ERROR: Failed to write from socket to display with error %s\n", strerror(errno));
                    exit(1);
                }
            }
        }
            
        if (revents_stdin & (POLLERR | POLLHUP))
        {
            fprintf(stderr, "STDIN HANGUP/ERROR with error %s\n", strerror(errno));
            ready_to_exit = 1;
        }
            
        if (revents_socket & (POLLERR | POLLHUP))
        {
            close(sockfd);
            ready_to_exit = 1;
        }
            
        if (ready_to_exit)
            break;
        
    }
    
    shutdown(sockfd, SHUT_RDWR); // shutdown socket
    exit(0);
}

void reset_input_mode()
{
    int tcset = tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
    if (tcset < 0)
    {
        fprintf(stderr, "ERROR: Failed to set attributes in reset_input_mode() with error %s\n", strerror(errno));
        exit(1); // system call failure
    }
}

void set_input_mode()
{
    struct termios new_attributes;
    
    // set terminal modes
    int tcget = tcgetattr(STDIN_FILENO, &new_attributes);
    if (tcget < 0)
    {
        fprintf(stderr, "ERROR: Failed to get attributes in set_input_mode() with error %s\n", strerror(errno));
        exit(1); // system call failure
    }
    
    //save terminal attributes to restore them later
    saved_attributes = new_attributes;
    
    new_attributes.c_iflag = ISTRIP; // only lower 7 bits
    new_attributes.c_oflag = 0; // no processing
    new_attributes.c_lflag = 0; // no processing
    int tcset = tcsetattr(STDIN_FILENO, TCSANOW, &new_attributes);
    if (tcset < 0)
    {
        fprintf(stderr, "ERROR: Failed to set attributes in set_input_mode() with error %s\n", strerror(errno));
        exit(1); // system call failure
    }
    
    atexit(reset_input_mode); // restore saved attributes before exit
}
