#include "daemon.h"

int fd_pid = 0;
int fd_fifo = 0;

int daemonize(char *name, char *path, char *in_file, char *out_file, char *err_file)
{
    if (!name)
        name = "POWER";
    if (!path)
        path = "/";
    if (!in_file)
        in_file = "/dev/null";
    if (!out_file)
        out_file = "/dev/null";
    if (!err_file)
        err_file = "/dev/null";

    pid_t pid = 0;

    if ((pid = fork()) == -1)
    {
        perror("ERROR: fork");
        exit(EXIT_FAILURE);
    }

    if (pid > 0)
        exit(EXIT_SUCCESS);

    if (setsid() == -1)
    {
        perror("ERROR: setsid");
        exit(EXIT_FAILURE);
    }
    
    sighandler_configuration();

    if ((pid = fork()) == -1)
    {
        perror("ERROR: fork");
        exit(EXIT_FAILURE);
    }
    if (pid > 0)
        exit(EXIT_SUCCESS);

    create_and_lock_pid_file(name);
    create_and_open_fifo(name);

    umask(0);
    if (chdir(path) == -1)
    {
        perror("ERROR: chdir");
        close(fd_pid);
        close(fd_fifo);
        exit(EXIT_FAILURE);
    }

    close_all_fd();
    open_in_out_err(in_file, out_file, err_file);

    openlog(name, LOG_PID, LOG_DAEMON);
    return 0;
}

void shutdown_demon(int num)
{
    close(fd_fifo);
    close(fd_pid);


    closelog();
    exit(EXIT_SUCCESS);
}

void sighandler_configuration()
{
    struct sigaction sa = {.sa_handler = SIG_IGN};

    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("ERROR: SIGCHLD sigaction");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGHUP, &sa, NULL) == -1)
    {
        perror("ERROR: SIGHUP sigaction");
        exit(EXIT_FAILURE);
    }
}

void sigmask_configuration(sigset_t *wait)
{
    sigemptyset(wait);
    sigaddset(wait, SIGUSR1);
    sigaddset(wait, SIGUSR2);
    sigaddset(wait, SIGALRM);
    sigaddset(wait, SIGTERM);
    sigaddset(wait, SIGQUIT);
    sigaddset(wait, SIGCHLD);
    sigaddset(wait, SIGHUP);
    sigaddset(wait, SIGINT);

    if (sigprocmask(SIG_BLOCK, wait, NULL) == -1)
    {
        perror("ERROR: sigprocmask");
        exit(EXIT_FAILURE);
    }
}

char *create_path(char *prefix, char *name, char *postfix)
{
    char *full_name = calloc(strlen(name) + strlen(prefix) + strlen(postfix) + 1, sizeof(char));
    strcpy(full_name, prefix);
    strcpy(full_name + strlen(prefix), name);
    strcpy(full_name + strlen(name) + strlen(prefix), postfix);

    return full_name;
}

void create_and_lock_pid_file(char *name)
{
    char *name_pid = create_path("/run/", name, ".pid");

    fd_pid = open(name_pid, O_RDWR | O_CREAT, 0666);

    free(name_pid);

    if (fd_pid == -1)
    {
        printf("Daemon with name \"%s\" already exists\n", name);
        exit(EXIT_FAILURE);
    }

    lock_pid_file(fd_pid, name);

    pid_t pid = getpid();

    if (write(fd_pid, &pid, 4) != 4)
    {
        perror("ERROR: write");
        close(fd_pid);
        exit(EXIT_FAILURE);
    }
}

void lock_pid_file(int fd_pid, char *name)
{
    struct flock lock = {.l_type = F_WRLCK, .l_whence = SEEK_SET, .l_start = 0, .l_len = 0};

    if (fcntl(fd_pid, F_SETLK, &lock) == -1)
    {
        if (errno == EAGAIN || errno == EACCES)
        {
            printf("Daemon with name \"%s\" already exists\n", name);
            exit(EXIT_SUCCESS);
        }
        else
        {
            perror("ERROR: fcntl");
            exit(EXIT_FAILURE);
        }
    }
}

void create_and_open_fifo(char *name)
{
    char *name_fifo = create_path("/tmp/", name, ".fifo");

    unlink(name_fifo);
    if (mkfifo(name_fifo, O_RDWR) == -1)
    {
        perror("ERROR: mkfifo");
        free(name_fifo);
        close(fd_pid);
        exit(EXIT_FAILURE);
    }

    if ((fd_fifo = open(name_fifo, O_RDONLY | O_NONBLOCK)) == -1)
    {
        perror("ERROR: open fifo");
        free(name_fifo);
        close(fd_pid);
        exit(EXIT_FAILURE);
    }

    free(name_fifo);
}

void close_all_fd()
{
    int fd = 0;

    for (fd = sysconf(_SC_OPEN_MAX); fd > 0; --fd)
    {
        if (fd != fd_pid && fd != fd_fifo)
            close(fd);
    }
}

void open_in_out_err(char *in_file, char *out_file, char *err_file)
{
    int in  = open(in_file,  O_RDONLY | O_CREAT,           00444);
    int out = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 00222);
    int err = open(err_file, O_WRONLY | O_CREAT | O_TRUNC, 00222);

    if (in == -1 || out == -1 || err == -1)
    {
        close(in);
        close(out);
        close(err);
        close(fd_pid);
        close(fd_fifo);
        exit(EXIT_FAILURE);
    }

    if (dup2(in, 0) == -1 || dup2(out, 1) == -1 || dup2(err, 2) == -1)
    {
        close(0);
        close(1);
        close(2);
        close(in);
        close(out);
        close(err);
        close(fd_pid);
        close(fd_fifo);
        exit(EXIT_FAILURE);
    }
}

int check_args(char* src, char* dst)
{
    DIR* dir = opendir(src);

    if (!dir)
    {   
        printf("Source directory can't be opened. Please try again!\n");
        return 0;
    }

    closedir(dir);

    return check_dst(dst);
}


int check_dst(char* dst)
{
    DIR* dir = opendir(dst);
                    
    if (!dir && errno == ENOENT)
    {
        if (mkdir(dst, 0666) == -1)
        {
            perror("ERROR: mkdir");
            return 0;
        }
    }
    else if (!dir)
    {
        printf("Destination directory can't be opened. Please try again!\n");
        return 0;
    }

    closedir(dir);

    return 1;
}