#include "ssh_server.h"
#include "pty.h"
#include "rudp.h"
#include "pam.h"
#include "security.h"

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

    syslog(LOG_INFO, "broadcast is recieving");

    while (1)
    {
        int len = recvfrom(server_socket, buffer, MAX_LEN - 1, 0, (struct sockaddr*)&client, &client_len);

        if (len == -1 && errno == EAGAIN)
        {
            continue;
        }
        else if (len == -1)
        {
            syslog(LOG_ERR, "ERROR: recvfrom(): %s", strerror(errno));
            return  ERROR_RECVFROM;
        }

        buffer[len] = '\0';

        syslog(LOG_NOTICE, "broadcast message received from %s:%d :: %s\n", inet_ntoa(((struct sockaddr_in*)&client)->sin_addr), ntohs(((struct sockaddr_in*)&client)->sin_port), buffer);

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

    syslog(LOG_WARNING, "broadcast socket close");
    
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
    struct timeval time = {.tv_sec = 1200};

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

//======================================================================================================================================

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
int exit_master_read_loop = 0;
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

void sigchld(int s)
{
    syslog(LOG_INFO, "ssh SIGCHLD errno = %d", errno);
    exit_master_read_loop = 1;
}

int ssh_server(in_addr_t address, in_port_t port, int type_connection)
{
    if (type_connection == SOCK_STREAM)
    {
        struct timeval time = {.tv_sec = STANTARD_TIME};
        int socket = rudp_socket(address, port, time, SOCK_STREAM);
        if (socket < 0) return RUDP_SOCKET;

        syslog(LOG_NOTICE, "daemon POWER[%d] rudp_socket created port %d", getpid(), ntohs(port));

        int result = listen(socket, SIZE_QUEUE);
        if (result == -1) return RUDP_LISTEN;

        syslog(LOG_NOTICE, "daemon POWER[%d] rudp_socket listened, errno = %d: %s", getpid(), errno, strerror(errno));


        struct sockaddr_in client = {};

        while (1)
        {
            int accepted_socket = rudp_accept(socket, &client, SOCK_STREAM, NULL);
            syslog(LOG_NOTICE, "daemon POWER[%d] %d", getpid(), accepted_socket);
            if (accepted_socket < 0) return RUDP_ACCEPT;

            if (accepted_socket > 0)
            {   
                syslog(LOG_ERR, "[RUDP] after accept pid = %d, errno = %d", getpid(), errno);
                syslog(LOG_NOTICE, "daemon POWER[%d] accepted", getpid());
                char buff[MAX_BUFFER] = "";
                char slave[MAX_LEN] = "";

                sigset_t mask = {};
                sigemptyset(&mask);
                sigaddset(&mask, SIGCHLD);
                if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
                {
                    syslog(LOG_ERR, "tcp-ssh sigprocmask(), errno = %d: %s", errno, strerror(errno));
                    return ERROR_SIGACTION;
                }

                struct sigaction sig = {.sa_handler = sigchld};
                if (sigaction(SIGCHLD, &sig, NULL) == -1)
                {
                    syslog(LOG_ERR, "tcp-ssh sigaction(), errno = %d: %s", errno, strerror(errno));
                    return ERROR_SIGACTION;                
                }
                
                unsigned char secret[MAX_LEN] = {};
                if (security_get_secret("/usr/share/powerssh/powerssh_public.key", secret, PUBLIC_SIDE, accepted_socket, SOCK_STREAM, NULL, NULL) < 0)
                {
                    return -1;
                }
                syslog(LOG_ERR, "tcp-ssh security_get_secret: \"%s\", errno = %d: %s", secret, errno, strerror(errno));


                int master_fd = 0;

                char username[MAX_LEN] = "";
                int result = rudp_recv(accepted_socket, username, MAX_LEN - 1, NULL, SOCK_STREAM, NULL);
                if (result < 0) return RUDP_RECV;

                pid_t pid = pty_fork(&master_fd, slave, MAX_LEN, NULL, NULL);
                if (pid < 0) syslog(LOG_ERR, "[RUDP] pty_fork pid = %d, errno = %d", pid, errno);
                if (pid == 0)
                {
                    result = login_into_user(username);
                    syslog(LOG_INFO, "[RUDP] server login into \"%s\" user %d, errno = %d", username, result, errno);
                    
                    if (result == -1)
                    {
                        rudp_close(accepted_socket, SOCK_STREAM, NULL, NULL, 0);
                        return -1;
                    } 

                    result = set_id(username);
                    if (result == -1)
                    {
                        syslog(LOG_INFO, "[PAM] set_id \"%s\" user %d, errno = %d", username, result, errno);
                        rudp_close(accepted_socket, SOCK_STREAM, NULL, NULL, 0);                        
                        return ERROR_SET_ID;
                    } 

                    char* bash_argv[] = {"bash", NULL};
                    execvp("bash", bash_argv);

                    return -1;
                }

               
                struct pollfd master[2] = {{.fd = master_fd, .events = POLL_IN}, {.fd = accepted_socket, .events = POLL_IN}};

                size_t n_write = 0;
                size_t n_read  = 0;

                while (1)
                {
                    if (exit_master_read_loop == 1) break;

                    int event = poll(master, 2, 100);
                    if (event > 0)
                    {
                        if (master[0].revents == POLL_IN)
                        {
                            n_read = read(master_fd, buff, sizeof(buff));
                            if (n_read == -1)
                            {
                                perror("read");
                                return -1;
                            }

                            syslog(LOG_INFO, "[RUDP] server read errno = %d", errno);

                            
                            n_write = rudp_send(accepted_socket, buff, n_read, NULL, SOCK_STREAM, NULL);
                            if (n_write == -1)
                            {
                                perror("rudp_recv(SOCK_STREAM)");
                                return -1;
                            }

                            syslog(LOG_INFO, "[RUDP] server send errno = %d", errno);
                        }
                        
                        if (master[1].revents == POLL_IN)
                        {
                            n_read = rudp_recv(accepted_socket, buff, sizeof(buff), NULL, SOCK_STREAM, NULL);
                            if (n_read == -1)
                            {
                                perror("rudp_send(SOCK_STREAM)");
                                return -1;
                            }
                            if (!n_read)
                            {
                                syslog(LOG_INFO, "[RUDP] client was shutdowned, errno = %d", errno);
                                rudp_close(accepted_socket, SOCK_STREAM, NULL, NULL, 0);
                                kill(pid, SIGKILL);
                                return 1;             
                            }
                            syslog(LOG_INFO, "[RUDP] server recv errno = %d", errno);

                            n_write = write(master_fd, buff, n_read);
                            if (n_write == -1)
                            {
                                perror("write()");
                                return -1;
                            }     

                            syslog(LOG_INFO, "[RUDP] server write errno = %d", errno);
                        }
                    }
                    else if (event == 0) continue;
                    else
                    {
                        break;
                    }
                }

                if (errno == EINTR) errno = 0;

                rudp_close(accepted_socket, SOCK_STREAM, NULL, NULL, 0);
                close(master_fd);

                return 0;
            }
        }

        return 0;

    } else if (type_connection == SOCK_DGRAM)
    {
        struct timeval time = {.tv_sec = STANTARD_TIME};
        int socket = rudp_socket(address, port, time, SOCK_DGRAM);
        if (socket < 0) return RUDP_SOCKET;

        syslog(LOG_NOTICE, "daemon POWER[%d] rudp_socket created port %d", getpid(), ntohs(port));

        struct rudp_header control = {};
        struct sockaddr_in client = {};

        while (1)
        {
            int accepted_socket = rudp_accept(socket, &client, SOCK_DGRAM, &control);
            syslog(LOG_NOTICE, "daemon POWER[%d] %d", getpid(), accepted_socket);
            syslog(LOG_INFO, "[RUDP] server %s:%d errno = %d: %s", inet_ntoa(client.sin_addr), ntohs(client.sin_port), errno, strerror(errno));

            if (accepted_socket < 0) return RUDP_ACCEPT;

            if (accepted_socket > 0)
            {   
                syslog(LOG_ERR, "[RUDP] after accept pid = %d, errno = %d", getpid(), errno);
                syslog(LOG_NOTICE, "daemon POWER[%d] accepted", getpid());
                syslog(LOG_INFO, "[RUDP] server %s:%d errno = %d: %s", inet_ntoa(client.sin_addr), ntohs(client.sin_port), errno, strerror(errno));

                char buff[MAX_BUFFER] = "";
                char slave[MAX_LEN] = "";

                sigset_t mask = {};
                sigemptyset(&mask);
                sigaddset(&mask, SIGCHLD);
                if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
                {
                    syslog(LOG_ERR, "tcp-ssh sigprocmask(), errno = %d: %s", errno, strerror(errno));
                    return ERROR_SIGACTION;
                }

                struct sigaction sig = {.sa_handler = sigchld};
                if (sigaction(SIGCHLD, &sig, NULL) == -1)
                {
                    syslog(LOG_ERR, "tcp-ssh sigaction(), errno = %d: %s", errno, strerror(errno));
                    return ERROR_SIGACTION;                
                }

                int master_fd = 0;

                char username[MAX_LEN] = "";
                int result = rudp_recv(accepted_socket, username, MAX_LEN - 1, &client, SOCK_DGRAM, &control);
                if (result < 0) return RUDP_RECV;

                pid_t pid = pty_fork(&master_fd, slave, MAX_LEN, NULL, NULL);
                if (pid < 0) syslog(LOG_ERR, "[RUDP] pty_fork pid = %d, errno = %d", pid, errno);
                if (pid == 0)
                {
                    result = login_into_user(username);
                    syslog(LOG_INFO, "[RUDP] server login into \"%s\" user %d, errno = %d", username, result, errno);
                    
                    if (result == -1)
                    {
                        rudp_close(accepted_socket, SOCK_DGRAM, &client, &control, 1);
                        rudp_recv(accepted_socket, buff, sizeof(buff), &client, SOCK_DGRAM, &control);
                        rudp_close(accepted_socket, SOCK_DGRAM, &client, &control, 3);
                        return -1;
                    } 

                    result = set_id(username);
                    if (result == -1)
                    {
                        syslog(LOG_INFO, "[PAM] set_id \"%s\" user %d, errno = %d", username, result, errno);
                        rudp_close(accepted_socket, SOCK_DGRAM, &client, &control, 1);
                        rudp_recv(accepted_socket, buff, sizeof(buff), &client, SOCK_DGRAM, &control);
                        rudp_close(accepted_socket, SOCK_DGRAM, &client, &control, 3);                     
                        return ERROR_SET_ID;
                    } 

                    syslog(LOG_INFO, "[RUDP] server bash errno = %d", errno);
                    char* bash_argv[] = {"bash", NULL};
                    execvp("bash", bash_argv);

                    return -1;
                }

                struct pollfd master[2] = {{.fd = master_fd, .events = POLL_IN}, {.fd = accepted_socket, .events = POLL_IN}};

                int n_write = 0;
                int n_read  = 0;

                while (1)
                {
                    if (exit_master_read_loop == 1) break;

                    int event = poll(master, 2, 100);
                    if (event > 0)
                    {
                        if (master[0].revents == POLL_IN)
                        {
                            n_read = read(master_fd, buff, sizeof(buff));
                            if (n_read == -1)
                            {
                                perror("read");
                                return -1;
                            }

                            syslog(LOG_INFO, "[RUDP] server read errno = %d: %s", errno, strerror(errno));

                            
                            n_write = rudp_send(accepted_socket, buff, n_read, &client, SOCK_DGRAM, &control);
                            if (n_write == -1)
                            {
                                perror("rudp_recv(SOCK_STREAM)");
                                return -1;
                            }

                            syslog(LOG_INFO, "[RUDP] server send %d, %s:%d errno = %d: %s", n_write, inet_ntoa(client.sin_addr), ntohs(client.sin_port), errno, strerror(errno));
                        }

                        if (master[1].revents == POLL_IN)
                        {
                            n_read = rudp_recv(accepted_socket, buff, sizeof(buff), &client, SOCK_DGRAM, &control);
                            buff[n_read] = '\0';
                            syslog(LOG_INFO, "[RUDP] server recv %d \"%s\" errno = %d", n_read, buff, errno);
                            if (n_read == -1)
                            {
                                perror("rudp_send(SOCK_STREAM)");
                                return -1;
                            }
                            if (!n_read)
                            {
                                rudp_close(accepted_socket, type_connection, &client, &control, 2);
                                syslog(LOG_INFO, "[RUDP] client was shutdowned, errno = %d", errno);
                                kill(pid, SIGKILL);
                                return 1;
                            }
                           
                            n_write = write(master_fd, buff, n_read);
                            if (n_write == -1)
                            {
                                perror("write()");
                                return -1;
                            }     

                            syslog(LOG_INFO, "[RUDP] server write errno = %d", errno);
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

                if (errno == EINTR) errno = 0;

                rudp_close(accepted_socket, SOCK_DGRAM, &client, &control, 1);
                rudp_recv(accepted_socket, buff, sizeof(buff), &client, SOCK_DGRAM, &control);
                rudp_close(accepted_socket, SOCK_DGRAM, &client, &control, 3);                     
                close(master_fd);

                return 0;
            }
        }

        return 0;
    }
    else
    {
        return RUDP_UNDEFINED_TYPE;
    }
}

//==========================================================================================================================

void sigusr1(int s)
{
    syslog(LOG_INFO, "copy_server SIGUSR1 errno = %d", errno);
    exit_master_read_loop = 1;
}


int copy_server(in_addr_t address, in_port_t port)
{
    struct timeval time = {.tv_sec = STANTARD_TIME};
    int socket = rudp_socket(address, port, time, SOCK_STREAM);
    if (socket < 0) return RUDP_SOCKET;

    syslog(LOG_NOTICE, "daemon POWER[%d] rudp_socket created port %d", getpid(), ntohs(port));

    int result = listen(socket, SIZE_QUEUE);
    if (result == -1) return RUDP_LISTEN;

    syslog(LOG_NOTICE, "daemon POWER[%d] rudp_socket listened, errno = %d: %s", getpid(), errno, strerror(errno));


    struct sockaddr_in client = {};

    while (1)
    {
        int accepted_socket = rudp_accept(socket, &client, SOCK_STREAM, NULL);
        syslog(LOG_NOTICE, "daemon POWER[%d] %d", getpid(), accepted_socket);
        if (accepted_socket < 0) return RUDP_ACCEPT;

        if (accepted_socket > 0)
        {   
            syslog(LOG_ERR, "[RUDP] after accept pid = %d, errno = %d", getpid(), errno);
            syslog(LOG_NOTICE, "daemon POWER[%d] accepted", getpid());
            char buff[MAX_BUFFER] = "";
            char slave[MAX_LEN] = "";

            sigset_t mask = {};
            sigemptyset(&mask);
            sigaddset(&mask, SIGUSR1);
            sigaddset(&mask, SIGCHLD);
            if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
            {
                syslog(LOG_ERR, "copy_server sigprocmask(), errno = %d: %s", errno, strerror(errno));
                return ERROR_SIGACTION;
            }

            struct sigaction sig = {.sa_handler = sigusr1};
            if (sigaction(SIGUSR1, &sig, NULL) == -1)
            {
                syslog(LOG_ERR, "copy_server sigaction(), errno = %d: %s", errno, strerror(errno));
                return ERROR_SIGACTION;                
            }

            struct file_copy_header header = {};
            int result = rudp_recv(accepted_socket, &header, sizeof(header), NULL, SOCK_STREAM, NULL);
            if (result < 0) return RUDP_RECV;

            syslog(LOG_INFO, "[COPY] header: path: \"%s\" username: \"%s\" size: %lu mode: %lo", header.path, header.username, header.size, header.mode);

            int master_fd = 0;

            pid_t pid = pty_fork(&master_fd, slave, MAX_LEN, NULL, NULL);
            if (pid < 0) syslog(LOG_ERR, "[RUDP] pty_fork pid = %d, errno = %d", pid, errno);
            if (pid == 0)
            {
                result = login_into_user(header.username);
                syslog(LOG_INFO, "[RUDP] server login into \"%s\" user %d, errno = %d", header.username, result, errno);
                
                if (result == -1)
                {
                    rudp_close(accepted_socket, SOCK_STREAM, NULL, NULL, 0);
                    return ERROR_LOGIN;
                } 

                kill(getppid(), SIGUSR1);

                result = set_id(header.username);
                if (result == -1)
                {
                    syslog(LOG_INFO, "[PAM] set_id \"%s\" user %d, errno = %d", header.username, result, errno);
                    rudp_close(accepted_socket, SOCK_STREAM, NULL, NULL, 0);                        
                    return ERROR_SET_ID;
                } 

                int fd = open(header.path, O_WRONLY | O_CREAT | O_LARGEFILE | O_TRUNC, header.mode);
                if (fd == -1)
                {
                    syslog(LOG_INFO, "[COPY] open, errno = %d: %s", errno, strerror(errno));
                    rudp_close(accepted_socket, SOCK_STREAM, NULL, NULL, 0);                        
                    return ERROR_OPEN;
                }

                while (!exit_master_read_loop);

                int result = rudp_send(accepted_socket, &header, sizeof(header), &client, SOCK_STREAM, NULL);
                if (result == -1)
                {
                    syslog(LOG_ERR, "[RUDP] copy_server send errno = %d", errno);
                    return -1;
                }

                syslog(LOG_ERR, "[RUDP] copy_server has been started receiving the file, errno = %d", errno);

                size_t size = 0;

                while (1)
                {
                    int n_read = rudp_recv(accepted_socket, buff, sizeof(buff), NULL, SOCK_STREAM, NULL);
                    if (n_read == -1)
                    {
                        syslog(LOG_INFO, "[RUDP] server recv errno = %d", errno);
                        return -1;
                    }
                    if (!n_read)
                    {
                        syslog(LOG_INFO, "[RUDP] client was shutdowned, errno = %d", errno);
                        rudp_close(accepted_socket, SOCK_STREAM, NULL, NULL, 0);
                        return 1;             
                    }
                    
                    syslog(LOG_INFO, "[RUDP] server recv errno = %d", errno);

                    int n_write = write(fd, buff, n_read);
                    if (n_write == -1)
                    {
                        syslog(LOG_ERR, "write(fd), errno = %d", errno);
                        return -1;
                    }

                    size += n_write;
                    if (size == header.size) break;
                }
                
                rudp_close(accepted_socket, SOCK_STREAM, NULL, NULL, 0);
                return 0;
            }

            
            struct pollfd master[2] = {{.fd = master_fd, .events = POLL_IN}, {.fd = accepted_socket, .events = POLL_IN}};

            size_t n_write = 0;
            size_t n_read  = 0;

            while (1)
            {
                if (exit_master_read_loop == 1) break;

                int event = poll(master, 2, 100);
                if (event > 0)
                {
                    if (master[0].revents == POLL_IN)
                    {
                        n_read = read(master_fd, buff, sizeof(buff));
                        if (n_read == -1)
                        {
                            perror("read");
                            return -1;
                        }

                        syslog(LOG_INFO, "[RUDP] server read errno = %d", errno);

                        
                        n_write = rudp_send(accepted_socket, buff, n_read, &client, SOCK_STREAM, NULL);
                        if (n_write == -1)
                        {
                            perror("rudp_recv(SOCK_STREAM)");
                            return -1;
                        }

                        syslog(LOG_INFO, "[RUDP] server send errno = %d", errno);
                    }
                    
                    if (master[1].revents == POLL_IN)
                    {
                        n_read = rudp_recv(accepted_socket, buff, sizeof(buff), NULL, SOCK_STREAM, NULL);
                        if (n_read == -1)
                        {
                            perror("rudp_send(SOCK_STREAM)");
                            return -1;
                        }
                        if (!n_read)
                        {
                            syslog(LOG_INFO, "[RUDP] client was shutdowned, errno = %d", errno);
                            rudp_close(accepted_socket, SOCK_STREAM, NULL, NULL, 0);
                            kill(pid, SIGKILL);
                            return 1;             
                        }
                        syslog(LOG_INFO, "[RUDP] server recv errno = %d", errno);

                        n_write = write(master_fd, buff, n_read);
                        if (n_write == -1)
                        {
                            perror("write()");
                            return -1;
                        }     

                        syslog(LOG_INFO, "[RUDP] server write errno = %d", errno);
                    }
                }
                else if (event == 0) continue;
                else
                {
                    break;
                }
            }

            if (errno == EINTR) errno = 0;
            syslog(LOG_INFO, "copy_server is waiting for the file transmission to finish. errno = %d", errno);
            kill(pid, SIGUSR1);

            waitpid(pid, NULL, 0);  

            rudp_close(accepted_socket, SOCK_STREAM, NULL, NULL, 0);
            return 0;
        }
    }

    return 0;
}
