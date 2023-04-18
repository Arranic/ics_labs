#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include "gfserver.h"
#include <errno.h>

<<<<<<< Updated upstream
#define BUFSIZE 4096

/*
||||||| constructed merge base
/* 
=======
#define BUFSIZE 64000

/*
>>>>>>> Stashed changes
 * Modify this file to implement the interface specified in
 * gfserver.h.
 */

struct gfserver_t
{
    /* Socket */
    int sock;

    /* Socket Info */
    unsigned short port;
    int maxPendingConn;
    gfstatus_t status;

    // callback
    ssize_t (*handler)(gfcontext_t *, char *, void *);
    void *handlerArgs;

    /* Metadata */
    char *path;
    size_t bytesSent;
};

struct gfcontext_t
{
    int clientContext;
};

void printHex(const char *s)
{
    while (*s)
        printf("%02x ", (unsigned int)*s++);
    printf("\n");
}

<<<<<<< Updated upstream
void gfs_abort(gfcontext_t *ctx)
{
    // fprintf(stderr, "BANG SMACK DAB AT THE BEGINNING OF GFS_ABORT\n");
    // exit(EXIT_SUCCESS); /* <---------------------------------------------------------------------------------------------------- */

    if (!ctx)
    {
        fprintf(stderr, "ERROR: Attempting to abort null context.\n");
        return;
    }

    close(ctx->clientContext);
    free(ctx);
}

gfserver_t *gfserver_create()
{
    gfserver_t *server = malloc(sizeof(gfserver_t));
    if (!server)
    {
        fprintf(stderr, "ERROR: Unable to allocate space for new server.\n");
        return (gfserver_t *)NULL;
    }

    memset(server, 0, sizeof(gfserver_t));
    server->maxPendingConn = 5;
    return server;
}

