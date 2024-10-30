#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sys/select.h>

#define INPUT_BUFF_SIZE 480

#define RECV_BUFF_SIZE 65536

/*function delcarations*/
ssize_t better_write(int fd, const char *buf, size_t count);
int reading_and_sending(int socket_fd, char *buf, int buff_size);

int main(int argc, char **argv){
    char *server_name, *port_name, *message;
    int socket_fd, gai_code;
    char buf_in[INPUT_BUFF_SIZE];
    char buf_recv[RECV_BUFF_SIZE];
    int res = 0; 
    if (argc < 3){
        // fprintf(stderr, "Not enough arugments, expected: <server-name> <port>\n");
        message = "Not enough arugments, expected: <server-name> <port>\n";
        better_write(1, message, strlen(message));
        return 1;
    }
    server_name = argv[1];
    port_name = argv[2];

    /*Openning UDP socket to send UDP packets to the server at the port indicated*/
    struct addrinfo hints, *result, *current;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    hints.ai_flags = 0;
    gai_code = getaddrinfo(server_name, port_name, &hints, &result);
    if (gai_code != 0){
      fprintf(stderr, "Error with getaddrinfo: %s\n", strerror(errno));
      return 1;
    }
    socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (socket_fd < 0){
        fprintf(stderr, "Error: could not create socket\n");
        return 1;
    }

    for (current = result; current != NULL; current = current->ai_next){
        if (connect(socket_fd, current->ai_addr, current->ai_addrlen) == 0){
            break;
        }
    }

    if (current==NULL){
        fprintf(stderr, "Error: could not connect.\n");
        freeaddrinfo(result);
        return 1;
    }

    // if (reading_and_sending(socket_fd, buf_in) > 0){
    //     fprintf(stderr, "Error: could not read.\n");
    //     close(socket_fd);
    //     freeaddrinfo(result);
    //     return 1;
    // }
    /**
     * TO-DO
     * We have read and send, 
     * --we need to receive and write to standard output--
     * Reading and writing needs to be done at the same time using select which is the next step
    */

    fd_set read_fds; 

    int max_fds = socket_fd > STDIN_FILENO ?  socket_fd : STDIN_FILENO; 
    int ready;
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(socket_fd, &read_fds);
        ready = select(max_fds + 1, &read_fds, NULL, NULL, NULL);
        if (ready < 0){
            message = "Error: select failed\n";
            better_write(1, message, strlen(message));
            res = 1;
            goto end;
        }
        if (FD_ISSET(STDIN_FILENO, &read_fds)){
            
            if (reading_and_sending(socket_fd, buf_in, INPUT_BUFF_SIZE)==1){
                message = "Error: an error occurred when reading and sending\n";
                better_write(1, message, strlen(message));
                res = 1;
                goto end;
            }   
        }
        ssize_t recv_length;
        if (FD_ISSET(socket_fd, &read_fds)){
            // recv_length = recv(socket_fd, buf_recv, sizeof(buf_recv), 0);
            recv_length = recv(socket_fd, buf_recv, RECV_BUFF_SIZE, 0);
            if (recv_length < 0){
                fprintf(stderr, "Error: could not receive from client.\n");
                res = 1;
                goto end;
            }
            if (recv_length == 0){
                break;
            }
            /*Once we read, we write to standard output with better write*/
            if (better_write(1, buf_recv, recv_length) < 0){
                fprintf(stderr, "Error: could not write.\n");
                close(socket_fd);
                res = 1;
                goto end;
            }
        }
    }

    end:

    close(socket_fd);
    freeaddrinfo(result);
    return res;
}

int reading_and_sending(int socket_fd, char *buf, int buff_size){
    ssize_t read_res;
    ssize_t bytes_sent;
    while ((read_res = read(0, buf, buff_size)) > 0){
        if (read_res < ((ssize_t) 0)) {
        /* There has been an error on read() */
            fprintf(stderr, "Error using read: %s\n", strerror(errno));
            return 1;
        }

        bytes_sent = send(socket_fd, buf, read_res, 0);
        if ( bytes_sent < 0) {
            /* There has been an error on send() */
            fprintf(stderr, "Error using send: %s\n",strerror(errno));
            close(socket_fd);
            return 1;
        }
    }
    bytes_sent = send(socket_fd, "", 0, 0);
    if ( bytes_sent < 0) {
        /* There has been an error on send() */
        fprintf(stderr, "Error using send: %s\n",strerror(errno));
        close(socket_fd);
        return 1;
    }
    // close(socket_fd);
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