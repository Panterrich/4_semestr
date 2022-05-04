#include "controller.h"

pid_t get_daemon_pid()
{
    int fd = open("/run/POWER.pid", O_RDONLY);

    if (fd == -1)
    {
        perror("ERROR: open_read_only pid");
        return  ERROR_OPEN_PID;
    }

    pid_t pid = 0;
    int result = read(fd, &pid, sizeof(pid_t));
    if (result == -1)
    {
        perror("ERROR: read pid");
        return  ERROR_READ_PID;
    }
    if (result != sizeof(pid_t))
    {
        perror("ERROR: a non-pid was read");
        return  ERROR_READ_PID;
    }

    close(fd);

    return pid;
}

int check_pid(pid_t pid)
{
    union sigval value = {-1};

    if (sigqueue(pid, SIGUSR1, value) == -1)
    {
        printf("Daemon POWER[%d] doesn't exist\n\n", pid);
        return 0;
    }

    return 1;
}

void menu(pid_t pid)
{
    screen_clear();

    while (1)
    {
        printf("# Daemon POWER[%d] controller\n"
           " (c) Panterrich, 2022\n\n"
           "Select\n"
           "[" RED(1) "] KILL\n"
           "[" RED(2) "] Exit \n\n\n", pid);
    
        if (processing_mode(pid)) return;
    }
}

int getkey()
{
    struct termios oldt = {};
    struct termios newt = {};

    int command = 0;

    tcgetattr(STDIN_FILENO, &oldt);

    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);

    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    command = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    return command;
}

void screen_clear()
{
    printf("\e[1;1H\e[2J");
}

int processing_mode(pid_t pid)
{
    int mode = 0;

    for (mode = getkey(); (mode != KEY_1) &&
                          (mode != KEY_2);
                                            mode = getkey());

    switch (mode)
    {
        case KEY_1:
            kill(pid, SIGTERM);
            return 1;

        case KEY_2: 
            return 1;

        default:
            printf("Unknown mode %d\n", mode);
            break;
    }

    return 1;
}

