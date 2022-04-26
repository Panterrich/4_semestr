#include "ssh_server.h"

void sigchild_handler(int s)
{
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

int broadcast_server_udp_interface(in_addr_t address_server, in_port_t broadcast_port)
{
    int server_socket = broadcast_socket_configuration(address_server, broadcast_port);
    if (server_socket < 0) return server_socket;

    struct sockaddr_in client = {};
    socklen_t client_len = sizeof(client);
    
    char buffer[MAX_LEN] = "";

    while (1)
    {
        int len = recvfrom(server_socket, buffer, MAX_LEN - 1, 0, (struct sockaddr*)&client, &client_len);

        if (len == -1 && errno == ETIMEDOUT)
        {
            syslog(LOG_WARNING, "Broadcast socket timedout");
            continue;
        }
        else if (len == -1)
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
    }

    syslog(LOG_WARNING, "Broadcast socket close");
    
    close(server_socket);

    return 0;
}

int broadcast_socket_configuration(in_addr_t address, in_port_t port)
{
    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);

    if (server_socket == -1)
    {
        perror("ERROR: socket()");
        return  ERROR_SOCKET;
    }

    int reuse = 1;
    int broadcast = 1;
    struct timeval time = {.tv_sec = 10};

    if ((setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse,     sizeof(reuse))     == -1) ||
        (setsockopt(server_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1) ||
        (setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO,  &time,      sizeof(time))      == -1))
    {
        perror("ERROR: setsockopt()");
        close(server_socket);
        return  ERROR_SETSOCKOPT;
    }

    struct sockaddr_in server_addr = {.sin_family = AF_INET, .sin_addr.s_addr = address, .sin_port = port};

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("ERROR: bind()");
        close(server_socket);
        return  ERROR_BIND;
    }

    return server_socket;
}
