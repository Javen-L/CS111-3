// NAME: Anh Mac
// EMAIL: anhmvc@gmail.com
// UID: 905111606

#include <termios.h> // for termios(3), tcgetattr(3), tcsetattr(3)
#include <unistd.h> // for termios(3), tcgetattr(3), tcsetattr(3), fork(2), read(2), write(2), exec(3), pipe(2), getopt_long(3)
#include <stdlib.h> // for atexit(3)
#include <stdio.h> // for fprintf(3)
#include <string.h> // for strerror(3)
#include <errno.h> // for errno(3)
#include <getopt.h> // for getopt_long(3)
#include <sys/types.h> // for waitpid(2)
#include <sys/wait.h> // for waitpid(2)
#include <poll.h> // for poll(2)
#include <signal.h> // for kill(3)

#define EOT '\004' // ^D
#define KILL '\003' // ^C

// store initial terminal attributes
struct termios saved_attributes;

int toShell[2];
int toTerminal[2];
int ready_to_exit = 0;
pid_t pid;

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


void create_pipe(int* fd)
{
    int pd = pipe(fd);
    if (pd != 0)
    {
        fprintf(stderr, "ERROR: Failure in creating pipes with error %s\n", strerror(errno));
        exit(1);
    }
}

void signal_handler(int signum)
{
    if (signum == SIGPIPE)
    {
        fprintf(stderr, "SIGPIPE SIGNAL RECEIVED!");
        ready_to_exit = 1;
    }
}

void noShellMode()
{
    // read from stdin one byte at a time
    char buf[8];
        
    // read input from the keyboard (stdin)
    while (1)
    {
        int rd = read(STDIN_FILENO, buf, 8);
        if (rd < 0)
        {
            fprintf(stderr, "ERROR: Failed to read input from stdin with error %s\n", strerror(errno));
            exit(1);
        }
    
        int i;
        for (i = 0; i < rd; i++)
        {
    
            if (buf[i] == '\r' || buf[i] == '\n')
            {
                buf[i] = '\r';
                if (write(STDOUT_FILENO, &buf[i], 1) < 0) // write to stdout
                {
                    fprintf(stderr, "ERROR: Error in writing to stdout with error %s\n", strerror(errno));
                    exit(1);
                }
                buf[i] = '\n';
                if (write(STDOUT_FILENO, &buf[i], 1) < 0) // write to stdout
                {
                    fprintf(stderr, "ERROR: Error in writing to stdout with error %s\n", strerror(errno));
                    exit(1);
                }
            }
        
            else if (buf[i] == EOT) // EOT -- ^D
            {
                // writing to stdout
                buf[i] = '^';
                if (write(STDOUT_FILENO, &buf[i], 1) < 0)
                {
                    fprintf(stderr, "ERROR: Error in writing to stdout with error %s\n", strerror(errno));
                    exit(1);
                }
                buf[i] = 'D';
                if (write(STDOUT_FILENO, &buf[i], 1) < 0)
                {
                    fprintf(stderr, "ERROR: Error in writing to stdout with error %s\n", strerror(errno));
                    exit(1);
                }
                exit(0);
            }
        
            else
            {
                // write to stdout
                if (write(STDOUT_FILENO, &buf[i], 1) < 0)
                {
                    fprintf(stderr, "ERROR: Error in writing to STDOUT with error %s\n", strerror(errno));
                    exit(1);
                }
            }
        }
    }
}

