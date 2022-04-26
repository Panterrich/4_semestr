#include "pty.h"

#define MAX_LEN 1000

int pty_master_open(char* slave_name, size_t slave_name_len)
{
    int master_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (master_fd == -1) 
    {
        perror("ERROR: posix_openpt()");
        return  ERROR_POSIX_OPENPT;
    }
    
    if (grantpt(master_fd) == -1)
    {
        perror("ERROR: grantpt()");
        close(master_fd);
        return  ERROR_GRANTPT;       
    }

    if (unlockpt(master_fd) == -1)
    {
        perror("ERROR: unlockpt()");
        close(master_fd);
        return  ERROR_UNLOCKPT;
    }

    char* pathname = ptsname(master_fd);
    if (pathname == NULL)
    {
        perror("ERROR: ptsname()");
        close(master_fd);
        return  ERROR_PTSNAME;
    }

    if (strlen(pathname) < slave_name_len)
    {
        strncpy(slave_name, pathname, slave_name_len);
    }
    else
    {  
        close(master_fd);
        errno = EOVERFLOW;
        perror("ERROR: overflow slave name buffer");
        return  ERROR_OVERFLOW;
    }

    return master_fd;
}

pid_t pty_fork(int* master_fd, char* slave_name, size_t slave_name_len,                \
               const struct termios* slave_termios, const struct winsize* slave_winsize)
{
    char slave_name_buffer[MAX_LEN] = "";

    int mfd = pty_master_open(slave_name_buffer, MAX_LEN);
    if (mfd < 0) return mfd;

    if (slave_name != NULL)
    {
        if (strlen(slave_name_buffer) < slave_name_len)
        {
            strncpy(slave_name, slave_name_buffer, slave_name_len);
        }
        else
        {
            close(mfd);
            errno = EOVERFLOW;
            perror("ERROR: overflow slave name buffer");
            return ERROR_OVERFLOW;
        }
    }

    pid_t pid = fork(); 

    if (pid == -1)
    {
        perror("ERROR: fork()");
        close(mfd);
        return  ERROR_FORK;      
    }

    if (pid) // parent
    {
        *master_fd = mfd;
        return pid;
    }   

    // child
    if (setsid() == -1)
    {
        perror("ERROR: pty_fork:setsid()");
        exit(EXIT_FAILURE);
    }

    close(mfd);

    int slave_fd = open(slave_name_buffer, O_RDWR);
    if (slave_fd == -1)
    {
        perror("ERROR: pty_fork:open()");
        exit(EXIT_FAILURE);
    }

#ifdef TIOCSCTTY

    if (ioctl(slave_fd, TIOCSCTTY, 0) == -1)
    {
        perror("ERROR: pty_fork:ioctl(TIOCSCTTY)");
        exit(EXIT_FAILURE);
    }

#endif // TIOCSTTY

    if (slave_termios != NULL)
    {
        if (tcsetattr(slave_fd, TCSANOW, slave_termios) == -1)
        {
            perror("ERROR: pty_fork:tcsetattr()");
            exit(EXIT_FAILURE);
        }
    }

    if (slave_winsize != NULL)
    {
        if (ioctl(slave_fd, TIOCSWINSZ, slave_winsize) == -1)
        {
            perror("ERROR: pty_fork:icontl(TIOCSWINSZ)");
            exit(EXIT_FAILURE);
        }
    }

    if (dup2(slave_fd, STDIN_FILENO) != STDIN_FILENO)
    {
        perror("ERROR: pty_fork:dup2 - STDIN_FILENO");
        exit(EXIT_FAILURE);
    }
    if (dup2(slave_fd, STDOUT_FILENO) != STDOUT_FILENO)
    {
        perror("ERROR: pty_fork:dup2 - STDOUT_FILENO");
        exit(EXIT_FAILURE);
    }
    if (dup2(slave_fd, STDERR_FILENO) != STDERR_FILENO)
    {
        perror("ERROR: pty_fork:dup2 - STDERR_FILENO");
        exit(EXIT_FAILURE);
    }

    if (slave_fd > STDERR_FILENO)
    {
        close(slave_fd);
    }

    return 0; // child
}   
