/*
 * client.c - a turn taking chat client
 */
 
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h> 
#include <poll.h> 
void nonblock(int fd) {
	
	int flags;
	if ((flags = fcntl(fd, F_GETFL, 0)) == -1) {
		perror("fcntl (get):");
		exit(1);
	}
	if (fcntl(fd, F_SETFL, flags | FNDELAY) == -1) {
		perror("fcntl (set):");
		exit(1);
	}
}
int main(int argc, char **argv) {
  	int status; 
  	int sockfd; 
	int opt; 
	int rdv1; 
	int rdv2; 
	int wrv1; 
	int wrv2; 
	int num_poll; 

	char * port = "5000"; 
     	char buffer[1024]; 
        char * host_name = "login02";	
        char * host_name = "login02";	 

	struct addrinfo hints; 
	struct addrinfo *result; 
	struct pollfd pollfds[2]; 
	setbuf(stdout, NULL); // for printf
	while ((opt = getopt(argc, argv, "h:p:")) != -1){ 
		switch(opt){
			case 'h': 
				host_name = optarg; 
				break; 

			case 'p':
				port = optarg; 
				break; 
				break;

			default: 
				printf("usage: ./server [-h] [-p port #]\n-h - this help message\n"); 
				printf("-p # - the port to use when connecting to the server\n");
		}	
	} 
	memset(&hints, 0, sizeof(hints)); 
	hints.ai_socktype = SOCK_STREAM; 
	hints.ai_family = AF_UNSPEC; 
						
	if ((getaddrinfo(host_name, port, & hints, & result)!= 0)) { 
		perror("getaddrinfo"); 
		exit(1);
	} 
	sockfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol); 
	if (sockfd < 0){ 
		perror("create socket error"); 
		exit(1); 
	}	
	if (connect(sockfd, result->ai_addr, result->ai_addrlen) != 0){ 
		perror("connection error"); 
		freeaddrinfo(result);  
		exit(1);
	} 
	
	printf("connected to the server...\n"); 
	
	pollfds[0].fd = STDIN_FILENO; //keyboard 
	pollfds[0].events = POLLIN;
	pollfds[1].fd = sockfd; //socket 
	pollfds[1].events = POLLIN;  
	
        nonblock(pollfds[0].fd); 	
	nonblock(pollfds[1].fd); 
	while(1){
		
		num_poll = poll(pollfds, 2, 0); // no time out. wait until you get the message. 
		if (num_poll == -1){
			perror("poll"); 
			exit(1);
		} 
		
		if (pollfds[0].revents & POLLIN){ 
			
			rdv1 = read(STDIN_FILENO, buffer, sizeof(buffer)); 
		       	
			if (rdv1 == -1){ 
				perror("Error");
				close(pollfds[1].fd); 	
				exit(1);
			}	
			
			else if (rdv1 == 0){ 
				printf("client hanging up.\n"); 	
				break;
			} 

			wrv1 = write(sockfd, buffer, rdv1); 

			if (wrv1 == -1){ 
				perror("error writing to socket.\n");  
				close(pollfds[1].fd); 
				exit(1); 
			}	
		} 
		else if (pollfds[1].revents & POLLIN){
			
			rdv2 = read(sockfd, buffer, sizeof(buffer)); 
			if (rdv2 == -1){ 
				perror("error: "); 
				close(pollfds[1].fd); 
				exit(1);
			} 
			
			else if (rdv2 == 0){ 
				printf("server hanging up.\n"); 
				break;
			} 
			wrv2 = write(STDOUT_FILENO, buffer, rdv2); 
			if (wrv2 == -1){ 
				perror("error writing it out.");  
				close(pollfds[1].fd); 
				exit(1); 
			}
		}	
	} 
			
	close(pollfds[1].fd);
	freeaddrinfo(result); 
  	return 0;
}
