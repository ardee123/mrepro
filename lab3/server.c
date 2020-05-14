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
#include "bot.h"

#define max(a,b) ( ( (a) > (b) ) ? (a) : (b) )
#define STDIN 0

int Socket(int family, int type, int protocol){
		int succ=socket(family, type, protocol);
		char *error;
		if(succ < 0){
				error=strerror(errno);
				printf("%s\n", error);
		} 
		return succ;
		
}

int Bind(int sockfd, const struct sockaddr *myaddr, int addrlen){
	char *error;
	int err;
	err=bind(sockfd, myaddr, addrlen);
	if(err!=0){
		error=strerror(errno);
		printf("%s\n", error);
		exit(4);
	}else{
		return 0;
	}
}

int Getaddrinfo(const char *host, const char *server, const struct addrinfo *hints,
				struct addrinfo **res){
		int succ=getaddrinfo(host, server, hints, res);
		char *error;
		if(succ){
			error=strerror(errno);
			printf("%s\n", error);	
		}
		return succ;
		
}

int Sendto(int sockfd, void *buff, size_t nbytes, int flags, const struct sockaddr* to, socklen_t addrlen){
		int succ=sendto(sockfd, buff, nbytes, flags, to, addrlen);
		if(succ!=nbytes){
				printf("sendto error %d, nbytes is %d errno %d\n", succ, nbytes, errno);
				errx(2, "%s", strerror(errno));
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

int RecvfromChar(int sock_fd,  char *msg, size_t nbytes, int flags, struct sockaddr* from, socklen_t *fromaddrlen){
		int recieved = recvfrom(sock_fd, msg, nbytes, 0, NULL, 0);
		if(recieved <0){
				printf("recvfrom dod not recieve messages\n");
				exit(2);
		}
		return recieved;
}

int main(int argc, char *argv[]){
		
		int opcije;
		int tcp_port, udp_port;
		int listen_socket, peer_socket=-5, udp_socket;
		
		char tcp_portnum[10]="1234";
		char udp_portnum[10]="1234";
		char payload[1024]="/0";
		char buff[1024];

		
		struct sockaddr_in myaddr, cliaddr;
		struct addrinfo hints;
		
		struct timeval tv={5,0};
		fd_set readfds;
		
		socklen_t len;
		
		if(argc%2==0){
			printf("Usage: ./server [-t tcp_port] [-u udp_port] [-p popis]\n");
			exit(1);
		}
		
		while((opcije=getopt(argc, argv, "p:t:u:"))!=-1){
				switch(opcije){
						case 't':
							strcpy(tcp_portnum, optarg);
							
							break;
						case 'u':
							strcpy(udp_portnum, optarg);
							
							break;
						case 'p':
							memset(payload, '\0', sizeof(payload));
							strcpy(payload, optarg);
							break;
						default:
							printf("Usage: ./server [-t tcp_port] [-u udp_port] [-p popis]\n");
							exit(1);
				}
		}
		tcp_port=atoi(tcp_portnum);
		if(tcp_port==0){
				struct servent *serv = getservbyname(tcp_portnum, "tcp");
				int pnum=htons(serv->s_port);
				tcp_port=pnum;
				printf("service port: %d\n", pnum);
		}else{
			sscanf(tcp_portnum, "%d", &tcp_port);	
		}
		
		udp_port=atoi(udp_portnum);
		if(udp_port==0){
				struct servent *serv = getservbyname(udp_portnum, "udp");
				int pnum=htons(serv->s_port);
				udp_port=pnum;
				printf("service port: %d\n", pnum);
		}else{
				sscanf(udp_portnum, "%d", &udp_port);	
		}
		//TCP DIO
		//=========================================================================
		listen_socket=Socket(AF_INET, SOCK_STREAM, 0);
		
		bzero(&myaddr, sizeof(myaddr));
		myaddr.sin_family=AF_INET;
		myaddr.sin_addr.s_addr=htonl(INADDR_ANY);
		myaddr.sin_port=htons(tcp_port);
	
		memset(myaddr.sin_zero, '\0', sizeof(myaddr.sin_zero));
		
		if(bind(listen_socket, (struct sockaddr *)&myaddr, sizeof(myaddr))!=0){
				printf("Couldnt bind\n");
				exit(-1);
		}
		printf("bind OK...\n");
		
		if(listen(listen_socket, 1) == -1){
				printf("Error on listen\n");
				exit(-1);
		}
		
		printf("listening...\n");
		//========================================================================
		
		//UDP DIO
		struct sockaddr_in udpaddr;
		memset(&udpaddr, 0, sizeof(udpaddr));

		udpaddr.sin_family=AF_INET;
		udpaddr.sin_addr.s_addr=INADDR_ANY;
		udpaddr.sin_port=htons(udp_port);
		memset(udpaddr.sin_zero, '\0', sizeof(udpaddr.sin_zero));
		
		udp_socket=Socket(AF_INET, SOCK_DGRAM, 0);

		Bind(udp_socket, (struct sockaddr*) &udpaddr, sizeof(udpaddr));


		printf("udp sock listening\n");
		//=========================================================================	
		
		FD_ZERO(&readfds);
		int fd_tcp=0;
		int max=max(listen_socket, udp_socket);
		printf("udp: %d, listen: %d\n", udp_socket, listen_socket);
		
		while(1){
			
			
				FD_ZERO(&readfds);
				
				//if(fd_tcp==0){
						FD_SET(listen_socket, &readfds);

				//}//
				FD_SET(STDIN, &readfds);
				FD_SET(udp_socket, &readfds);	

				tv.tv_sec=3;
				int retval=select(max+1, &readfds, NULL, NULL, &tv);
				
			
				
				if(FD_ISSET(STDIN, &readfds)){
						char msg[1024+5];
						memset(msg, 0, sizeof(msg));
						read(STDIN, msg, sizeof(msg));
						if(strcmp(msg, "PRINT\n")==0){
								printf("%s\n", payload);
						}
						else if(strncmp(msg, "SET", 3)==0){
							strcpy(payload, msg+4);
							printf("new payload: %s", payload);
								
						}
						else if(strcmp(msg, "QUIT\n")==0){
								exit(0);
						}
				}
				
				//if(fd_tcp==0){
				if(FD_ISSET(listen_socket, &readfds)){
					//printf("tcp set\n");
					len=sizeof(cliaddr);
					if((peer_socket=accept(listen_socket, (struct sockaddr*)&cliaddr, &len))==-1){
								printf("Accept error\n");
								exit(-1);
					}
					//close(listen_socket);
					bzero(buff, sizeof(buff));
					int nread=0;
							while(1){
								int bytes = read(peer_socket, (void*) (buff+nread), sizeof(buff));
								printf("Bytes read: %d\n", bytes);
								if(bytes<0){
										printf("error reading from TCP socket\n");
										close(peer_socket);
										break;
								}
								if(bytes==0){
									close(peer_socket);
									break;
								}
								nread+=bytes;
								
								if(buff[nread-1] =='\n' || buff[nread-1] =='\0') break;
							}
					if(strcmp(buff, "HELLO\n")==0){
								printf("%s", buff);
								char pay[1024];
								memset(pay, '\0', sizeof(pay));
								strcpy(pay, payload);
								printf("Pay: %s", pay);
								if(Write(peer_socket, (void*)&pay, sizeof(buff))!=sizeof(buff)){
										printf("could not write succ\n");
										exit(1);
								}
					}
					//close(peer_socket);	
					//close(listen_socket);
					FD_CLR(listen_socket, &readfds);
					
					fd_tcp=1;
					}
				//}
				
				if(FD_ISSET(udp_socket, &readfds)){
						char buff[1024];
						memset(buff, '\0', sizeof(buff));
						len=sizeof(cliaddr);
						
						bzero(buff, sizeof(buff));
						int bytes_recieved = recvfrom(udp_socket, buff, sizeof(buff), 0, (struct sockaddr*)&cliaddr, &len);
						if(strcmp(buff, "HELLO\n")==0){
							printf("%s", buff);
							char pay[1024];
							memset(pay, '\0', sizeof(pay));
							strcpy(pay, payload);
							int b=Sendto(udp_socket, pay, sizeof(buff), 0, (struct sockaddr*) &cliaddr, sizeof(cliaddr));
							printf("Bytes sent: %d\n", b);
						}
				}
		}
		
		return 0;
}
