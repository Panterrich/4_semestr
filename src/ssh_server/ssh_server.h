#ifndef SSH_SERVER_H
#define SSH_SERVER_H

#define _GNU_SOURCE

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
#include <wait.h>
#include <limits.h>
#include <time.h>
#include <wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sched.h>
#include <poll.h>

//============================================================================================

#define MAX_LEN 1000
#define MAX_BUFFER (2 << 20)

#define UDP_LISTEN_PORT  33000
#define TCP_LISTEN_PORT  34000
#define BROADCAST_PORT   35000
#define COPY_LISTEN_PORT 36000

#define PUBLIC_KEY_PATH "/usr/share/powerssh/powerssh_public.key"

//============================================================================================


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
    ERROR_FORK            = -13,
    ERROR_SET_ID          = -14,
    ERROR_OPEN            = -15,
    ERROR_LOGIN           = -16,
    ERROR_UNSHARE         = -17,
    ERROR_CGROUP          = -18,
};

void sigchild_handler(int s);

int broadcast_server_udp_interface(in_addr_t address_server, in_port_t broadcast_port);

int broadcast_socket_configuration(in_addr_t address, in_port_t port);

int ssh_server(in_addr_t address, in_port_t port, int type_connection);

int copy_server(in_addr_t address, in_port_t port);

int add_pid_power_cgroup(pid_t pid);

#endif // SSH_SERVER_H