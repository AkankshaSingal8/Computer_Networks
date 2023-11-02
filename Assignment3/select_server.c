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

#define NUM_SIZE 4000

#define SERVER_PORT 8080
#define BACKLOG 10

long long factorial(long long n){

	unsigned long long ans = 1;
	for (int i = 1 ; i < n + 1 ; i++){
		ans *= i;
	}
	return ans;

}

int main ()
{
    int listener = 1;
    socklen_t length;
    struct sockaddr_in server_address, client_address;
    char recv_message[100];


    listener = socket (AF_INET, SOCK_STREAM, 0);
    if (listener < 0){
        perror("socket error");
        exit(1);
    }

    memset(&server_address, '\0', sizeof(server_address));
	server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
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


    socklen_t addrlen;
    fd_set fds, readfds;
    FD_ZERO(&fds);
    FD_SET(listener, &fds);
    int fdmax = listener;
    int newsocket;

    while(1){

        readfds = fds;

        if( select(fdmax + 1 , &readfds, NULL, NULL, NULL) == -1){
            perror("select error");
        }
        
        for( int fd = 0; fd < (fdmax + 1); fd++){
            
            if( FD_ISSET( fd, &readfds)){  

                if( fd == listener){
                    newsocket = accept(listener, (struct sockaddr*)&client_address, &addrlen);
                    if(newsocket < 0){
                        exit(1);
                    }

                    char *IP = inet_ntoa(client_address.sin_addr);
                    int PORT_NO = ntohs(client_address.sin_port);
                    

                    
                    printf("Connection accepted from IP : %s: and PORT : %d\n", IP, PORT_NO);

                    FD_SET(newsocket, &fds);
                    if( newsocket > fdmax){
                        fdmax = newsocket;
                    }

                }
                else{  
                    memset(recv_message, 0, 100);

                    ssize_t numbytes = recv( fd, &recv_message, sizeof(recv_message), 0);
                    
                    long long num = atoi(recv_message);
                    
                    send(fd, &recv_message, sizeof(recv_message), 0);

                }
            }
        }
        
    }
    return 0;
}
