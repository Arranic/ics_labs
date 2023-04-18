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

#define BUFSIZE 4096

// global variables
// char *respBuf;
const char *REQFORMAT = "%s %s %s\r\n\r\n"; // GETFILE GET <path> \r\n\r\n

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
        return (gfcrequest_t *)NULL;
    }

    memset(gfcreq, 0, sizeof(gfcrequest_t)); // zero the request memory so it's nice and clean
    gfcreq->bytesReceived = 0;
    gfcreq->fileLen = 0;

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
        return -7;
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
        return -8;
    }

    // if we get this far, return the open socket for use!
    return sock;
}

void printHex(const char *s)
{
    while (*s)
        printf("%02x ", (unsigned int)*s++);
    printf("\n");
}

int parseHeader(gfcrequest_t *gfr, char *buffer)
{
<<<<<<< Updated upstream
    // printf("[header] ");
    // printHex(buffer);
    int bufLen = strlen(buffer);
    char *buffCopy = malloc(bufLen + 1);
    memset(buffCopy, 0, bufLen + 1);
    char *token;
    snprintf(buffCopy, bufLen + 1, "%s", buffer);
    // asprintf(&buffCopy, "%s", buffer);

    // Check for termination set
    // printf("[header copy] ");
    // printHex(buffCopy);
    // if (strstr(buffCopy, "\r\n\r\n") == NULL)
    if (strcasestr(buffCopy, "\r\n\r\n") == NULL)
    {
        gfr->status = GF_INVALID;
        gfr->fileLen = -1;
        free(buffCopy);
        return -4;
    }

    // printf("[raw buffer] %x\n", buffCopy);
    token = strtok(buffCopy, " "); // first call

    printf("[TOKEN]::SCHEME   :: %s\n", token);
||||||| constructed merge base
    char *buffCopy;
    char *token;
    asprintf(&buffCopy, "%s", buffer);
    token = strtok(buffCopy, " "); // first call

    // printf("[token] %s\n", token);
=======
    char scheme[100], status[100], content[BUFSIZE];
    ssize_t size;
    memset(&scheme, 0, 100);
    memset(&status, 0, 100);
    memset(&content, 0, BUFSIZE);
>>>>>>> Stashed changes

    if (sscanf(buffer, "%s %s %zd\r\n\r\n%s", scheme, status, &size, content) == EOF)
    {
        fprintf(stderr, "ERROR: Error while reading data from the buffer.\n");
        return -123;
    }

<<<<<<< Updated upstream
    /* CHECK THE STATUS THAT WAS RETURNED BY THE SERVER */
    token = strtok(NULL, " ");
    printf("[TOKEN]::STATUS   :: %s\n", token);
    if (token == NULL || strlen(token) < 2) // check length of string in the token to make sure it's not too short
||||||| constructed merge base
    /* CHECK THE STATUS THAT WAS RETURNED BY THE SERVER */
    token = strtok(NULL, " ");
    // printf("[token] %s\n", token);
    if (token == NULL || strlen(token) < 2) // check length of string in the token to make sure it's not too short
=======
    // Check the scheme
    if (strcmp(scheme, "GETFILE") != 0)
>>>>>>> Stashed changes
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
<<<<<<< Updated upstream
    else // Anything else is returned except OK
    {
        gfr->status = GF_INVALID;
        gfr->fileLen = -1;
        free(buffCopy);
        return -11;
    }

    // // Check for termination set
    // if (strstr(buffCopy, "\r\n\r\n") == NULL)
    // {
    //     gfr->status = GF_INVALID;
    //     gfr->fileLen = -1;
    //     free(buffCopy);
    //     return -4;
    // }
||||||| constructed merge base
    else // Anything else is returned except OK
    {
        gfr->status = GF_INVALID;
        gfr->fileLen = -1;
        free(buffCopy);
        return -1;
    }

    // Check for termination set
    if (strstr(buffer, "\r\n\r\n") == NULL)
    {
        gfr->status = GF_INVALID;
        gfr->fileLen = -1;
        free(buffCopy);
        return -4;
    }
=======
    else // better safe than sorry :D
    {
        gfr->status = GF_INVALID;
        gfr->fileLen = -1;
        return -45;
    }
>>>>>>> Stashed changes

    // Check the size
    if (size == 0) // if the size is 0, we have an error
    {
        gfr->status = GF_INVALID;
        gfr->fileLen = -1;
        return -6;
    }
    gfr->fileLen = size;
<<<<<<< Updated upstream

    /*
        Next portion of the message could contain data so we'll check for that.
        If we have any, write it to the writeFunc
    */
    token = strtok(NULL, "\r\n\r\n");
    printf("[TOKEN]::DATA     :: %s\n", token);
    if (token != NULL)
||||||| constructed merge base

    /*
        Next portion of the message could contain data so we'll check for that.
        If we have any, write it to the writeFunc
    */
    token = strtok(NULL, "\r\n\r\n");
    printf("[TOKEN]::XTRA DATA:: %s\n", token);
    if (token != NULL)
=======
    if (content[0] != '\0')
>>>>>>> Stashed changes
    {
<<<<<<< Updated upstream
        printHex(token);
        int len = strlen(token) + 1;
        gfr->bytesReceived = gfr->bytesReceived + len;
        gfr->writefunc(token, len, gfr->writeArgs);
        // free(buffCopy);
||||||| constructed merge base
        int len = strlen(token) + 1;
        gfr->bytesReceived = gfr->bytesReceived + len;
        gfr->writefunc(token, len, gfr->writeArgs);
        // free(buffCopy);
=======
        size_t len = strlen(content);
        gfr->bytesReceived += len;
        gfr->writefunc(content, len, gfr->writeArgs);
>>>>>>> Stashed changes
    }

    return 0;

    printf("[header] ");
    printHex(buffer);

    // // GETFILE OK 123456
    // char scheme[64], status[64];
    // int dataPos = 0;
    // int bufferLen = strlen(buffer);

    // // printf("[buffer] %s\n", buffer);
    // sscanf(buffer, "%s %s %zu\r\n\r\n%n", scheme, status, &gfr->fileLen, &dataPos);

    // printf("[scheme] %s [status] %s [fileLen] %zu\n", scheme, status, gfr->fileLen);

    // if (strcmp(scheme, "GETFILE") != 0)
    // {
    //     gfr->status = GF_INVALID;
    //     return -60;
    // }
    // if (strcmp(status, "OK") == 0)
    //     gfr->status = GF_OK;
    // else if (strcmp(status, "FILE_NOT_FOUND") == 0)
    //     gfr->status = GF_FILE_NOT_FOUND;
    // else if (strcmp(status, "ERROR") == 0)
    //     gfr->status = GF_ERROR;
    // else
    //     gfr->status = GF_INVALID;

    // if (dataPos < bufferLen)
    // {
    //     printf("[bufferLen] %d\n[dataPos] %d\n[data] ", bufferLen, dataPos);
    //     printHex(buffer + dataPos);
    //     size_t toWrite = bufferLen - dataPos;
    //     gfr->writefunc(buffer + dataPos, toWrite, gfr->writeArgs);
    //     gfr->bytesReceived += toWrite;
    //     printf("[header bytesReceived] %zu\n [header toWrite] %zu\n", gfr->bytesReceived, toWrite);
    // }

    // return 0;
}

