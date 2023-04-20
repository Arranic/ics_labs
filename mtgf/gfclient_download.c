#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>

#include "workload.h"
#include "gfclient.h"
#include "steque.h"

#define USAGE                                                           \
    "usage:\n"                                                          \
    "  webclient [options]\n"                                           \
    "options:\n"                                                        \
    "  -h                  Show this help message\n"                    \
    "  -n [num_requests]   Requests download per thread (Default: 1)\n" \
    "  -p [server_port]    Server port (Default: 8080)\n"               \
    "  -s [server_addr]    Server address (Default: 0.0.0.0)\n"         \
    "  -t [nthreads]       Number of threads (Default 1)\n"             \
    "  -w [workload_path]  Path to workload file (Default: workload.txt)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"workload-path", required_argument, NULL, 'w'},
    {"nthreads", required_argument, NULL, 't'},
    {"nrequests", required_argument, NULL, 'n'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

typedef struct threadInfo_t
{
    int numRequests;
    int numThreads;
    int returnCode;
    char *server;
    unsigned short port;
} threadInfo_t;

pthread_mutex_t mutexQ;
pthread_cond_t condQ;
int numRequests = 0;
steque_t *request_queue;
bool isReady = false;

static void Usage()
{
    fprintf(stdout, "%s", USAGE);
}

static void localPath(char *req_path, char *local_path)
{
    static int counter = 0;

    sprintf(local_path, "%s-%06d", &req_path[1], counter++);
}

static FILE *openFile(char *path)
{
    char *cur, *prev;
    FILE *ans;

    /* Make the directory if it isn't there */
    prev = path;
    while (NULL != (cur = strchr(prev + 1, '/')))
    {
        *cur = '\0';

        if (0 > mkdir(&path[0], S_IRWXU))
        {
            if (errno != EEXIST)
            {
                perror("Unable to create directory");
                exit(EXIT_FAILURE);
            }
        }

        *cur = '/';
        prev = cur;
    }

    if (NULL == (ans = fopen(&path[0], "w")))
    {
        perror("Unable to open file");
        exit(EXIT_FAILURE);
    }

    return ans;
}

/* Callbacks ========================================================= */
static void writecb(void *data, size_t data_len, void *arg)
{
    FILE *file = (FILE *)arg;

    fwrite(data, 1, data_len, file);
}

int worker(threadInfo_t *thread_info, int tNum)
{
    int nrequests = thread_info->numRequests;
    char *req_path;       // the path to the file
    char local_path[512]; // to store the path to the local file
    FILE *file;
    gfcrequest_t *gfr; // gfcrequest item
    char server[100];
    strncpy(server, thread_info->server, 99);
    unsigned short port = thread_info->port;
    int returncode;
    int returnStatus = 0;

    fprintf(stderr, "Inside thread #%d. Making requests.\n", tNum);
    // fprintf(stderr, "Inside thread #%ld. Making requests.\n", pthread_self());
    /*Making the requests...*/
    for (int i = 0; i < nrequests; i++)
    {

        req_path = workload_get_path();

        if (strlen(req_path) > 256)
        {
            fprintf(stderr, "Request path exceeded maximum of 256 characters\n.");
            returnStatus = -20;
            // exit(EXIT_FAILURE);
        }

        localPath(req_path, local_path);

        file = openFile(local_path);

        gfr = gfc_create();
        gfc_set_server(gfr, server);
        gfc_set_path(gfr, req_path);
        gfc_set_port(gfr, port);
        gfc_set_writefunc(gfr, writecb);
        gfc_set_writearg(gfr, file);

        fprintf(stderr, "Requesting %s%s\n", server, req_path);

        if (0 > (returncode = gfc_perform(gfr)))
        {
            fprintf(stderr, "gfc_perform returned an error %d\n", returncode);
            fclose(file);
            if (0 > unlink(local_path))
            {
                fprintf(stderr, "unlink failed on %s\n", local_path);
                returnStatus = -21;
            }
        }
        else
        {
            fclose(file);
        }

        if (gfc_get_status(gfr) != GF_OK)
        {
            if (0 > unlink(local_path))
            {
                fprintf(stderr, "unlink failed on %s\n", local_path);
                returnStatus = -22;
            }
        }

        fprintf(stderr, "Status: %s\n", gfc_strstatus(gfc_get_status(gfr)));
        fprintf(stderr, "Received %zu of %zu bytes\n", gfc_get_bytesreceived(gfr), gfc_get_filelen(gfr));

        gfc_cleanup(gfr);
    }

    fprintf(stderr, "Returning from worker function with status: %d.\n", returncode);
    return returnStatus;
}

/* ultra totally thread safety hopefully I think */
void *thread_routine(void *args)
{
    int tNum = *((int *)args);
    fprintf(stderr, "thread #%d started\n", tNum);

    int workerStatus; // return value from worker
    steque_node_t *node;
    // struct timespec ts;
    // clock_gettime(CLOCK_REALTIME, &ts);
    // ts.tv_sec += 1;

    pthread_mutex_lock(&mutexQ); // lock the Q so we're the only one to use it

    // while (steque_size(request_queue) == 0)
    // {
    //     pthread_cond_wait(&condQ, &mutexQ);
    // }

    while (!isReady)
    {
        fprintf(stderr, "thread #%d [isReady] %d \n", tNum, isReady);
        // int timeOut = pthread_cond_timedwait(&condQ, &mutexQ, &ts);
        pthread_cond_wait(&condQ, &mutexQ);
        // if (timeOut == ETIMEDOUT)
        // {
        //     fprintf(stderr, "thread #%d [queue empty] %d\n", tNum, steque_isempty(request_queue));
        //     pthread_mutex_unlock(&mutexQ);
        //     return (void *)(unsigned long)0;
        // }
        if (!isReady && steque_isempty(request_queue))
        {
            fprintf(stderr, "thread #%d [queue empty] %d\n", tNum, steque_isempty(request_queue));
            pthread_mutex_unlock(&mutexQ);
            return (void *)(unsigned long)0;
        }
        // fprintf(stderr, "[thread] %ld [isReady] %d \n", pthread_self(), isReady);
    }

    if (!steque_isempty(request_queue))
    {
        fprintf(stderr, "thread #%d received request\n", tNum);
        node = steque_pop(request_queue); // get the request at the front of the queue
    }

    if (steque_isempty(request_queue))
        isReady = false;

    // pthread_cond_signal(&condQ);
    pthread_mutex_unlock(&mutexQ); // now that we're done modifying the queue, unlock it

    fprintf(stderr, "Inside thread #%d. Calling worker function.\n", tNum);
    // fprintf(stderr, "Inside thread #%ld. Calling worker function.\n", pthread_self());
    workerStatus = worker((threadInfo_t *)node, tNum); // finally call the worker to handle the connection to the server

    fprintf(stderr, "Returning from thread with status %d.\n", workerStatus);

    return (void *)(unsigned long)workerStatus;
}

/* enqueue an item in a thread-safe manner and increase the numRequests tracker variable */
void submit_to_queue(steque_t *q, threadInfo_t *element)
{
    pthread_mutex_lock(&mutexQ);             // lock the mutex for thread safety
    steque_enqueue(q, (steque_item)element); // enque the element

    fprintf(stderr, "Added item #%d to queue\n", steque_size(request_queue));
    pthread_mutex_unlock(&mutexQ);  // unlock the mutex
    isReady = true;                 // tell the thread pool that the element is ready to use
    pthread_cond_broadcast(&condQ); // signal to thread pool to start work
}

/* Main ========================================================= */
int main(int argc, char **argv)
{
    /* COMMAND LINE OPTIONS ============================================= */
    char *server = "localhost";
    unsigned short port = 8080;
    char *workload_path = "workload.txt";
    threadInfo_t *info;
    int option_char = 0;
    int nrequests = 1;
    int nthreads = 1;

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "s:p:w:n:t:h", gLongOptions, NULL)) != -1)
    {
        switch (option_char)
        {
        case 's': // server
            server = optarg;
            break;
        case 'p': // port
            port = atoi(optarg);
            break;
        case 'w': // workload-path
            workload_path = optarg;
            break;
        case 'n': // nrequests
            nrequests = atoi(optarg);
            break;
        case 't': // nthreads
            nthreads = atoi(optarg);
            break;
        case 'h': // help
            Usage();
            exit(0);
            break;
        default:
            Usage();
            exit(1);
        }
    }

    if (EXIT_SUCCESS != workload_init(workload_path))
    {
        fprintf(stderr, "Unable to load workload file %s.\n", workload_path);
        exit(EXIT_FAILURE);
    }

    gfc_global_init();

    fprintf(stderr, "nrequests: %d, nthreads: %d\n", nrequests, nthreads);

    pthread_t thread_pool[nthreads];   // pool of threads to use
    pthread_mutex_init(&mutexQ, NULL); // initialize the mutex
    pthread_cond_init(&condQ, NULL);   // initialize the condition

    // fprintf(stderr, "request_queue address: %p\n", &request_queue);
    request_queue = (steque_t *)malloc(sizeof(steque_t));
    steque_init(request_queue);                          // initialize the queue
    info = (threadInfo_t *)malloc(sizeof(threadInfo_t)); // malloc space for the thread info
    info->numRequests = nrequests;
    info->numThreads = nthreads;
    info->port = port;
    info->server = server;

    int threadNumber[nthreads];
    // create the threads for the thread pool
    for (int i = 0; i < nthreads; i++)
    {
        threadNumber[i] = i;
        if (pthread_create(&thread_pool[i], NULL, thread_routine, &threadNumber[i]) != 0)
        {
            fprintf(stderr, "Failed to create threads.\n");
        }
    }

    for (int j = 0; j < nrequests; j++)
        submit_to_queue(request_queue, info);

    for (int k = 0; k < nthreads; k++)
    {
        pthread_join(thread_pool[k], NULL);
        fprintf(stderr, "thread #%d joined\n", threadNumber[k]);
    }
    // free(info);
    //  pthread_cond_broadcast(&condQ); // wake all the remaining threads so they can exit
    fprintf(stderr, "All threads joined.\n");

    pthread_exit(NULL); // ask the program to wait until the workers are done
    pthread_mutex_destroy(&mutexQ);
    pthread_cond_destroy(&condQ);
    steque_destroy(request_queue);
    free(request_queue);
    free(info);
    gfc_global_cleanup();

    fprintf(stderr, "Program execution complete. Exiting...\n");
    exit(0);
}