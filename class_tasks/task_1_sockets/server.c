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
};

void sigchild_handler(int s)
{
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

int server_socket = 0;

int server_tcp_interface(in_addr_t address, in_port_t port, int size_queue);

int server_udp_interface(in_addr_t address, in_port_t port);

int main(int argc, char* argv[])
{
       if (argc != 4)
    {
        printf("Please, run the program in the following format: \n"
               "./server [-t | -u] <host> <port>\n");
        return ERROR_INVALID_ARGC;
    }

    if (strcmp(argv[1], "-t") != 0 && strcmp(argv[1], "-u") != 0)
    {
        printf("Please, run the program in the following format: \n"
               "./server [-t | -u] <host> <port>\n");
        return ERROR_INVALID_ARGV;
    }
    struct sockaddr_in address = {};

    if (inet_ntop(AF_INET, &address.sin_addr, argv[2], strlen(argv[2])) == NULL)
    {
        printf("Please, run the program in the following format: \n"
               "./server [-t | -u] <host> <port>\n");
        return ERROR_INVALID_ADDRESS;
    }

    address.sin_port = htons(atoi(argv[3]));

    if (strcmp(argv[1], "-t") == 0)
    {
        server_tcp_interface(address.sin_addr.s_addr, address.sin_port, 10);
    }

    if (strcmp(argv[1], "-u") == 0)
    {
        server_udp_interface(address.sin_addr.s_addr, address.sin_port);
    }

    return 0;
}

int server_tcp_interface(in_addr_t address, in_port_t port, int size_queue)
{
    if (size_queue < 0) return 0;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket == -1)
    {
        perror("ERROR: socket()");
        return  ERROR_SOCKET;
    }

    int reuse = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1)
    {
        perror("ERROR: setsockopt()");
        return  ERROR_SETSOCKOPT;
    }

    struct sockaddr_in server = {.sin_family = AF_INET, .sin_addr.s_addr = address, .sin_port = port};

    if (bind(server_socket, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == -1)
    {
        perror("ERROR: bind()");
        return  ERROR_BIND;
    }

    if (listen(server_socket, size_queue) == -1)
    {
        perror("ERROR: listen()");
        return  ERROR_LISTEN;
    }

    struct sigaction sa = {.sa_handler = sigchild_handler, .sa_flags = SA_RESTART};
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("ERROR: sigaction()");
        return  ERROR_SIGACTION;
    }

    struct sockaddr_storage client_address = {};
    socklen_t client_address_len = sizeof(client_address);

    while (1)
    {
        int client_fd = accept(server_socket, (struct sockaddr*)&client_address, &client_address_len);
        if (client_fd == -1)
        {
            perror("WARNING: accept()");
            continue;
        }

        if (!fork()) // CHILD
        {
            close(server_socket);
            
            char buffer[MAX_LEN] = "";
            int read_size = 0;

            while ((read_size = recv(client_fd, buffer, MAX_LEN - 1, 0)) != -1)
            {
                if (read_size == MAX_LEN - 1) 
                {
                    fprintf(stderr, "WARNING: large message\n");
                }
                
                if (read_size == 0) break;

                buffer[read_size] = '\0';
                
                printf("%s:%u :: %s\n", inet_ntoa(((struct sockaddr_in*)&client_address)->sin_addr), ntohs(((struct sockaddr_in*)&client_address)->sin_port), buffer);
                memset(buffer, 0, MAX_LEN);
            }

            close(client_fd);
            return 0;
        }

        close(client_fd);
    }

    return 0;
}

int server_udp_interface(in_addr_t address, in_port_t port)
{
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);

    if (server_socket == -1)
    {
        perror("ERROR: socket()");
        return  ERROR_SOCKET;
    }

    int reuse = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1)
    {
        perror("ERROR: setsockopt()");
        return  ERROR_SETSOCKOPT;
    }

    struct sockaddr_in server = {.sin_family = AF_INET, .sin_addr.s_addr = address, .sin_port = port};

    if (bind(server_socket, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == -1)
    {
        perror("ERROR: bind()");
        return  ERROR_BIND;
    }

    char buffer[MAX_LEN] = "";
    struct sockaddr client_address = {};
    socklen_t client_address_len = sizeof(client_address);

    while (1)
    {
        int read_size = recvfrom(server_socket, buffer, MAX_LEN - 1, 0, &client_address, &client_address_len);

        if (read_size == -1)
        {
            perror("ERROR: recvfrom()");
            return  ERROR_RECVFROM;
        }

        buffer[read_size] = '\0';
                
        printf("%s:%u :: ", inet_ntoa(((struct sockaddr_in*)&client_address)->sin_addr), ntohs(((struct sockaddr_in*)&client_address)->sin_port));
        fflush(stdout);
        if (write(STDOUT_FILENO, buffer, read_size) == -1)
        {
            perror("ERROR: write()");
            return  ERROR_WRITE;
        }
        printf("\n");

        memset(buffer, 0, MAX_LEN);
    }
}

