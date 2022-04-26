#include "rudp.h"

int rudp_socket(in_addr_t address, in_port_t port, struct timeval time_rcv, int type_connection)
{
    if (type_connection == SOCK_STREAM)
    {
        int TCP_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (TCP_socket == -1) return RUDP_SOCKET;

        int reuse = 1;

        if ((setsockopt(TCP_socket, SOL_SOCKET, SO_REUSEADDR, &reuse,    sizeof(int)) == -1) ||
            (setsockopt(TCP_socket, SOL_SOCKET, SO_RCVTIMEO,  &time_rcv, sizeof(struct timeval)) == -1))
        {
            return RUDP_SETSOCKOPT;
        }
        
        struct sockaddr_in addr = {.sin_family = AF_INET, .sin_addr.s_addr = address, .sin_port = port};

        if (bind(TCP_socket, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1)
        {
            return RUDP_BIND;
        }

        return TCP_socket;
    }
    else if (type_connection == SOCK_DGRAM)
    {
        int UDP_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (UDP_socket == -1) return RUDP_SOCKET;

        int reuse = 1;

        if ((setsockopt(UDP_socket, SOL_SOCKET, SO_REUSEADDR, &reuse,    sizeof(int)) == -1) ||
            (setsockopt(UDP_socket, SOL_SOCKET, SO_RCVTIMEO,  &time_rcv, sizeof(struct timeval)) == -1))
        {
            return  RUDP_SETSOCKOPT;
        }

        struct sockaddr_in addr = {.sin_family = AF_INET, .sin_addr.s_addr = address, .sin_port = port};

        if (bind(UDP_socket, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1)
        {
            return RUDP_BIND;
        }

        return UDP_socket;
    }
    else
    {
        return RUDP_UNDEFINED_TYPE;
    }
}

int rudp_recv(int socket, void* message, size_t length, struct sockaddr_in* address, \
              int type_connection, struct rudp_header* control)
{
    if (type_connection == SOCK_STREAM)
    {
        socklen_t socklen = sizeof(struct sockaddr_in);
    
        int len = recv(socket, message, length, 0);
        if (len == -1 && errno == EAGAIN)
        {
            return RUDP_CLOSE_CONNECTION;
        }
        else if (len == -1)
        {
            return RUDP_RECV;
        }

        return len;
    }
    else if (type_connection == SOCK_DGRAM)
    {
        char* buffer = (char*) calloc(length + sizeof(struct rudp_header), sizeof(char));
        if (!buffer) return RUDP_CALLOC; 

        char answer[MAX_ANSWER] = "";
        socklen_t len_addr = sizeof(struct sockaddr_in);

        int n_recv = 0;
        int i = 0;
        int flag_end_connection = 0;

        for (; i < N_ATTEMPT; i++)
        {
            n_recv = recvfrom(socket, buffer, length + sizeof(struct rudp_header), 0, (struct sockaddr*) address, &len_addr);
            if (n_recv == -1 && errno == EAGAIN) 
            {
                free(buffer);
                return RUDP_CLOSE_CONNECTION;
            }
            else if (n_recv == -1)
            {
                continue; // FAILTURE
            }
            
            if (control->acknowledgement_number == ((struct rudp_header*)buffer)->sequence_number)
            {
                control->acknowledgement_number += n_recv;
            }

            if (((struct rudp_header*)buffer)->flag == FIN)
            {
                flag_end_connection = 1;
            }

            *(struct rudp_header*)answer = *control;
            ((struct rudp_header*)answer)->flag = ACK;

            len_addr = sizeof(struct sockaddr_in);
            int n_sent = sendto(socket, answer, MAX_ANSWER, 0, (struct sockaddr*) address, len_addr);

            if (n_sent == -1 && errno == EAGAIN) 
            {
                free(buffer);
                return RUDP_CLOSE_CONNECTION;
            }
            else if (n_sent == -1 || n_sent != MAX_ANSWER)
            {
                continue; //FAILTURE
            }
            else
            {
                break;  // SUCCESS
            }
        }

        if (i == N_ATTEMPT) 
        {
            free(buffer);
            return RUDP_EXCEEDED_N_ATTEMPTS; // FAILTURE
        }

        if (!flag_end_connection)
        {
            memcpy(message, buffer + sizeof(struct rudp_header), length);
            free(buffer);

            return n_recv;
        }
        else
        {
            return 0;
        }
        
    }
    else
    {
        return RUDP_UNDEFINED_TYPE;
    }
}

int rudp_send(int socket, const void* message, size_t length, const struct sockaddr_in* dest_addr, \
              int type_connection, struct rudp_header* control)
{
    if (type_connection == SOCK_STREAM)
    {       
        int n_sent = sendto(socket, message, length, 0, (struct sockaddr*) dest_addr, sizeof(struct sockaddr_in));
        if (n_sent == -1 && errno == EAGAIN)
        {
            return RUDP_CLOSE_CONNECTION;
        }
        else if (n_sent == -1 || n_sent != length)
        {
            return RUDP_RECV;
        }

        return n_sent;
    }
    else if (type_connection == SOCK_DGRAM)
    {
        char* buffer = (char*) calloc(length + sizeof(struct rudp_header), sizeof(char));
        if (!buffer) return RUDP_CALLOC; 

        *(struct rudp_header*)buffer = *control;
        memcpy(buffer + sizeof(struct rudp_header), message, length);

        control->sequence_number += length + sizeof(struct rudp_header);

        struct timeval ack_time      = {.tv_sec = ACK_TIME};
        struct timeval standard_time = {.tv_sec = STANTARD_TIME};
        socklen_t time_len = sizeof(struct timeval);
        if (getsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &standard_time, &time_len) == -1)
        {
            return RUDP_GETSOCKOPT;
        }

        if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &ack_time, sizeof(struct timeval)) == -1)
        {
            return RUDP_SETSOCKOPT;
        }

        char answer[MAX_ANSWER] = "";
        struct sockaddr_in addr = {};
        socklen_t      len_addr = sizeof(struct sockaddr_in);

        int n_sent = 0;
        int i = 0;
        for (; i < N_ATTEMPT; i++)
        {
            n_sent = sendto(socket, buffer, length + sizeof(struct rudp_header), 0, (struct sockaddr*) dest_addr, sizeof(struct sockaddr_in));
            if (n_sent == -1 || errno == EAGAIN)
            {
                free(buffer);
                return RUDP_CLOSE_CONNECTION;
            }
            else if (n_sent == -1 || n_sent != length + sizeof(struct rudp_header))
            {
                continue; // FAILTURE
            }
            
            len_addr = sizeof(struct sockaddr_in);
            int n_recv = recvfrom(socket, answer, MAX_ANSWER, 0, (struct sockaddr*) &addr, &len_addr);
            if (n_recv == -1 && errno == EAGAIN) 
            {
                free(buffer);
                return RUDP_CLOSE_CONNECTION;
            }
            else if (n_recv == -1)
            {
                continue; // FAILTURE
            }

            if (((struct rudp_header*)answer)->flag == ACK &&
                ((struct rudp_header*)answer)->acknowledgement_number == control->sequence_number && 
                ((dest_addr->sin_addr.s_addr != INADDR_ANY && dest_addr->sin_addr.s_addr == addr.sin_addr.s_addr) || dest_addr->sin_addr.s_addr == INADDR_ANY) &&
                  dest_addr->sin_port == addr.sin_port)
            {
                break; // SUCCESS
            }
            else
            {
                continue; // FAILTURE
            }
        }

        free(buffer);

        if (i == N_ATTEMPT) return RUDP_EXCEEDED_N_ATTEMPTS; // FAILTURE

        if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &standard_time, sizeof(struct timeval)) == -1)
        {
            return RUDP_SETSOCKOPT; 
        }

        return n_sent;
    }
    else
    {
        return RUDP_UNDEFINED_TYPE;
    }
}

