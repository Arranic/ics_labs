#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <getopt.h>
#include <uchar.h>

/* Be prepared accept a response of this length */
#define BUFSIZE 4096

#define USAGE                                                                      \
    "usage:\n"                                                                     \
    "  echoclient [options]\n"                                                     \
    "options:\n"                                                                   \
    "  -h                  Show this help message\n"                               \
    "  -m                  Message to send to server (Default: \"Hello World!\"\n" \
    "  -p                  Port (Default: 8080)\n"                                 \
    "  -s                  Server (Default: localhost)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"message", required_argument, NULL, 'm'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

/* Main ========================================================= */
int main(int argc, char **argv)
{
    struct addrinfo hints, *results, *record; // contains settings for the connection, output of getaddrinfo() goes here, pointer to single record in the results linked list
    memset(&hints, 0, sizeof(struct addrinfo));
    int clientSocket, option_char = 0;
    char *hostname = "localhost";
    unsigned short portno = 8080;
    char recvBuf[BUFSIZE];
    char *message = "Hello World!";
    char *port = "8080";

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "s:p:m:h", gLongOptions, NULL)) != -1)
    {
        switch (option_char)
        {
        case 's': // server
            hostname = optarg;
            break;
        case 'p': // listen-port
            portno = atoi(optarg);
            port = optarg;
            break;
        case 'm': // server
            message = optarg;
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

    if (NULL == message)
    {
        fprintf(stderr, "%s @ %d: invalid message\n", __FILE__, __LINE__);
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

    /* Socket Code Here */

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; // Stream socket
    hints.ai_protocol = IPPROTO_TCP; // TCP

    if ((getaddrinfo(hostname, port, &hints, &results)) != 0)
    {
        perror("Failure to translate client socket");
        exit(EXIT_FAILURE);
    }

    // iterate through all the records returned by getaddrinfo()
    for (record = results; record != NULL; record = record->ai_next)
    {
        clientSocket = socket(record->ai_family, record->ai_socktype, record->ai_protocol);
        if (clientSocket < 0)
            continue;
        if (connect(clientSocket, record->ai_addr, record->ai_addrlen) != -1)
            break;

        close(clientSocket);
    }

    if (record == NULL)
    {
        fprintf(stderr, "failed to create or connect client socket.\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(results);

    if (send(clientSocket, message, strlen(message), 0) == -1)
    {
        fprintf(stderr, "Failure to send message.");
        exit(EXIT_FAILURE);
    }

    memset(recvBuf, 0, sizeof(recvBuf)); // make sure the recvBuf is actually empty
    recv(clientSocket, recvBuf, BUFSIZE, 0);
    printf("%s", recvBuf);

    close(clientSocket);
    exit(0);
}