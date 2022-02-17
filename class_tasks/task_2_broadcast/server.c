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

#define SIZE_QUEUE 10
#define MAX_LEN 10000


enum ERRORS
{
    ERROR_INVALID_ARGC    = -1,
    ERROR_INVALID_ARGV    = -2,
    ERROR_INVALID_ADDRESS = -3,
    ERROR_SOCKET          = -4,
    ERROR_BIND            = -5,
    ERROR_LISTEN          = -6,
    ERROR_SIGACTION       = -7,
    ERROR_SETSOCKOPT      = -8,
    ERROR_RECVFROM        = -9,
    ERROR_WRITE           = -10,
    ERROR_READ            = -11,
    ERROR_SEND            = -12,
};

void sigchild_handler(int s)
{
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

int server_socket = 0;

int broadcast_server_interface(in_addr_t address, in_port_t port, in_addr_t broadcast_address, in_port_t broadcast_port);

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        printf("Please, run the program in the following format: \n"
               "./server <host> <port>\n");
        return ERROR_INVALID_ARGC;
    }

    struct sockaddr_in address = {};

    if (inet_ntop(AF_INET, &address.sin_addr, argv[1], strlen(argv[1])) == NULL)
    {
        printf("Please, run the program in the following format: \n"
               "./server <host> <port>\n");
        return ERROR_INVALID_ADDRESS;
    }

    address.sin_port = htons(atoi(argv[2]));

    return broadcast_server_interface(address.sin_addr.s_addr, address.sin_port, INADDR_BROADCAST, htons(27312));
}

int broadcast_server_interface(in_addr_t address, in_port_t port, in_addr_t broadcast_address, in_port_t broadcast_port)
{
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);

    if (server_socket == -1)
    {
        perror("ERROR: socket()");
        return  ERROR_SOCKET;
    }

    int reuse = 1;
    int broadcast = 1;
    struct timeval time = {.tv_sec = 120};

    if ((setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse,     sizeof(int)) == -1) ||
        (setsockopt(server_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(int)) == -1) ||
        (setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO,  &time,      sizeof(time)) == -1))
    {
        perror("ERROR: setsockopt()");
        return  ERROR_SETSOCKOPT;
    }

    struct sockaddr_in server = {.sin_family = AF_INET, .sin_addr.s_addr = address, .sin_port = port};

    struct sockaddr_in client = {.sin_family = AF_INET, .sin_addr.s_addr = broadcast_address, .sin_port = broadcast_port};
    socklen_t client_len = sizeof(client);

    if (bind(server_socket, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == -1)
    {
        perror("ERROR: bind()");
        return  ERROR_BIND;
    }

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

    int data = sendto(server_socket, buffer, len + 1, 0, (struct sockaddr*)&client, sizeof(client));

    if (data == -1)
    {
        perror("ERROR: send()");
        return  ERROR_SEND;
    }

    while (1)
    {
        int read_size = recvfrom(server_socket, buffer, MAX_LEN - 1, 0, (struct sockaddr*)&client, &client_len);

        if (read_size == -1)
        {
            perror("ERROR: recvfrom()");
            close(server_socket);
            return  ERROR_RECVFROM;
        }

        buffer[read_size] = '\0';
                    
        printf("The response was received from %s:%d :: %s\n", inet_ntoa(((struct sockaddr_in*)&client)->sin_addr), ntohs(((struct sockaddr_in*)&client)->sin_port), buffer);
    }
}

