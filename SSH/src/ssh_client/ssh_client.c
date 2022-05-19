#include "ssh_client.h"
#include "pty.h"
#include "rudp.h"
#include "security.h"

void help_message()
{
    printf("Usage: ./ssh_client [command] [options]...                                                                                                                        \n"
           "Commands:                                                                                                                                                         \n"
           "  [ -b | --broadcast ] [ <port> ]                           Find accessible SSH servers using broadcast by <port> and write <ip:port> to \"/tmp/.ssh_broadcast\". \n"
           "                                                            Default broadcast <port> is 35000.                                                                    \n"
           "  [ -h | --help ]                                           Dispay this information.                                                                              \n"
           "  [ -s | --ssh  ] <name>@<ip> [ -t | --tcp || -u | --udp ]  Connect to ssh server under <name> user by <ip> by using <tcp> or <udp> protocol.                     \n"
           "                                                            Default protocol is tcp                                                                               \n"
           "  [ -c | --copy ] </path_src> <name>@<ip>:</path_dst>       Copy the file from the local machine on </path_src> to the ssh-server on </path_dst>.                 \n"
           "Options:                                                                                                                                                          \n"
           "  [ -j | --systemlog | -f <file> | --filelog <file> ]       Logging to the system journal or to <file>.                                                           \n"
           "                                                            Default mode is system journaling                                                                     \n"
           "                                                            Default log <file> is \"/tmp/.ssh-log\"                                                               \n");
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

    struct sockaddr_in server = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_BROADCAST), .sin_port = broadcast_port};
    socklen_t server_len = sizeof(server);

    int data = sendto(client_socket, message, sizeof(message), 0, (struct sockaddr*)&server, sizeof(server));
    if (data == -1)
    {
        fprintf(stderr, "ERROR: send() %d: %s\n", errno, strerror(errno));
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

    struct sockaddr_in client = {.sin_family = AF_INET, .sin_addr.s_addr = address_client, .sin_port = 0};

    if (bind(client_socket, (struct sockaddr*)&client, sizeof(struct sockaddr_in)) == -1)
    {
        perror("ERROR: bind()");
        return  ERROR_BIND;
    }

    int reuse = 1;
    int broadcast = 1;
    struct timeval timer = {.tv_sec = 15};

    if ((setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &reuse,     sizeof(int))   == -1) ||
        (setsockopt(client_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(int))   == -1) ||
        (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO,  &timer,     sizeof(timer)) == -1))
    {
        perror("ERROR: setsockopt()");
        return  ERROR_SETSOCKOPT;
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

int print_ssh_history()
{
    int fd = open(DEFAULT_HISTORY, O_RDONLY | O_LARGEFILE);
    if (fd == -1)
    {
        printf("Server history not found. See \'ssh_client --help\'\n");
        return ERROR_OPEN;
    }

    size_t size = file_size(fd);
    if (!size) 
    {
        close(fd);
        return ERROR_OPEN;
    }

    char* buffer = (char*) calloc(size, sizeof(char));
    if (!buffer) 
    {
        close(fd);
        return ERROR_ALLOCATE;
    }

    if (read(fd, buffer, size) != size)
    {
        close(fd);
        free(buffer);
        perror("ERROR: read():");
        return  ERROR_READ;
    }

    close(fd);

    if (write(STDOUT_FILENO, buffer, size) == -1)
    {   
        free(buffer);
        perror("ERROR: write():");
        return  ERROR_WRITE;
    }

    free(buffer);
    return 0;
}

//=====================================================================================================

int ssh_client(in_addr_t address, char* username, int type_connection)
{
    if (type_connection != SOCK_STREAM && type_connection != SOCK_DGRAM)
    {
        fprintf(stderr, "ssh_client(): undefined type connection\n");
        return RUDP_UNDEFINED_TYPE;
    }

    struct rudp_header control = {};
    struct sockaddr_in server  = {.sin_family = AF_INET, .sin_addr.s_addr = address, 
                                  .sin_port = (type_connection == SOCK_STREAM) ? htons(TCP_LISTEN_PORT) : htons(UDP_LISTEN_PORT)};
    struct sockaddr_in connected_address = {};


    struct termios term = {};
    struct termios oldt = {};

    if (tcgetattr(STDIN_FILENO, &oldt) == -1)
    {
        perror("tcsetattr()");
        return ERROR_TCSETATTR;
    }

    cfmakeraw(&term);
    if (tcsetattr(STDIN_FILENO, TCSANOW, &term) == -1)
    {
        perror("tcsetattr()");
        return ERROR_TCSETATTR;
    }
    
    char     buff[MAX_BUFFER] = "";
    char enc_buff[MAX_BUFFER] = "";

    struct timeval time = {.tv_sec = STANTARD_TIME};   
    int socket = rudp_socket(INADDR_ANY, 0, time, type_connection);
    if (socket < 0) 
    {
        perror("rudp_socket()");
        return  RUDP_SOCKET;
    }

    int result = rudp_connect(socket, &server, type_connection, &control, &connected_address);
    if (result < 0) return RUDP_CONNECT;

    unsigned char secret[MAX_LEN] = {};
    int secret_size = security_get_secret(PRIVATE_KEY_PATH, secret, PRIVATE_SIDE, socket, type_connection, &connected_address, &control);
    if (secret_size < 0)
    {   
        if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) == -1)
        {
            perror("tcsetattr()");
            return ERROR_TCSETATTR;
        }
        return -1;
    }
    
    RC4_KEY key = {};
    RC4_set_key(&key, secret_size, secret);

    int username_len = strlen(username);
    char* enc_username = (char*) calloc(username_len, sizeof(char));
    if (!enc_username) return ERROR_ALLOCATE;

    RC4(&key, username_len, (unsigned char*) username, (unsigned char*) enc_username);

    result = rudp_send(socket, enc_username, username_len, &connected_address, type_connection, &control);
    if (result < 0) return RUDP_SEND;

    free(enc_username);

    struct pollfd master[2] = {{.fd = socket, .events = POLL_IN}, {.fd = STDIN_FILENO, .events = POLL_IN}};

    size_t n_write = 0;
    size_t n_read  = 0;
 
    while (1)
    {
        int event = poll(master, 2, 100);
        if (event > 0)
        {   
            if (master[0].revents == POLL_IN)
            {
                if (type_connection == SOCK_STREAM)
                {
                    n_read = rudp_recv(socket, enc_buff, sizeof(enc_buff), NULL, SOCK_STREAM, NULL);
                    if (n_read == -1)
                    {
                        perror("rudp_recv(SOCK_STREAM)");
                        return -1;
                    }

                    if (n_read == 0)
                    {
                        rudp_close(socket, SOCK_STREAM, NULL, NULL, 0);
                        break;
                    }
                }
                else
                {
                    n_read = rudp_recv(socket, enc_buff, sizeof(enc_buff), &connected_address, SOCK_DGRAM, &control);
                    if (n_read == -1)
                    {
                        perror("rudp_recv(SOCK_DGRAM");
                        return -1;
                    }

                    if (n_read == 0)
                    {   
                        if (type_connection == SOCK_DGRAM)
                        {
                            rudp_close(socket, SOCK_DGRAM, &connected_address, &control, 2);
                            rudp_close(socket, SOCK_DGRAM, &connected_address, &control, 3);
                        }

                        else if (type_connection == SOCK_STREAM)
                        {
                            rudp_close(socket, SOCK_STREAM, &connected_address, &control, 0);
                        }
                        break;
                    }
                }

                RC4(&key, n_read, (unsigned char*) enc_buff, (unsigned char*) buff);

                n_write = write(STDOUT_FILENO, buff, n_read);
                if (n_write == -1)
                {
                    perror("write()");
                    return -1;
                }

            }
            
            if (master[1].revents == POLL_IN)
            {
                n_read = read(STDIN_FILENO, buff, sizeof(buff) - 1);
                if (n_read == -1)
                {
                    perror("read()");
                    return -1;
                }

                if (*buff == 4) // ctrl + D
                {   
                    if (type_connection == SOCK_DGRAM)
                    {
                        rudp_close(socket, SOCK_DGRAM, &connected_address, &control, 1);
                        rudp_recv(socket, buff, sizeof(buff), &connected_address, SOCK_DGRAM, &control);
                        rudp_close(socket, SOCK_DGRAM, &connected_address, &control, 3);
                    }

                    else if (type_connection == SOCK_STREAM)
                    {
                        rudp_close(socket, SOCK_STREAM, &connected_address, &control, 0);
                    }
                    
                    break;
                }
                
                RC4(&key, n_read, (unsigned char*) buff, (unsigned char*) enc_buff);

                if (type_connection == SOCK_STREAM)
                {
                    n_write = rudp_send(socket, enc_buff, n_read, NULL, SOCK_STREAM, NULL);
                    if (n_write == -1)
                    {
                        perror("rudp_send(SOCK_STREAM)");
                        return -1;
                    }
                }
                else
                {
                    n_write = rudp_send(socket, enc_buff, n_read, &connected_address, SOCK_DGRAM, &control);
                    if (n_write == -1)
                    {
                        perror("rudp_send(SOCK_DGRAM");
                        return -1;
                    }
                }
            }
        }
        else if (event == 0) 
        {
            continue;
        }
        else 
        {
            break;
        }
    }

    if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) == -1)
    {
        perror("tcsetattr()");
        return ERROR_TCSETATTR;
    }

    return 0;
}

