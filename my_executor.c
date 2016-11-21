#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdlib.h>
#include<string.h> 
#include<unistd.h>
#include<errno.h>

#define DEFAULT_ARGS 32
#define SYM_READ 1024
#define ERR(string, ...) fprintf (stderr, string, ## __VA_ARGS__)
#define DEFAULT_NUM 100
typedef struct pids {
    int pid;
    char name[100];
} pids;

char* sort_file(char *input_path) {
    int input_fd = -1, output_fd = -1;
    int status;
    input_fd = open(input_path, O_RDONLY);
    char* output_path = calloc(strlen(input_path) + 8, sizeof(char));
    strcpy(output_path, input_path);
    strcat(output_path, "[sorted]");
    output_fd = open(output_path, O_WRONLY | O_CREAT | O_TRUNC, 0664);
    if (input_fd == -1) {
        ERR("Can't open file %s for sorting: %s\n", input_path, strerror(errno));
        output_path = NULL;
        goto sort_out;
    }
    if (output_fd == -1) {
        ERR("Can't create file %s for sorting: %s\n", output_path, strerror(errno));
        output_path = NULL;
        goto sort_out;
    }
    int pid = fork();
    if (pid == -1) {
        ERR("Can't make child process in sorting file %s: %s\n", input_path, strerror(errno));
        output_path = NULL;
        goto sort_out;
    }
    if (pid == 0) {
        if (execlp("sort", "sort", "-n", input_path, "-o", output_path, NULL) == -1) {
            ERR("Can't sort file %s with execlp(sort). %s\n", input_path, strerror(errno));
            output_path = NULL;
            if (input_fd >= 0) close(input_fd);
            if (output_fd >= 0) close(output_fd);
            exit(EXIT_FAILURE);
        }
        else {
            if (input_fd >= 0) close(input_fd);
            if (output_fd >= 0) close(output_fd);
            exit(EXIT_SUCCESS);
        }
    }
    if (waitpid(pid, &status, 0) == -1) {
        ERR("Error with wait process terminating %s\n", strerror(errno));
        output_path = NULL;
    } else {
        goto sort_out;
    }
sort_out:
    if (input_fd >= 0) close(input_fd);
    if (output_fd >= 0) close(output_fd);
    return output_path;
}

void arg_err(char* prog_name) {
    ERR("Incorrect amount of arguments\n");
    ERR("Has to be:\n \t %s FILE\n", prog_name);
}

int run_program(char** args, pids* prid, int place) {
    int fork_ret = fork();
    int i = 0;
    switch(fork_ret) {
        case 0: { 
            if (execvp(args[0], args) == -1) {
                ERR("Execvp error with %s: %s\n", args[0], strerror(errno));
                exit(EXIT_FAILURE);
            }
            break;
                }
        case -1: {
            printf("Error while forking\n");
            return 1;
                 }
        default: {
            prid->pid = fork_ret;
            strcpy(prid->name, args[0]);
            //printf("Added a data of process with number %d of %s to %d place in processes list\n", prid->pid, prid->name, place); 
                 }
    }
    return 0;
}

int strta(char* path) {
    char** args = calloc((DEFAULT_ARGS+2), sizeof(char*));
    char* buff; 
    pids* prid = calloc(DEFAULT_NUM, sizeof(pids));
    int i = 0, j = 0, ret = 1, status, line = 1;
    long time = 0, delay = 0;
    int current_num = DEFAULT_NUM;
    if (prid == NULL) {
        ERR("Error while allocating memory for processes list: %s", strerror(errno));
        ret = 1;
        goto out;
    }
    char* endptr = NULL;
    char* sep = " \n";
    char* path_sorted = NULL;
    if ((path_sorted = sort_file(path)) == NULL) {
        ret = 1;
        goto out;
    }
    FILE* file = fopen(path_sorted, "r");
    if (file == NULL) {
        ERR("Can't open the file %s: %s\n", path, strerror(errno));
        ret = 1;
        goto out;
    }
   // printf("file is opened\n");
   char string[SYM_READ];
    while(fgets(string, SYM_READ, file) != NULL) {
        //printf("String is: %s\n", string);
        i = 0;
        buff = strtok(string, sep);
        while(buff != NULL) {
            if (i == 0) {
                delay = strtol(buff, &endptr, 10);
                //printf("time is %ld, endptr is %s\n", delay, endptr);
                if (strcmp(endptr, "\0") != 0) {
                    ERR("Incorrect time argument in program on line %d: %s, has to be NUMBER\n", line, buff);
                    //ret = 1;
                    //goto out;
                    break;
                }
                buff = strtok(NULL, sep);
               // printf("buff is %s\n", buff);
            }
            args[i] = calloc(128, sizeof(char));
            if (buff == NULL) { 
                ERR("No program to execute on line %d\n", line); 
                goto out;
                ret = 1;
            }
            strcpy(args[i], buff);
            //printf("separated %d elem is:%s\n",i,args[i]);
            i++;
            buff = strtok(NULL, sep);

          // printf("final buff is %s\n", buff);
        }
        if (delay-time >= 0) {
            //printf("i sleep %ld\n", delay-time);
            sleep(delay - time);
            time += delay;
        }
        if (args[0] != NULL) {
            run_program(args, &prid[j], j);
            j++;
            for (i = 0; i < DEFAULT_ARGS; i++) {
                args[i] = 0;
            }
        }
        if (j > current_num) {
            prid = reallocf(prid, 2*current_num);
            if (prid == NULL) { 
                ERR("Error while reallocating memory for processes list: %s", strerror(errno));
                ret = 1;
                goto out;
            }
        }
    }

    for (i = 0; i<j; i++) {
        int waitpid_ret = waitpid(prid[i].pid, &status, 0);
        if (waitpid_ret == -1) {
            ERR("Process isn't closed in pid: %s %d\n", strerror(errno), getpid()); 
        }
        else {
            printf("%s exit code: %d\n", prid[i].name, WEXITSTATUS(status));
            ret = 0;
        }
    }
out:
free(*args);
free(args);
free(prid);
free(path_sorted);
return ret;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        arg_err(argv[0]);
        return 1;
        }
    return strta(argv[1]);
}
