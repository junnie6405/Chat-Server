/*
 * server.c - a chat server (and monitor) that uses pipes and sockets
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/file.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h> 
#include <sys/wait.h>
#include <netdb.h> 
#include <poll.h> 
#define MAX_CLIENTS 10
// constants for pipe FDs
#define WFD 1
#define RFD 0

/**
 * nonblock - a function that makes a file descriptor non-blocking
 * @param fd file descriptor
 */
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

 /*
 * monitor - provides a local chat window
 * @param srfd - server read file descriptor
 * @param swfd - server write file descriptor
 */

void monitor(int srfd, int swfd) {
	char buffer[1024]; 
	struct pollfd pfds[2]; 
	
	int poll_nums; 
	int rdv1; 
	int wrv1; 
	int rdv2; 
	int wrv2; 
	setbuf(stdout, NULL); 
	pfds[0].fd = STDIN_FILENO; 
	pfds[0].events = POLLIN; 
	pfds[1].fd = srfd; 
	pfds[1].events = POLLIN; 
	nonblock(pfds[0].fd); 
	nonblock(pfds[1].fd); 
	while(1){
		poll_nums = poll(pfds, 2, 0); // no time out. 
		
		if (poll_nums == - 1){
			perror("poll");
			exit(1); 
		}	
		
		else if (poll_nums == 0){
			continue;
		}
		
		if (pfds[0].revents & POLLIN){
			rdv1 = read(STDIN_FILENO, buffer, sizeof(buffer)); 
			if (rdv1 == -1){
				perror("Read error"); 
				exit(1); 
			}
			if (rdv1 == 0){ 
				printf("Server hanging up.\n"); 
				break;
			} 
			wrv1 = write(swfd, buffer, rdv1); 
			
			if (wrv1 == -1){
				perror("write error"); 
				exit(1); 
			}
		}
		else if (pfds[1].revents & POLLIN){
				rdv2 = read(srfd, buffer, sizeof(buffer)); 
				
				if (rdv2 == -1){
					perror("Read Error"); 
					exit(1); 
				}
				else if (rdv2 == 0){
					break;
				}
				wrv2 = write(STDOUT_FILENO, buffer, rdv2); 
				if (wrv2 == -1){
					perror("Write Error");
					exit(1); 
				}
		}
	}
	close(srfd); 
	close(swfd);  
}

