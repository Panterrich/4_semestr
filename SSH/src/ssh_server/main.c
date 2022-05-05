#include "daemon.h"
#include "ssh_server.h"


int main(int argc, char *argv[])
{
    if (daemonize(NULL, NULL, NULL, NULL, NULL) != 0)
    {
        perror("ERROR: daemonize");
        exit(EXIT_FAILURE);
    }

    sigset_t sig = {};
    sigmask_configuration(&sig);

    int signum = 0;
    siginfo_t info = {};

    syslog(LOG_NOTICE, "daemon has been started");

    int pid_broadcast = fork();
    if (pid_broadcast == -1) return ERROR_FORK;
    if (pid_broadcast == 0)
    {   
        syslog(LOG_NOTICE, "broadcast has been started");
        int result = broadcast_server_udp_interface(INADDR_ANY, htons(BROADCAST_PORT));
        syslog(LOG_NOTICE, "broadcast has been stopped, error %d: %s", result, strerror(errno));
        return result;
    }

    int pid_tcp = fork();
    if (pid_tcp == -1) return ERROR_FORK;
    if (pid_tcp == 0)
    {   
        syslog(LOG_NOTICE, "tcp-ssh has been started");
        int result = ssh_server(INADDR_ANY, htons(TCP_LISTEN_PORT), SOCK_STREAM);
        syslog(LOG_NOTICE, "tcp-ssh has been stopped, error %d: %s", result, strerror(errno));
        return result;
    }

    int pid_udp = fork();
    if (pid_udp == -1) return ERROR_FORK;
    if (pid_udp == 0)
    {   
        syslog(LOG_NOTICE, "udp-ssh has been started");
        int result = ssh_server(INADDR_ANY, htons(UDP_LISTEN_PORT), SOCK_DGRAM);
        syslog(LOG_NOTICE, "udp-ssh has been stopped, error %d: %s", result, strerror(errno));
        return result;
    }

    while (1)
    {   
        signum = sigwaitinfo(&sig, &info);

        switch (signum)
        {
            case SIGQUIT:
            case SIGTERM:
            case SIGCHLD:
            {
                kill(pid_broadcast, SIGKILL);
                kill(pid_tcp,       SIGKILL);
                kill(pid_udp,       SIGKILL);
                syslog(LOG_NOTICE, "terminated");
                shutdown_demon(signum);
            }
            break;

            default:
                syslog(LOG_WARNING, "undefined signal");
                break;
        }
    }

    kill(pid_broadcast, SIGKILL);
    kill(pid_tcp,       SIGKILL);
    kill(pid_udp,       SIGKILL);
    syslog(LOG_NOTICE, "daemon has been finished");
    shutdown_demon(0);
}