int rudp_accept(int socket, struct sockaddr_in* address, int type_connection, struct rudp_header* control)
{
    if (type_connection == SOCK_STREAM)
    {
        socklen_t len = sizeof(struct sockaddr_in);

        int accepted_socket = accept(socket, (struct sockaddr*) address, &len);
        if (accepted_socket == -1)
        {
            return RUDP_ACCEPT;
        }

        pid_t pid = fork();
        if (pid == -1) return RUDP_FORK;

        if (!pid) // CHILD
        {
            if (close(socket) == -1)
            {
                return RUDP_CLOSE;
            }

            return accepted_socket;
        }

        if (close(accepted_socket) == -1)
        {
            return RUDP_CLOSE;
        }

        return 0;
    }
    else if (type_connection == SOCK_DGRAM)
    {
        char buffer[MAX_ANSWER] = "";
        char answer[MAX_ANSWER] = "";

        struct sockaddr_in addr     = {};
        struct sockaddr_in addr_ack = {};
        socklen_t          len_addr = sizeof(struct sockaddr_in);

        int i = 0;
        int accepted_socket = 0;

        for (; i < N_ATTEMPT; i++)
        {
            int n_recv = recvfrom(socket, buffer, MAX_ANSWER, 0, (struct sockaddr*) &addr, &len_addr);
            if (n_recv == -1 && errno == EAGAIN) 
            {
                return RUDP_CLOSE_CONNECTION;
            }
            else if (n_recv == -1)
            {
                continue; // FAILTURE
            }
            
            if (((struct rudp_header*)buffer)->flag != SYN)
            {
                continue; // FAILTURE
            }

            control->sequence_number        = MAX_ANSWER;
            control->acknowledgement_number = MAX_ANSWER;

            *(struct rudp_header*)answer        = *control;
            ((struct rudp_header*)answer)->flag = ACK | SYN;

            struct timeval time = {.tv_sec = STANTARD_TIME};

            accepted_socket = rudp_socket(INADDR_ANY, 0, time, SOCK_DGRAM);
            if (accepted_socket < 0) return RUDP_SOCKET;

            struct timeval ack_time      = {.tv_sec = ACK_TIME};
            struct timeval standard_time = {.tv_sec = STANTARD_TIME};
            socklen_t time_len           = sizeof(struct timeval);

            if (getsockopt(accepted_socket, SOL_SOCKET, SO_RCVTIMEO, &standard_time, &time_len) == -1)
            {
                return RUDP_GETSOCKOPT;
            }

            if (setsockopt(accepted_socket, SOL_SOCKET, SO_RCVTIMEO, &ack_time, sizeof(struct timeval)) == -1)
            {
                return RUDP_SETSOCKOPT;
            }

            len_addr = sizeof(struct sockaddr_in);
        
            int n_sent = sendto(accepted_socket, answer, MAX_ANSWER, 0, (struct sockaddr*) &addr, len_addr);
            if (n_sent == -1 && errno == EAGAIN) 
            {   
                close(accepted_socket);
                return RUDP_CLOSE_CONNECTION;
            }
            else if (n_sent == -1 || n_sent != MAX_ANSWER)
            {
                close(accepted_socket);
                continue; //FAILTURE
            }
            
            n_recv = recvfrom(accepted_socket, buffer, MAX_ANSWER, 0, (struct sockaddr*) &addr_ack, &len_addr);
            if (n_recv == -1 && errno == EAGAIN) 
            {   
                close(accepted_socket);
                return RUDP_CLOSE_CONNECTION;
            }
            else if (n_recv == -1)
            {   
                close(accepted_socket);
                continue; // FAILTURE
            }

            if (setsockopt(accepted_socket, SOL_SOCKET, SO_RCVTIMEO, &standard_time, sizeof(struct timeval)) == -1)
            {
                return RUDP_SETSOCKOPT; 
            }

            if (memcmp(&addr, &addr_ack, sizeof(struct sockaddr_in)) != 0)
            {   
                close(accepted_socket);
                continue;
            }
            else
            {   
                if (!address) memcpy(address, &addr, sizeof(struct sockaddr_in));
                break;
            }
        }

        if (i == N_ATTEMPT) 
        {   
            return RUDP_EXCEEDED_N_ATTEMPTS; // FAILTURE
        }

        pid_t pid = fork();
        if (pid == -1) return RUDP_FORK;

        if (!pid) // CHILD
        {
            if (close(socket) == -1)
            {
                return RUDP_CLOSE;
            }

            return accepted_socket;
        }

        if (close(accepted_socket) == -1)
        {
            return RUDP_CLOSE;
        }

        return 0;
    }
    else
    {
        return RUDP_UNDEFINED_TYPE;
    }
}

