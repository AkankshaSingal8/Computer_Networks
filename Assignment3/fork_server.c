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

    int listener;
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
        int new = accept(listener, (struct sockaddr *)&client_address, &addrlen);
        if (new < 0) {
            perror("accept error");
            continue;
        }

        int pid = fork();
        if (pid == -1) {
            perror("fork error");
            close(new);
            
        } 
        else if (pid == 0) {
            close(listener);
            ssize_t numbytes;

            while ((numbytes = recv(new, buffer, BUFFER_SIZE, 0)) > 0) {
                
                long long num = atoll(buffer);
                            
                if (num > 20){
                    sprintf(buffer, "%lld", factorial(20));
                    
                }
                else{
                    sprintf(buffer, "%lld", factorial(num));
                }
                
                
                if (send(new, &buffer, sizeof(buffer), 0) < 0){
                    perror("send error");
                    exit(1);
                }
            }
            if (numbytes == -1){
                perror("recv error");
            
            }
            
            close(new);
            exit(0); 
        }
        else { 
            close(new); 
        }
    }
    
    close(listener);
    return 0;
}