int recvHeader(gfcrequest_t *gfr)
{
    if (gfr == NULL)
        return -18;

    char *buffer = malloc(BUFSIZE + 1);
    if (!buffer)
        return -30;
    memset(buffer, 0, BUFSIZE);
    int parseCheck = -1;
    ssize_t received = 0;

<<<<<<< Updated upstream
    // received = recv(gfr->sock, buffer, BUFSIZE, MSG_PEEK);
    // char *end = strstr(buffer, "\r\n\r\n");

    while ((received = recv(gfr->sock, buffer, BUFSIZE, 0)) > 0)
||||||| constructed merge base
    char buffer[BUFSIZE];
    memset(&buffer, 0, sizeof(buffer));
    int offset = 0;
    while (1)
=======
    char buffer[100];
    memset(&buffer, 0, 100);
    while (1)
>>>>>>> Stashed changes
    {
<<<<<<< Updated upstream
        if (received == 0)
        {
            gfr->status = GF_INVALID;
            return -40;
        }
        else if (received < 0)
        {
            gfr->status = GF_ERROR;
            return -50;
        }

        // printf("[header] ");
        // printHex(buffer);
||||||| constructed merge base
        int received = recv(gfr->sock, buffer + offset, BUFSIZE - offset, 0);
        // printf("Data Received\n");
        if (received <= 0)
            return parseHeader(gfr, buffer);

        // printf("[raw header buffer] %s\n", buffer);
=======
        ssize_t received = recv(gfr->sock, buffer, 21, 0);
        fprintf(stderr, "[raw header]::::");
        printHex(buffer, 1);
>>>>>>> Stashed changes

<<<<<<< Updated upstream
        if ((parseCheck = parseHeader(gfr, buffer)) == 0)
||||||| constructed merge base
        // printf("Parsing Header\n");
        if (parseHeader(gfr, buffer) == 0)
            return 0;
        printf("Checking buffer size and offset\n");
        if (strlen(buffer) > 26 || offset >= BUFSIZE)
=======
        if (strlen(buffer) > 27)
>>>>>>> Stashed changes
        {
            free(buffer);
            return parseCheck;
        }
    }

    // printf("Parse checking returned %d\n", parseCheck);

<<<<<<< Updated upstream
    // if (received <= 0)
    // {
    //     printf("Receive returned %zd\n", received);
    //     parseCheck = parseHeader(gfr, buffer);
    //     free(buffer);
    //     return parseCheck;
    // }

    if (buffer != NULL)
    {
        free(buffer);
||||||| constructed merge base
        offset += received;
=======
        int parseCheck = parseHeader(gfr, buffer);
        fprintf(stderr, "[parseCheck] %d\n", parseCheck);
        if (parseCheck == -45)
        {
            // recv 6 more bytes and call parseHeader again...might've been a FILE_NOT_FOUND error
            ssize_t recv2 = recv(gfr->sock, buffer + received, 6, 0);
            received += recv2;

            parseCheck = parseHeader(gfr, buffer); // call parseCheck again
        }

        return parseCheck;
>>>>>>> Stashed changes
    }
    printf("Parse checking returned %d\n", parseCheck);
    return parseCheck;
}

