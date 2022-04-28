#include "daemon.h"
#include "ssh_server.h"



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
    int result = 0;

    while (1)
    {   
        broadcast_server_udp_interface(address.sin_addr.s_addr, address.sin_port);

        if (result) 
        {
            syslog(LOG_NOTICE, "daemon POWER[%d] has been stopped, error broadcast %d", getpid(), result);
            return result;
        }

        signum = sigwaitinfo(&sig, &info);

        switch (signum)
        {
            case SIGUSR1:
            {
                if (info.si_int == -1)
                {
                    info.si_int = 0;
                    syslog(LOG_NOTICE, "controller check this demon");
                    break;
                }
            }
            break;

            case SIGUSR2:
            {
                syslog(LOG_NOTICE, "controller send SIGUSR2");
            }
            break;

            case SIGHUP:
            {
                syslog(LOG_NOTICE, "controller send SIGHUP");
            }
            break;

            case SIGQUIT:
            case SIGTERM:
            {
                syslog(LOG_NOTICE, "terminated");
                shutdown_demon(signum);
            }
            break;

            case SIGINT:
            {
                if (run)
                {
                    syslog(LOG_NOTICE, "daemon POWER[%d] has been stopped", getpid());
                }
                else
                {
                    syslog(LOG_NOTICE, "daemon POWER[%d] has been started", getpid());
                }

                run = !run;
            }
            break;

            case SIGCHLD:
            break;

            default:
                syslog(LOG_WARNING, "undefined signal");
                break;
        }
    }

    syslog(LOG_NOTICE, "daemon POWER[%d] started", getpid());
    ssh_server(INADDR_ANY, htons(TCP_LISTEN_PORT), SOCK_STREAM);
    syslog(LOG_NOTICE, "finished");
    closelog();

    return 0;
}

