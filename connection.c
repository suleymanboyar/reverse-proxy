/*
 * This is a file that implements the operation on TCP sockets that are used by
 * all of the programs used in this assignment.
 *
 * *** YOU MUST IMPLEMENT THESE FUNCTIONS ***
 *
 * The parameters and return values of the existing functions must not be
 * changed. You can add function, definition etc. as required.
 */
#include "connection.h"
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int backlog = 3;

// initializes a socket for the host machine and returns this socket, if there
// is an error -1 is returned.
int tcp_connect(char *hostname, int port) {
    // convert IP-address from string-format to binary form
    struct in_addr ip_addr;
    struct sockaddr_in addr;
    int ret, sock;
    // converts hostname string into something sockaddr_in can handle
    if ((ret = inet_pton(AF_INET, hostname, &ip_addr)) == -1) {
        perror("inet_pton()");
        return -1;
    }
    if (!ret) {
        fprintf(stderr, "inet_pton(): Invalid IP address %s\n", hostname);
        return -1;
    }

    // creates the socket FD
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket()");
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = ip_addr;

    // initializes the created socket with correct ipaddress and port number
    if (connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
        perror("connect()");
        return -1;
    }

    return sock;
}

// reads n bytes from socket sock into to buffer, adds a null byte at the end of
// the last read byte. If there is an error errno is printed and -1 is returned.
int tcp_read(int sock, char *buffer, int n) {
    int ret = read(sock, buffer, n);
    if (ret == -1) {
        perror("read()");
        return -1;
    }

    buffer[ret] = '\0';
    return ret;
}

// writes
int tcp_write(int sock, char *buffer, int bytes) {
    int ret = write(sock, buffer, bytes);
    if (ret == -1)
        perror("write()");

    return ret;
}

int tcp_write_loop(int sock, char *buffer, int bytes) {
    int written_bytes = 0;
    while (written_bytes < bytes) {
        if ((written_bytes = write(sock, buffer + written_bytes, bytes - written_bytes)) == -1) {
            perror("write()");
            return -1;
        }
    }

    return written_bytes;
}

void tcp_close(int sock) {
    close(sock);
}

/*Creates a new socket in listening mode.
  Returns a file descriptor for the new socket, or -1 for errors and prints the errno to stderr.*/
int tcp_create_and_listen(int port) {
    int sock;
    struct sockaddr_in addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket()");
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1)
        perror("bind()");

    if (listen(sock, backlog) == -1)
        perror("listen()");

    return sock;
}

/*Accepts the a new connection on socket server_sock.
  On success return a FD for the accepted connection. On error, return -1 and
  print errno to stderr.*/
int tcp_accept(int server_sock) {
    int ret;
    if ((ret = accept(server_sock, NULL, NULL)) == -1)
        perror("accept()");
    return ret;
}

int tcp_wait(fd_set *waiting_set, int wait_end) {
    int ret = select(wait_end, waiting_set, NULL, NULL, NULL);
    if (ret < 0) {
        perror("select()");
        return -1;
    }

    return ret;
}

int tcp_wait_timeout(fd_set *waiting_set, int wait_end, int seconds) {
    struct timeval tv = {.tv_sec = seconds, .tv_usec = 0};
    int ret = select(wait_end, waiting_set, NULL, NULL, &tv);
    if (ret < 0) {
        perror("select()");
        return -1;
    }

    return ret;
}
