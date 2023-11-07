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
#define BACKLOG 4000
#define MAX_CLIENTS 4000
#define BUFFER_SIZE 1024

long long factorial(long long n){

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

    int fdmax, fd_new, numbytes;
    socklen_t addrlen;
    fd_set fds, readfds;
    FD_ZERO(&fds);
    FD_SET(listener, &fds);
    fdmax = listener; 
   
    while (1) {
        readfds = fds; 

        if (select(fdmax + 1, &readfds, NULL, NULL, NULL) == -1) {
            perror("select error");
            exit(EXIT_FAILURE);
        }
       
        for(int fd = 0; fd <= fdmax; fd++) {

            if (FD_ISSET(fd, &readfds)) {

                if (fd == listener) {
                    
                    addrlen = sizeof(client_address);
                    if ((fd_new = accept(listener, (struct sockaddr *)&client_address, &addrlen)) < 0) {
                        perror("accept error");
                    }
                    else {

                        FD_SET(fd_new, &fds);
                        if (fd_new > fdmax) {
                            fdmax = fd_new;
                        }
                        
                    }
                }
                else {
                    
                    if ((numbytes = recv(fd, buffer, sizeof(buffer), 0)) <= 0) {
                        close(fd);
                        FD_CLR(fd, &fds); 
                    }
                    else {
                        
                        long long num = atoll(buffer);
                        long long result;
                       
                        if (num > 20) { 
                            result = factorial(20);
                        }
                        else {
                            result = factorial(num);
                        }
                       
                        sprintf(buffer, "%llu", result);
                        if (send(fd, buffer, strlen(buffer), 0) == -1) {
                            perror("send error");
                        }
                    }
                }
            }
        }
    }
   
    return 0;
}


