#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netdb.h>

#include "gfclient.h"

#define BUFSIZE 64000

// global variables
// char *respBuf;
const char *REQFORMAT = "%s %s %s\r\n\r\n";     // GETFILE GET <path> \r\n\r\n
const char *RESPFORMAT = "%s %s %zd\r\n\r\n%n"; // GETFILE OK 123456\r\n\r\n <content>
const char *HDRFORMAT = "%s %s %zd\r\n\r\n";    // GETFILE OK SIZE\r\n\r\n

struct gfcrequest_t
{
    /* Socket */
    int sock;

    /* Socket Info */
    char *server;
    unsigned short port;

    /* Protocol Info */
    char *path; // file path

    /* Metadata */
    gfstatus_t status;
    size_t bytesReceived;
    size_t fileLen;

    // callbacks
    void (*headerfunc)(void *, size_t, void *);
    void *headerArgs;
    void (*writefunc)(void *, size_t, void *);
    void *writeArgs;
};

void printHex(const char *s, int test)
{
    if (test)
    {
        while (*s)
            fprintf(stderr, "%02x ", (unsigned int)*s++);
        fprintf(stderr, "\n");
    }
    else
    {
        while (*s)
            printf("%02x ", (unsigned int)*s++);
        printf("\n");
    }
}

void gfc_cleanup(gfcrequest_t *gfr)
{
    if (gfr == NULL)
    {
        fprintf(stderr, "(gfc_cleanup) ERROR: Attempting to free null gfcrequest_t struct. \n");
        return;
    }

    // if (gfr->request != NULL)
    // {
    //     free(gfr->request); // free the request buffer
    // }
    free(gfr); // free the struct memory
}

gfcrequest_t *gfc_create()
{
    gfcrequest_t *gfcreq = malloc(sizeof(gfcrequest_t)); // allocate memory for the request
    if (gfcreq == NULL)                                  // malloc failed
    {
        fprintf(stderr, "(gfc_create) ERROR: Allocation of memory failed with error %d", errno);
        // exit(EXIT_FAILURE);
        return (gfcrequest_t *)NULL;
    }

    memset(gfcreq, 0, sizeof(gfcrequest_t)); // zero the request memory so it's nice and clean
    gfcreq->bytesReceived = 0;
    gfcreq->fileLen = 0;

    // malloc space for the gfr request
    // gfcreq->request = malloc(BUFSIZE);
    // if (gfcreq->request == NULL)
    // {
    //     fprintf(stderr, "(gfc_create) ERROR: Unable to allocate space for request. Error Code: %d\n", errno);
    //     free(gfcreq);
    //     return (gfcrequest_t *)NULL;
    // }

    return gfcreq;
}

size_t gfc_get_bytesreceived(gfcrequest_t *gfr)
{
    if (!gfr)
    {
        fprintf(stderr, "(gfc_get_bytesreceived) ERROR: Attempt to check bytes received on a NULL object.\n");
        return -1; // -1 for error
    }

    return gfr->bytesReceived;
}

size_t gfc_get_filelen(gfcrequest_t *gfr)
{
    if (!gfr)
    {
        fprintf(stderr, "(gfc_get_filelen) ERROR: Attempting to retrieve file length on a NULL object.\n");
        return -1;
    }

    return gfr->fileLen;
}

gfstatus_t gfc_get_status(gfcrequest_t *gfr)
{
    if (!gfr)
    {
        fprintf(stderr, "(gfc_get_status) ERROR: Attempting to retrieve status from a NULL object.\n");
        return -1;
    }

    return gfr->status;
}

void gfc_global_init()
{
}

void gfc_global_cleanup()
{
}

