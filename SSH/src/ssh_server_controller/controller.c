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
           "[" RED(1) "] Start/Stop daemon \"POWER\"\n"
           "[" RED(2) "] Change source/destination\n"
           "[" RED(3) "] Change interval\n"
           "[" RED(4) "] Change mod - classic or inotify\n"
           "[" RED(5) "] KILL\n"
           "[" RED(6) "] Exit \n\n\n", pid);
    
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
                          (mode != KEY_2) &&
                          (mode != KEY_3) &&
                          (mode != KEY_4) &&
                          (mode != KEY_5) &&
                          (mode != KEY_6);
                                            mode = getkey());

    switch (mode)
    {
        case KEY_1:
            return Mode_start_stop(pid);
            break;

        case KEY_2:
            return Mode_change_src_dst(pid);
            break;

        case KEY_3:
            return Mode_change_interval(pid);
            break;

        case KEY_4:
            return Mode_change_backup_mode(pid);
            break;

        case KEY_5:
            kill(pid, SIGTERM);
            return 1;

        case KEY_6: 
            return 1;

        default:
            printf("Unknown mode %d\n", mode);
            break;
    }

    return 1;
}

int Mode_start_stop(pid_t pid)
{
    screen_clear();

    int result = kill(pid, SIGINT);

    if (result == -1)
    {   
        perror("kill: ");
        printf("Daemon \"POWER\" doesn't exist.\n");
        return 1;
    }
    
    printf("The condition of the daemon has been changed\n\n\n");
    return 0;
}

int Mode_change_src_dst(pid_t pid)
{
    screen_clear();
    
    union sigval value = {0};
 
    if (sigqueue(pid, SIGUSR2, value) == -1)
    {
        printf("Daemon \"POWER\" doesn't exist.\n");
        return 1;
    }

    printf("A new directory has been set\n\n\n");
    return 0;
}

int Mode_change_interval(pid_t pid)
{
    screen_clear();

    union sigval value = {};

    printf("Please, enter a new interval: ");
    
    if (scanf("%d", &value.sival_int) != 1)
    {
        printf("New interval was not recognized\n");
        return 1;
    }

    int result = sigqueue(pid, SIGUSR1, value);
    if (result == -1)
    {
        printf("Daemon \"POWER\" doesn't exist.\n");
        return 1;
    }

    printf("A new interval has been set\n\n\n");
    return 0;
}

int Mode_change_backup_mode(pid_t pid)
{
    screen_clear();

    union sigval value = {};
    
    printf("Please, select a new mode: \n"
           "[" RED(1) "] Classic\n"
           "[" RED(2) "] Inotify\n");

    int mode = 0;

    for (mode = getkey(); (mode != KEY_1) &&
                          (mode != KEY_2);
                                            mode = getkey());
    
    if (mode != KEY_1 && mode != KEY_2)
    {
        screen_clear();
        printf("Undefined mode\n");
        return 1;
    }

    screen_clear();

    value.sival_int = mode - KEY_1;

    int result = sigqueue(pid, SIGHUP, value);
    if (result == -1)
    {
        printf("Daemon \"POWER\" doesn't exist.\n");
        return 1;
    }

    printf("A new mode has been set\n\n\n");
    return 0;
}
