#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/epoll.h>
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
#define MAX_CONNECTIONS 4000
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

    int bind_val = bind(listener, (struct sockaddr*)&server_address, sizeof(server_address));
	if (bind_val < 0){
        perror("bind error");
        exit(1);
    }

    if (listen(listener, BACKLOG) < 0) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    int efd;
    socklen_t addrlen;
    int nfds = 0;
    struct epoll_event ev, ep_event [MAX_CONNECTIONS];
    
    if ((efd = epoll_create1(0)) == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    ev.data.fd = listener;
    ev.events = EPOLLIN ; 
    
    if (epoll_ctl(efd, EPOLL_CTL_ADD, listener, &ev) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }


    while (1) {
        int i;

        nfds = epoll_wait(efd, ep_event, MAX_CONNECTIONS, -1);
        for (int i = 0; i < nfds; i++) {
            if ((ep_event [i].events & EPOLLIN) == EPOLLIN) {
                
                if (listener == ep_event[i].data.fd) {
                
                    addrlen = sizeof(client_address);
                    int fd_new;
                    fd_new = accept(listener, (struct sockaddr *)&client_address, &addrlen);
                    if (fd_new == -1) {
                        perror("accept");
                    }

                    if (fcntl(listener, F_SETFL, O_NONBLOCK) < 0) {
                        perror("fcntl O_NONBLOCK failed");
                        exit(EXIT_FAILURE);
                    }


                    ev.data.fd = fd_new;
                    ev.events = EPOLLIN ;
                    if (epoll_ctl(efd, EPOLL_CTL_ADD, fd_new, &ev) == -1) {
                        perror("epoll_ctl");
                        abort();
                    }
                

            }
            else {
                
                memset (&buffer, '\0', sizeof (buffer));
                ssize_t numbytes = recv (ep_event[i].data.fd, &buffer, sizeof (buffer), 0);
   
                if (numbytes == -1){
                    perror ("recv");
                    exit(EXIT_FAILURE);
                }
                    
                else if (numbytes == 0) {
                    if (epoll_ctl (efd, EPOLL_CTL_DEL, ep_event [i].data.fd, &ev) == -1){
                        perror ("epoll_ctl");
                        exit(EXIT_FAILURE);
                    }
                    
                    if (close (ep_event [i].data.fd) == -1){
                        perror ("close");
                        exit(EXIT_FAILURE);
                    }
                
                }

                else 
                {
                    long long num = atoll(buffer);
                    unsigned long long result;
                    
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

    close(listener);
    return EXIT_SUCCESS;
}
