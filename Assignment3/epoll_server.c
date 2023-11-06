#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdint.h>
#include <time.h>

#define SERVER_PORT 8080
#define BACKLOG 4000
#define MAX_EVENTS 4000
#define BUFFER_SIZE 1024

long long factorial(long long n) {
    long long result = 1;
    for (long long i = 1; i <= n; i++) {
        result *= i;
    }
    return result;
}

int make_socket_non_blocking(int sfd) {
    int flags, s;

    flags = fcntl(sfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl(sfd, F_SETFL, flags);
    if (s == -1) {
        perror("fcntl");
        return -1;
    }

    return 0;
}

int main() {
    int listener, new_sock, epoll_fd, s;
    socklen_t addrlen;
    struct sockaddr_in server_addr, client_addr;
    struct epoll_event event, events[MAX_EVENTS];
    int optval = 1;

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    s = make_socket_non_blocking(listener);
    if (s == -1) {
        exit(EXIT_FAILURE);
    }

    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0) {
        perror("setsockopt error");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("10.0.2.4"); // Make sure to use the correct IP address
    server_addr.sin_port = htons(SERVER_PORT);
    memset(&(server_addr.sin_zero), '\0', 8);

    if (bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind error");
        exit(EXIT_FAILURE);
    }

    if (listen(listener, BACKLOG) < 0) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    event.data.fd = listener;
    event.events = EPOLLIN | EPOLLET; // Read operation | Edge-triggered behavior
    s = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener, &event);
    if (s == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    // Buffer where events are returned
    events[MAX_EVENTS];

    // The event loop
    while (1) {
        int n, i;

        n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (i = 0; i < n; i++) {
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) ||
                (!(events[i].events & EPOLLIN))) {
                // An error has occured on this fd, or the socket is not ready for reading
                fprintf(stderr, "epoll error\n");
                close(events[i].data.fd);
                continue;
            } else if (listener == events[i].data.fd) {
                // We have a notification on the listening socket, which means one or more incoming connections.
                while (1) {
                    addrlen = sizeof(client_addr);
                    new_sock = accept(listener, (struct sockaddr *)&client_addr, &addrlen);
                    if (new_sock == -1) {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                            // We have processed all incoming connections.
                            break;
                        } else {
                            perror("accept");
                            break;
                        }
                    }

                    s = make_socket_non_blocking(new_sock);
                    if (s == -1) {
                        abort();
                    }

                    event.data.fd = new_sock;
                    event.events = EPOLLIN | EPOLLET; // Read operation | Edge-triggered behavior
                    s = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_sock, &event);
                    if (s == -1) {
                        perror("epoll_ctl");
                        abort();
                    }
                }
                continue;
            } else {
                // We have data on the fd waiting to be read. Read and display it.
                int done = 0;

                while (1) {
                    ssize_t count;
                    char buf[BUFFER_SIZE];

                    count = read(events[i].data.fd, buf, sizeof buf);
                    if (count == -1) {
                        // If errno == EAGAIN, that means we have read all data.
                        if (errno != EAGAIN) {
                            perror("read");
                            done = 1;
                        }
                        break;
                    } else if (count == 0) {
                        // End of file. The remote has closed the connection.
                        done = 1;
                        break;
                    }

                    // Write the buffer to standard output
                    s = write(1, buf, count);
                    if (s == -1) {
                        perror("write");
                        abort();
                    }

                    // Echo the data back to the client
                    s = send(events[i].data.fd, buf, count, 0);
                    if (s == -1) {
                        perror("send");
                        abort();
                    }
                }

                if (done) {
                    printf("Closed connection on descriptor %d\n", events[i].data.fd);

                    // Closing the descriptor will make epoll remove it from the set of descriptors which are monitored.
                    close(events[i].data.fd);
                }
            }
        }
    }

    close(listener);
    return EXIT_SUCCESS;
}
