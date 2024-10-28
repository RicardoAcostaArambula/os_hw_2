#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
 #include <unistd.h>
#include <netdb.h>
#define SIZE 65536
/*Function declarations*/
static int convert_port_name(uint16_t *port, const char *port_name);
ssize_t better_write(int fd, const char *buf, size_t count);


int main(int argc, char **argv){
    char *message, *port_name;
    uint16_t port_number;
    int socket_fd;
    char buf[SIZE];
    /*Checking we have enough arguments*/
    if (argc < 2){
        message = "Not enough arugments, expected: <client-name> <port>\n";
        better_write(1, message, strlen(message));
        return 1;
    }
    port_name = argv[1];
    /*We are converting the port name */
    if (convert_port_name(&port_number, port_name) < 0){
        message = "Could not get the port number\n";
        better_write(1, message, strlen(message));
        return 1;
    }

    /*We get the socket file descriptor */
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0){
        fprintf(stderr, "Error: could not create socket\n");
        return 1;
    }
    struct sockaddr_in serveraddr, clientaddr;
    socklen_t clientaddr_len = sizeof(clientaddr); 

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port_number);

    /* we are binding the socket file descriptor */
    if (bind(socket_fd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0){
        fprintf(stderr, "Error: Could not bind socket\n");
        if (close(socket_fd) < 0){
            fprintf(stderr, "Error: could not close socket fd.\n");
        }
        return 1;
    }
    ssize_t recv_length;
    // printf("reached 1\n");
    while (1){
        // recv_length = recvfrom(socket_fd, buf, sizeof(buf), 0, (struct sockaddr *) &clientaddr, &clientaddr_len);
        recv_length = recvfrom(socket_fd, buf, SIZE, 0, (struct sockaddr *) &clientaddr, &clientaddr_len);
        // printf("length: %ld\n", recv_length);
        if(recv_length < 0){
            fprintf(stderr, "Error: could not receive from client.");
            return 1;
        }
        /*Once we read, we need to send back the UDP packets*/
        ssize_t send_bytes = sendto(socket_fd, buf, recv_length, 0, (struct sockaddr *) &clientaddr, clientaddr_len);
        if (send_bytes < 0){
            fprintf(stderr, "Error: sendto failed.\n");
            if (close(socket_fd) < 0){
                fprintf(stderr, "Error: could not close socket fd.\n");
            }
            return 1;
        }
    }

    if (close(socket_fd) < 0){
        fprintf(stderr, "Error: could not close socket fd.\n");
    }

    return 0;
}

static int convert_port_name(uint16_t *port, const char *port_name) {
    char *end;
    long long int nn;
    uint16_t t;
    long long int tt;
    if (port_name == NULL) return -1;
    if (*port_name == '\0') return -1;
    nn = strtoll(port_name, &end, 0);
    if (*end != '\0') return -1;
    if (nn < ((long long int) 0)) return -1;
    t = (uint16_t) nn;
    tt = (long long int) t;
    if (tt != nn) return -1;
    *port = t;
    return 0;
}

ssize_t better_write(int fd, const char *buf, size_t count) {
    size_t already_written, to_be_written, written_this_time, max_count;
    ssize_t res_write;

    if (count == ((size_t) 0)) return (ssize_t) count;
    already_written = (size_t) 0;
    to_be_written = count;
    while (to_be_written > ((size_t) 0)) {
        max_count = to_be_written;
        if (max_count > ((size_t) 8192)) {
            max_count = (size_t) 8192;
        }
        res_write = write(fd, &(((const char *) buf)[already_written]), max_count);
        if (res_write < ((size_t) 0)) {
            /* Error */
            return res_write;
        }
        if (res_write == ((ssize_t) 0)) {
        /* Nothing written, stop trying */
            
        }
        written_this_time = (size_t) res_write;
        already_written += written_this_time;
        to_be_written -= written_this_time;
    }
    return (ssize_t) already_written;
}