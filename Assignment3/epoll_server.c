// include statements used from the link provided
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

#define SERVER_PORT 8080
#define BACKLOG 4000
#define MAX_EVENTS 4000

long long factorial(long long n){

	unsigned long long result = 1;
	for (int i = 1 ; i < n + 1 ; i++){
		result *= i;
	}
	return result;

}

int main (){
    int listener, optval = 1;
    socklen_t length;
    struct sockaddr_in server_address, client_address;
    char recv_message[1024];
    
	
    listener = socket (AF_INET, SOCK_STREAM, 0);
    if (listener < 0){
        perror("socket error");
        exit(1);
    }
    
    if (setsockopt (listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (int)) < 0){
        perror("setsockopt error");
        exit(1);
    }
           

    memset(&server_address, '\0', sizeof(server_address));
	server_address.sin_addr.s_addr = inet_addr("10.0.2.4");
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(SERVER_PORT);


	int bind_val = bind(listener, (struct sockaddr*)&server_address, sizeof(server_address));
	if (bind_val < 0){
        perror("bind error");
        exit(1);
    }

    if(listen(listener, BACKLOG) < 0){
        perror("listen erro\n");
    }

    int efd;
    if ((efd = epoll_create1 (0)) == -1){
        perror ("epoll_create1 error");
    }
	
    struct epoll_event ev, ep_event [4001];

    ev.events = EPOLLIN;
    ev.data.fd = listener;
    if (epoll_ctl (efd, EPOLL_CTL_ADD, listener, &ev) == -1){
        perror ("epoll_ctl error");
    }

    int nfds = 0;

    socklen_t addrlen;

    while (1) {
        
        if ((nfds = epoll_wait (efd, ep_event, 4001,  -1)) == -1){
            perror ("epoll_wait error");
        }
            

        for (int i = 0; i < nfds; i++) {

            if 	((ep_event [i].events & EPOLLIN) == EPOLLIN) {
                
                if (ep_event [i].data.fd == listener) {
                    addrlen = sizeof (client_address);
                    int fd_new;
                    fd_new = accept(listener, (struct sockaddr*)&client_address, &addrlen);
                    if(fd_new < 0){
                        perror("accept error");
                        exit(1);
                    }

                    ev.events = EPOLLIN;
                    ev.data.fd = fd_new;
                    if( epoll_ctl( efd, EPOLL_CTL_ADD, fd_new, &ev) == -1){
                        perror("epoll_ctl error");
                    }

                    char *ip_addr = inet_ntoa(clienAddr.sin_addr);
                    int port = ntohs(clienAddr.sin_port);
                    printf("Connection IP : %s: and PORT : %d\n", ip_addr, port);


                }
                else{
                    
                    memset(&recv_message, '\0', 1024);
                    ssize_t numbytes = recv (ep_event [i].data.fd, &recv_message, sizeof (struct message), 0);

                    if (numbytes == -1){
                        perror("recv error");
                        exit(1);
                    }
                    long long num = atoi(recv_message);
                    if (num > 20){
                        sprintf(recv_message, "%lld", factorial(20));
                        
                    }
                    else{
                        sprintf(recv_message, "%lld", factorial(num));
                    }
                    if (send(fd, &recv_message, sizeof(recv_message), 0) < 0){
                        perror("send error");
                        exit(1);
                    }


                }
            
            }
        }
    }

    close(listener);
    return 0;

}