#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>
#include <errno.h>

#define cache_num 1000

void arg_err(char* prog_name) {
    fprintf(stderr, "Incorrect amount of arguments\n");
    fprintf(stderr, "Has to be:\n \t %s FROM TO\n", prog_name);
}

int logic(char* from_path, char* to_path) {
    char cache[cache_num+1];
    int from = -1;
    int to = -1;
    int size = 1;
    int ret = 0;
    int written = 0;
    from = open(from_path, O_RDONLY);
    if (from == -1) {
        fprintf(stderr, "Can't open the file %s: %s\n", from_path, strerror(errno));
        ret = 1;
        goto out;
    }
    to = open(to_path,O_WRONLY | O_CREAT | O_EXCL, 0664);
    if (to == -1) {
        fprintf(stderr, "Error while creating new file %s: %s\n", to_path, strerror(errno));
        ret = 1;
        goto out;
    }
    while (size > 0) {
        size = read(from, cache, cache_num);
        if (size == -1) {
            fprintf(stderr, "Error while reading from file %s: %s\n", from_path, strerror(errno));
            ret = 1;
            goto out;
        }
        cache[size] = 0;
        written = write(to, cache, size);
        if (written == -1) {
            fprintf(stderr, "Error while writing to a file %s: %s\n", to_path, strerror(errno));
            ret = 1;
            goto out;
        }
       
     }
out:
    if (to >= 0) close(to);
    if (from >= 0) close(from);
    return ret;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        arg_err(argv[0]);
        return 1;
        }
    return logic(argv[1], argv[2]);
}