int ssh_copy_file(in_addr_t address, char* username, char* path_src, char* path_dst)
{
    struct sockaddr_in server  = {.sin_family = AF_INET, .sin_addr.s_addr = address, .sin_port = htons(COPY_LISTEN_PORT)};

    struct termios term = {};
    struct termios oldt = {};

    if (tcgetattr(STDIN_FILENO, &oldt) == -1)
    {
        perror("tcsetattr()");
        return ERROR_TCSETATTR;
    }

    cfmakeraw(&term);
    if (tcsetattr(STDIN_FILENO, TCSANOW, &term) == -1)
    {
        perror("tcsetattr()");
        return ERROR_TCSETATTR;
    } 

    struct stat info_src = {};

    int result = stat(path_src, &info_src);
    if (result == -1 && errno == ENOENT)
    {
        fprintf(stderr, "File \"%s\" not found!\n", path_src);
        return 0;
    }
    if (result == -1)
    {
        perror("ERROR: stat file ");
        return 0;
    }

    int fd = open(path_src, O_RDONLY);
    if (fd == -1) return ERROR_OPEN;

    struct file_copy_header header = {};
    strncpy(header.username, username, sizeof(header.username));
    strncpy(header.path,     path_dst, sizeof(header.path));
    header.mode = info_src.st_mode;
    header.size = info_src.st_size;

    char buff[MAX_BUFFER] = "";

    struct timeval time = {.tv_sec = STANTARD_TIME};   
    int socket = rudp_socket(INADDR_ANY, 0, time, SOCK_STREAM);
    if (socket < 0) 
    {
        perror("rudp_socket()");
        return  RUDP_SOCKET;
    }

    result = rudp_connect(socket, &server, SOCK_STREAM, NULL, NULL);
    if (result < 0) return RUDP_CONNECT;

    result = rudp_send(socket, &header, sizeof(header), NULL, SOCK_STREAM, NULL);
    if (result < 0) return RUDP_SEND;

    struct pollfd master[2] = {{.fd = socket, .events = POLL_IN}, {.fd = STDIN_FILENO, .events = POLL_IN}};

    size_t n_write = 0;
    size_t n_read  = 0;
 
    while (1)
    {
        int event = poll(master, 2, 100);
        if (event > 0)
        {   
            if (master[0].revents == POLL_IN)
            { 
                n_read = rudp_recv(socket, buff, sizeof(buff), NULL, SOCK_STREAM, NULL);
                if (n_read == -1)
                {
                    perror("rudp_recv(SOCK_STREAM)");
                    return -1;
                }

                if (n_read == sizeof(struct file_copy_header) && !memcmp(buff, &header, sizeof(struct file_copy_header))) break;

                if (n_read == 0)
                {
                    rudp_close(socket, SOCK_STREAM, NULL, NULL, 0);
                    break;
                }

                n_write = write(STDOUT_FILENO, buff, n_read);
                if (n_write == -1)
                {
                    perror("write()");
                    return -1;
                }

            }
            
            if (master[1].revents == POLL_IN)
            {
                n_read = read(STDIN_FILENO, buff, sizeof(buff) - 1);
                if (n_read == -1)
                {
                    perror("read()");
                    return -1;
                }

                if (*buff == 4) // ctrl + D
                {   
                    rudp_close(socket, SOCK_STREAM, NULL, NULL, 0);
                    break;
                }
                
                n_write = rudp_send(socket, buff, n_read, NULL, SOCK_STREAM, NULL);
                if (n_write == -1)
                {
                    perror("rudp_send(SOCK_STREAM)");
                    return -1;
                }
            }
        }
        else if (event == 0) 
        {
            continue;
        }
        else 
        {
            break;
        }
    }

    size_t size       = 0;
    size_t size_write = 0;
    char packet[MAX_LEN] = "";

    while (1)
    {
        n_read = read(fd, packet, MAX_LEN - 1);
        if (n_read == -1)
        {
            perror("read()");
            return -1;
        }
        
        n_write = rudp_send(socket, packet, n_read, NULL, SOCK_STREAM, NULL);
        if (n_write == -1)
        {
            perror("rudp_send(SOCK_STREAM)");
            return -1;
        }

        size       += n_read;
        size_write += n_write;
        if (size != size_write)
        {
            if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) == -1)
            {
                perror("tcsetattr()");
                return ERROR_TCSETATTR;
            }
            fprintf(stderr, "Error has occurred, please try again!\n");
            return -1;
        }

        if (size_write == header.size) break;
    }

    if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) == -1)
    {
        perror("tcsetattr()");
        return ERROR_TCSETATTR;
    }
    
    return 0;
}

int check_file(char* path_src)
{
    struct stat info_src = {};

    int result = stat(path_src, &info_src);
    if (result == -1 && errno == ENOENT)
    {
        fprintf(stderr, "File \"%s\" not found!\n", path_src);
        return 0;
    }
    if (result == -1)
    {
        perror("ERROR: stat file ");
        return 0;
    }

    if (S_ISFIFO(info_src.st_mode) || S_ISREG(info_src.st_mode) || S_ISLNK(info_src.st_mode))
    {   
        return 1;
    }

    fprintf(stderr, "File \"%s\" is not link, regular or fifo file!\n", path_src);
    return 0;
}
