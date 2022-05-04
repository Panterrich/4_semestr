#include "rudp.h"

int main()
{
    struct timeval time = {.tv_sec = STANTARD_TIME};
    int socket = rudp_socket(INADDR_ANY, htons(31000), time, SOCK_DGRAM);
    perror("10 ");
    
    struct sockaddr_in addr = {INADDR_ANY, .sin_port = htons(36000)};
    struct rudp_header header = {};

    while (1)
    {
        int result = rudp_accept(socket, &addr, SOCK_DGRAM, &header);
        perror("20 ");

        if (result == 0) 
        {   
            ;
        }
        else
        {
            char buffer[1001] = "Hi, Power";
            rudp_send(result, buffer, 1000, &addr, SOCK_DGRAM, &header);
            perror("30 ");

            char answer[1001] = "";
            rudp_recv(result, answer, 1000, &addr, SOCK_DGRAM, &header);
            perror("40 ");

            printf("%s\n", answer);

            rudp_recv(result, answer, 1000, &addr, SOCK_DGRAM, &header);
            rudp_close(result, SOCK_DGRAM, NULL, NULL, 2);
            return 0;
        }
    }
    
    return 0;
}