int buildSocket(char *serverName, char *port) // Builds the socket and returns the socket file descriptor
{
    // contains settings for the connection, output of getaddrinfo() goes here, pointer to single record in the results linked list
    struct addrinfo hints, *results, *record;
    int sock; // socket for communicating to server
    memset(&hints, 0, sizeof(struct addrinfo));

    // Some connection settings
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // get the address information for the server
    int servAddrInfo = getaddrinfo(serverName, port, &hints, &results);
    if (servAddrInfo != 0)
    {
        fprintf(stderr, "ERROR: %s.\n", gai_strerror(servAddrInfo));
        return -1;
    }

    // iterate through all the records returned by getaddrinfo()
    for (record = results; record != NULL; record = record->ai_next)
    {
        sock = socket(record->ai_family, record->ai_socktype, record->ai_protocol);
        if (sock < 0)
            continue; // socket creation on this record failed so try the next
        if (connect(sock, record->ai_addr, record->ai_addrlen) != -1)
            break;

        // if we get this far, then we've failed to either find a working
        // socket, or failed to connect. Close the socket.
        close(sock);
    }

    freeaddrinfo(results); // we no longer need the results returned by getaddrinfo()

    // if we didn't get a record from above, just exit the program
    if (record == NULL)
    {
        fprintf(stderr, "Failed to create or connect client socket.\n");
        freeaddrinfo(record);
        return -1;
    }

    // if we get this far, return the open socket for use!
    return sock;
}

