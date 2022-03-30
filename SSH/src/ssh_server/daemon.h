#ifndef DAEMON_H
#define DAEMON_H

#define _LARGEFILE64_SOURCE

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
#include <sys/socket.h>
#include <netinet/in.h>
#include <wait.h>
#include <arpa/inet.h>

#define MAX_LEN 4096


//=========================================================================================

int daemonize(char* name, char* path, char* in_file, char* out_file, char* err_file);

void sighandler_configuration();

void sigmask_configuration(sigset_t* wait);

char* create_path(char* prefix, char* name, char* postfix);

void create_pid_file(char* name);

void create_and_lock_pid_file(char* name);

void lock_pid_file(int fd_pid, char* name);

void create_and_open_fifo(char* name);

void close_all_fd();

void open_in_out_err(char* in_file, char* out_file, char* err_file);

void shutdown_demon(int num);

int check_args(char* src, char* dst);

int check_dst(char* dst);

//=========================================================================================

#endif // DAEMON_H