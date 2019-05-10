#include<err.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>
#include<errno.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

#define PORT 1234

int Socket(int family, int type, int protocol){
		int succ=socket(family, type, protocol);
		char *error;
		if(succ < 0){
				error=strerror(errno);
				printf("%s\n", error);
		} 
		return succ;
		
}

ssize_t Write(int fd, const void *vptr, size_t n){
		size_t num_left;
		ssize_t num_written;
		const char *ptr;
		
		ptr=vptr;
		num_left=n;
		
		while(num_left>0){
				if((num_written=write(fd, ptr, num_left))<=0){
						if(num_written < 0 && errno == EINTR){
								num_written=0;
						}
						else return -1;
				}
				
				num_left-=num_written;
				ptr+=num_written;
		}
		
		return n;
}

int main(int argc, char *argv[]){
	
		int pf;
		int opcije;
		int server_socket, peer_socket;
		
		struct sockaddr_in myaddr;
		struct addrinfo hints;
		
		char portnum[20];
	
		if(!(argc==1 || argc==3)){
				printf("Usage: ./tcpserver [-p port]\n");
				exit(1);
		}
		while((opcije=getopt(argc, argv, "p:"))!=-1){
				switch(opcije){
						case 'p':
							strcpy(portnum, optarg);
							pf=1;
							break;
						default:
							printf("Usage: ./tcpserver [-p port]\n");
							exit(1);
				}
		}
		int port;
		sscanf(portnum, "%d", &port);
		
		server_socket=Socket(PF_INET, SOCK_STREAM, 0);
		
		memset(&hints, 0, sizeof(hints));
		hints.ai_family=AF_INET;
		hints.ai_socktype=SOCK_STREAM;
		hints.ai_flags|=AI_CANONNAME;
		
		if(pf==0){
				myaddr.sin_port=htons(PORT);
				printf("Portnum as int: %d\n", PORT);

		} else {
				myaddr.sin_port=htons(port);
				printf("Portnum as int: %d\n", port);

		}
		myaddr.sin_family=AF_INET;
		myaddr.sin_addr.s_addr=INADDR_ANY;
		memset(myaddr.sin_zero, '\0', sizeof(myaddr.sin_zero));
		
		if(bind(server_socket, (struct sockaddr *)&myaddr, sizeof(myaddr))!=0){
				printf("Couldnt bind\n");
				exit(-1);
		}
		printf("bind OK...\n");
		
		if(listen(server_socket, 1) == -1){
				printf("Error on listen\n");
				exit(-1);
		}
		
		printf("listening...\n");
		
		
		
		//=============================================================== Do tud je sve oke.
		
		while(1){
			
			if((peer_socket=accept(server_socket, NULL, NULL))==-1){
				printf("Accept error\n");
				exit(-1);
			}
			printf("accepted connection...\n");
			
			int offset;
			char filename[1024-3];
			char request[1024];
			int nread=0;
			
			while(1){
					printf("waiting for request\n");
					int bytes = read(peer_socket, (void*) (request+nread), 1024);
					if(bytes<=0){
							printf("eof or error when reading offset\n");
							exit(1);
					}
					nread+=bytes;
					
					if(nread>4 && (request[nread-1] =='\0')) break;
			}
			
			offset=0;
			offset=ntohl(*((int*)request));
			strcpy(filename, request+4);
			
			printf("serve file %s from offset %d\n", filename, offset);
			
			
			int filefd = open(filename, O_RDONLY);
			lseek(filefd, offset, SEEK_SET);
			
			char code = 0x00;
			if(Write(peer_socket, (void*)&code, 1)!=1){
					printf("could not write succ\n");
					exit(1);
			}
			printf("sent code...\n");
			
			const unsigned int max_loc_buffer = 10000;
			char buffer[max_loc_buffer];
			
			int bytes_read;
			while((bytes_read=read(filefd, (void*)buffer, max_loc_buffer))>0){
					if(Write(peer_socket, (void*)buffer, bytes_read)!=bytes_read){
							printf("cant write to socket\n");
							exit(1);
					}
					printf("sending: %s", buffer);
			}
			close(peer_socket);
			printf("done serving this peer...\n");
		}
		close(server_socket);
		printf("server closed\n");
		return 0;
}