void server(int mrfd, int mwfd, char *portno) {
	int status; 
	int current_client_fd;
	int listen_sfd; 
	int accept_sfd; 
	int monitor_rv; 
	int monitor_wrv;
        int client_rv;
	int client_wrv; 
	int backlog = 10; 
	int val = 1; // used for setsockopt  
	int fd_size = 12; 
	                  
	int poll_cnt; 
	char buffer[1024]; 
	struct addrinfo hints; 
	struct addrinfo *result; 
	struct sockaddr_storage new_addr; 
	socklen_t addr_size; 
	
	struct pollfd pollfds[12]; // 10 clients + pipe read fd + listen socket fd 
	setbuf(stdout, NULL); // so that printf is not buffered during the execution. 
	memset(&hints, 0, sizeof(hints)); 
	hints.ai_socktype = SOCK_STREAM; 
	hints.ai_family = AF_UNSPEC; 
	hints.ai_flags = AI_PASSIVE; 
	if ((getaddrinfo(NULL, portno, &hints, &result) != 0)){
		perror("Error:"); 
		exit(1);
	}
	listen_sfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listen_sfd < 0){
		perror("Socket creation Error"); 
		freeaddrinfo(result); 
		close(listen_sfd);
		exit(1);
	}	
	setsockopt(listen_sfd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val)); 
	
	if (bind(listen_sfd, result->ai_addr, result->ai_addrlen) != 0){
		perror("bind failed.\n"); 
		exit(1);
	} 
	if (listen(listen_sfd, backlog) != 0) { 
		perror("Failed to listen.\n"); 
		exit(1); 
	} 
	
	pollfds[0].fd = listen_sfd; 
	pollfds[0].events = POLLIN; 
	pollfds[1].fd = mrfd; 
	pollfds[1].events = POLLIN; 
	
	nonblock(pollfds[0].fd); 
	nonblock(pollfds[1].fd); 
	for (int i = 2; i <fd_size; i++){ 
		pollfds[i].fd = -1; // deactivate all the client fds before the main while loop 
	}
	while (1){
		
		poll_cnt = poll(pollfds, 12, 100); //time out: 0.1sec, pollfds size = 12      						
		if (poll_cnt == -1){ 
			perror("poll"); 
			exit(1);  
		} 
		else if (poll_cnt == 0){ 
			continue;  
		}  
		
		// now we have positive value for poll so lets see which poll fd is ready. Check listen fd and monitor fd first 
		
		if (pollfds[0].revents & POLLIN){           //accept and assign new slots. 
			addr_size = sizeof(new_addr); 
			accept_sfd = accept(listen_sfd, (struct sockaddr *)&new_addr, &addr_size);
			
			if (accept_sfd < 0){ 
				perror("accept error."); 
				exit(1);  
			} 
			
			for (int idx = 2; idx < fd_size; idx++){  // if fd is -1, we can use this slot. 
				if (pollfds[idx].fd == -1){ // change it to and if poll_cnt still > 0 	
					pollfds[idx].fd = accept_sfd; 
					pollfds[idx].events = POLLIN; 
					nonblock(pollfds[idx].fd); 
				        break; // no need to add anymore. 	
				}
			}
			
			poll_cnt -= 1; 
		}
		if (pollfds[1].revents & POLLHUP){ // to see if the server hung up. 
			
			for (int close_fd = 0; close_fd < fd_size; close_fd ++){
				close(pollfds[close_fd].fd); 
			}
			
			close(mwfd); 
			freeaddrinfo(result); 
			exit(1);
		}
		if (pollfds[1].revents & POLLIN){ // check if monitor is ready.
			
			monitor_rv = read(mrfd, buffer, sizeof(buffer)); 
			
			if (monitor_rv == -1){
				perror("Monitor Error");  
				exit(1);
			}
				
			// there is a message. Now write to all the active clients.

			for (int client = 2; client < fd_size; client++){ 
				
				if (pollfds[client].fd != -1){
					
					monitor_wrv = write(pollfds[client].fd, buffer, monitor_rv); // do error check for this.	
					if (monitor_wrv == -1){
						perror("Monitor Error"); 
						exit(1); 
					}
				}
			} 
			poll_cnt -= 1;
		}
		 
		if (poll_cnt > 0){        // find which client is set & ready 
			for (int c = 2; c < fd_size; c++){
				
				if (pollfds[c].fd != -1){ 
				
					if (pollfds[c].revents & POLLIN){ // active -> read  
						current_client_fd = c; 
						client_rv = read(pollfds[c].fd, buffer, sizeof(buffer)); 
						if (client_rv == -1){ 
							perror("Error");
							exit(1); 
						}
						
						else if (client_rv == 0){ // deactivate this client.
							pollfds[c].fd = -1; //inactive
							close(pollfds[c].fd); 
						}
						// write to monitor first.  
						client_wrv = write(mwfd, buffer, client_rv); 
						if (client_wrv == -1){ 
							perror("Error");
							exit(1); 
						}
						// write to clients now. 
						for (int index = 2; index < fd_size; index++){ 
							
							if ((index != current_client_fd) && (pollfds[index].fd != -1)){
								
								client_wrv = write(pollfds[index].fd, buffer, client_rv);
								if (client_wrv == -1){ 
									perror("Error"); 
									exit(1);	
								}	       
							}
						}
					}
				}
			}
		}
	}
}
int main(int argc, char **argv) {
	int server_to_monitor[2]; 
	int monitor_to_server[2]; 
	char * prt = "5000"; 
	pid_t pid; 
	int opt; 
	while ((opt = getopt(argc, argv, "hp:")) != -1){ 

		switch(opt){ 
			case 'h': 
				printf("help message."); 
				printf("./server [-h host] [-p port]\n"); 
			        printf("This is a helper message.\n"); 	
				return 0;

			case 'p': 
				prt = optarg; 
				break;  
		}  
	} 
	if (pipe(server_to_monitor) == -1){ 
		perror("pipe error"); 
		exit(1); 
	}	
	if (pipe(monitor_to_server) == -1){
		perror("pipe error"); 
		exit(1); 
	} 
	pid = fork(); 
	if (pid == -1){ 
		perror("fork"); 
		exit(1);
	} 
	
	else if (pid == 0){ 
		close(server_to_monitor[WFD]); 
		close(monitor_to_server[RFD]); 
		monitor(server_to_monitor[RFD], monitor_to_server[WFD]); 
	}
	else{ 
		close(monitor_to_server[WFD]); 
		close(server_to_monitor[RFD]); 
		server(monitor_to_server[RFD], server_to_monitor[WFD], prt); 
		wait(NULL); 
	} 
	return 0;
}