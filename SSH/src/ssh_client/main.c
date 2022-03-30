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

int exit_loop = 0;

void sig_alrm_handler(int signum)
{
    exit_loop = 1;
}

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
    ERROR_SIGACTION       = -12,
};

int broadcast_client_interface(in_addr_t address, in_port_t port, in_addr_t broadcast_address, in_port_t broadcast_port);

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

    return broadcast_client_interface(address.sin_addr.s_addr, address.sin_port, INADDR_BROADCAST, htons(35000));
}


int broadcast_client_interface(in_addr_t address, in_port_t port, in_addr_t broadcast_address, in_port_t broadcast_port)
{
    int client_socket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);

    if (client_socket == -1)
    {
        perror("ERROR: socket()");
        return  ERROR_SOCKET;
    }

    int reuse = 1;
    int broadcast = 1;
    struct timeval time = {.tv_sec = 15};

    if ((setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &reuse,     sizeof(int)) == -1) ||
        (setsockopt(client_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(int)) == -1) ||
        (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO,  &time,      sizeof(time)) == -1))
    {
        perror("ERROR: setsockopt()");
        return  ERROR_SETSOCKOPT;
    }

    struct sockaddr_in client = {.sin_family = AF_INET, .sin_addr.s_addr = address, .sin_port = port};

    if (bind(client_socket, (struct sockaddr*)&client, sizeof(struct sockaddr_in)) == -1)
    {
        perror("ERROR: bind()");
        return  ERROR_BIND;
    }

    struct sockaddr_in server = {.sin_family = AF_INET, .sin_addr.s_addr = broadcast_address, .sin_port = broadcast_port};
    socklen_t server_len = sizeof(server);

    static const char message[] = "Hi, POWER!";

    int data = sendto(client_socket, message, sizeof(message), 0, (struct sockaddr*)&server, sizeof(server));

    if (data == -1)
    {
        perror("ERROR: send()");
        return  ERROR_SEND;
    }

    char buffer[MAX_LEN] = "";

    static int delay = 10;
    sigset_t sig = {};
    sigemptyset(&sig);
    sigaddset(&sig, SIGALRM);
    struct sigaction sig_handler = {.__sigaction_handler = {sig_alrm_handler}, .sa_mask = sig};

    if (sigaction(SIGALRM, &sig_handler, NULL) == -1)
    {
        close(client_socket);

        perror("ERROR: sigaction()");
        return  ERROR_SIGACTION;
    }

    exit_loop = 0;
    alarm(delay);

    while (!exit_loop)
    {
        int read_size = recvfrom(client_socket, buffer, MAX_LEN - 1, 0, (struct sockaddr*)&server, &server_len);

        if (read_size == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
        {
            perror("ERROR: recvfrom()");
            close(client_socket);
            return  ERROR_RECVFROM;
        }

        buffer[read_size] = '\0';

        if (!errno && strcmp(buffer, "Hi, Makima!") == 0)
        {
            printf("The response was received from %s:%d \n", inet_ntoa(((struct sockaddr_in*)&server)->sin_addr), ntohs(((struct sockaddr_in*)&server)->sin_port));
        }
    }

    printf("Finished\n");
    return 0;
}