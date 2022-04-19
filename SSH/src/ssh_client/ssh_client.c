#include "ssh_client.h"

void help_message()
{
    printf("Usage: ./ssh_client [command] [options]...                                                                                                                  \n"
           "Commands:                                                                                                                                                   \n"
           "  [ -b | --broadcast ] [ <port> ]                     Find accessible SSH servers using broadcast by <port> and write <ip:port> to \"/tmp/.ssh_broadcast\". \n"
           "                                                      Default broadcast <port> is 35000.                                                                    \n"
           "  [ -h | --help ]                                     Dispay this information.                                                                              \n"
           "                                                                                                                                                            \n"
           "Options:                                                                                                                                                    \n"
           "  [ -j | --systemlog | -f <file> | --filelog <file> ] Logging to the system journal or to <file>.                                                           \n"
           "                                                      Default mode is system journaling                                                                     \n"
           "                                                      Default log <file> is \"/tmp/.ssh-log\"                                                               \n");
}

size_t file_size(int fd)
{
    struct stat info = {};

    if (fstat(fd, &info) == -1) return 0;

    return (size_t)info.st_size;
}

//=====================================================================================================

//!!!!!!!!!!!!!!!!
int exit_loop = 0;
//!!!!!!!!!!!!!!!!

void sig_alrm_handler(int signum)
{
    exit_loop = 1;
}

int broadcast_client_interface(in_addr_t address_client, in_port_t broadcast_port)
{
    int client_socket = broadcast_socket_configuration(address_client);
    if (client_socket < 0) return client_socket;

    static const char message[] = "Hi, POWER!";

    struct sockaddr_in server = {.sin_family = AF_INET, .sin_addr.s_addr = INADDR_BROADCAST, .sin_port = broadcast_port};
    socklen_t server_len = sizeof(server);

    int data = sendto(client_socket, message, sizeof(message), 0, (struct sockaddr*)&server, sizeof(server));
    if (data == -1)
    {
        perror("ERROR: send()");
        return  ERROR_SEND;
    }

    int timer = set_timer_configuration();
    if (timer < 0)
    {
        close(client_socket);
        return timer;
    }

    size_t number_servers = 0;
    printf("Search for available servers on port %d:\n", ntohs(server.sin_port));

    int file_history_servers = ssh_history_setup(broadcast_port);
    if (file_history_servers < 0)
    {
        close(client_socket);
        return file_history_servers;
    }

    exit_loop = 0;

    int delay = 10;
    alarm(delay);

    char buffer[MAX_LEN] = "";

    while (!exit_loop)
    {   
        errno = 0;
        int read_size = recvfrom(client_socket, buffer, MAX_LEN - 1, 0, (struct sockaddr*)&server, &server_len);

        if (read_size == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
        {
            perror("ERROR: recvfrom()");
            close(client_socket);
        }

        buffer[read_size] = '\0';

        if (!errno && strcmp(buffer, "Hi, Makima!") == 0)
        {
            printf("Server[%lu] %s\n", ++number_servers, inet_ntoa(server.sin_addr));
            
            int result = ssh_history_add_server(file_history_servers, number_servers, server.sin_addr);
            if (result < 0)
            {
                close(client_socket);
                return result;
            }
        }
    }

    if (number_servers == 0)
    {
        int result = ssh_history_no_server(file_history_servers);
        if (result < 0)
        {
            close(client_socket);
            return result;
        } 
    }

    close(client_socket);
    ssh_history_end(file_history_servers);
    return 0;
}

int broadcast_socket_configuration(in_addr_t address_client)
{
    int client_socket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);

    if (client_socket == -1)
    {
        perror("ERROR: socket()");
        return  ERROR_SOCKET;
    }

    int reuse = 1;
    int broadcast = 1;
    struct timeval timer = {.tv_sec = 15};

    if ((setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &reuse,     sizeof(int)) == -1) ||
        (setsockopt(client_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(int)) == -1) ||
        (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO,  &timer,     sizeof(timer)) == -1))
    {
        perror("ERROR: setsockopt()");
        return  ERROR_SETSOCKOPT;
    }

    struct sockaddr_in client = {.sin_family = AF_INET, .sin_addr.s_addr = address_client, .sin_port = 0};

    if (bind(client_socket, (struct sockaddr*)&client, sizeof(struct sockaddr_in)) == -1)
    {
        perror("ERROR: bind()");
        return  ERROR_BIND;
    }

    return client_socket;
}

int set_timer_configuration()
{
    sigset_t sig = {};
    sigemptyset(&sig);
    sigaddset(&sig, SIGALRM);

    struct sigaction handler = {.__sigaction_handler = {sig_alrm_handler}, .sa_mask = sig};

    if (sigaction(SIGALRM, &handler, NULL) == -1)
    {
        perror("ERROR: sigaction()");
        return  ERROR_SIGACTION;
    }

    return 0;
}

//=====================================================================================================

int ssh_history_setup(in_port_t port)
{
    int file_history_servers = open(DEFAULT_HISTORY, O_WRONLY | O_CREAT | O_LARGEFILE | O_TRUNC, 0666);

    if (file_history_servers == -1)
    {
        perror("ERROR: doesn't open history file");
        return  ERROR_OPEN;
    }

    time_t current_time = time(NULL);
    if (current_time == (time_t)-1)
    {
        close(file_history_servers);
        perror("ERROR: time():");
        return  ERROR_TIME;
    }
    
    struct tm* current_local_time = localtime(&current_time);

    char buffer[MAX_LEN] = "";
    snprintf(buffer, MAX_LEN - 1, "List servers on port %d (Date: %02d:%02d:%02d %02d/%02d/%04d):\n", ntohs(port), 
                                                            current_local_time->tm_hour, current_local_time->tm_min, current_local_time->tm_sec,
                                                            current_local_time->tm_mday, current_local_time->tm_mon, current_local_time->tm_year + 1900);

    if (write(file_history_servers, buffer, strlen(buffer)) == -1)
    {
        perror("ERROR: write()");
        close(file_history_servers);
        return  ERROR_RECVFROM;
    }

    return file_history_servers;
}

int ssh_history_add_server(int file_history_server, size_t number_servers, struct in_addr addr)
{
    static char server_ip_adress[MAX_LEN] = "";
    snprintf(server_ip_adress, MAX_LEN - 1, "Server[%lu] %s\n", number_servers, inet_ntoa(addr));
            
    if (write(file_history_server, server_ip_adress, strlen(server_ip_adress)) == -1)
    {
        perror("ERROR: write()");
        close(file_history_server);
        return  ERROR_WRITE;
    }

    return 0;
}

int ssh_history_no_server(int file_history_server)
{
    static char buffer[MAX_LEN] = "";
    snprintf(buffer, MAX_LEN - 1, "Servers not found. Run SSH server and try again!\n");

    if (write(file_history_server, buffer, strlen(buffer)) == -1)
    {
        perror("ERROR: write()");
        close(file_history_server);
        return  ERROR_WRITE;
    }

    return 0;
}

void ssh_history_end(int file_history_server)
{
    close(file_history_server);
}
