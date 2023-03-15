#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

#define BUFSIZE 4096
#define bool int
#define false 0
#define true 1

#define USAGE                                               \
    "usage:\n"                                              \
    "  transferclient [options]\n"                          \
    "options:\n"                                            \
    "  -h                  Show this help message\n"        \
    "  -o                  Output file (Default foo.txt)\n" \
    "  -p                  Port (Default: 8080)\n"          \
    "  -s                  Server (Default: localhost)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"output", required_argument, NULL, 'o'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

int buildSocket(char *, char *);
void dumpHex(char *, int);

/* Main ========================================================= */
int main(int argc, char **argv)
{
    int option_char = 0;
    char *hostname = "localhost";
    unsigned short portno = 8080;
    char port[6] = "8080";
    char *filename = "foo.txt";
    int filefd, sock;
    // ssize_t dataRead;
    ssize_t dataWritten = 0;
    // uint32_t recvLen = 0;
    char *fileBuf = malloc(BUFSIZE);
    if (!fileBuf)
    {
        fprintf(stderr, "Unable to allocate memory with error %d.\n", errno);
    }

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "s:p:o:h", gLongOptions, NULL)) != -1)
    {
        switch (option_char)
        {
        case 's': // server
            hostname = optarg;
            break;
        case 'p': // listen-port
            portno = atoi(optarg);
            strncpy(port, optarg, 5); // store the string version of the port for use
            break;
        case 'o': // filename
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

    // Basic input checks
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

    if (NULL == hostname)
    {
        fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
        exit(1);
    }

    /* Open or Create Output File */
    if ((filefd = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) == -1)
    {
        fprintf(stderr, "Failed to; open %s with error %d.\n", filename, errno);
        exit(EXIT_FAILURE);
    }

    /* Socket */
    if ((sock = buildSocket(hostname, port)) < 0)
    {
        fprintf(stderr, "Socket build failure.\n");
        exit(EXIT_FAILURE);
    }

    memset(fileBuf, 0, BUFSIZE);
    // int totalRecv = 0, totalWrit = 0, totalPackets = 0;
    // dataRead = recv(sock, fileBuf, BUFSIZE, 0);
    uint32_t netFileSize = 0;
    uint32_t fileSize = 0;

    /* Get the file size from the server and convert to host order */
    if (recv(sock, &netFileSize, sizeof(netFileSize), 0) < 0)
    {
        fprintf(stderr, "Error receiving file size from server.\n");
    }
    fileSize = ntohl(netFileSize); // convert the file size from network to host order

    uint32_t recvBytes = 0;

    while (recvBytes < fileSize)
    {
        ssize_t awaitingBytes = BUFSIZE;
        if (fileSize - recvBytes < awaitingBytes)
        {
            awaitingBytes = fileSize - recvBytes;
        }
        ssize_t received = recv(sock, fileBuf, awaitingBytes, 0);
        if (received < 0)
        {
            fprintf(stderr, "Error while receiving data.\n");
            break;
        }
        dataWritten = write(filefd, fileBuf, received);
        if (dataWritten < 0)
        {
            fprintf(stderr, "Error while writing to file.\n");
            break;
        }
        recvBytes += received;
    }

    // while ((dataRead = recv(sock, fileBuf, BUFSIZE, 0)) > 0)
    // {
    //     if (dataRead == 0)
    //     {
    //         // fprintf(stdout, "Connection closed by server.\n");
    //         break;
    //     }
    //     if (dataRead > sizeof(uint32_t))
    //     {
    //         dataRead = dataRead - sizeof(uint32_t); // subtract the 4-bytes corresponding to the length of the message, so we just have the size of the data
    //         totalRecv += dataRead;
    //     }
    //     if (totalPackets == 0) // if this is the first loop, get the size of the file from the server, no writes
    //     {
    //         memcpy(&fileSize, fileBuf, sizeof(uint32_t)); // copy the first 4 bytes of fileBuf into fileSize
    //         fileSize = ntohl(fileSize);
    //         printf("[First packet] file size: %d bytes (%d KB)\n", fileSize, fileSize / 1000);
    //     }
    //     else
    //     {
    //         printf("dataRead %lu\n", dataRead);
    //         // fileBuf[dataRead] = '\0';
    //         //  printf("%s\n", fileBuf);
    //         /*
    //             The packet should look like this:
    //             {4096-byte} buffer [{4-byte} int msgLen, {4092-byte} char *msg]
    //         */

    //         // fileBuf[dataRead] = '\0';
    //         // printf("[FILEBUF]: %s\n", fileBuf);
    //         // memcpy(&recvLen, fileBuf, sizeof(uint32_t)); // recvLen should be 4096 which == the size of the message and length value combined
    //         // if (dataRead == recvLen)                     // if we read in all the data we were supposed to
    //         // {
    //         //     fileBuf = fileBuf + sizeof(uint32_t); // move the buffer forwards 8 to get to the data
    //         //     ;
    //         // }

    //         // fileBuf = fileBuf + sizeof(uint32_t);
    //         // printf("%s\n", fileBuf);
    //         // printf("recvLen: %u\n", recvLen);
    //         dataWritten = write(filefd, fileBuf, dataRead);
    //         if (dataWritten == -1)
    //         {
    //             fprintf(stderr, "Write failed with error %d.\n", errno);
    //         }
    //         while (dataWritten < fileSize)
    //         {
    //         }

    //         if (dataWritten != dataRead)
    //         {
    //             fprintf(stderr, "Unable to write all data.\n");
    //             // exit(EXIT_FAILURE);
    //         }

    //         totalWrit += dataWritten;
    //     }

    //     totalPackets++; // increment total packets
    //     /* Now that we've written the buffer data to file, we can clear it for use with the next set of data */
    //     // fileBuf = fileBuf - sizeof(uint32_t); // reset fileBuf back to its original location
    //     memset(fileBuf, 0, BUFSIZE); // clear the buffer
    // }

    // // printf("[PACKETS] %d [RECVD] %d bytes [WRITTEN] %d bytes\n", totalPackets, totalRecv, totalWrit);
    // printf("[PACKETS] %d \n", totalPackets);

    // free the file buffer
    free(fileBuf);
    close(filefd);
    return 0;
}

int buildSocket(char *serverName, char *port)
{
    // contains settings for the connection, output of getaddrinfo() goes here, pointer to single record in the results linked list
    struct addrinfo hints, *results, *record;
    int sock; // socket for communicating to server

    // Some connection settings
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // get the address information for the server
    if ((getaddrinfo(serverName, port, &hints, &results)) != 0)
    {
        fprintf(stderr, "Failure to translate address information");
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

    // if we didn't get a record from above, just exit the program
    if (record == NULL)
    {
        fprintf(stderr, "Failed to create or connect client socket.\n");
        return -1;
    }

    freeaddrinfo(results); // we no longer need the results returned by getaddrinfo()

    // if we get this far, return the open socket for use!
    return sock;
}

void dumpHex(char *buf, int size)
{
    int i;
    for (i = 0; i < size; i++)
    {
        printf("0x%x ", buf[i]);
    }
    printf("\n[CHAR COUNT] %d\n", i);
}