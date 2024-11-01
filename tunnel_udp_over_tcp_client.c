#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdint.h>
#include <sys/select.h>
#define UDP_BUFFER_SIZE 216
#define TCP_BUFFER_SIZE (216 + 2)

//Print error message 
void error_exit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

//Setup udp socket
int setup_udp_socket(uint16_t port) {
    int udp_sock;
    struct sockaddr_in udp_addr;
    //Create socket
    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        error_exit("UDP Socket creation failed");
    //Bind socket to port
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = htons(port);
    if (bind(udp_sock, (struct sockaddr *)&udp_addr, sizeof(udp_addr)) < 0)
        error_exit("Binding UDP socket failed");
    return udp_sock;
}

//Setup tcp connection to specified server and port
int setup_tcp_connection(const char *server, const char *port) {
    int tcp_sock;
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(server, port, &hints, &res) != 0)
        error_exit("getaddrinfo failed");
    if ((tcp_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
        error_exit("TCP Socket creation failed");
    if (connect(tcp_sock, res->ai_addr, res->ai_addrlen) < 0)
        error_exit("TCP connection failed");
    freeaddrinfo(res);
    return tcp_sock;
}

//Transfer data between udp and tcp 
void handle_data_transfer(int udp_sock, int tcp_sock) {
    fd_set read_fds;
    char udp_buffer[UDP_BUFFER_SIZE];
    char tcp_buffer[TCP_BUFFER_SIZE];
    struct sockaddr_in udp_sender;
    socklen_t sender_len = sizeof(udp_sender);
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(udp_sock, &read_fds);
        FD_SET(tcp_sock, &read_fds);
        int max_fd = (udp_sock > tcp_sock) ? udp_sock : tcp_sock;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0)
            error_exit("select error");
        if (FD_ISSET(udp_sock, &read_fds)) {
            ssize_t udp_len = recvfrom(udp_sock, udp_buffer, UDP_BUFFER_SIZE, 0,
                                       (struct sockaddr *)&udp_sender, &sender_len);
            if (udp_len < 0) error_exit("recvfrom failed");
            uint16_t net_len = htons((uint16_t) udp_len);
            memcpy(tcp_buffer, &net_len, 2);
            memcpy(tcp_buffer + 2, udp_buffer, udp_len);
            if (write(tcp_sock, tcp_buffer, udp_len + 2) < 0)
                error_exit("write to TCP failed");
        }
        if (FD_ISSET(tcp_sock, &read_fds)) {
            ssize_t tcp_len = read(tcp_sock, tcp_buffer, TCP_BUFFER_SIZE);
            if (tcp_len < 0) error_exit("read from TCP failed");
            if (tcp_len == 0) break;
            uint16_t udp_len;
            memcpy(&udp_len, tcp_buffer, 2);
            udp_len = ntohs(udp_len);
            if (sendto(udp_sock, tcp_buffer + 2, udp_len, 0,
                       (struct sockaddr *)&udp_sender, sender_len) < 0)
                error_exit("sendto failed");
        }
    }
}

//Setup socketw and start data transfer 
int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <UDP Port> <TCP Server> <TCP Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    uint16_t udp_port = (uint16_t) atoi(argv[1]);
    const char *tcp_server = argv[2];
    const char *tcp_port = argv[3];
    int udp_sock = setup_udp_socket(udp_port);
    int tcp_sock = setup_tcp_connection(tcp_server, tcp_port);
    handle_data_transfer(udp_sock, tcp_sock);

    close(udp_sock);
    close(tcp_sock);
    return 0;
}