int rudp_connect(int socket, const struct sockaddr_in* address, int type_connection, struct rudp_header* control, struct sockaddr_in* connected_address)
{
    if (type_connection == SOCK_STREAM)
    {
        if (connect(socket, (struct sockaddr*) address, sizeof(struct sockaddr_in)) == -1)
        {
            return RUDP_CONNECT;
        }
    }
    else if (type_connection == SOCK_DGRAM)
    {
        struct timeval ack_time      = {.tv_sec = ACK_TIME};
        struct timeval standard_time = {.tv_sec = STANTARD_TIME};
        socklen_t time_len = sizeof(struct timeval);

        if (getsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &standard_time, &time_len) == -1)
        {
            return RUDP_GETSOCKOPT;
        }

        if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &ack_time, sizeof(struct timeval)) == -1)
        {
            return RUDP_SETSOCKOPT;
        }

        char buffer[MAX_ANSWER] = "";
        char answer[MAX_ANSWER] = "";

        control->sequence_number        = MAX_ANSWER;
        control->acknowledgement_number = 0;

        *(struct rudp_header*)buffer = *control;
        ((struct rudp_header*)buffer)->flag = SYN;

        struct sockaddr_in addr = {};
        socklen_t      len_addr = sizeof(struct sockaddr_in);
        
        int i = 0;
        for (; i < N_ATTEMPT; i++)
        {
            int n_sent = sendto(socket, buffer, MAX_ANSWER, 0, (struct sockaddr*) address, sizeof(struct sockaddr_in));
            if (n_sent == -1 || errno == EAGAIN)
            {
                return RUDP_CLOSE_CONNECTION;
            }
            else if (n_sent == -1 || n_sent != MAX_ANSWER)
            {
                continue; // FAILTURE
            }
            
            int n_recv = recvfrom(socket, answer, MAX_ANSWER, 0, (struct sockaddr*) &addr, &len_addr);
            if (n_recv == -1 && errno == EAGAIN) 
            {
                return RUDP_CLOSE_CONNECTION;
            }
            else if (n_recv == -1)
            {
                continue; // FAILTURE
            }

            if ((((struct rudp_header*)answer)->flag != (ACK | SYN)) && 
                ((address->sin_addr.s_addr != INADDR_ANY && address->sin_addr.s_addr == addr.sin_addr.s_addr) || 
                  address->sin_addr.s_addr == INADDR_ANY))
            {
                continue; // FAILTURE
            }

            memcpy(connected_address, &addr, sizeof(struct sockaddr_in));

            control->acknowledgement_number = MAX_ANSWER;
            *(struct rudp_header*)buffer = *control;
            ((struct rudp_header*)buffer)->flag = ACK;

            len_addr = sizeof(struct sockaddr_in);
            n_sent = sendto(socket, buffer, MAX_ANSWER, 0, (struct sockaddr*) &addr, len_addr);

            if (n_sent == -1 && errno == EAGAIN) 
            {
                return RUDP_CLOSE_CONNECTION;
            }
            else if (n_sent == -1 || n_sent != MAX_ANSWER)
            {
                continue; //FAILTURE
            }
            else
            {   
                break;  // SUCCESS
            }            
        }

        if (i == N_ATTEMPT) return RUDP_EXCEEDED_N_ATTEMPTS; // FAILTURE

        if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &standard_time, sizeof(struct timeval)) == -1)
        {
            return RUDP_SETSOCKOPT; 
        }

        return 0;
    }
    else
    {
        return RUDP_UNDEFINED_TYPE;
    }

    return 0;
}

