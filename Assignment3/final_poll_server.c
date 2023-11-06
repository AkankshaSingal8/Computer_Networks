#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

#define SERVER_PORT 8080
#define BACKLOG 4000
#define MAX_CLIENTS 4000
#define BUFFER_SIZE 1024

long long factorial(long long n) {
    if (n > 20) return 0; // Factoring numbers greater than 20 could cause overflow, handle as error case.
    unsigned long long result = 1;
    for (int i = 2; i <= n; ++i) {
        result *= i;
    }
    return result;
}

int main() {
    int listener;
    struct sockaddr_in server_addr;
    struct pollfd client_fds[MAX_CLIENTS];
    char buffer[BUFFER_SIZE];
    int optval = 1;

    // Create listening socket
    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    // Set listener socket to non-blocking mode
    // Use fcntl to get and then set socket properties
    // ... (code to set non-blocking mode)
    if (fcntl(listener, F_SETFL, O_NONBLOCK) < 0) {
        perror("fcntl O_NONBLOCK failed");
        exit(EXIT_FAILURE);
    }


    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0) {
        perror("setsockopt error");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("10.0.2.4");
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

    // Initialize the array of pollfds
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i].fd = -1;
    }
    client_fds[0].fd = listener;
    client_fds[0].events = POLLIN;

    // Main loop
    while (1) {
        int poll_count = poll(client_fds, MAX_CLIENTS, -1);
        if (poll_count < 0) {
            perror("poll error");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_fds[i].fd < 0) continue;

            if (client_fds[i].revents & POLLIN) {
                if (client_fds[i].fd == listener) {
                    // New connection
                    struct sockaddr_in client_addr;
                    socklen_t addrlen = sizeof(client_addr);
                    int new_sock = accept(listener, (struct sockaddr *)&client_addr, &addrlen);
                    if (new_sock < 0) {
                        perror("accept error");
                    } else {
                        // Set the new socket to non-blocking
                        fcntl(new_sock, F_SETFL, O_NONBLOCK);

                        // Add to client_fds
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (client_fds[j].fd < 0) {
                                client_fds[j].fd = new_sock;
                                client_fds[j].events = POLLIN;
                                break;
                            }
                        }
                        printf("New connection from %s on socket %d\n",
                               inet_ntoa(client_addr.sin_addr), new_sock);
                    }
                } else {
                    // Handle data from client
                    memset(buffer, 0, BUFFER_SIZE);
                    int nbytes = recv(client_fds[i].fd, buffer, sizeof(buffer), 0);
                    if (nbytes <= 0) {
                        // Error or connection closed
                        close(client_fds[i].fd);
                        client_fds[i].fd = -1; // Mark as available slot
                    } else {
                        // Process data
                        long long num = atoll(buffer);
                        long long result = factorial(num);

                        snprintf(buffer, BUFFER_SIZE, "%lld", result);
                        send(client_fds[i].fd, buffer, strlen(buffer), 0);
                    }
                }
            }
        }
    }

    // Close the listening socket
    close(listener);
    return 0;
}

