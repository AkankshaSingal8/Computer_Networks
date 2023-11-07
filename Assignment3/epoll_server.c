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
                    event.events = EPOLLIN | EPOLLET; // Read operation | Edge-triggered behavior
                    s = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_sock, &event);
                    if (s == -1) {
                        perror("epoll_ctl");
                        abort();
                    }
                }
                continue;
            }
            else {
                
                int done = 0;

                while (1) {
                    ssize_t count;
                    char buf[BUFFER_SIZE];

                    count = read(events[i].data.fd, buf, sizeof buf);
                    if (count == -1) {
                        // If errno == EAGAIN, that means we have read all data.
                        if (errno != EAGAIN) {
                            perror("read");
                            done = 1;
                        }
                        break;
                    } else if (count == 0) {
                        // End of file. The remote has closed the connection.
                        done = 1;
                        break;
                    }

                    // Write the buffer to standard output
                    s = write(1, buf, count);
                    if (s == -1) {
                        perror("write");
                        abort();
                    }

                    // Echo the data back to the client
                    s = send(events[i].data.fd, buf, count, 0);
                    if (s == -1) {
                        perror("send");
                        abort();
                    }
                }

                if (done) {
                    printf("Closed connection on descriptor %d\n", events[i].data.fd);

                    // Closing the descriptor will make epoll remove it from the set of descriptors which are monitored.
                    close(events[i].data.fd);
                }
            }
        }
    }

    close(listener);
    return EXIT_SUCCESS;
}
