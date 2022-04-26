#ifndef PTY_H
#define PTY_H

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

enum ERRORS
{
    ERROR_POSIX_OPENPT = -1,
    ERROR_GRANTPT      = -2,
    ERROR_UNLOCKPT     = -3,    
    ERROR_PTSNAME      = -4,
    ERROR_OVERFLOW     = -5,
    ERROR_FORK         = -6
};

int pty_master_open(char* slave_name, size_t slave_name_len);

pid_t pty_fork(int* master_fd, char* slave_name, size_t slave_name_len,                     \
               const struct termios* slave_termios, const struct winsize* slave_winsize);

#endif // PTY_H