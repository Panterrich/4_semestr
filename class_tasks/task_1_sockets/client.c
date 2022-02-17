#include <stdio.h>    
#include <stdlib.h>  
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <syslog.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/file.h>
#include <dirent.h>
#include <sys/time.h>
#include <utime.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <wait.h>
#include <arpa/inet.h>

#define MAX_LEN 1000

enum ERRORS
{
    ERROR_INVALID_ARGC    = -1,
    ERROR_INVALID_ARGV    = -2,
    ERROR_OPEN            = -3,
    ERROR_SOCKET          = -4,
    ERROR_CONNECT         = -5,
    ERROR_SEND            = -6,
    ERROR_INVALID_ADDRESS = -7,
    ERROR_READ            = -8
};

int client_tcp_interface(in_addr_t address, in_port_t port);

int client_udp_interface(in_addr_t address, in_port_t port);

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        printf("Please, run the program in the following format: \n"
               "./client [-t | -u] <host> <port>\n");
        return ERROR_INVALID_ARGC;
    }

    if (strcmp(argv[1], "-t") != 0 && strcmp(argv[1], "-u") != 0)
    {
        printf("Please, run the program in the following format: \n"
               "./client [-t | -u] <host> <port>\n");
        return ERROR_INVALID_ARGV;
    }
    struct sockaddr_in address = {};

    if (inet_ntop(AF_INET, &address.sin_addr, argv[2], strlen(argv[2])) == NULL)
    {
        printf("Please, run the program in the following format: \n"
               "./client [-t | -u] <host> <port>\n");
        return ERROR_INVALID_ADDRESS;
    }

    address.sin_port = htons(atoi(argv[3]));

    if (strcmp(argv[1], "-t") == 0)
    {
        client_tcp_interface(address.sin_addr.s_addr, address.sin_port);
    }

    if (strcmp(argv[1], "-u") == 0)
    {
        client_udp_interface(address.sin_addr.s_addr, address.sin_port);
    }

    return 0;
}

int client_tcp_interface(in_addr_t address, in_port_t port)
{
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (client_socket == -1)
    {
        perror("ERROR: socket()");
        return  ERROR_SOCKET;
    }

    struct sockaddr_in addr = {.sin_family = AF_INET, .sin_addr.s_addr = address, .sin_port = port};

    if (connect(client_socket, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1)
    {
        perror("ERROR: connect()");
        return  ERROR_CONNECT;
    }

    printf("\nInput: ");
    fflush(stdout);

    char buffer[MAX_LEN] = "";
    int len = read(STDIN_FILENO, buffer, MAX_LEN);

    if (len == -1)
    {
        perror("ERROR: read()");
        return  ERROR_READ;
    }

    buffer[len] = '\0';

    int data = send(client_socket, buffer, len + 1, 0);

    if (data == -1)
    {
        perror("ERROR: send()");
        return  ERROR_SEND;
    }

    close(client_socket);

    return 0;
}

int client_udp_interface(in_addr_t address, in_port_t port)
{
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);

    if (client_socket == -1)
    {
        perror("ERROR: socket()");
        return  ERROR_SOCKET;
    }

    struct sockaddr_in addr = {.sin_family = AF_INET, .sin_addr.s_addr = address, .sin_port = port};

    printf("\nInput: ");
    fflush(stdout);

    char buffer[MAX_LEN] = "";
    int len = read(STDIN_FILENO, buffer, MAX_LEN);

    printf("\n");

    if (len == -1)
    {
        perror("ERROR: read()");
        return  ERROR_READ;
    }

    buffer[len] = '\0';

    int data = sendto(client_socket, buffer, len + 1, 0, (struct sockaddr*)&addr, sizeof(addr));

    if (data == -1)
    {
        perror("ERROR: send()");
        return  ERROR_SEND;
    }

    close(client_socket);

    return 0;
}