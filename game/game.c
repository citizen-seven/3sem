#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "tools.h"

#define QLENMAX 1000

long params[6]; 
int queue[QLENMAX];
int qsize = 0;

pthread_cond_t doc_ready = PTHREAD_COND_INITIALIZER;
pthread_cond_t scan_ready = PTHREAD_COND_INITIALIZER;
pthread_mutex_t queue_guard = PTHREAD_MUTEX_INITIALIZER;

void usage() {
    printf (
            "Incorrect arguments for the programm: \n"
            "\t Requires: NC ND TD NS TS L \n"
            "SYNOPSIS: \n"
            "NC - number of clerks \n"
            "ND - number of documents for each \n"
            "TD - time required for a document to be prepared for scanning \n"
            "NS - number of scanners \n"
            "TS - time required for a document to be scanned \n"
            "L - amount of places on a queue \n"
            );
}

int parse (char** args) {
    int i = 0;
    char* endptr = NULL;
    for (i = 0; i < 6; i++) {
        params[i] = strtol(args[i+1], &endptr, 10);
        if ((endptr != NULL) && (endptr[0] != '\0')) {
            usage ();
            return -1;
        }
    }
    return 0;
}


int scan() {
again:
    pthread_mutex_lock(&queue_guard);
    while (qsize == 0) {
        PRINT(GREEN "SCANNER:" RESET "\t Waiting for a document\n");
        pthread_cond_wait(&doc_ready, &queue_guard); //пока нет готовых документов, ожидаем условия
    }
    int ret = queue[0];
    qsize--;
    memmove(queue, queue+1, sizeof(int)*qsize);
    pthread_cond_broadcast(&scan_ready);
    pthread_mutex_unlock(&queue_guard);
    return ret;
    goto again;
}

void put(int doc)  {
    pthread_mutex_lock(&queue_guard);
    while (qsize == L) {
        PRINT(YELLOW "CLERK:" RESET "\t waiting for a place on scanner\n");
        pthread_cond_wait(&scan_ready, &queue_guard); // ожидаем условия
    }                    
    PRINT(YELLOW "CLERK:" RESET "\t Putting doc 0x%0.4x to %d place in queue\n", doc, qsize);
    queue[qsize] = doc;
    qsize++;
    pthread_cond_broadcast(&doc_ready);
    pthread_mutex_unlock(&queue_guard);
}

void* clerk(void *arg) {
    int doc = 16*16*(int)arg;
    for (int i = 0; i<ND; i++) {
        doc+= i;
        PRINT(YELLOW "CLERK%d:" RESET "\t Preparing document 0x%0.4x\n", (int)arg, doc);
        sleep(TD);
        printf(YELLOW "CLERK:" RESET "\t Produced document number: 0x%0.4x\n", doc);
        put(doc);
    }
    PRINT("clerk # %d is free to go home\n", (int)arg);
    return 0;
}

void* scanner(void *arg) {
    int doc;
    while (1) {
        sleep(TS);       
        doc = scan();
        printf(GREEN "SCANNER:" RESET "\t Document 0.%0.4x had been scanned\n", doc);
    }
}

    
int main (int argv, char** argc) {
    if (argv != 7) {
        usage();
        return 0;
    }
    parse(argc); 
    uintptr_t i = 0;
    pthread_t t[NS+NC];
    for (i = 0; i<NC; i++)
        ERRTEST(pthread_create( &t[i], 0,  clerk, (void*)i));
    for (i = 0; i<NS; i++)     
        ERRTEST(pthread_create( &t[i+NC], 0, scanner, 0));
    for (i = 0; i<NC; i++) {
        ERRTEST(pthread_join(t[i], 0));
    }
    printf("Job is done\n");
    return 0;
}