void readFromShell()
{
    char buf[256];
    
    // read input from the shell
    int rd = read(toTerminal[0], buf, 256);
    if (rd < 0)
    {
        fprintf(stderr, "ERROR: Failed to read input from stdin with error %s\n", strerror(errno));
        exit(1);
    }
        
    // write to stdout
    int i;
    for (i = 0; i < rd; i++)
    {
        if (buf[i] == '\n') // if received <lf> write to stdout as <cr><lf>
        {
            buf[i] = '\r';
            if (write(STDOUT_FILENO, &buf[i], 1) < 0) // write to stdout
            {
                fprintf(stderr, "ERROR: Error in writing from shell to stdout with error %s\n", strerror(errno));
                exit(1);
            }
            buf[i] = '\n';
            if (write(STDOUT_FILENO, &buf[i], 1) < 0) // write to stdout
            {
                fprintf(stderr, "ERROR: Error in writing from shell to stdout with error %s\n", strerror(errno));
                exit(1);
            }
        }
        
        else if (buf[i] == EOT) // ^D
        {
            buf[i] = '^';
            if (write(STDOUT_FILENO, &buf[i], 1) < 0)
            {
                fprintf(stderr, "ERROR: Error in writing from shell to stdout with error %s\n", strerror(errno));
                exit(1);
            }
            buf[i] = 'D';
            if (write(STDOUT_FILENO, &buf[i], 1) < 0)
            {
                fprintf(stderr, "ERROR: Error in writing from shell to stdout with error %s\n", strerror(errno));
                exit(1);
            }
            ready_to_exit = 1;
        }
        
        else
        {
            // write to STDOUT
            if (write(STDOUT_FILENO, &buf[i], 1) < 0)
            {
                fprintf(stderr, "ERROR: Error in writing from shell to stdout with error %s\n", strerror(errno));
                exit(1);
            }
        }
    }
}

void readFromKeyboard()
{
    char buf[8];
        
    // read input from the keyboard (stdin)
    int rd = read(STDIN_FILENO, buf, 8);
    if (rd < 0)
    {
        fprintf(stderr, "ERROR: Failed to read input from stdin with error %s\n", strerror(errno));
        exit(1);
    }
    
    int i;
    for (i = 0; i < rd; i++)
    {
    
        if (buf[i] == '\r' || buf[i] == '\n') // if received <cr> or <lf> mapped to <cr><lf> for stdout but go to shell as <lf>
        {
            buf[i] = '\r';
            if (write(STDOUT_FILENO, &buf[i], 1) < 0) // write to stdout
            {
                fprintf(stderr, "ERROR: Error in writing to stdout with error %s\n", strerror(errno));
                exit(1);
            }
            buf[i] = '\n';
            if (write(STDOUT_FILENO, &buf[i], 1) < 0) // write to stdout
            {
                fprintf(stderr, "ERROR: Error in writing to stdout with error %s\n", strerror(errno));
                exit(1);
            }
            if (write(toShell[1], &buf[i], 1) < 0) // write to shell
            {
                fprintf(stderr, "ERROR: Error in writing to SHELL with error %s\n", strerror(errno));
                exit(1);
            }
        
        }
        
        else if (buf[i] == EOT) // EOT -- ^D
        {
            // writing to stdout
            buf[i] = '^';
            if (write(STDOUT_FILENO, &buf[i], 1) < 0)
            {
                fprintf(stderr, "ERROR: Error in writing to stdout with error %s\n", strerror(errno));
                exit(1);
            }
            buf[i] = 'D';
            if (write(STDOUT_FILENO, &buf[i], 1) < 0)
            {
                fprintf(stderr, "ERROR: Error in writing to stdout with error %s\n", strerror(errno));
                exit(1);
            }
            
            close(toShell[1]);
            
        }
        
        else if (buf[i] == KILL) // KILL -- ^C
        {
            // writing to stdout
            buf[i] = '^';
            if (write(STDOUT_FILENO, &buf[i], 1) < 0)
            {
                fprintf(stderr, "ERROR: Error in writing to stdout with error %s\n", strerror(errno));
                exit(1);
            }
            buf[i] = 'C';
            if (write(STDOUT_FILENO, &buf[i], 1) < 0)
            {
                fprintf(stderr, "ERROR: Error in writing to stdout with error %s\n", strerror(errno));
                exit(1);
            }
            
            // send a SIGINT to the shell process
            if (kill(pid, SIGINT) < 0)
            {
                fprintf(stderr, "ERROR: Killing shell process with error %s\n", strerror(errno));
                exit(1);
            }
        }
        
        else
        {
            // write to stdout
            if (write(STDOUT_FILENO, &buf[i], 1) < 0)
            {
                fprintf(stderr, "ERROR: Error in writing to STDOUT with error %s\n", strerror(errno));
                exit(1);
            }
        
            // write to shell
            if (write(toShell[1], &buf[i], 1) < 0)
            {
                fprintf(stderr, "ERROR: Error in writing to shell with error %s\n", strerror(errno));
                exit(1);
            }
        }
    }
}

