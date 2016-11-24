#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>

void execute(char **, int, char **);
void handle_signal(int);
int parse(char *, char **, char **, int *);
void chop(char *);

#define STRING_SIZE 256

#define ERR(string, ...) fprintf (stderr, string, ## __VA_ARGS__)
#define NORMAL 				0
#define OUTPUT_REDIRECTION 	1
#define INPUT_REDIRECTION 	2
#define PIPELINE 			3

int main() {
    printf("=======================================\n Hello, this is simple shell  emulator!\n=======================================\n");
    printf("Please enter one expression using | > < symbols for redirecting streams\n");
	int mode = NORMAL, cmdArgc;
	size_t len = STRING_SIZE;
	char  *inputString, *program[STRING_SIZE], *supplement = NULL;
	inputString = (char*)malloc(sizeof(char)*STRING_SIZE);
    while (1) {
        mode = NORMAL;
	    getline (&inputString, &len, stdin);
        if (strcmp(inputString, "exit\n") == 0) {
            free(inputString);
            exit(0);
        }
	    cmdArgc = parse(inputString, program, &supplement, &mode);
	    execute(program, mode, &supplement);
    }
    free(inputString);
	return 0;
}

int parse(char *inputString, char *program[], char **buff, int *modePtr)
{
	int cmdArgc = 0, terminate = 0;
	char *srcPtr = inputString;
	while(*srcPtr != '\0' && *srcPtr != ' ' && terminate == 0 && *srcPtr != '\n') {
		*program = srcPtr;
		cmdArgc++;
		while(*srcPtr != ' ' && *srcPtr != '\t' && *srcPtr != '\0' && *srcPtr != '\n' && terminate == 0) {
			switch(*srcPtr) {
				case '>':
					*modePtr = OUTPUT_REDIRECTION;
					*program = NULL;
					srcPtr++;
					while(*srcPtr == ' ' || *srcPtr == '\t')
						srcPtr++;
					*buff = srcPtr;
					chop(*buff);
					terminate = 1;
					break;
				case '<':
					*modePtr = INPUT_REDIRECTION;
					*program = NULL;
					srcPtr++;
					while(*srcPtr == ' ' || *srcPtr == '\t')
						srcPtr++;
					*buff = srcPtr;
					chop(*buff);
					terminate = 1;
					break;
				case '|':
					*modePtr = PIPELINE;
					*program = NULL;
					srcPtr++;
					while(*srcPtr == ' ' || *srcPtr == '\t')
						srcPtr++;
					*buff = srcPtr;
					terminate = 1;
					break;
			}
			srcPtr++;
		}
		while((*srcPtr == ' ' || *srcPtr == '\t' || *srcPtr == '\n') && terminate == 0) {
			*srcPtr = '\0';
			srcPtr++;
		}
		program++;
	}
	*program = NULL;
	return cmdArgc;
}

void chop(char *srcPtr)
{
	while(*srcPtr != ' ' && *srcPtr != '\t' && *srcPtr != '\n') {
		srcPtr++;
	}
	*srcPtr = '\0';
}

void execute(char **program, int mode, char **buff)
{
	int fd = 0, fd0 = STDIN_FILENO, fd1 = STDOUT_FILENO;
    pid_t pid = 0, pid2 = 0;
    int counter = 0, cc = 0;
    int fdc[2];
	int mode2 = NORMAL, status1;
	char *program2[STRING_SIZE], *supplement2 = NULL;
	int myPipe[2];
	if (mode == PIPELINE) {
		if(pipe(myPipe)) {
			ERR("Error while making pipe:%s", strerror(errno));
			exit(-1);
		}
		parse(*buff, program2, &supplement2, &mode2);
    }
    //printf("programs %s, %s\n", program[0], program2[0]);
	pid = fork();
	if (pid < 0) {
		ERR("Error while forking:%s", strerror(errno));
		exit(-1);
	}
	else if (pid == 0) {
		switch(mode)
		{
			case OUTPUT_REDIRECTION:
				 fd = open(*buff, O_WRONLY | O_CREAT | O_TRUNC, 0664);
                 if (fd == -1) {
                     ERR("Can't create file %s for redirection: %s\n", *buff, strerror(errno));
                     goto out;
                 }
                 dup2(fd, fd1);
                 close(fd);
                 break;
			case INPUT_REDIRECTION:
				fd = open(*buff, O_RDONLY);
                if (fd == -1) {
                    ERR("Can't create file %s for redirection: %s\n", *buff, strerror(errno));
                    goto out;
                }
				dup2(fd, fd0);
				break;
			case PIPELINE:
				close(myPipe[0]);		//close input of pipe
				dup2(myPipe[1], fd1);
				close(myPipe[1]);
				break;
            default: {
                //printf("Normal mode\n");
                execvp(*program, program);           
                exit(-1);
            }

		}
		if (execvp(*program, program) == -1) {
            ERR("Execvp error with %s: %s\n", *program, strerror(errno));
            goto out;
        }
    }
    else {
		if(mode == PIPELINE) {
			waitpid(pid, &status1, 0);		//wait for process 1 to finish
			pid2 = fork();
			if (pid2 < 0) {
				ERR("Error while forking:%s", strerror(errno));
				exit(-1); //close files
			}
			else if (pid2 == 0) {
				close(myPipe[1]);		//close output to pipe
				dup2(myPipe[0], fd0);
				close(myPipe[0]);
                pipe(fdc);
                if (fork() == 0) {
                    dup2(fdc[1], 1);
                    close(fdc[0]);
                    close(fdc[1]);
				    if (execvp(*program2, program2) == -1) {
                        ERR("Execvp error with %s: %s\n", *program2, strerror(errno));
                        goto out;
                    }
                }  
                close(fdc[1]);
                dup2(fdc[0], 0);
                close(fdc[0]);
                char buffc[300];
                while ((cc = read(0, buffc, 300)) > 0) {
                    printf("%s", buffc);
                    counter+=cc;
                }
                printf("Bytes in output: %d\n", counter);
                exit(0);
			} else {
				close(myPipe[0]);
				close(myPipe[1]);
                waitpid(pid2, &status1, 0);
			}
		
   		
        }
    }
out: 
    if (fd > 0) close(fd);
}
