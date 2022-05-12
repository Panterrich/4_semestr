#ifndef RUDP_H
#define RUDP_H

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
#include <wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <stdint.h>
#include <netinet/in.h>
#include <time.h>

enum RUDP_ERRORS
{
    RUDP_UNDEFINED_TYPE      = -1,
    RUDP_SOCKET              = -2,
    RUDP_BIND                = -3,
    RUDP_CLOSE               = -4,
    RUDP_CONNECT             = -5,
    RUDP_ACCEPT              = -6,
    RUDP_RECV                = -7,
    RUDP_SEND                = -8,
    RUDP_CALLOC              = -9,
    RUDP_GETSOCKOPT          = -10,
    RUDP_SETSOCKOPT          = -11,
    RUDP_EXCEEDED_N_ATTEMPTS = -12,
    RUDP_FORK                = -13,
    RUDP_CLOSE_CONNECTION    = -14,
    RUDP_LISTEN              = -15,
};

enum RUDP_PACKET_FLAGS
{
    ACK = 1 << 1,
    SYN = 1 << 2,
    FIN = 1 << 3
};

struct rudp_header 
{
    u_int8_t flag;

    size_t sequence_number;
    size_t acknowledgement_number;
};

struct file_copy_header
{
    char username[MAX_INPUT];
    char path[NAME_MAX];
    size_t size;
    uintmax_t mode;
};

#define N_ATTEMPT     3
#define SIZE_QUEUE    100
#define ACK_TIME      5
#define STANTARD_TIME 1200
#define MAX_ANSWER    sizeof(struct rudp_header)

int rudp_socket(in_addr_t address, in_port_t port, struct timeval time_rcv, int type_connection);

int rudp_recv(int socket, void* message, size_t length, struct sockaddr_in* address, \
              int type_connection, struct rudp_header* control);

int rudp_send(int socket, const void* message, size_t length, const struct sockaddr_in* dest_addr, \
              int type_connection, struct rudp_header* control);

int rudp_accept(int socket, struct sockaddr_in* address, int type_connection, struct rudp_header* control);

int rudp_connect(int socket, const struct sockaddr_in* address, int type_connection, struct rudp_header* control, struct sockaddr_in* connected_address);

int rudp_close(int socket, int type_connection, const struct sockaddr_in* address, struct rudp_header* control, int step);

#endif // RUDP_H