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
int setup_udp_socket(const char *server, const char *port) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(server, port, &hints, &res) != 0)
        error_exit("getaddrinfo failed");
    int udp_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (udp_sock < 0)
        error_exit("UDP Socket creation failed");
    freeaddrinfo(res);
    return udp_sock;
}

//setup tcp server socket 
int setup_tcp_server(uint16_t port) {
    int tcp_sock;
    struct sockaddr_in tcp_addr;
    if ((tcp_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error_exit("TCP Socket creation failed");
    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    tcp_addr.sin_port = htons(port);
    if (bind(tcp_sock, (struct sockaddr *)&tcp_addr, sizeof(tcp_addr)) < 0)
        error_exit("Binding TCP socket failed");
    if (listen(tcp_sock, 1) < 0)
        error_exit("Listen failed");

    return tcp_sock;
}

//handle the data transfer between udp and tcp 
void handle_data_transfer(int udp_sock, int tcp_sock) {
    fd_set read_fds;
    char udp_buffer[UDP_BUFFER_SIZE];
    char tcp_buffer[TCP_BUFFER_SIZE];
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(udp_sock, &read_fds);
        FD_SET(tcp_sock, &read_fds);
        int max_fd = (udp_sock > tcp_sock) ? udp_sock : tcp_sock;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0)
            error_exit("select error");
        if (FD_ISSET(udp_sock, &read_fds)) {
            ssize_t udp_len = recv(udp_sock, udp_buffer, UDP_BUFFER_SIZE, 0);
            if (udp_len < 0) error_exit("recv failed");
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
            if (send(udp_sock, tcp_buffer + 2, udp_len, 0) < 0)
                error_exit("send failed");
        }
    }
}

//Parse arguments, setup sockets and start data transfer
int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <TCP Port> <UDP Server> <UDP Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    uint16_t tcp_port = (uint16_t) atoi(argv[1]);
    const char *udp_server = argv[2];
    const char *udp_port = argv[3];
    int tcp_sock = setup_tcp_server(tcp_port);
    int udp_sock = setup_udp_socket(udp_server, udp_port);
    int client_sock = accept(tcp_sock, NULL, NULL);
    if (client_sock < 0) error_exit("accept failed");
    handle_data_transfer(udp_sock, client_sock);

    close(client_sock);
    close(tcp_sock);
    close(udp_sock);
    return 0;
}
