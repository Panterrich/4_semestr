#include "rudp.h"

int main(int argc, char* argv[])
{
    struct timeval time = {.tv_sec = STANTARD_TIME};
    int socket = rudp_socket(inet_addr("127.0.0.1"), htons(36000), time, SOCK_DGRAM);
    
    struct sockaddr_in addr = {.sin_family = AF_INET, .sin_addr.s_addr = inet_addr("127.0.0.1"), .sin_port = htons(31000)};
    struct sockaddr_in connected_addr = {};
    struct rudp_header header = {};
    perror("1 ");

    int result = rudp_connect(socket, &addr, SOCK_DGRAM, &header, &connected_addr);
    
    char answer[1000] = "Hi, Makima";
    rudp_recv(socket, answer, 1000, &connected_addr, SOCK_DGRAM, &header);
    perror("3 ");
    printf("%s\n", answer);

    char buffer[1000] = "Hi, Makima";
    rudp_send(socket, buffer, 1000, &connected_addr, SOCK_DGRAM, &header);
    perror("4 ");


    rudp_close(socket, SOCK_DGRAM, &connected_addr, &header, 1);
    rudp_close(socket, SOCK_DGRAM, &connected_addr, &header, 3);
    return 0;
}