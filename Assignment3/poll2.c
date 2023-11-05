#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

#define SERVER_PORT 8080
#define BACKLOG 4000

long long factorial(long long n) {
    unsigned long long result = 1;
    for (int i = 1; i < n + 1; i++) {
        result *= i;
    }
    return result;
}

int main() {
    int listener, optval = 1;
    struct sockaddr_in server_address, client_address;
    char recv_message[1024];

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("socket error");
        exit(1);
    }

    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0) {
        perror("setsockopt error");
        exit(1);
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_addr.s_addr = inet_addr("10.0.2.4");
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);

    int bind_val = bind(listener, (struct sockaddr*)&server_address, sizeof(server_address));
    if (bind_val < 0) {
        perror("bind error");
        exit(1);
    }

    if (listen(listener, BACKLOG) < 0) {
        perror("listen error");
        exit(1);
    }

    struct pollfd fds[BACKLOG + 1]; 
    memset(fds, 0, sizeof(fds));
    fds[0].fd = listener;
    fds[0].events = POLLIN;

    int nfds = 1;

    while (1) {
        int poll_res = poll(fds, nfds, -1);

        if (poll_res == -1) {
            perror("poll error");
            exit(1);
        }

        for (int i = 0; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                if (fds[i].fd == listener) {
                    socklen_t addrlen = sizeof(client_address);
                    int fd_new = accept(listener, (struct sockaddr*)&client_address, &addrlen);
                    if (fd_new < 0) {
                        perror("accept error");
                        exit(1);
                    }

                    char *ip_addr = inet_ntoa(client_address.sin_addr);
                    int port = ntohs(client_address.sin_port);
                    printf("Connection IP: %s, PORT: %d\n", ip_addr, port);

                    if (nfds < BACKLOG) {
                        fds[nfds].fd = fd_new;
                        fds[nfds].events = POLLIN;
                        nfds++;
                    } else {
                        fprintf(stderr, "Too many connections.\n");
                        close(fd_new);
                    }
                } else {
                    ssize_t numbytes = recv(fds[i].fd, recv_message, sizeof(recv_message), 0);
                    if (numbytes == -1) {
                        perror("recv error");
                        exit(1);
                    }

                    long long num = atoi(recv_message);
                    if (num > 20) {
                        sprintf(recv_message, "%lld", factorial(20));
                    } else {
                        sprintf(recv_message, "%lld", factorial(num));
                    }

                    if (send(fds[i].fd, recv_message, sizeof(recv_message), 0) < 0) {
                        perror("send error");
                        exit(1);
                    }
                }
            }
        }
    }

    close(listener);
    return 0;
}
