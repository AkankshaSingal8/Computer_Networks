#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#define SERVER_PORT 8080
#define BACKLOG 10
#define BUFFER_SIZE 1024

long long factorial(long long n){
    
    long long result = 1;
    for (int i = 1; i <= n; ++i){
        result *= i;
    }
    return result;
}

int main() {
    int listener, new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen;
    char buffer[BUFFER_SIZE];

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == -1) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("10.0.2.4");
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind error");
        exit(EXIT_FAILURE);
    }

    if (listen(listener, BACKLOG) == -1) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on %d\n", SERVER_PORT);

    while (1) {
        addrlen = sizeof(client_addr);
        new_sock = accept(listener, (struct sockaddr *)&client_addr, &addrlen);
        if (new_sock == -1) {
            perror("accept error");
            continue;
        }

        int pid = fork();
        if (pid == -1) {
            perror("fork error");
            close(new_sock);
            continue;
        } else if (pid == 0) { // Child process
            close(listener); // Child doesn't need the listener
            ssize_t bytes_read;
            long long num, result;

            // Receive data from client
            while ((bytes_read = recv(new_sock, buffer, BUFFER_SIZE, 0)) > 0) {
                buffer[bytes_read] = '\0'; // Null-terminate the buffer
                num = atoll(buffer);
                if (num < 0 || num > 20) {
                    num = (num < 0) ? 0 : 20; // Clamp the value between 0 and 20
                }
                result = factorial(num);
                sprintf(buffer, "%lld", result);
                send(new_sock, buffer, strlen(buffer), 0);
            }

            if (bytes_read == -1) {
                perror("recv error");
            }

            close(new_sock);
            exit(0); // Terminate child process
        } else { // Parent process
            close(new_sock); // Parent doesn't need this socket
        }
    }
    // Close the listener in parent (not reached in this example)
    close(listener);
    return 0;
}