int rudp_close(int socket, int type_connection, const struct sockaddr_in* address, struct rudp_header* control, int step)
{
    if (type_connection == SOCK_STREAM)
    {
        if (close(socket) == -1)
        {
            return RUDP_CLOSE;
        }
    }
    else if (type_connection == SOCK_DGRAM)
    {
        if (step == 1 && step == 2)
        {
            struct timeval ack_time      = {.tv_sec = ACK_TIME};
            struct timeval standard_time = {.tv_sec = STANTARD_TIME};
            socklen_t time_len = sizeof(struct timeval);
            if (getsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &standard_time, &time_len) == -1)
            {
                return RUDP_GETSOCKOPT;
            }

            if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &ack_time, sizeof(struct timeval)) == -1)
            {
                return RUDP_SETSOCKOPT;
            }

            char buffer[MAX_ANSWER] = "";
            char answer[MAX_ANSWER] = "";

            *(struct rudp_header*)buffer = *control;
            ((struct rudp_header*)buffer)->flag = FIN;

            control->sequence_number += MAX_ANSWER;

            struct sockaddr_in addr = {};
            socklen_t      len_addr = sizeof(struct sockaddr_in);

            int n_sent = 0;
            int i = 0;
            for (; i < N_ATTEMPT; i++)
            {
                n_sent = sendto(socket, buffer, MAX_ANSWER, 0, (struct sockaddr*) address, sizeof(struct sockaddr_in));
                if (n_sent == -1 || errno == EAGAIN)
                {
                    return RUDP_CLOSE_CONNECTION;
                }
                else if (n_sent == -1 || n_sent != MAX_ANSWER)
                {
                    continue; // FAILTURE
                }
                
                int n_recv = recvfrom(socket, answer, MAX_ANSWER, 0, (struct sockaddr*) &addr, &len_addr);
                if (n_recv == -1 && errno == EAGAIN) 
                {
                    return RUDP_CLOSE_CONNECTION;
                }
                else if (n_recv == -1)
                {
                    continue; // FAILTURE
                }

                if (((struct rudp_header*)answer)->flag == ACK &&
                    ((struct rudp_header*)answer)->acknowledgement_number == control->sequence_number &&
                    address->sin_addr.s_addr == addr.sin_addr.s_addr && address->sin_port == addr.sin_port)
                {
                    break; // SUCCESS
                }
                else
                {
                    continue; // FAILTURE
                }
            }

            if (i == N_ATTEMPT) return RUDP_EXCEEDED_N_ATTEMPTS; // FAILTURE

            if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &standard_time, sizeof(struct timeval)) == -1)
            {
                return RUDP_SETSOCKOPT; 
            }

            return 0;
        }
        
        if (step == 2 && step == 3)
        {
            if (close(socket) == -1)
            {
                return RUDP_CLOSE;
            }
        }
    }
    else
    {
        return RUDP_UNDEFINED_TYPE;
    }

    return 0;
}