int parseHeader(gfcrequest_t *gfr, char *buffer)
{
    char scheme[100], status[100], content[BUFSIZE];
    ssize_t size;
    memset(&scheme, 0, 100);
    memset(&status, 0, 100);
    memset(&content, 0, 100);

    // fprintf(stderr, "[header buffer] ");
    // printHex(buffer, 1);
    fprintf(stderr, "[header buffer] %s\n", buffer);

    if (sscanf(buffer, "%s %s %zd\r\n\r\n%s", scheme, status, &size, content) == EOF)
    {
        fprintf(stderr, "ERROR: Error while reading data from the buffer.\n");
        return -123;
    }

    // fprintf(stderr, "[scheme] %s, [status] %s, [size] %zd, [content] %s\n", scheme, status, size, content);

    // Check the scheme
    if (strcmp(scheme, "GETFILE") != 0)
    {
        gfr->status = GF_INVALID;
        gfr->fileLen = -1;
        return -2; // bad scheme
    }
    // Make sure status is a valid status
    if (strcmp(status, "OK") == 0)
        gfr->status = GF_OK;
    else if (strcmp(status, "FILE_NOT_FOUND") == 0 || strcmp(status, "FILE_NOT_FOUND\r\n\r\n") == 0 || strcmp(status, "FILE_NOT_FOUND \r\n\r\n") == 0)
    {
        gfr->status = GF_FILE_NOT_FOUND;
        return 0;
    }
    else if (strcmp(status, "ERROR") == 0 || strcmp(status, "ERROR\r\n\r\n") == 0 || strcmp(status, "ERROR \r\n\r\n") == 0)
    {
        gfr->status = GF_ERROR;
        return 0;
    }
    else // better safe than sorry :D
    {
        gfr->status = GF_INVALID;
        gfr->fileLen = -1;
        return -45;
    }

    // Check the size
    if (size == 0) // if the size is 0, we have an error
    {
        gfr->status = GF_INVALID;
        gfr->fileLen = -1;
        return -6;
    }
    gfr->fileLen = size;
    // printHex(content, 1);
    if (content != NULL)
    {
        // fprintf(stderr, "[TOKEN]::DATA     :: %s\n", token);
        // printHex(content, 1);
        int len = strlen(content);
        gfr->bytesReceived += len;
        gfr->writefunc(content, len + 1, gfr->writeArgs);
    }
    // char *buffCopy;
    // char *token;
    // asprintf(&buffCopy, "%s", buffer);
    // token = strtok(buffCopy, " "); // first call

    // // printf("[token] %s\n", token);

    // // get the scheme
    // if (strcmp("GETFILE", token) != 0)
    // {
    //     gfr->status = GF_INVALID;
    //     gfr->fileLen = -1;
    //     free(buffCopy);
    //     return -2; // bad scheme
    // }

    // /* CHECK THE STATUS THAT WAS RETURNED BY THE SERVER */
    // token = strtok(NULL, " ");
    // // printf("[token] %s\n", token);
    // if (token == NULL || strlen(token) < 2) // check length of string in the token to make sure it's not too short
    // {
    //     gfr->status = GF_INVALID;
    //     gfr->fileLen = -1;
    //     free(buffCopy);
    //     return -3; // bad string length or token was null
    // }
    // if (strcmp("OK", token) == 0) // OK is returned
    // {
    //     ; // using this as a jumper
    // }
    // else if ((strcmp("FILE_NOT_FOUND\r\n\r\n", token) == 0) || (strcmp("FILE_NOT_FOUND \r\n\r\n", token) == 0) || (strcmp("FILE_NOT_FOUND", token) == 0))
    // { // FILE_NOT_FOUND is returned
    //     gfr->status = GF_FILE_NOT_FOUND;
    //     gfr->fileLen = -1;
    //     free(buffCopy);
    //     return 0;
    // }
    // else if ((strcmp("ERROR", token) == 0) || (strcmp("ERROR\r\n\r\n", token) == 0) || (strcmp("ERROR \r\n\r\n", token) == 0)) // ERROR is returned
    // {
    //     gfr->status = GF_ERROR;
    //     gfr->fileLen = -1;
    //     free(buffCopy);
    //     return 0;
    // }
    // else // Anything else is returned except OK
    // {
    //     gfr->status = GF_INVALID;
    //     gfr->fileLen = -1;
    //     free(buffCopy);
    //     return -1;
    // }

    // // Check for termination set
    // if (strstr(buffer, "\r\n\r\n") == NULL)
    // {
    //     gfr->status = GF_INVALID;
    //     gfr->fileLen = -1;
    //     free(buffCopy);
    //     return -4;
    // }

    // // final token returned is the file size
    // token = strtok(NULL, "\r\n\r\n");
    // printf("[TOKEN]::FILE SIZE:: %s\n", token);
    // if (token == NULL)
    // {
    //     gfr->status = GF_INVALID;
    //     gfr->fileLen = -1;
    //     free(buffCopy);
    //     return -5;
    // }
    // int size = atoi(token); // convert the token to integer
    // if (size == 0)          // if the size is 0, we have an error
    // {
    //     gfr->status = GF_INVALID;
    //     gfr->fileLen = -1;
    //     free(buffCopy);
    //     return -6;
    // }

    // gfr->status = GF_OK;
    // gfr->fileLen = size;

    // /*
    //     Next portion of the message could contain data so we'll check for that.
    //     If we have any, write it to the writeFunc
    // */
    // // token = strtok(NULL, "\r\n\r\n");
    // char *ptr = strstr(buffCopy, "\r\n\r\n");
    // char *data = ptr + 4;
    // if (data != NULL)
    // {
    //     fprintf(stderr, "[ptr] %s [data] %s\n", ptr, data);
    //     // fprintf(stderr, "[TOKEN]::DATA     :: %s\n", token);
    //     int len = strlen(data);
    //     gfr->bytesReceived += len;
    //     gfr->writefunc(data, len, gfr->writeArgs);
    // }
    // // if (token != NULL)
    // // {
    // //     fprintf(stderr, "[TOKEN]::DATA     :: %s\n", token);
    // //     int len = strlen(token) + 1;
    // //     gfr->bytesReceived = gfr->bytesReceived + len;
    // //     gfr->writefunc(token, len, gfr->writeArgs);
    // //     // free(buffCopy);
    // // }

    // free(buffCopy); // free the allocated space for the copied buffer

    return 0;
}

int recvHeader(gfcrequest_t *gfr)
{
    if (gfr == NULL)
        return -1;

    char buffer[BUFSIZE];
    memset(&buffer, 0, sizeof(buffer));
    int offset = 0;
    while (1)
    {
        int received = recv(gfr->sock, buffer + offset, BUFSIZE - offset, 0);
        // printf("Data Received\n");
        if (received <= 0)
            return parseHeader(gfr, buffer);

        // printf("[raw header buffer] %s\n", buffer);

        // printf("Parsing Header\n");
        if (parseHeader(gfr, buffer) == 0)
            return 0;
        printf("Checking buffer size and offset\n");
        if (strlen(buffer) > 26 || offset >= BUFSIZE)
        {
            fprintf(stderr, "header length too large (%lu)\n ::[header]%s\n", strlen(buffer), buffer);
            return -1;
        }

        offset += received;
    }
}

