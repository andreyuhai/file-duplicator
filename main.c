#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <aio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

//struct to pass to threads as an argument since it just accepts (void *)
struct t_thread_arguments {

    int fd_source; //source file descriptor
    int fd_destination; //destination file descriptor
    long int buffer_size; //buffer size for the threads reading before the last thread.
    long int buffer_size_last; //buffer size for the last thread in case there is a remainder.
    long int offset;
    int last_flag; //to determine if the thread is the last thread so we can assign buffer size accordingly.

};

//worker thread to do all the work.
void *worker(void *thread_args) {

    // declare structs for aiocb and arguments.
    struct aiocb *aiocb;
    struct t_thread_arguments *arguments = thread_args;

    // malloc for aiocb struct.
    if((aiocb = malloc(sizeof(struct aiocb))) == NULL){
        perror("Error callocing aiocb");
        exit(-1);
    }

    // declaring aiocb variables for later use.
    aiocb->aio_fildes   = arguments->fd_source;
    aiocb->aio_offset   = arguments->offset;
    aiocb->aio_buf      = malloc(((arguments->last_flag) ? arguments->buffer_size_last : arguments->buffer_size));
    aiocb->aio_nbytes   = (arguments->last_flag) ? arguments->buffer_size_last : arguments->buffer_size;

    if(aio_read(aiocb) == -1) {
        perror("Error while reading");
        exit(-1);
    }

    aio_suspend(&aiocb, 1, NULL);

    __ssize_t nbytesRead = aio_return(aiocb);
    printf("Bytes READ = %ld\n", nbytesRead);

    // change fildes to write the buffer into the file.
    aiocb->aio_fildes = arguments->fd_destination;

    if (aio_write(aiocb) == -1) {
        perror("Error while writing");
        exit(-1);
    }

    aio_suspend(&aiocb, 1, NULL);
    __ssize_t bytesWritten = aio_return(aiocb);

    printf("Bytes written %ld\n", bytesWritten);
    perror("Return status");

    free(aiocb->aio_buf);
    free(aiocb);

    pthread_exit(NULL);
}

// Returns a random alphabetic character
int getrandomChar() {

    int letterTypeFlag = (rand() % 2);

    if (letterTypeFlag)
        return ('a' + (rand() % 26));
    else
        return ('A' + (rand() % 26));
}

void createRandomFile(char *source, int numberofBytes) {

    FILE *fp = fopen(source, "w");

    for (int i = 0; i < numberofBytes; i++) {
        fprintf(fp, "%c", getrandomChar());
    }

    fclose(fp);

}

int main(int argc, char *argv[]) {

    char *source_path = argv[1];
    char *destination_path = argv[2];
    int nthreads = (int) strtol(argv[3], NULL, 10);
    int fd_source = open(source_path, O_RDONLY);
    int fd_destination = open(destination_path, O_WRONLY | O_CREAT | O_TRUNC,
                              S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    pthread_t tID[nthreads];

    // Set the seed for srand.
    srand(time(NULL));

    // Get random number of bytes to write to create the random file.
    int numberofBytes = 10 + (rand() % 100000000);

    // Create a random filled file at the source path.
    createRandomFile(source_path, numberofBytes);

    // Calculate the payload for each thread.
    int payload = numberofBytes / nthreads;
    // Calculate the payload for the last thread in case there is a remainder.
    int payloadLast = payload + (numberofBytes % nthreads);

    // Create thread arguments to pass to each thread for later use.
    struct t_thread_arguments *thread_arguments = calloc(nthreads, sizeof(struct t_thread_arguments));

    // Set arguments for each thread.
    for(int i = 0; i < nthreads; i++) {
        thread_arguments[i].fd_destination  = fd_destination;
        thread_arguments[i].fd_source       = fd_source;
        thread_arguments[i].offset          = i * payload;
        thread_arguments[i].buffer_size     = payload;
        thread_arguments[i].buffer_size_last= payloadLast;
        thread_arguments[i].last_flag       = (i == nthreads -1) ? 1 : 0;
    }

    // Create threads.
    for (int i = 0; i < nthreads; i++) {
        pthread_create(&tID[i], NULL, worker,(void *) &thread_arguments[i]);
    }

    // Wait for all threads to return.
    for(int j = 0; j < nthreads; j++) {
        pthread_join(tID[j], NULL);
    }

    close(fd_source);
    close(fd_destination);
    free(thread_arguments);

    return 0;
}