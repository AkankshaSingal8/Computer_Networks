// include statements used from the link provided
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>
#include <time.h>
#include <fcntl.h>

#define SERVER_PORT 8080
#define BACKLOG 4000
#define MAX_CLIENTS 4000
#define BUFFER_SIZE 1024

unsigned long long factorial(long long n){

	unsigned long long result = 1;
	for (int i = 1 ; i < n + 1 ; i++){
		result *= i;
	}
	return result;

}

int main() {

    int listener;
    struct sockaddr_in server_address, client_address;
    char buffer[BUFFER_SIZE];
    int optval = 1;

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    if (fcntl(listener, F_SETFL, O_NONBLOCK) < 0) {
        perror("fcntl O_NONBLOCK failed");
        exit(EXIT_FAILURE);
    }


    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0) {
        perror("setsockopt error");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("10.0.2.4");
    server_address.sin_port = htons(SERVER_PORT);
    memset(&(server_address.sin_zero), '\0', 8);
   
    if (bind(listener, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("bind error");
        exit(EXIT_FAILURE);
    }

    if (listen(listener, BACKLOG) < 0) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    socklen_t addrlen;
    struct pollfd pollfds[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) {
        pollfds[i].fd = -1;
    }
    pollfds[0].fd = listener;
    pollfds[0].events = POLLIN;
    pollfds[0].revents = 0;
    int numbytes;

    
    while (1) {

        if (poll(pollfds, MAX_CLIENTS, -1) < 0) {
            perror("poll error");
            exit(EXIT_FAILURE);
        }

        for (int fd = 0; fd < MAX_CLIENTS; fd++) {
            if (pollfds[fd].fd < 0) continue;

            if (pollfds[fd].revents & POLLIN) {
                if (pollfds[fd].fd == listener) {
                    
                    addrlen = sizeof(client_address);
                    int fd_new = accept(listener, (struct sockaddr *)&client_address, &addrlen);
                    if (fd_new < 0) {
                        perror("accept error");
                    }
                    else {
                        
                        fcntl(fd_new, F_SETFL, O_NONBLOCK);
   
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (pollfds[j].fd < 0) {
                                pollfds[j].fd = fd_new;
                                pollfds[j].events = POLLIN;
                                break;
                            }
                        }
                        
                    }
                }
                else {
                    
                    memset(buffer, 0, BUFFER_SIZE);
                    
                    if ((numbytes = recv(pollfds[fd].fd, buffer, sizeof(buffer), 0)) <= 0) {
                        
                        close(pollfds[fd].fd);
                        pollfds[fd].fd = -1;

                    }
                    else {
                        
                        long long num = atoll(buffer);
                        unsigned long long result;
                       
                        if (num > 20) { 
                            result = factorial(20);
                        }
                        else {
                            result = factorial(num);
                        }
                       
                        sprintf(buffer, "%llu", result);
                        if (send(pollfds[fd].fd, buffer, strlen(buffer), 0) == -1) {
                            perror("send error");
                        }
                    }
                }
            }
        }
    }

    close(listener);
    return 0;
}

