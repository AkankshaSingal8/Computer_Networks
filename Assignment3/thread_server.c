#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>

#define SERVER_PORT 8080
#define BACKLOG 4000
#define BUFFER_SIZE 2048

long long factorial(long long n){
    
    long long result = 1;
    for (int i = 1; i <= n; ++i){
        result *= i;
    }
    return result;
}

void *handle_client(void *arg) {
    int client_sock = *(int*)arg;
    free(arg); // Free the allocated memory for the integer pointer

    char buffer[BUFFER_SIZE];
    ssize_t nbytes;

    // Read data from client
    while ((nbytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[nbytes] = '\0'; // Null-terminate the received data
        long long num = atoll(buffer);
        long long result;

        // Calculate factorial, with boundary checks
        if (num < 0) {
            strcpy(buffer, "Error: Negative number\n");
        } else {
            result = factorial(num);
            snprintf(buffer, BUFFER_SIZE, "%lld\n", result);
        }

        // Send response to client
        if (send(client_sock, buffer, strlen(buffer), 0) == -1) {
            perror("send error");
        }
    }

    if (nbytes == -1) {
        perror("recv error");
    }

    close(client_sock); // Close client connection
    return NULL;
}

int main() {

    int listener, *new_sock;
    struct sockaddr_in server_address, client_address;
    socklen_t addrlen;
    char buffer[BUFFER_SIZE];

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }
    
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_addr.s_addr = inet_addr("10.0.2.4");
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(SERVER_PORT);

    if (bind(listener, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("bind error");
        exit(EXIT_FAILURE);
    }

    if (listen(listener, BACKLOG) == -1) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    

    while (1) {
        addrlen = sizeof(client_address);
        new_sock = malloc(sizeof(int)); 
        if (!new_sock) {
            perror("malloc error");
            continue;
        }

        *new_sock = accept(listener, (struct sockaddr *)&client_address, &addrlen);
        if (*new_sock < 0) {
            perror("accept error");
            free(new_sock);
            continue;
        }

        
        if (pthread_create(&thread_id, NULL, handle_client, new_sock) != 0) {
            perror("pthread_create error");
            close(*new_sock);
            free(new_sock);
            continue;
        }

        pthread_detach(thread_id); 
    }

   
    close(listener);
    return 0;
}
