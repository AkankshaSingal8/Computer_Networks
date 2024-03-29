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

    int listener, new_sock, epoll_fd, s;
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
    struct epoll_event event, events[MAX_CLIENTS];
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    event.data.fd = listener;
    event.events = EPOLLIN | EPOLLET; 
    s = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener, &event);
    if (s == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    
    events[MAX_CLIENTS];

    while (1) {
        int n, i;

        n = epoll_wait(epoll_fd, events, MAX_CLIENTS, -1);
        for (i = 0; i < n; i++) {
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) ||
                (!(events[i].events & EPOLLIN))) {
                
                fprintf(stderr, "epoll error\n");
                close(events[i].data.fd);
                continue;
            } else if (listener == events[i].data.fd) {
                
                while (1) {
                    addrlen = sizeof(client_address);
                    new_sock = accept(listener, (struct sockaddr *)&client_address, &addrlen);
                    if (new_sock == -1) {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                            
                            break;
                        } else {
                            perror("accept");
                            break;
                        }
                    }

                    if (fcntl(listener, F_SETFL, O_NONBLOCK) < 0) {
                        perror("fcntl O_NONBLOCK failed");
                        exit(EXIT_FAILURE);
                    }


                    event.data.fd = new_sock;
                    event.events = EPOLLIN | EPOLLET;
                    s = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_sock, &event);
                    if (s == -1) {
                        perror("epoll_ctl");
                        abort();
                    }
                }
                continue;

            }
            else {
                
                bool done = false;
                while (1) {
                    char buf[BUFFER_SIZE] = {0};
                    ssize_t count = read(events[i].data.fd, buf, sizeof(buf) - 1); 
                    if (count == -1) {
                        if (errno != EAGAIN) {
                            perror("read error");
                            done = true;
                        }
                        break; 
                    } else if (count == 0) {
                        
                        done = true;
                        break;
                    }

                    
                    char *endptr;
                    unsigned long long int n = strtoull(buf, &endptr, 10);
                    if (endptr == buf) {
                        // No number found, send error message back to client
                        snprintf(buf, sizeof(buf), "Error: No valid number provided.\n");
                        count = strlen(buf);
                    } else {
                        // Calculate factorial and convert the result back to string
                        unsigned long long result = factorial(n);
                        snprintf(buf, sizeof(buf), "%llu\n", result);
                        count = strlen(buf);
                    }

                    
                    if (send(events[i].data.fd, buf, count, 0) == -1) {
                        perror("send error");
                        done = true;
                    }
                }

                if (done) {        
                    close(events[i].data.fd);
                }
            
            }
        }
    }

    close(listener);
    return EXIT_SUCCESS;
}
