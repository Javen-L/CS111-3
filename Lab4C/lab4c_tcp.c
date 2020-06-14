// NAME: Anh Mac
// EMAIL: anhmvc@gmail.com
// UID: 905111606

#include <stdio.h>  // for fprintf(3), atoi(3)
#include <stdlib.h> // for atoi(3)
#include <errno.h> // for errno(3)
#include <getopt.h> // for getopt_long(3)
#include <unistd.h> // for getopt_long(3), write(2)
#include <string.h> // for strerror(3), atoi(3)
#include <fcntl.h> // for creat(2)
#include <sys/stat.h> // for creat(2)
#include <sys/types.h> // for creat(2)
#include <signal.h>
#include <poll.h> // for poll(2)
#include <math.h>
#include <time.h> // for localtime(3)
#include <sys/types.h> // for socket(2), connect(2)
#include <sys/socket.h> // for socket(2), connect(2)
#include <netdb.h> // for gethostbyname(3)
#include <arpa/inet.h> // for htons(3)
#include <mraa/aio.h>

const int B = 4275; // B value of the thermistor
const int R0 = 100000; // R0 = 100k

int period = 1; // default to 1 second
int scaleF = 1; // default to Fahreinheit
int scaleC = 0;
int genReport = 0;
int isStop = 0;
int ID;
char* hostname;
int PORT = -1;
long val;
char *next;

void exit_invalid_parameters(char* message) {
    fprintf(stderr, "%s\n", message);
    exit(1);
}

void exit_runtime_failures(char* message) {
    fprintf(stderr, "%s\n", message);
    exit(2);
}

void segfault_handler() {
    fprintf(stderr, "ERROR: Segmentation fault.\n");
    exit(1);
}

void processInput(const char *command) {
    if (strcmp(command,"SCALE=F") == 0) {
        scaleF = 1;
        scaleC = 0;
    }
    else if (strcmp(command,"SCALE=C") == 0)
        scaleC = 1;
    else if (strncmp(command,"PERIOD=",strlen("PERIOD=")) == 0)
        period = atoi(command+7);
    else if (strcmp(command,"STOP") == 0)
        genReport = 0;
    else if (strcmp(command,"START") == 0)
        genReport = 1;
    else if (strncmp(command,"LOG",strlen("LOG")) == 0)
        {/*do nothing*/}
    else if (strcmp(command,"OFF") == 0)
        isStop = 1;
}

