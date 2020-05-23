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
#include <mraa/gpio.h>
#include <mraa/aio.h>

sig_atomic_t volatile run_flag = 1;

const int B = 4275; // B value of the thermistor
const int R0 = 100000; // R0 = 100k

int period = 1; // default to 1 second
int scaleF = 1; // default to Fahreinheit
int scaleC = 0;
int genReport = 1;
int log_flag = 0;

void stop() {
    run_flag = 0;
}

void exit_with_error(char* message) {
    fprintf(stderr, "%s with error: %s\n", message, strerror(errno));
    exit(1);
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
        stop();
}

int main(int argc, char *argv[]) {
    char *log_file;
    static const struct option long_options[] = {
        {"period",  optional_argument, 0, 'p'},
        {"scale",   optional_argument, 0, 's'},
        {"log",     optional_argument, 0, 'l'},
        {0,0,0,0}
    };

    int opt;
    while (1) {
        int option_index = 0;
        opt = getopt_long(argc, argv, "p:s:l:", long_options, &option_index);

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
                log_flag = 1;
                break;
            default:
                exit_with_error("ERROR: Unrecognized argument. Correct usage: lab4b '--period=#' '--scale=[C] or [F]' '--log=[filename]'");
        }
    }

    signal(SIGSEGV, segfault_handler);

    // initialize log file
    int log_fd;
    if (log_flag) {
        log_fd = creat(log_file, 0666);
        if (log_fd < 0)
            exit_with_error("ERROR: Failed to open log file");
    }
    
    mraa_aio_context tempSensor;
    mraa_gpio_context button;
    // initialize I/O devices
    tempSensor = mraa_aio_init(1); // Analog A0/A1 connector as I/O pin #1
    if (tempSensor == NULL) {
        fprintf(stderr, "Failed to initialize AIO\n");
        mraa_deinit();
        exit(1);
    }
    button = mraa_gpio_init(60); // GPIO_50 connector as I/O pin #60
    if (button == NULL) {
        fprintf(stderr, "Failed to initialize GPIO %d\n", 60);
        mraa_deinit();
        exit(1);
    }

    mraa_gpio_dir(button, MRAA_GPIO_IN); // configure button GPIO interface to be an input pin
    mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &stop, NULL); // stop collecting data when button is pressed

    // initialize poll
    struct pollfd keyboard;
    keyboard.fd = STDIN_FILENO;
    keyboard.events = POLLIN | POLLERR | POLLHUP;

    time_t rawStart;
    time(&rawStart);

    while (run_flag) {
        int ret = poll(&keyboard,1,0);
        if (ret < 0)
            exit_with_error("ERROR: Failed to poll for stdin");
        
        if (keyboard.revents & POLLIN) {
            char command[20];
            fgets(command,20,stdin);
            if (log_flag) {
                if (write(log_fd, command, strlen(command)) < 0)
                        exit_with_error("ERROR: Unable to write command to log file");
            }
            strtok(command, "\n");
            processInput(command);
        }

        if (keyboard.revents & (POLLHUP|POLLERR))
            exit_with_error("ERROR: STDIN hangup/err");
        
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
                sprintf(str, "%d:%d:%d %.1f\n", end->tm_hour, end->tm_min, end->tm_sec, temperatureF);
            if (scaleC)
                sprintf(str, "%d:%d:%d %.1f\n", end->tm_hour, end->tm_min, end->tm_sec, temperatureC);
            
            // write report to stdout (fd 1)
            if (genReport) {
                if (write(1, str, strlen(str)) < 0)
                    exit_with_error("ERROR: Unable to write to output");

                if (log_flag) {
                    if (write(log_fd, str, strlen(str)) < 0)
                        exit_with_error("ERROR: Unable to write to log file");
                }
            }
            rawStart = rawEnd;
        }
    }

    time_t rawFinal;
    struct tm *final;
    time(&rawFinal);
    final = localtime(&rawFinal);
    char exitMsg[20];
        sprintf(exitMsg, "%d:%d:%d SHUTDOWN\n", final->tm_hour, final->tm_min, final->tm_sec);
    //if (write(1, exitMsg, strlen(exitMsg)) < 0)
        //exit_with_error("ERROR: Unable to write to output");
    if (log_flag) {
        if (write(log_fd, exitMsg, strlen(exitMsg)) < 0)
            exit_with_error("ERROR: Unable to write to log file");
    }
    
    // close I/O devices
    mraa_result_t status;
    status = mraa_aio_close(tempSensor);
    if (status != MRAA_SUCCESS)
        exit_with_error("Failed to close temperature sensor");
    status = mraa_gpio_close(button);
    if (status != MRAA_SUCCESS) 
        exit_with_error("Failed to close button");

    exit(0);
}