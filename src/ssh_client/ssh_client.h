
#ifndef SSH_CLIENT_H
#define SSH_CLIENT_H

#define _GNU_SOURCE
#define _LARGEFILE64_SOURCE
#define __USE_FILE_OFFSET64

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
#include <time.h>
#include <pthread.h>
#include <poll.h>

#define MAX_LEN 1000
#define MAX_SERVERS 100
#define MAX_BUFFER (2 << 20)

#define COPY_LISTEN_PORT 36000
#define BROADCAST_PORT   35000
#define TCP_LISTEN_PORT  34000
#define UDP_LISTEN_PORT  33000

#define DEFAULT_LOG      "/tmp/.ssh-log"
#define DEFAULT_HISTORY  "/tmp/.ssh-history"
#define PRIVATE_KEY_PATH "../powerssh.key"

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
    ERROR_ALLOCATE        = -13,
    ERROR_WRITE           = -14,
    ERROR_TIME            = -15,
    ERROR_TCSETATTR       = -16,
};

void help_message();

int print_ssh_history();

int ssh_client(in_addr_t address, char* username, int type_connection);

//=====================================================================================================

int broadcast_client_interface(in_addr_t address_client, in_port_t broadcast_port);

int broadcast_socket_configuration(in_addr_t address_client);

int set_timer_configuration();

size_t file_size(int fd);


//=====================================================================================================

int ssh_history_setup(in_port_t port);

int ssh_history_add_server(int file_history_server, size_t number_servers, struct in_addr addr);

int ssh_history_no_server(int file_history_server);

void ssh_history_end(int file_history_server);

//=====================================================================================================

int check_file(char* path_src);

int ssh_copy_file(in_addr_t address, char* username, char* path_src, char* path_dst);

#endif //! SSH_CLIENT_H