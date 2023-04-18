#include <stdlib.h>
#include <fcntl.h>
#include <curl/curl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "gfserver.h"
#include "content.h"

#define BUFFER_SIZE 4096

typedef struct threadArgs_t threadArgs_t;
struct threadArgs_t
{
	gfcontext_t *ctx;
	char *path;
	void *arg;
};

struct gfcontext_t
{
	int clientContext;
};

size_t numThreads; // global variable for the thread size
ssize_t status;
size_t totalBytesTransferred;
void *threadRoutine(void *arg);

/* Set the global threadSize */
void setNumThreads(size_t num)
{
	numThreads = num;
}

size_t getNumThreads(void)
{
	return numThreads;
}

/* Basically the handler just initializes the threads, which calls the threadRoutine to actually transfer the file back */
ssize_t handler_get(gfcontext_t *ctx, char *path, void *arg)
{
	printf("start creating threads\n");
	pthread_t threads[numThreads];
	// pthread_attr_t attr;
	// pthread_attr_init(&attr);
	// pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	threadArgs_t *arguments = malloc(sizeof(threadArgs_t *));
	arguments->ctx = ctx;
	arguments->path = path;
	arguments->arg = arg;
	int *threadRet = NULL;

	printf("start creating threads\n");

	int i;
	for (i = 0; i < numThreads; i++)
	{
		pthread_create(&threads[i], NULL, threadRoutine, arguments);
	}

	for (i = 0; i < numThreads; i++)
	{
		pthread_join(threads[i], (void *)threadRet);
	}

	return 0;
}

void *threadRoutine(void *args)
{
	threadArgs_t *arguments = (threadArgs_t *)args; // this seems like a really stupid way to have to deal with this
	int fildes;
	size_t file_len, bytes_transferred;
	ssize_t read_len, write_len;
	char buffer[BUFFER_SIZE];

	if (0 > (fildes = content_get(arguments->path)))
		status = gfs_sendheader(arguments->ctx, GF_FILE_NOT_FOUND, 0);

	/* Calculating the file size */
	file_len = lseek(fildes, 0, SEEK_END);

	gfs_sendheader(arguments->ctx, GF_OK, file_len);

	/* Sending the file contents chunk by chunk. */
	bytes_transferred = 0;
	while (bytes_transferred < file_len)
	{
		read_len = pread(fildes, buffer, BUFFER_SIZE, bytes_transferred);
		if (read_len <= 0)
		{
			fprintf(stderr, "handle_with_file read error, %zd, %zu, %zu", read_len, bytes_transferred, file_len);
			gfs_abort(arguments->ctx);
			return (void *)-1;
		}
		write_len = gfs_send(arguments->ctx, buffer, read_len);
		if (write_len != read_len)
		{
			fprintf(stderr, "handle_with_file write error");
			gfs_abort(arguments->ctx);
			return (void *)-1;
		}
		bytes_transferred += write_len;
	}

	close(arguments->ctx->clientContext); // close the connection to the client
	return (void *)bytes_transferred;
}