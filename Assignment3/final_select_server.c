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
#define MAX_CLIENTS 4000  // Define max clients
#define BUFFER_SIZE 1024

long long factorial(long long n){

	unsigned long long result = 1;
	for (int i = 1 ; i < n + 1 ; i++){
		result *= i;
	}
	return result;

}

int main() {
    int listener, new_sock, fdmax, i, nbytes;
    socklen_t addrlen;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    fd_set master_fds, read_fds;
    int optval = 1;
   
    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    // Set listener socket to non-blocking mode
    // Use fcntl to get and then set socket properties
    // ... (code to set non-blocking mode)
    if (fcntl(listener, F_SETFL, O_NONBLOCK) < 0) {
        perror("fcntl O_NONBLOCK failed");
        exit(EXIT_FAILURE);
    }


    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0) {
        perror("setsockopt error");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("10.0.2.4");
    server_addr.sin_port = htons(SERVER_PORT);
    memset(&(server_addr.sin_zero), '\0', 8);
   
    if (bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind error");
        exit(EXIT_FAILURE);
    }

    if (listen(listener, BACKLOG) < 0) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    // Set up the master file descriptor set
    FD_ZERO(&master_fds);
    FD_SET(listener, &master_fds);
    fdmax = listener; // Initially, the listener has the highest file descriptor value
   
    while (1) {
        read_fds = master_fds; // copy it

        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select error");
            exit(EXIT_FAILURE);
        }
       
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof(client_addr);
                    if ((new_sock = accept(listener, (struct sockaddr *)&client_addr, &addrlen)) < 0) {
                        perror("accept error");
                    } else {
                        FD_SET(new_sock, &master_fds); // add to master set
                        if (new_sock > fdmax) { // keep track of the max
                            fdmax = new_sock;
                        }
                        printf("New connection from %s on socket %d\n",
                            inet_ntoa(client_addr.sin_addr), new_sock);
                    }
                } else {
                    // handle data from a client
                    if ((nbytes = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
                        
                        close(i); // bye!
                        FD_CLR(i, &master_fds); // remove from master set
                    } else {
                        // we got some data from a client
                        long long num = atoll(buffer);
                        long long result;
                       
                        if (num > 20 || num < 0) { // Cap factorial to 20 to avoid overflow or negative input
                            result = factorial(20);
                        } else {
                            result = factorial(num);
                        }
                       
                        sprintf(buffer, "%lld", result);
                        if (send(i, buffer, strlen(buffer), 0) == -1) {
                            perror("send error");
                        }
                    }
                }
            }
        }
    }
   
    return 0; // This won't happen as the server is in a while(1) loop
}