ssize_t gfs_send(gfcontext_t *ctx, void *data, size_t len)
{
    // fprintf(stderr, "=======================\n");
    // fprintf(stderr, "BANG SMACK DAB AT THE TOP OF GFS_SEND\n");
    // exit(EXIT_SUCCESS); /* <---------------------------------------------------------------------------------------------------- */

    if (!ctx)
        return -1;
    if (!data)
        return -1;
    if ((void *)len == NULL || len < 0)
        return -1;

    // fprintf(stderr, "[ctx] %d [len] %zu\n", ctx->clientContext, len);

    // char buffer[len];
    // strcpy

    ssize_t dataSent = 0, sent = 0;
    size_t numBytesToSend = len;

    // printf("[sending buffer] ");
    // printHex(data);

    while (numBytesToSend > 0)
    {
        // printf("[numBytesToSend] %zi\n", numBytesToSend);
        // fprintf(stderr, "Sending data...\n");
        sent = send(ctx->clientContext, data, numBytesToSend, 0);

        if (sent < 0)
        {
            fprintf(stderr, "ERROR: gfs_send function failed during send operation with error %d.\n", errno);
            return -1; // error
        }
        if (sent == 0)
        {
            break;
        }
        numBytesToSend -= sent;
        dataSent += sent;
    }

    if (dataSent < len)
    {
        fprintf(stderr, "ERROR: data sent does not match the length of data.\n");
        fprintf(stderr, "[data sent] %zi\n", dataSent);
    }
    // else
    // fprintf(stderr, "[numBytesSent] %zi\n", dataSent);

    return dataSent;
||||||| constructed merge base
gfserver_t* gfserver_create(){
    return (gfserver_t *)NULL;
=======
void gfs_abort(gfcontext_t *ctx)
{
    // fprintf(stderr, "BANG SMACK DAB AT THE BEGINNING OF GFS_ABORT\n");
    // exit(EXIT_SUCCESS); /* <---------------------------------------------------------------------------------------------------- */

    if (!ctx)
    {
        fprintf(stderr, "ERROR: Attempting to abort null context.\n");
        return;
    }

    close(ctx->clientContext);
    free(ctx);
>>>>>>> Stashed changes
}

<<<<<<< Updated upstream
char *statusCheck(gfstatus_t status)
{
    char *s;
    switch (status)
    {
    case GF_OK:
        s = "GF_OK";
        break;
    case GF_FILE_NOT_FOUND:
        s = "GF_FILE_NOT_FOUND";
        break;
    case GF_ERROR:
        s = "GF_ERROR";
        break;
    default:
        s = NULL;
    }

    return s;
||||||| constructed merge base
ssize_t gfs_send(gfcontext_t *ctx, void *data, size_t len){
    return -1;
=======
gfserver_t *gfserver_create()
{
    gfserver_t *server = malloc(sizeof(gfserver_t));
    if (!server)
    {
        fprintf(stderr, "ERROR: Unable to allocate space for new server.\n");
        return (gfserver_t *)NULL;
    }

    memset(server, 0, sizeof(gfserver_t));
    server->maxPendingConn = 5;
    return server;
>>>>>>> Stashed changes
}

<<<<<<< Updated upstream
ssize_t gfs_sendheader(gfcontext_t *ctx, gfstatus_t status, size_t file_len)
{
    /* Input checking */
    // Check the context
    if (ctx == NULL || ctx->clientContext == 0)
    {
        fprintf(stderr, "ERROR: Attempting to send header to a NULL client.\n");
        return -1;
    }
    // Check the status is valid
    if (statusCheck(status) == NULL)
    {
        fprintf(stderr, "ERROR: Value of status is either NULL or is invalid.\n");
        return -1;
    }

    fprintf(stderr, "[status] %s\n", statusCheck(status));

    if (file_len < 0)
    {
        fprintf(stderr, "ERROR: File length must be greater than or equal to 0.\n");
        return -1;
    }

    char *hdrBuf; // buffer to store header
    int hdrSize = 0;
    ssize_t dataSent = 0;
    switch (status)
    {
    case GF_OK: // since status is GF_OK, send the file size with the header
        hdrSize = asprintf(&hdrBuf, "%s %s %zu\r\n\r\n", "GETFILE", "OK", file_len);
        break;
    case GF_ERROR: // status is GF_ERROR, don't send the file size
        hdrSize = asprintf(&hdrBuf, "%s %s\r\n\r\n", "GETFILE", "ERROR");
        break;
    case GF_FILE_NOT_FOUND: // status is GF_FILE_NOT_FOUND, don't send the file size
        hdrSize = asprintf(&hdrBuf, "%s %s\r\n\r\n", "GETFILE", "FILE_NOT_FOUND");
        break;
    }

    // printf("[header] %s\n", hdrBuf);
    // printf("[header] ");
    // printHex(hdrBuf);
    // dataSent = gfs_send(ctx, hdrBuf, hdrSize); // send the data
    ssize_t sent = send(ctx->clientContext, hdrBuf + dataSent, hdrSize - dataSent, 0);
    if (sent < 0)
        fprintf(stderr, "Header send error.\n");

    // dataSent = 0;
    // while (dataSent < hdrSize)
    // {
    //     ssize_t sent = send(ctx->clientContext, hdrBuf + dataSent, hdrSize - dataSent, 0);
    //     if (sent < 0)
    //         fprintf(stderr, "File send error.\n");
    //     dataSent += sent;
    // }

    free(hdrBuf); // done with the header buffer
    fprintf(stderr, "[return (data sent)] %zd\n", sent);
    // exit(EXIT_SUCCESS); /* <---------------------------------------------------------------------------------------------------- */
    return sent; // return the amount of data we sent
||||||| constructed merge base
ssize_t gfs_sendheader(gfcontext_t *ctx, gfstatus_t status, size_t file_len){
    return -1;
=======
ssize_t gfs_send(gfcontext_t *ctx, void *data, size_t len)
{
    // fprintf(stderr, "=======================\n");
    // fprintf(stderr, "BANG SMACK DAB AT THE TOP OF GFS_SEND\n");
    // exit(EXIT_SUCCESS); /* <---------------------------------------------------------------------------------------------------- */

    if (!ctx)
        return -1;
    if (!data)
        return -1;
    if ((void *)len == NULL || len < 0)
        return -1;

    // fprintf(stderr, "[ctx] %d [len] %zu\n", ctx->clientContext, len);

    // char buffer[len];
    // strcpy

    ssize_t dataSent = 0, sent = 0;
    size_t numBytesToSend = len;

    // printf("[sending buffer] ");
    // printHex(data);

    while (numBytesToSend > 0)
    {
        // printf("[numBytesToSend] %zi\n", numBytesToSend);
        // fprintf(stderr, "Sending data...\n");
        sent = send(ctx->clientContext, data, numBytesToSend, 0);

        if (sent < 0)
        {
            fprintf(stderr, "ERROR: gfs_send function failed during send operation with error %d.\n", errno);
            return -1; // error
        }
        if (sent == 0)
        {
            break;
        }
        numBytesToSend -= sent;
        dataSent += sent;
    }

    if (dataSent < len)
    {
        fprintf(stderr, "ERROR: data sent does not match the length of data.\n");
        fprintf(stderr, "[data sent] %zi\n", dataSent);
    }
    // else
    // fprintf(stderr, "[numBytesSent] %zi\n", dataSent);

    return dataSent;
>>>>>>> Stashed changes
}

<<<<<<< Updated upstream
/* Helper function to parse the requests we receive */
ssize_t parseRequest(gfserver_t *gfs, void *buffer)
{
    if (!gfs)
    {
        fprintf(stderr, "ERROR: gfserver_t object NULL when parsing request.\n");
        return -1;
    }
    if (!buffer)
    {
        fprintf(stderr, "ERROR: Buffer object NULL when parsing request.\n");
        return -1;
    }

    char *bufferCopy;
    char *token;
    asprintf(&bufferCopy, "%s", (char *)buffer);

    if (strstr(bufferCopy, "\r\n\r\n") == NULL)
    {
        gfs->status = GF_FILE_NOT_FOUND;
        free(bufferCopy);
        return -1;
    }

    // First token is the scheme
    token = strtok(bufferCopy, " ");
    // fprintf(stderr, "[scheme] %s\n", token);
    if (strcmp("GETFILE", token) != 0)
    {
        gfs->status = GF_FILE_NOT_FOUND;
        free(bufferCopy);
        return -1;
    }

    // Second token is the method
    token = strtok(NULL, " ");
    // fprintf(stderr, "[method] %s\n", token);
    if (strcmp("GET", token) != 0)
    {
        gfs->status = GF_FILE_NOT_FOUND;
        free(bufferCopy);
        return -1;
    }

    // Third token is the path
    gfs->path = strtok(NULL, "\r\n\r\n");
    // fprintf(stderr, "[path] %s\n", gfs->path);
    if (gfs->path == NULL || gfs->path[0] != '/')
    {
        gfs->status = GF_FILE_NOT_FOUND;
        free(bufferCopy);
        return -1;
    }
||||||| constructed merge base
void gfserver_serve(gfserver_t *gfs){
=======
char *statusCheck(gfstatus_t status)
{
    char *s;
    switch (status)
    {
    case GF_OK:
        s = "GF_OK";
        break;
    case GF_FILE_NOT_FOUND:
        s = "GF_FILE_NOT_FOUND";
        break;
    case GF_ERROR:
        s = "GF_ERROR";
        break;
    default:
        s = NULL;
    }
>>>>>>> Stashed changes

<<<<<<< Updated upstream
    // if (bufferCopy != NULL)
    //     free(bufferCopy);
    return 0; // got the path, we're good
||||||| constructed merge base
=======
    return s;
>>>>>>> Stashed changes
}

<<<<<<< Updated upstream
/* Helper function to open the server's socket */
int newListenerFd(char *port, int maxConnections)
{
    struct addrinfo hints;    // hints/settings for getaddrinfo()
    struct addrinfo *results; // where the results of getaddrinfo() will be stored
    struct addrinfo *record;  // used in a loop to find a working address to use for communication
    int listfd;               // used for the listener file descriptor

    // zero out the hints struct
    memset(&hints, 0, sizeof(struct addrinfo));

    // set the hints struct default values
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    hints.ai_flags |= AI_NUMERICSERV;

    /* get the server's address information */
    if ((getaddrinfo(NULL, port, &hints, &results)) != 0)
    {
        fprintf(stderr, "Failure to translate server socket.\n");
        return -1;
    }

    // loop through the linked list of records returned by getaddrinfo to find a working socket
    // break the loop if we succeed
    for (record = results; record != NULL; record = record->ai_next)
    {
        if ((listfd = socket(record->ai_family, record->ai_socktype, record->ai_protocol)) < 0)
            continue; /* socket failed, try the next */

        // set the socket options so it can be reused
        // syntax: setsockopt(int sockfd, int level, int option_name, const void *option_val, socklen_t option_length)
        int val = 1; // value to enable the SO_REUSEADDR option
        setsockopt(listfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&val, sizeof(int));

        // bind the socket to a port
        // syntax: bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
        // The second argument wants a pointer to the struct that contains the port information
        if (bind(listfd, record->ai_addr, record->ai_addrlen) == 0)
            break; // binding was successful and move on
||||||| constructed merge base
void gfserver_set_handlerarg(gfserver_t *gfs, void* arg){
=======
ssize_t gfs_sendheader(gfcontext_t *ctx, gfstatus_t status, size_t file_len)
{
    /* Input checking */
    // Check the context
    if (ctx == NULL || ctx->clientContext == 0)
    {
        fprintf(stderr, "ERROR: Attempting to send header to a NULL client.\n");
        return -1;
    }
    // Check the status is valid
    if (statusCheck(status) == NULL)
    {
        fprintf(stderr, "ERROR: Value of status is either NULL or is invalid.\n");
        return -1;
    }

    fprintf(stderr, "[status] %s\n", statusCheck(status));

    if (file_len < 0)
    {
        fprintf(stderr, "ERROR: File length must be greater than or equal to 0.\n");
        return -1;
    }

    char *hdrBuf; // buffer to store header
    int hdrSize = 0;
    ssize_t dataSent = 0;
    switch (status)
    {
    case GF_OK: // since status is GF_OK, send the file size with the header
        hdrSize = asprintf(&hdrBuf, "%s %s %zu\r\n\r\n", "GETFILE", "OK", file_len);
        break;
    case GF_ERROR: // status is GF_ERROR, don't send the file size
        hdrSize = asprintf(&hdrBuf, "%s %s\r\n\r\n", "GETFILE", "ERROR");
        break;
    case GF_FILE_NOT_FOUND: // status is GF_FILE_NOT_FOUND, don't send the file size
        hdrSize = asprintf(&hdrBuf, "%s %s\r\n\r\n", "GETFILE", "FILE_NOT_FOUND");
        break;
    }
>>>>>>> Stashed changes

<<<<<<< Updated upstream
        close(listfd); // if anything failed, close this socket and try the next record returned from getaddrinfo()
    }
    if (!record)
        return -1;

    // free the results since we don't need them any more
    freeaddrinfo(results);

    //
    if (listen(listfd, maxConnections) < 0)
    {
        close(listfd);
        return -1;
    }

    return listfd; // return the connected socket file descriptor
||||||| constructed merge base
=======
    // printf("[header] %s\n", hdrBuf);
    // printf("[header] ");
    // printHex(hdrBuf);
    // dataSent = gfs_send(ctx, hdrBuf, hdrSize); // send the data
    ssize_t sent = send(ctx->clientContext, hdrBuf + dataSent, hdrSize - dataSent, 0);
    if (sent < 0)
        fprintf(stderr, "Header send error.\n");

    // dataSent = 0;
    // while (dataSent < hdrSize)
    // {
    //     ssize_t sent = send(ctx->clientContext, hdrBuf + dataSent, hdrSize - dataSent, 0);
    //     if (sent < 0)
    //         fprintf(stderr, "File send error.\n");
    //     dataSent += sent;
    // }

    free(hdrBuf); // done with the header buffer
    fprintf(stderr, "[return (data sent)] %zd\n", sent);
    // exit(EXIT_SUCCESS); /* <---------------------------------------------------------------------------------------------------- */
    return sent; // return the amount of data we sent
>>>>>>> Stashed changes
}

<<<<<<< Updated upstream
void gfserver_serve(gfserver_t *gfs)
{
    if (!gfs)
    {
        fprintf(stderr, "ERROR: gfserver_t object is NULL.\n");
        return;
    }

    // Covert the port number to a string
    char portno[6];
    snprintf(portno, 5, "%u", gfs->port);

    // Open the server socket listener
    if ((gfs->sock = newListenerFd(portno, gfs->maxPendingConn)) < 0)
    {
        fprintf(stderr, "ERROR: Error opening server socket.\n");
        return;
    }

    // fprintf(stderr, "Starting server...\n");

    // Main server loop
    while (1)
    {
        // ssize_t dataRead = 0;
        char buf[BUFSIZE];
        struct sockaddr_storage clientAddr;
        socklen_t clientAddrLen = sizeof(struct sockaddr_storage);
        gfcontext_t *client = malloc(sizeof(struct gfcontext_t));
        memset(client, 0, sizeof(gfcontext_t));
||||||| constructed merge base
void gfserver_set_handler(gfserver_t *gfs, ssize_t (*handler)(gfcontext_t *, char *, void*)){
=======
/* Helper function to parse the requests we receive */
ssize_t parseRequest(gfserver_t *gfs, void *buffer)
{
    if (!gfs)
    {
        fprintf(stderr, "ERROR: gfserver_t object NULL when parsing request.\n");
        return -1;
    }
    if (!buffer)
    {
        fprintf(stderr, "ERROR: Buffer object NULL when parsing request.\n");
        return -1;
    }

    char *bufferCopy;
    char *token;
    asprintf(&bufferCopy, "%s", (char *)buffer);

    if (strstr(bufferCopy, "\r\n\r\n") == NULL)
    {
        gfs->status = GF_FILE_NOT_FOUND;
        free(bufferCopy);
        return -1;
    }

    // First token is the scheme
    token = strtok(bufferCopy, " ");
    // fprintf(stderr, "[scheme] %s\n", token);
    if (strcmp("GETFILE", token) != 0)
    {
        gfs->status = GF_FILE_NOT_FOUND;
        free(bufferCopy);
        return -1;
    }

    // Second token is the method
    token = strtok(NULL, " ");
    // fprintf(stderr, "[method] %s\n", token);
    if (strcmp("GET", token) != 0)
    {
        gfs->status = GF_FILE_NOT_FOUND;
        free(bufferCopy);
        return -1;
    }
>>>>>>> Stashed changes

<<<<<<< Updated upstream
        /* Accept connections on server socket */
        if ((client->clientContext = accept(gfs->sock, (struct sockaddr *)&clientAddr, &clientAddrLen)) < 0)
        {
            fprintf(stderr, "ERROR: Failed to accept client connection.\n");
            return;
        }

        memset(&buf, 0, sizeof(buf));
        // dataRead = recv(client->clientContext, buf, BUFSIZE, 0);
        recv(client->clientContext, buf, BUFSIZE, 0);
        ssize_t reqCheck = parseRequest(gfs, buf);
        if (reqCheck < 0)
        {
            fprintf(stderr, "ERROR: Check of client request returned an ERROR.\n");
            gfs_sendheader(client, GF_FILE_NOT_FOUND, 0);
        }
        fprintf(stderr, "Data received. Initiating file transfer.\n");
        ssize_t handlerCheck = gfs->handler(client, gfs->path, gfs->handlerArgs);
        if (handlerCheck < 0)
        {
            fprintf(stderr, "ERROR: Handler function returned an error. [handlerCheck] %zd\n", handlerCheck);
            gfs_abort(client);
            break;
        }
        fprintf(stderr, "FILE TRANSFER COMPLETE! [handlerCheck] %zd\n", handlerCheck);
        gfs->bytesSent += handlerCheck;
        gfs_abort(client);
    }

    // server is done
    close(gfs->sock);
    return;
}

void gfserver_set_handlerarg(gfserver_t *gfs, void *arg)
{
    if (!gfs)
        fprintf(stderr, "ERROR: Attempting to set handler arguments on null object.\n");
    gfs->handlerArgs = arg;
||||||| constructed merge base
=======
    // Third token is the path
    gfs->path = strtok(NULL, "\r\n\r\n");
    // fprintf(stderr, "[path] %s\n", gfs->path);
    if (gfs->path == NULL || gfs->path[0] != '/')
    {
        gfs->status = GF_FILE_NOT_FOUND;
        free(bufferCopy);
        return -1;
    }

    // if (bufferCopy != NULL)
    //     free(bufferCopy);
    return 0; // got the path, we're good
}

/* Helper function to open the server's socket */
int newListenerFd(char *port, int maxConnections)
{
    struct addrinfo hints;    // hints/settings for getaddrinfo()
    struct addrinfo *results; // where the results of getaddrinfo() will be stored
    struct addrinfo *record;  // used in a loop to find a working address to use for communication
    int listfd;               // used for the listener file descriptor

    // zero out the hints struct
    memset(&hints, 0, sizeof(struct addrinfo));

    // set the hints struct default values
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    hints.ai_flags |= AI_NUMERICSERV;

    /* get the server's address information */
    if ((getaddrinfo(NULL, port, &hints, &results)) != 0)
    {
        fprintf(stderr, "Failure to translate server socket.\n");
        return -1;
    }

    // loop through the linked list of records returned by getaddrinfo to find a working socket
    // break the loop if we succeed
    for (record = results; record != NULL; record = record->ai_next)
    {
        if ((listfd = socket(record->ai_family, record->ai_socktype, record->ai_protocol)) < 0)
            continue; /* socket failed, try the next */

        // set the socket options so it can be reused
        // syntax: setsockopt(int sockfd, int level, int option_name, const void *option_val, socklen_t option_length)
        int val = 1; // value to enable the SO_REUSEADDR option
        setsockopt(listfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&val, sizeof(int));

        // bind the socket to a port
        // syntax: bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
        // The second argument wants a pointer to the struct that contains the port information
        if (bind(listfd, record->ai_addr, record->ai_addrlen) == 0)
            break; // binding was successful and move on

        close(listfd); // if anything failed, close this socket and try the next record returned from getaddrinfo()
    }
    if (!record)
        return -1;

    // free the results since we don't need them any more
    freeaddrinfo(results);

    //
    if (listen(listfd, maxConnections) < 0)
    {
        close(listfd);
        return -1;
    }

    return listfd; // return the connected socket file descriptor
>>>>>>> Stashed changes
}

<<<<<<< Updated upstream
void gfserver_set_handler(gfserver_t *gfs, ssize_t (*handler)(gfcontext_t *, char *, void *))
{
    if (!gfs)
        fprintf(stderr, "ERROR: Attempting to set handler callback on null object.\n");

    if (!handler)
        fprintf(stderr, "ERROR: Attempting to set handler callback, but no handler was passed in.\n");
||||||| constructed merge base
void gfserver_set_maxpending(gfserver_t *gfs, int max_npending){
=======
void gfserver_serve(gfserver_t *gfs)
{
    if (!gfs)
    {
        fprintf(stderr, "ERROR: gfserver_t object is NULL.\n");
        return;
    }

    // Covert the port number to a string
    char portno[6];
    snprintf(portno, 5, "%u", gfs->port);

    // Open the server socket listener
    if ((gfs->sock = newListenerFd(portno, gfs->maxPendingConn)) < 0)
    {
        fprintf(stderr, "ERROR: Error opening server socket.\n");
        return;
    }

    // fprintf(stderr, "Starting server...\n");

    // Main server loop
    while (1)
    {
        // ssize_t dataRead = 0;
        char buf[BUFSIZE];
        struct sockaddr_storage clientAddr;
        socklen_t clientAddrLen = sizeof(struct sockaddr_storage);
        gfcontext_t *client = malloc(sizeof(struct gfcontext_t));
        memset(client, 0, sizeof(gfcontext_t));

        /* Accept connections on server socket */
        if ((client->clientContext = accept(gfs->sock, (struct sockaddr *)&clientAddr, &clientAddrLen)) < 0)
        {
            fprintf(stderr, "ERROR: Failed to accept client connection.\n");
            return;
        }

        memset(&buf, 0, sizeof(buf));
        // dataRead = recv(client->clientContext, buf, BUFSIZE, 0);
        recv(client->clientContext, buf, BUFSIZE, 0);
        fprintf(stderr, "[request] %s\n", buf);
        ssize_t reqCheck = parseRequest(gfs, buf);
        if (reqCheck < 0)
        {
            fprintf(stderr, "ERROR: Check of client request returned an ERROR.\n");
            gfs_sendheader(client, GF_FILE_NOT_FOUND, 0);
        }
        else
        {
            fprintf(stderr, "Data received. Initiating file transfer.\n");
            ssize_t handlerCheck = gfs->handler(client, gfs->path, gfs->handlerArgs);
            if (handlerCheck < 0)
            {
                fprintf(stderr, "ERROR: Handler function returned an error. [handlerCheck] %zd\n", handlerCheck);
                gfs_abort(client);
                break;
            }
            fprintf(stderr, "FILE TRANSFER COMPLETE! [handlerCheck] %zd\n", handlerCheck);
            gfs->bytesSent += handlerCheck;
        }
        gfs_abort(client);
    }

    // server is done
    close(gfs->sock);
    return;
}

void gfserver_set_handlerarg(gfserver_t *gfs, void *arg)
{
    if (!gfs)
        fprintf(stderr, "ERROR: Attempting to set handler arguments on null object.\n");
    gfs->handlerArgs = arg;
}

void gfserver_set_handler(gfserver_t *gfs, ssize_t (*handler)(gfcontext_t *, char *, void *))
{
    if (!gfs)
        fprintf(stderr, "ERROR: Attempting to set handler callback on null object.\n");

    if (!handler)
        fprintf(stderr, "ERROR: Attempting to set handler callback, but no handler was passed in.\n");
>>>>>>> Stashed changes

    gfs->handler = handler;
}

void gfserver_set_maxpending(gfserver_t *gfs, int max_npending)
{
    if (!gfs)
        fprintf(stderr, "ERROR: Attempting to set maximum pending connections on null object.\n");

    gfs->maxPendingConn = max_npending;
}

void gfserver_set_port(gfserver_t *gfs, unsigned short port)
{
    if (!gfs)
        fprintf(stderr, "ERROR: Attempting to set port on null object.\n");

<<<<<<< Updated upstream
    gfs->port = port;
}
||||||| constructed merge base
=======
    gfs->port = port;
}
>>>>>>> Stashed changes