int recvFile(gfcrequest_t *gfr)
{
    fprintf(stderr, "Inside recvFile.\n");
    if (!gfr)
        return -1;

    char *buf = malloc(BUFSIZE);
    memset(buf, 0, BUFSIZE);
    size_t needToRecv = gfr->fileLen - gfr->bytesReceived; // amount of data we have left to receive and then write to file
    ssize_t written = 0;                                   // amount of data we've written in this function locally

    while (written < needToRecv)
    {
        ssize_t dataRead = recv(gfr->sock, buf, BUFSIZE, 0);
        if (dataRead < 0)
        {
            // Error!
            fprintf(stderr, "ERROR: Error while receiving file from server.\n");
            break;
        }
        if (dataRead == 0)
        {
            break;
        }
        // Write the data
        gfr->writefunc(buf, dataRead, gfr->writeArgs);
        written += dataRead;
        fprintf(stderr, "[written] %zd\n", written);
    }
    gfr->bytesReceived += written;
    fprintf(stderr, "[bytesReceived] %zu, [fileLen] %zu\n", gfr->bytesReceived, gfr->fileLen);
    // fprintf(stderr, "[gfr->bytesReceived] %zu.\n", gfr->bytesReceived);
    // while (gfr->bytesReceived < gfr->fileLen)
    // {
    //     // check to see if we've written all the data
    //     // if (gfr->bytesReceived == gfr->fileLen)
    //     //     return 0;

    //     dataRead = recv(gfr->sock, buf, BUFSIZE, 0);

    //     // if we've received all the data, recv will return 0
    //     if (dataRead == 0)
    //     {
    //         // break;
    //         // handle the case where we received less data than the file size
    //         if (gfr->bytesReceived < gfr->fileLen)
    //         {
    //             // printf("Connection prematurely closed");
    //             free(buf);
    //             return -1;
    //         }

    //         break;
    //         // return 0;
    //     }
    //     else if (dataRead < 0)
    //     {
    //         fprintf(stderr, "Error while receiving data. Error code: %d\n", errno);
    //         break;
    //     }

    //     gfr->writefunc(buf, dataRead, gfr->writeArgs);
    //     // memset(&buf, 0, sizeof(buf)); // now that the data is written, reset the buffer
    //     gfr->bytesReceived += dataRead;
    // }

    // handle the case where we received less data than the file size
    if (gfr->bytesReceived < gfr->fileLen)
    {
        // printf("Connection prematurely closed");
        free(buf);
        return -1;
    }

    free(buf);
    return 100;
}

int sendRequest(gfcrequest_t *gfr, char *buffer, int size)
{
    int dataSent = 0, numBytesToSend = size;
    while (numBytesToSend > 0)
    {
        ssize_t sent = send(gfr->sock, buffer, numBytesToSend, 0);
        if (sent < 0)
        {
            return -1;
        }
        numBytesToSend -= sent;
        dataSent += sent;
    }
    return dataSent;
}

int gfc_perform(gfcrequest_t *gfr)
{
    if (gfr == NULL)
    {
        fprintf(stderr, "(gfc_perform) ERROR: gfcrequest_t input value is NULL.\n");
        return -1;
    }

    // int dataRead = 0;
    char portno[6];
    snprintf(portno, 5, "%u", gfr->port);

    if ((gfr->sock = buildSocket(gfr->server, portno)) < 0)
    {
        fprintf(stderr, "Socket build failure.\n");
        return -1;
    }

    /*
        Build the request data to send to the server.
        SYNTAX: <scheme> <method> <path>\r\n\r\n
    */
    char *reqBuf;
    int requestSize = asprintf(&reqBuf, REQFORMAT, "GETFILE", "GET", gfr->path);

    // send the request
    int sendStatus = sendRequest(gfr, reqBuf, requestSize);
    if (sendStatus < 0)
    {
        fprintf(stderr, "ERROR: Error sending file request.\n");
        free(reqBuf);
        return -1;
    }
    free(reqBuf); // done with the request buffer

    shutdown(gfr->sock, SHUT_WR); // we've sent all we need to send, shutdown send actions over the socket

    int hdrStatus = recvHeader(gfr); // receive the header information
    if (hdrStatus < 0)
    {
        close(gfr->sock);
        return (hdrStatus);
    }

    if (gfr->status != GF_OK)
    {
        close(gfr->sock);
        return 0;
    }

    int recvFileStatus = recvFile(gfr);
    if (recvFileStatus < 0)
    {
        close(gfr->sock);
        return recvFileStatus;
    }
    fprintf(stderr, "FILE TRANSFER COMPLETE!\n");
    close(gfr->sock);
    return 0;
}

