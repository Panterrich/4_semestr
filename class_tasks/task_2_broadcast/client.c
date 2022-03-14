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
    ERROR_READ            = -8,
    ERROR_BIND            = -9,
    ERROR_RECVFROM        = -10,
    ERROR_SETSOCKOPT      = -11,
};

int client_udp_interface(in_addr_t address, in_port_t port);

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        printf("Please, run the program in the following format: \n"
               "./client <host> <port>\n");
        return ERROR_INVALID_ARGC;
    }

    struct sockaddr_in address = {};

    if (inet_ntop(AF_INET, &address.sin_addr, argv[1], strlen(argv[1])) == NULL)
    {
        printf("Please, run the program in the following format: \n"
               "./client <host> <port>\n");
        return ERROR_INVALID_ADDRESS;
    }

    address.sin_port = htons(atoi(argv[2]));

    return client_udp_interface(address.sin_addr.s_addr, address.sin_port);
}

int client_udp_interface(in_addr_t address, in_port_t port)
{
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);

    if (client_socket == -1)
    {
        perror("ERROR: socket()");
        return  ERROR_SOCKET;
    }

    int reuse = 1;
    int broadcast = 1;
    struct timeval time = {.tv_sec = 120};

    if ((setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &reuse,     sizeof(int)) == -1) ||
        (setsockopt(client_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(int)) == -1) ||
        (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO,  &time,      sizeof(time)) == -1))
    {
        perror("ERROR: setsockopt()");
        return  ERROR_SETSOCKOPT;
    }

    struct sockaddr_in client_addr = {.sin_family = AF_INET, .sin_addr.s_addr = address, .sin_port = port};

    if (bind(client_socket, (struct sockaddr*)&client_addr, sizeof(struct sockaddr_in)) == -1)
    {
        perror("ERROR: bind()");
        return  ERROR_BIND;
    }


    struct sockaddr_in server = {};
    socklen_t server_len = sizeof(server);
    
    char buffer[MAX_LEN] = "";
    int len = recvfrom(client_socket, buffer, MAX_LEN, 0, (struct sockaddr*)&server, &server_len);

    if (len == -1)
    {
        perror("ERROR: recvfrom()");
        return  ERROR_RECVFROM;
    }

    buffer[len] = '\0';

    printf("Broadcast message received from %s:%d :: %s\n", inet_ntoa(((struct sockaddr_in*)&server)->sin_addr), ntohs(((struct sockaddr_in*)&server)->sin_port), buffer);

    int data = sendto(client_socket, buffer, len + 1, 0, (struct sockaddr*)&server, server_len);

    if (data == -1)
    {
        perror("ERROR: send()");
        return  ERROR_SEND;
    }

    close(client_socket);

    return 0;
}