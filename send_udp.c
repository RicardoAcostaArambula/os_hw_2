#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SIZE 460

int main(int argc, char ** argv){
    char *message, *server_name, *port_name;
    int socket_fd, gai_code;
    char buf[SIZE];
    ssize_t read_res;
    ssize_t bytes_sent;
    if (argc < 3){
        message = "Not enough arugments, expected: <server-name> <port>\n";
        better_write(1, message, sizeof(message));
        return 1;
    }
    server_name = argv[1];
    port_name = argv[2];
    struct addrinfo hints, *result;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    hints.ai_flags = 0;
    gai_code = getaddrinfo(server_name, port_name, &hints, &result);
    
    if (getaddrinfo(server_name, port_name, &hints, &result)){
        fprintf(stderr, "Error wiht getaddrinfo: %s\n", strerror(errno));
        return 1;
    }
    
    socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (socket_fd < 0){
        fprintf(stderr, "Error: could not create socket\n", strerror(errno));
        return 1;
    }
    /* we need to read from standard input and send the data to the server
     * instead of writing to standard input, we send to the socket_fd
     * do we need to connect after creating the socket?
    */



    while ((read_res = read(0, buf, sizeof(buf)) > 0)){
        if (read_res < ((ssize_t) 0)) {
        /* There has been an error on read() */
        fprintf(stderr, "Error using read: %s\n", strerror(errno));
        return 1;
        }

        bytes_sent = send(socket_fd, buf, sizeof(buf), 0);
        if ( bytes_sent < 0) {
            /* There has been an error on send() */
            fprintf(stderr, "Error using send: %s\n",strerror(errno));
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
    close(socket_fd);
    freeaddrinfo(result);

    







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
      return (ssize_t) already_written;
    }
    written_this_time = (size_t) res_write;
    already_written += written_this_time;
    to_be_written -= written_this_time;
  }
  return (ssize_t) already_written;
}