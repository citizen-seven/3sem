#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>

struct sigaction act; 
sigset_t set;

#define ERR(...)\
    {\
        fprintf(stderr, __VA_ARGS__);\
        fprintf(stderr, "\nERRNO: %s\n", strerror(errno));\
        exit(EXIT_FAILURE);\
    }

#define ERRTEST(...) \
    if ((__VA_ARGS__) == -1) \
        ERR("Code line: [" #__VA_ARGS__ "]")

#define SET_SIGNAL(signum, handler) \
    act.sa_handler = handler; \
    ERRTEST(sigaction(signum, &act, NULL))

int signal_bit;
int is_eof;

#define SIG0 SIGUSR1
#define SIG1 SIGUSR2
#define SIGEOF SIGCONT

void bit_accepted(int signo) {
    switch (signo) {
        case SIG0: {
            signal_bit = 0;
            break;
        }
        case SIG1: {
            signal_bit = 1;
            break;
        }
        case SIGEOF: {
            is_eof = 1;
            break;
        }
    }
}

// 0 -- bit 0
// 1 -- bit 1
// SIGCONT -- EOF

#define SEND_AND_CHECK(signum)\
    ERRTEST(kill(pid, signum))\
    sigsuspend(&set);

void send_data(int data, int pid) {
    switch (data) {
        case 0: {
            SEND_AND_CHECK(SIG0)
            break;
        }
        case 1: {
            SEND_AND_CHECK(SIG1)
            break;
        }
        default: { // EOF
            SEND_AND_CHECK(SIGEOF)
            break;
        }
    }
}

void do_child(int pid, char* filename) {
    int fd;
    ERRTEST(fd = open(filename, O_RDONLY))
    char byte;
    int i, bit;
    while (read(fd, &byte, 1)) {
        for(i = 0; i < 8; i++) {
            bit = (byte >> i) & 1;
            send_data(bit, pid);
        }
    }
    send_data(-1, pid);
    close(fd);
}

#define SEND_RESPONSE ERRTEST(kill(pid, SIG0))

void child_death() {
    ERR("Child has died, terminating")
}

void process_bit(char bit, int fd) {
    static char byte = 0;
    static unsigned int count = 0;
    byte |= (bit << count++);
    if (count == 8) {
        write(fd, &byte, 1);
        byte = count = 0;
    }
}

void do_parent(const int pid, char* filename) {
    int fd;
    ERRTEST(fd = open(filename, O_WRONLY | O_CREAT, 0664))
    while(1) {
        sigsuspend(&set);
        if (is_eof) { // eof
            SET_SIGNAL(SIGCHLD, SIG_IGN)
            SEND_RESPONSE
            break;
        }
        else {
            //printf("signal: %d\n", signal_bit);
            process_bit(signal_bit, fd);
            SEND_RESPONSE
        }  
    }
    close(fd);
}

int main(int argc, char *argv[]) {    
    sigemptyset(&set);   

    SET_SIGNAL(SIGCHLD, child_death) // child dies => parent do as well
    SET_SIGNAL(SIG0, bit_accepted)
    SET_SIGNAL(SIG1, bit_accepted)
    SET_SIGNAL(SIGEOF, bit_accepted)

    sigaddset(&set, SIG0); 
    sigaddset(&set, SIG1);
    sigprocmask(SIG_SETMASK, &set, NULL); // Block user signals till fork()
    sigemptyset(&set);

    if (argc != 3)
        ERR("Usage: %s [filename from] [filename to]" , *argv)
    int fork_ret;
    int parent_pid = getpid();
    ERRTEST(fork_ret = fork())
    if (fork_ret)
        do_parent(fork_ret, argv[2]);
    else {
        do_child(parent_pid, argv[1]);
    }
    return 0;  
}
