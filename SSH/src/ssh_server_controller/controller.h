#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdio.h>    
#include <stdlib.h>  
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <syslog.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <dirent.h>

#include "Colors.h"

enum ERRORS
{
    ERROR_OPEN_PID        = -1,
    ERROR_READ_PID        = -2,
};

enum KEYS
{
    KEY_1 = 49,   
    KEY_2 = 50,
    KEY_3 = 51,   
    KEY_4 = 52,
    KEY_5 = 53,
    KEY_6 = 54,
    KEY_7 = 55,
};

//=========================================================================
pid_t get_daemon_pid();

int check_pid(pid_t pid);

void menu(pid_t pid);

int getkey();

void screen_clear();

int processing_mode(pid_t pid);

int Mode_start_stop(pid_t pid);

int Mode_change_src_dst(pid_t pid);

int Mode_change_interval(pid_t pid);

int Mode_change_backup_mode(pid_t pid);

#endif // CONTROLLER_H