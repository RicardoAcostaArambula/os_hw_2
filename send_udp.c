#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

int main(int argc, char ** argv){
    char *message;
    int socket_fd;
    if (argc < 3){
        message = "Not enough arugments, expected: <server> <port name>";
        better_write(1, message, sizeof(message));
        return 1;
    }
    socket_fd = socket(AF_INET,SOCK_DGRAM, 0);
    if (socket_fd < 0){
        fprintf(stderr, "Error: could not create socket", strerror(errno));
        return 1;
    }
    

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