int recvFile(gfcrequest_t *gfr)
{
    fprintf(stderr, "Inside recvFile.\n");
    if (!gfr)
        return -19;

<<<<<<< Updated upstream
    char buf[BUFSIZE];
    // printf("[bytesReceived] %zu\n", gfr->bytesReceived);
    memset(&buf, 0, sizeof(buf));
    while (gfr->bytesReceived < gfr->fileLen)
    {
        size_t dataRead = recv(gfr->sock, buf, BUFSIZE, 0);
        // printf("[received buffer] ");
        // printHex(buf);
        gfr->bytesReceived += dataRead;
        // printf("[data read] %d\n", dataRead);
        //  if we've received all the data, recv will return 0
||||||| constructed merge base
    char buf[BUFSIZE];
    memset(&buf, 0, sizeof(buf));
    while (1)
    {
        // check to see if we've written all the data
        if (gfr->bytesReceived == gfr->fileLen)
            return 0;

        int dataRead = 0;
        dataRead = recv(gfr->sock, buf, BUFSIZE, 0);

        // if we've received all the data, recv will return 0
=======
    char *buf = malloc(BUFSIZE);
    memset(buf, 0, BUFSIZE);

    fprintf(stderr, "[bytesReceived] %zd\n", gfr->bytesReceived);
    while (gfr->bytesReceived < gfr->fileLen)
    {
        ssize_t dataRead = recv(gfr->sock, buf, BUFSIZE, 0);
        fprintf(stderr, "[data received in file transfer] %zd\n", dataRead);

        // if we've received all the data, recv will return 0
>>>>>>> Stashed changes
        if (dataRead == 0)
        {
<<<<<<< Updated upstream
            break;
||||||| constructed merge base
            // break;
            // handle the case where we received less data than the file size
            if (gfr->bytesReceived < gfr->fileLen)
            {
                // printf("Connection prematurely closed");
                return -1;
            }

            return 0;
=======
            // handle the case where we received less data than the file size
            if (gfr->bytesReceived < gfr->fileLen)
            {
                // printf("Connection prematurely closed");
                free(buf);
                return -1;
            }

            break;
            // return 0;
        }
        else if (dataRead < 0)
        {
            fprintf(stderr, "Error while receiving data. Error code: %d\n", errno);
            break;
>>>>>>> Stashed changes
        }
        gfr->writefunc(buf, dataRead, gfr->writeArgs);
<<<<<<< Updated upstream
        memset(&buf, 0, sizeof(buf)); // now that the data is written, reset the buffer
||||||| constructed merge base
        memset(&buf, 0, sizeof(buf)); // now that the data is written, reset the buffer
        gfr->bytesReceived += dataRead;
=======
        gfr->bytesReceived += dataRead;
>>>>>>> Stashed changes
    }

    // handle the case where we received less data than the file size
    if (gfr->bytesReceived < gfr->fileLen)
    {
<<<<<<< Updated upstream
        printf("ERROR: Connection prematurely closed when receiving file.\n");
        gfr->status = GF_ERROR;
        return -21;
||||||| constructed merge base
        // printf("Connection prematurely closed");
        return -1;
=======
        // printf("Connection prematurely closed");
        free(buf);
        return -1;
>>>>>>> Stashed changes
    }

    free(buf);
    return 0;
}

int sendRequest(gfcrequest_t *gfr, char *buffer, int size)
{
    int dataSent = 0, numBytesToSend = size;
    while (numBytesToSend > 0)
    {
        ssize_t sent = send(gfr->sock, buffer, numBytesToSend, 0);
        if (sent < 0)
        {
            return -13;
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
        return -22;
    }

    char portno[6];
    snprintf(portno, 5, "%u", gfr->port);

    if ((gfr->sock = buildSocket(gfr->server, portno)) < 0)
    {
        fprintf(stderr, "Socket build failure.\n");
        return -14;
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
        return -15;
    }
    free(reqBuf); // done with the request buffer

    // shutdown(gfr->sock, SHUT_WR); // we've sent all we need to send, shutdown send actions over the socket

    int hdrStatus = recvHeader(gfr); // receive the header information
    if (hdrStatus < 0)
    {
        close(gfr->sock);
        return hdrStatus;
    }

    if (gfr->status != GF_OK)
    {
        close(gfr->sock);
        return 0;
    }
    // printf("perform [bytesReceived] %zu\n", gfr->bytesReceived);
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