void gfc_set_headerarg(gfcrequest_t *gfr, void *headerarg)
{
    if (gfr != NULL && headerarg != NULL)
        gfr->headerArgs = headerarg;
    else
    {
        if (gfr == NULL)
            fprintf(stderr, "(gfc_set_headerarg) ERROR: Passed-in value for gfcrequest_t is NULL.\n");
        if (headerarg == NULL)
            fprintf(stderr, "(gfc_set_headerarg) ERROR: Passed-in value for headerarg is NULL.\n");
    }
}

void gfc_set_headerfunc(gfcrequest_t *gfr, void (*headerfunc)(void *, size_t, void *))
{
    if (gfr != NULL && headerfunc != NULL)
        gfr->headerfunc = headerfunc;
    else
    {
        if (gfr == NULL)
            fprintf(stderr, "(gfc_set_headerfunc) ERROR: Passed-in value for gfcrequest_t is NULL.\n");
        if (headerfunc == NULL)
            fprintf(stderr, "(gfc_set_headerfunc) ERROR: Passed-in value for headerfunc is NULL.\n");
    }
}

void gfc_set_path(gfcrequest_t *gfr, char *path)
{
    if (path != NULL && path[0] == '/')
    {
        gfr->path = path;
    }
    else
        fprintf(stderr, "(gfc_set_path) ERROR: Attempting to set gfcrequest path to invalid value.\n");
}

void gfc_set_port(gfcrequest_t *gfr, unsigned short port)
{

    if ((port < 1025) || (port > 65535))
        fprintf(stderr, "(gfc_set_port) ERROR: Attempting to set gfcrequest port value to NULL.\n");
    else
        gfr->port = port;
}

void gfc_set_server(gfcrequest_t *gfr, char *server)
{
    if (server != NULL)
        gfr->server = server;
    else
        fprintf(stderr, "(gfc_set_server) ERROR: Attempting to set gfcrequest server value to NULL.\n");
}

void gfc_set_writearg(gfcrequest_t *gfr, void *writearg)
{
    if (gfr != NULL && writearg != NULL)
        gfr->writeArgs = writearg;
    else
    {
        if (gfr == NULL)
            fprintf(stderr, "(gfc_set_writearg) ERROR: Passed-in value for gfcrequest_t is NULL.\n");
        if (writearg == NULL)
            fprintf(stderr, "(gfc_set_writearg) ERROR: Passed-in value for writearg is NULL.\n");
    }
}

void gfc_set_writefunc(gfcrequest_t *gfr, void (*writefunc)(void *, size_t, void *))
{
    if (gfr != NULL && writefunc != NULL)
        gfr->writefunc = writefunc;
    else
    {
        if (gfr == NULL)
            fprintf(stderr, "(gfc_set_writefunc) ERROR: Passed-in value for gfcrequest_t is NULL.\n");
        if (writefunc == NULL)
            fprintf(stderr, "(gfc_set_writefunc) ERROR: Passed-in value for writefunc is NULL.\n");
    }
}

char *gfc_strstatus(gfstatus_t status)
{
    // if ((void *)status == NULL)
    //     return "Passed in status value is NULL.\n";

    char *s;
    switch (status)
    {
    case GF_OK:
        s = "OK";
        break;
    case GF_FILE_NOT_FOUND:
        s = "FILE_NOT_FOUND";
        break;
    case GF_ERROR:
        s = "ERROR";
        break;
    case GF_INVALID:
        s = "INVALID";
        break;
    default:
        s = "Status value is not valid";
    }

    return s;
}