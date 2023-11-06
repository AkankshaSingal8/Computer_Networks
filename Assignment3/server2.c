// include statements used from the link provided
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
#include <sys/select.h>
#include <ctype.h>
#include <stdint.h>
#include <time.h>
#include <fcntl.h>

#define SERVER_PORT 8080
#define MAX_CLIENTS 4000

long long factorial(long long n) {
    unsigned long long result = 1;
    for (int i = 1; i <= n; i++) {
        result *= i;
    }
    return result;
}

int main() {
    int listener, optval = 1;
    socklen_t length;
    struct sockaddr_in server_address, client_address;
    char recv_message[1024];

    listener = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0); // Set the socket to non-blocking mode
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    perror("accept error");
                }
            } else {
                char *ip_addr = inet_ntoa(client_address.sin_addr);
                int port = ntohs(client_address.sin_port);
                printf("Connection IP: %s and PORT: %d\n", ip_addr, port);
                FD_SET(fd_new, &fds);
                if (fd_new > fdmax) {
                    fdmax = fd_new;
                }

                // Set the new client socket to non-blocking mode
                if (fcntl(fd_new, F_SETFL, O_NONBLOCK) < 0) {
                    perror("fcntl error");
                }

                // Store the new client socket
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i] == 0) {
                        clients[i] = fd_new;
                        break;
                    }
                }
            }
        }

        // Handle data for existing connections
        for (int fd = 0; fd <= fdmax; fd++) {
            if (fd != listener && FD_ISSET(fd, &readfds)) {
                memset(&recv_message, 0, sizeof(recv_message));
                ssize_t numbytes = recv(fd, recv_message, sizeof(recv_message) - 1, 0);
                if (numbytes == -1) {
                    if (errno != EWOULDBLOCK && errno != EAGAIN) {
                        perror("recv error");
                    }
                } else if (numbytes == 0) {
                    // Connection closed by client
                    close(fd);
                    FD_CLR(fd, &fds);

                    // Remove the closed socket from the array
                    for (int i = 0; i < MAX_CLIENTS; i++) {
                        if (clients[i] == fd) {
                            clients[i] = 0;
                            break;
                        }
                    }
                } else {
                    long long num = atoll(recv_message);
                    if (num > 20) {
                        sprintf(recv_message, "%lld", factorial(20));
                    } else {
                        sprintf(recv_message, "%lld", factorial(num));
                    }
                    if (send(fd, recv_message, strlen(recv_message), 0) < 0) {
                        perror("send error");
                    }
                }
            }
        }
    }

    close(listener);
    return 0;
}
