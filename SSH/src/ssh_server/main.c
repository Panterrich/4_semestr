#include "daemon.h"

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
};

void sigchild_handler(int s)
{
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

int broadcast_server_udp_interface(in_addr_t address, in_port_t port);

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Please, run the program in the following format: \n"
               "./server <host> <port>\n");
        return ERROR_INVALID_ARGC;
    }

    struct sockaddr_in address = {};

    if (inet_ntop(AF_INET, &address.sin_addr, argv[1], strlen(argv[1])) == NULL)
    {
        printf("Please, run the program in the following format: \n"
               "./server <host> <port>\n");
        return ERROR_INVALID_ADDRESS;
    }

    address.sin_port = htons(atoi(argv[2]));

    if (daemonize(NULL, NULL, NULL, NULL, NULL) != 0)
    {
        perror("ERROR: daemonize");
        exit(EXIT_FAILURE);
    }

    sigset_t sig = {};
    sigmask_configuration(&sig);

    sigdelset(&sig, SIGCHLD);

    int signum = 0;
    siginfo_t info = {};

    int run = 1;

    while (1)
    {
        int result = broadcast_server_udp_interface(address.sin_addr.s_addr, address.sin_port);

        if (result) 
        {
             syslog(LOG_NOTICE, "daemon POWER[%d] has been stopped, error broadcast", getpid());
            return result;
        }

        // signum = sigwaitinfo(&sig, &info);

        // switch (signum)
        // {
        //     case SIGUSR1:
        //     {
        //         if (info.si_int == -1)
        //         {
        //             info.si_int = 0;
        //             syslog(LOG_NOTICE, "controller check this demon");
        //             break;
        //         }
        //     }
        //     break;

        //     case SIGUSR2:
        //     {
        //         syslog(LOG_NOTICE, "controller send SIGUSR2");
        //     }
        //     break;

        //     case SIGHUP:
        //     {
        //         syslog(LOG_NOTICE, "controller send SIGHUP");
        //     }
        //     break;

        //     case SIGQUIT:
        //     case SIGTERM:
        //     {
        //         syslog(LOG_NOTICE, "terminated");
        //         shutdown_demon(signum);
        //     }
        //     break;

        //     case SIGINT:
        //     {
        //         if (run)
        //         {
        //             syslog(LOG_NOTICE, "daemon POWER[%d] has been stopped", getpid());
        //         }
        //         else
        //         {
        //             syslog(LOG_NOTICE, "daemon POWER[%d] has been started", getpid());
        //         }

        //         run = !run;
        //     }
        //     break;

        //     case SIGCHLD:
        //     break;

        //     default:
        //         syslog(LOG_WARNING, "undefined signal");
        //         break;
        // }
    }

    
    syslog(LOG_NOTICE, "finished");
    closelog();

    return 0;
}


int broadcast_server_udp_interface(in_addr_t address, in_port_t port)
{
    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);

    if (server_socket == -1)
    {
        perror("ERROR: socket()");
        return  ERROR_SOCKET;
    }

    int reuse = 1;
    int broadcast = 1;
    struct timeval time = {.tv_sec = 120};

    if ((setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse,     sizeof(reuse))     == -1) ||
        (setsockopt(server_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)  == -1) ||
        (setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO,  &time,      sizeof(time))      == -1)))
    {
        perror("ERROR: setsockopt()");
        return  ERROR_SETSOCKOPT;
    }

    struct sockaddr_in server_addr = {.sin_family = AF_INET, .sin_addr.s_addr = address, .sin_port = port};

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("ERROR: bind()");
        return  ERROR_BIND;
    }

    struct sockaddr_in client = {};
    socklen_t client_len = sizeof(client);
    
    char buffer[MAX_LEN] = "";
    int len = recvfrom(server_socket, buffer, MAX_LEN, 0, (struct sockaddr*)&client, &client_len);

    if (len == -1)
    {
        perror("ERROR: recvfrom()");
        return  ERROR_RECVFROM;
    }

    buffer[len] = '\0';

    syslog(LOG_NOTICE, "Broadcast message received from %s:%d :: %s\n", inet_ntoa(((struct sockaddr_in*)&client)->sin_addr), ntohs(((struct sockaddr_in*)&client)->sin_port), buffer);

    if (strcmp(buffer, "Hi, POWER!") == 0)
    {
        static const char message[] = "Hi, Makima!";

        int data = sendto(server_socket, message, sizeof(message), 0, (struct sockaddr*)&client, client_len);

        if (data == -1)
        {
            perror("ERROR: send()");
            return  ERROR_SEND;
        }
    }
    
    close(server_socket);

    return 0;
}
