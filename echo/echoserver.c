#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <uchar.h>

#if 0
/*
 * Structs exported from netinet/in.h (for easy reference)
 */

/* Internet address */
struct in_addr {
  unsigned int s_addr; 
};

/* Internet style socket address */
struct sockaddr_in  {
  unsigned short int sin_family; /* Address family */
  unsigned short int sin_port;   /* Port number */
  struct in_addr sin_addr;	 /* IP address */
  unsigned char sin_zero[...];   /* Pad to size of 'struct sockaddr' */
};

/*
 * Struct exported from netdb.h
 */

/* Domain name service (DNS) host entry */
struct hostent {
  char    *h_name;        /* official name of host */
  char    **h_aliases;    /* alias list */
  int     h_addrtype;     /* host address type */
  int     h_length;       /* length of address */
  char    **h_addr_list;  /* list of addresses */
}
#endif

#define BUFSIZE 4096
#define MAXSIZE 1024

#define USAGE                                             \
    "usage:\n"                                            \
    "  echoserver [options]\n"                            \
    "options:\n"                                          \
    "  -h                  Show this help message\n"      \
    "  -n                  Maximum pending connections\n" \
    "  -p                  Port (Default: 8080)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"port", required_argument, NULL, 'p'},
    {"maxnpending", required_argument, NULL, 'n'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

/* won't pull from the netdb.h file because I have no clue why...dumb */
#ifdef addrinfo
struct addrinfo
{
    int ai_flags;             /* Input flags.  */
    int ai_family;            /* Protocol family for socket.  */
    int ai_socktype;          /* Socket type.  */
    int ai_protocol;          /* Protocol for socket.  */
    socklen_t ai_addrlen;     /* Length of socket address.  */
    struct sockaddr *ai_addr; /* Socket address for socket.  */
    char *ai_canonname;       /* Canonical name for service location.  */
    struct addrinfo *ai_next; /* Pointer to next in list.  */
};
#endif

ssize_t echoResponse(int);
int newListFd(char *, int);

/* MAIN */
int main(int argc, char **argv)
{
    int optionChar;
    int portno = 8080; /* port to listen on */
    int maxnpending = 5;
    int serverSock; /* server socket file descriptor */
    char port[6] = "8080";

    // Parse and set command line arguments
    while ((optionChar = getopt_long(argc, argv, "p:n:h", gLongOptions, NULL)) != -1)
    {
        switch (optionChar)
        {
        case 'p': // listen-port
            portno = atoi(optarg);
            strncpy(port, optarg, 5);
            break;
        case 'n': // server
            maxnpending = atoi(optarg);
            break;
        case 'h': // help
            fprintf(stdout, "%s", USAGE);
            exit(0);
            break;
        default:
            fprintf(stderr, "%s", USAGE);
            exit(1);
        }
    }

    if ((portno < 1025) || (portno > 65535))
    {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }

    if (maxnpending < 1)
    {
        fprintf(stderr, "%s @ %d: invalid pending count (%d)\n", __FILE__, __LINE__, maxnpending);
        exit(1);
    }

    serverSock = newListFd(port, maxnpending);
    while (1)
    {
        int clientSock;
        struct sockaddr_storage clientAddr;
        socklen_t clientAddrLen = sizeof(struct sockaddr_storage);

        if ((clientSock = accept(serverSock, (struct sockaddr *)&clientAddr, &clientAddrLen)) < 0)
        {
            fprintf(stderr, "Failed to accept client connection.\n");
            exit(EXIT_FAILURE);
        }

        echoResponse(clientSock);
        close(clientSock);
    }
    close(serverSock);
    free(port);
    return 0;
}

/*
    Reads input from an socket that is already connected, immediately writes
    the output back to the socket, and returns the number of bytes written.
*/
ssize_t echoResponse(int fd)
{
    size_t n = BUFSIZE;
    char inBuf[BUFSIZE];
    memset(inBuf, 0, n);
    // char outBuf[BUFSIZE];
    // memset(outBuf, 0, n);
    int inLen;
    int bytesWritten;

    inLen = recv(fd, inBuf, n, 0);
    printf("Received %d bytes\n", (int)inLen);

    if (inLen == -1)
    {
        fprintf(stderr, "Unable to read from input on connection %d.\n", fd);
        bytesWritten = 0;
    }

    bytesWritten = send(fd, inBuf, inLen, 0);

    if (bytesWritten == inLen)
        printf("output matches input\n");

    return bytesWritten; // return the number of bytes written
}

int newListFd(char *port, int maxConnections)
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
}