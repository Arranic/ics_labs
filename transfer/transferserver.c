#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

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
#define bool int
#define false 0
#define true 1

#define USAGE                                             \
    "usage:\n"                                            \
    "  transferserver [options]\n"                        \
    "options:\n"                                          \
    "  -f                  Filename (Default: bar.txt)\n" \
    "  -h                  Show this help message\n"      \
    "  -p                  Port (Default: 8080)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"filename", required_argument, NULL, 'f'},
    {"port", required_argument, NULL, 'p'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

int newListenerFd(char *, int);

int main(int argc, char **argv)
{
    int option_char;
    int serverSock, filefd; /* server socket file descriptor */
    int portno = 8080;      /* port to listen on */
    char port[6] = "8080";
    char *filename = "bar.txt";      /* file to transfer */
    char *fileBuf = malloc(BUFSIZE); // buffer
    size_t dataRead = 0;
    ssize_t dataSent = 0;

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "f:p:h", gLongOptions, NULL)) != -1)
    {
        switch (option_char)
        {
        case 'p': // listen-port
            portno = atoi(optarg);
            strncpy(port, optarg, 5); // store the string version of the port for use
            break;
        case 'f': // listen-port
            filename = optarg;
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

    if (NULL == filename)
    {
        fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
        exit(1);
    }

    if ((portno < 1025) || (portno > 65535))
    {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }

    /* Open the server's listener socket */
    serverSock = newListenerFd(port, 5);

    /* Open the input file */
    if ((filefd = open(filename, S_IRUSR | S_IWUSR)) == -1)
    {
        fprintf(stderr, "Failed to open %s with error code %d.\n", filename, errno);
        exit(EXIT_FAILURE);
    }

    // get the size of the file in bytes
    struct stat fileStats;
    fstat(filefd, &fileStats);
    int fileSize = fileStats.st_size;
    printf("file name: %s\nfile size: %d bytes (%d KB)\n", filename, fileSize, fileSize / 1000);

    while (1)
    {
        int clientSock;
        int totalPackets = 0;
        struct sockaddr_storage clientAddr;
        socklen_t clientAddrLen = sizeof(struct sockaddr_storage);

        /* Accept socket connections */
        if ((clientSock = accept(serverSock, (struct sockaddr *)&clientAddr, &clientAddrLen)) < 0)
        {
            fprintf(stderr, "Failed to accept client connection.\n");
            exit(EXIT_FAILURE);
        }

        /*
            - Read the data in from the file in 100 byte chunks at a time and store in fileBuf.
            - Send the data in fileBuf to the clientSock
        */

        memset(fileBuf, 0, BUFSIZE); // zero out the buffer allocated with BUFSIZE bytes

        dataSent = 0;               // track the amount of data we send
        while (dataSent < fileSize) // read the data until the end of the file 4088 bytes at a time
        {
            dataRead = read(filefd, fileBuf, BUFSIZE); // read the data from the file
            if (dataRead < 0)
            {
                fprintf(stderr, "File read error.\n");
                exit(EXIT_FAILURE);
            }
            ssize_t numBytesToSend = dataRead; // amount of data to send based on the amount of data we read from the file
            // printf("[BYTES TO SEND] %lu\n", numBytesToSend);
            while (numBytesToSend > 0) // if we didn't send all the data that we read in, loop until we send it all
            {
                /* Send the data */
                // fileBuf + dataRead - numBytesToSend calculates where in fileBuf to start sending data from
                ssize_t sent = send(clientSock, fileBuf + dataRead - numBytesToSend, numBytesToSend, 0);
                if (sent < 0)
                {
                    fprintf(stderr, "File send error.\n");
                }
                numBytesToSend -= sent; // subtract the number of bytes we just sent from the total number of bytes to send
                dataSent += sent;       // the the amount of data sent
            }
            memset(fileBuf, 0, BUFSIZE); // reset buffer to read in more data

            totalPackets++; // track the number of packets we send
        }

        lseek(filefd, 0, SEEK_SET); // reset the file offset pointer back to the beginning of the file
        close(clientSock);          // close the client socket when done with it
        printf("dataSent %lu\n", dataSent);
        printf("[PACKETS] %d\n", totalPackets);
    }

    free(fileBuf);     // free the file buffer
    close(serverSock); // close the server socket
    exit(EXIT_SUCCESS);
}

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
}