int main(int argc, char * argv[])
{
    int shell_flag = 0;
    char *arg[2];
    char *program;
    
        
    static const struct option long_options[] =
    {
        {"shell", required_argument, 0, 's'},
        {0,0,0,0}
    };
    
    char opt;
    while (1)
    {
        int option_index = 0;
        opt = getopt_long(argc, argv, "s:", long_options, &option_index);
    
        if (opt == -1)
            break;
        
        switch (opt)
        {
            case 's':
                program = optarg;
                shell_flag = 1;
                break;
            default:
                fprintf(stderr, "ERROR: Unrecognized argument.\n");
                exit(1);
        }
    }
    
    set_input_mode(); // set mode to character-at-a-time, no-echo mode
        
    if (shell_flag)
    {
        // creating pipes and forking
        create_pipe(toShell);
        create_pipe(toTerminal);
        
        signal(SIGPIPE, signal_handler);
        
        pid = fork();
        if (pid < 0)
        {
            fprintf(stderr, "ERROR: Failure in forking! with error %s\n", strerror(errno));
            exit(1);
        }
        else if (pid == 0) // Child process [SHELL]
        {
            // child process closes input in toTerminal and only allow to send output (write) to Terminal
            close(toTerminal[0]);
            // child process closes output toShell and only allow to receive input (read) from Terminal
            close(toShell[1]);
                
            int dp;
            close(STDIN_FILENO);
            dp = dup(toShell[0]);
            if (dp == -1)
            {
                fprintf(stderr, "ERROR: Failed to duplicate stdin in child process with error %s\n", strerror(errno));
                exit(1);
            }
            close(toShell[0]);
            
            close(STDOUT_FILENO);
            dp = dup(toTerminal[1]);
            if (dp == -1)
            {
                fprintf(stderr, "ERROR: Failed to duplicate stdout in child process with error %s\n", strerror(errno));
                exit(1);
            }
            close(STDERR_FILENO);
            dp = dup(toTerminal[1]);
            if (dp == -1)
            {
                fprintf(stderr, "ERROR: Failed to duplicate stderr in child process with error %s\n", strerror(errno));
                exit(1);
            }
            close(toTerminal[1]);
                
            // exec a shell
            arg[0] = program;
            arg[1] = NULL;
                
            if (execvp(program, arg) == -1)
            {
                fprintf(stderr, "ERROR: Failed to exec shell with error %s\n", strerror(errno));
                exit(1);
            }
        }
        else // Parent process [TERMINAL]
        {
            // parent process closes output in toTerminal and only receive stdin (read) from Shell
            close(toTerminal[1]);
            // parent process closes input toShell and only send output (write) to Shell
            close(toShell[0]);
            ready_to_exit = 0;
                
            // read input from keyboard, echo to stdout, and forward to the shell
            
            // create an array of 2 pollfd structures
            struct pollfd fds[2];
            int exitstatus;
                
            fds[0].fd = STDIN_FILENO;
            fds[0].events = POLLIN;
                
            fds[1].fd = toTerminal[0];
            fds[1].events = POLLIN;
                
            while (1)
            {
                int ret = poll(fds, 2, 0);
                if (ret == -1)
                {
                    fprintf(stderr, "ERROR: Error in poll process with error %s\n", strerror(errno));
                    exit(1);
                }
                    
                short revents_stdin = fds[0].revents;
                short revents_shell = fds[1].revents;
                    
                // pending input from keyboard
                if (revents_stdin & POLLIN)
                {
                    readFromKeyboard();
                }
                
                if (revents_stdin & (POLLHUP|POLLERR))
                {
                    fprintf(stderr, "STDIN hangup/error.\n");
                    ready_to_exit = 1;
                }
                // pending input from shell
                if (revents_shell & POLLIN)
                {
                    readFromShell();
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
                
            if (waitpid(pid, &exitstatus, 0) < 0)
            {
                fprintf(stderr, "ERROR: Waiting child process with error %s\n", strerror(errno));
                exit(1);
            }
                
            // output shell's exit status
                
            int SIGNAL = 0x007f & exitstatus;
            int STATUS = exitstatus>>8;
                
            fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", SIGNAL, STATUS);
            exit(0);
        }
    }
            
        
    else
    {
        noShellMode();
    }
    
    exit(0);
}