int main(int argc, char *argv[]) {
    char *log_file;
    static const struct option long_options[] = {
        {"period",  optional_argument, 0, 'p'},
        {"scale",   optional_argument, 0, 's'},
        {"log",     required_argument, 0, 'l'},
        {"id",      required_argument, 0, 'i'},
        {"host",    required_argument, 0, 'h'},
        {0,0,0,0}
    };

    if (argc < 4)
        exit_invalid_parameters("ERROR: Invalid number of arguments. Correct usage: lab4c optional: '--period=#' '--scale=[C][F]' mandatory: --log=[filename] --id=[9-digit-number] --host=[name/address] [port#]");

    int opt;
    while (1) {
        int option_index = 0;
        opt = getopt_long(argc, argv, "p:s:l:i:h:", long_options, &option_index);

        if (opt == -1)
            break;
        switch (opt) {
            case 'p':
                period = atoi(optarg);
                break;
            case 's':
                if (optarg[0] == 'C')
                    scaleC = 1;
                if (optarg[0] == 'F')
                    scaleF = 1;
                break;
            case 'l':
                log_file = optarg;
                break;
            case 'i':
                val = strtol(optarg, &next, 10); // make sure id is 9-digit input
                // Check for empty string and characters left after conversion.
                if ((next == optarg) || (*next != '\0') || strlen(optarg) != 9)
                    exit_invalid_parameters("ERROR: Incorrect ID format. Correct usage: --id=[9-digit-number]");
                ID = val;
                break;
            case 'h':
                hostname = optarg;
                break;
            default:
                exit_invalid_parameters("ERROR: Unrecognized argument. Correct usage: lab4c '--period=#' '--scale=[C][F]' --log=[filename] --id=[9-digit-number] --host=[name/address] [port#]");
        }
    }
    // process non-switch parameters, check if there's bogus non-option parameters
    val = strtol(argv[optind], &next, 10);
    if ((next == argv[optind]) || (*next != '\0'))
        exit_invalid_parameters("ERROR: Invalid arguments");

    PORT = atoi(argv[optind]);
    if (PORT == -1)
        exit_invalid_parameters("ERROR: No port number specified");

    signal(SIGSEGV, segfault_handler);

    // initialize log file
    int log_fd = creat(log_file, 0666);
    if (log_fd < 0)
        exit_runtime_failures("ERROR: Unable to open log file");

    // create socket, sockfd returns socket file descriptor
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        exit_runtime_failures("ERROR: Unable to create socket");
    
    struct hostent* serverHost;
    struct sockaddr_in address;
    int address_length = sizeof(address);

    serverHost = gethostbyname(hostname);
    if (!serverHost)
        exit_runtime_failures("ERROR: Unable to get hostname");
    
    // initialize address struct
    memset(&address, '0', address_length);
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    memcpy((char*) &address.sin_addr.s_addr, (char*) serverHost->h_addr, serverHost->h_length);
    
    if (connect(sockfd, (struct sockaddr*) &address, address_length) < 0)
        exit_runtime_failures("ERROR: Unable to connect to remote host");
    
    FILE* fp = fdopen(sockfd, "r");
    
    char ID_buf[30];
    sprintf(ID_buf, "ID=%d\n", ID);
    if (write(sockfd, ID_buf, strlen(ID_buf)) < 0)
        exit_runtime_failures("ERROR: Unable to write to socket"); // send ID to socket
    if (write(log_fd, ID_buf, strlen(ID_buf)) < 0)
        exit_runtime_failures("ERROR: Unable to write to log file"); // log ID 

    // initialize I/O devices
    mraa_aio_context tempSensor;
    tempSensor = mraa_aio_init(1); // Analog A0/A1 connector as I/O pin #1
    if (tempSensor == NULL) {
        mraa_deinit();
        exit_runtime_failures( "ERROR: Unable to initialize AIO");
    }

    // initialize poll
    struct pollfd socket;
    socket.fd = sockfd;
    socket.events = POLLIN | POLLERR | POLLHUP;

    time_t rawStart;
    time(&rawStart);

    while (1) {
        int ret = poll(&socket,1,0);
        if (ret < 0)
            exit_runtime_failures("ERROR: Unable to poll for socket");
        
        if (socket.revents & POLLIN) {
            char command[20];
            fgets(command, 20, fp);
            if (write(log_fd, command, strlen(command)) < 0)
                exit_runtime_failures("ERROR: Unable to write command to log file");
            strtok(command, "\n");
            processInput(command);
        }

        if (socket.revents & (POLLHUP|POLLERR)) {
            close(sockfd);
            fprintf(stderr, "ERROR: Poll hangup or error\n");
            break;
        }
        
        if (isStop)
            break;
        
        // read temperature from tempSensor
        float a = mraa_aio_read(tempSensor);
        float R = 1023.0/a-1.0;
        R = R0*R;

        float temperatureC = 1.0/(log(R/R0)/B+1/298.15)-273.15; // convert reading into celsius temperature
        float temperatureF = temperatureC * 1.8 + 32.0; // convert to Fahreinheit

        time_t rawEnd;
        struct tm *end;
        time(&rawEnd);
        end = localtime(&rawEnd);

        if (difftime(rawEnd,rawStart) == period) {
            char str[20];
            if (scaleF)
                sprintf(str, "%02d:%02d:%02d %.1f\n", end->tm_hour, end->tm_min, end->tm_sec, temperatureF);
            if (scaleC)
                sprintf(str, "%02d:%02d:%02d %.1f\n", end->tm_hour, end->tm_min, end->tm_sec, temperatureC);
            
            // write report to stdout (fd 1)
            if (genReport) {
                if (write(sockfd, str, strlen(str)) < 0)
                    exit_runtime_failures("ERROR: Unable to write to socket");
                if (write(log_fd, str, strlen(str)) < 0)
                    exit_runtime_failures("ERROR: Unable to write to log file");
            }
            rawStart = rawEnd;
        }
    }

    time_t rawFinal;
    struct tm *final;
    time(&rawFinal);
    final = localtime(&rawFinal);
    char exitMsg[20];
        sprintf(exitMsg, "%02d:%02d:%02d SHUTDOWN\n", final->tm_hour, final->tm_min, final->tm_sec);
    if (write(sockfd, exitMsg, strlen(exitMsg)) < 0)
        exit_runtime_failures("ERROR: Unable to write to socket");
    if (write(log_fd, exitMsg, strlen(exitMsg)) < 0)
        exit_runtime_failures("ERROR: Unable to write to log file");
    
    // close I/O devices
    mraa_result_t status;
    status = mraa_aio_close(tempSensor);
    if (status != MRAA_SUCCESS)
        exit_runtime_failures("ERROR: Unable to close temperature sensor");

    shutdown(sockfd, SHUT_RDWR);
    exit(0);
}