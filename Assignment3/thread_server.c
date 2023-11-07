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

unsigned long long factorial(long long n){
    
    unsigned long long result = 1;
    for (int i = 1; i <= n; ++i){
        result *= i;
    }
    return result;
}

void *thread_function(void *arg) {
    int new = *(int*)arg;
    free(arg); 

    char buffer[BUFFER_SIZE];
    ssize_t numbytes;

    
    while ((numbytes = recv(new, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[numbytes] = '\0';
        
        long long num = atoll(buffer);
        unsigned long long result;

        
        if (num > 20) { 
        	result = factorial(20);
        }
        else {
        	result = factorial(num);
        }
                       
        sprintf(buffer, "%llu", result);

        
        if (send(new, buffer, strlen(buffer), 0) == -1) {
            perror("send error");
        }
    }

    if (numbytes == -1) {
        perror("recv error");
    }

    close(new); 
    return NULL;
}

int main() {

    int listener;
    struct sockaddr_in server_address, client_address;
    

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }
    
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_addr.s_addr = inet_addr("10.0.2.4");
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(SERVER_PORT);

    if (bind(listener, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("bind error");
        exit(EXIT_FAILURE);
    }

    if (listen(listener, BACKLOG) < 0) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    int *new;
    socklen_t addrlen;

    while (1) {
        addrlen = sizeof(client_address);
        new = malloc(sizeof(int)); 
        if (!new) {
            perror("malloc error");
            continue;
        }

        *new = accept(listener, (struct sockaddr *)&client_address, &addrlen);
        if (*new < 0) {
            perror("accept error");
            free(new);
            continue;
        }

        pthread_t pid;
        
        if (pthread_create(&pid, NULL, thread_function, new) != 0) {
            perror("pthread_create error");
            close(*new);
            free(new);
            continue;
        }

        pthread_detach(pid); 
    }

   
    close(listener);
    return 0;
}
