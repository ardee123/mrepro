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
#include "server.h"


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
		
		int pf, tf;
		int port, ntimeout;
		int opcije;
		
		int server_socket, peer_socket; //tcp sockets
		
		int udp_socket; //udp socket
		
		struct sockaddr_in myaddr;
		struct addrinfo hints;
		
		char portnum[20]="1234";
		char zadani_timeout_char[20]="20";
		char error[10]="ERROR\n";
	
		if(argc<3){
				printf("Usage: server [-p tcp_port] [-t timeout] ip1 udp1 {ip2 udp2 ... ip5 udp5}\n");
				exit(1);
		}
		if((argc==3) && (((strcmp(argv[1], "-p"))==0) || (strcmp(argv[1], "-t"))==0)){
				printf("Usage: server [-p tcp_port] [-t timeout] ip1 udp1 {ip2 udp2 ... ip5 udp5}\n");
				exit(1);
		}
		if(argc==5 && (strcmp(argv[3], "-t")==0)){
				printf("Usage: server [-p tcp_port] [-t timeout] ip1 udp1 {ip2 udp2 ... ip5 udp5}\n");
				exit(1);
		}
		while((opcije=getopt(argc, argv, "p:t:"))!=-1){
				switch(opcije){
						case 'p':
							strcpy(portnum, optarg);
							pf=1;
							break;
						case 't':
							strcpy(zadani_timeout_char, optarg);
							tf=1;
							break;
						default:
							printf("Usage: server [-p tcp_port] [-t timeout] ip1 udp1 {ip2 udp2 ... ip5 udp5}\n");
							exit(1);
				}
		}
		;
		port=atoi(portnum);
		if(port==0){
				struct servent *serv = getservbyname(portnum, "tcp");
				int pnum=htons(serv->s_port);
				port=pnum;
				printf("service port: %d\n", pnum);
		}
		
		
		//sscanf(portnum, "%d", &port);
		sscanf(zadani_timeout_char, "%d", &ntimeout);
		struct timeval cekaj={ntimeout,0};
		
		//dodavanje parova udp adresa i portova u strukturu
		int pairs_number=0;
		udp_pairs udp_pairss;
		for(int i=argc-1; i>0; i=i-2){
				char udp_port[20];
				strcpy(udp_port, argv[i]);
				char udp_ip[100];
				strcpy(udp_ip, argv[i-1]);
				if(strcmp("-p", udp_ip)==0 || strcmp("-t", udp_ip)==0){
						break;
				}
				udp_addresses udp_address;
				strcpy(udp_address.ip, udp_ip);
				strcpy(udp_address.port, udp_port);
				udp_pairss.pairs[pairs_number++]=udp_address;
		}
		char pairs_char[20];
		
		memset(&hints, 0, sizeof(hints));
		hints.ai_family=AF_INET;
		hints.ai_socktype=SOCK_STREAM;
		hints.ai_flags|=AI_CANONNAME;
			
		myaddr.sin_port=htons(port);
		myaddr.sin_family=AF_INET;
		myaddr.sin_addr.s_addr=INADDR_ANY;
		
		memset(myaddr.sin_zero, '\0', sizeof(myaddr.sin_zero));
		
		//socket UDP
		udp_socket=Socket(AF_INET, SOCK_DGRAM, 0);
		
		for(int i=0; i<pairs_number; i++){
			
			struct addrinfo hints, *res;
			memset(&hints, 0, sizeof(hints));
			
			hints.ai_family=AF_INET;
			hints.ai_socktype=SOCK_DGRAM;
			hints.ai_flags=AI_PASSIVE;
			Getaddrinfo(udp_pairss.pairs[i].ip, udp_pairss.pairs[i].port, &hints, &res);
			
			struct sockaddr_in res2;
			memcpy(&res2, res->ai_addr, sizeof(res2));
				
			//printf("getting ready to send...\n");
			//salji podatak INIT\n na adresu koja je upisana u resultu
			Sendto(udp_socket, "INIT\n", strlen("INIT\n"), 0, (struct sockaddr*) &res2, sizeof(struct sockaddr_in));
			//printf("sending...\n");
		}
		
		//=========================================================================
		//socket TCP
		server_socket=Socket(PF_INET, SOCK_STREAM, 0);
		//bindanje TCP socketa
		if(bind(server_socket, (struct sockaddr *)&myaddr, sizeof(myaddr))!=0){
				printf("Couldnt bind tcp socket\n");
				exit(-1);
		}
		//printf("bind TCP je OK...\n");
		
		if(listen(server_socket, 1) == -1){
				printf("Error on listen\n");
				exit(-1);
		}
		//printf("listening...\n");
		
		while(1){
			char clientAddr[100];
			socklen_t clilen = sizeof(myaddr);
			if((peer_socket=accept(server_socket, (struct sockaddr *)&myaddr, &clilen))==-1){
				printf("Accept error\n");
				exit(-1);
			}
			inet_ntop(AF_INET, &(myaddr.sin_addr), clientAddr, 100);
			uint16_t portNo = ntohs(myaddr.sin_port);
			printf("Spojio se: %s:%d\n", clientAddr, portNo);
			int q=0;
			int peer=0;
			  while(1){
				
				char command[1000];
				memset(command, '\0', sizeof(command));
				int nread=0;
				while(1){
						int bytes = read(peer_socket, (void*) (command+nread), 1000);
						if(bytes<=0){
								printf("error reading from tcp socket\n");
								close(peer_socket);
								peer=1;
								break;
						}
						nread+=bytes;
						
						if(command[nread-1] =='\n' || command[nread-1] =='\0') break;
				}
				if(peer==1) break;
				printf("Naredba: %s", command);
				
				if(strcmp(command, "status\n")==0){
						char buff[512];
						
						sprintf(buff, "%d\n", pairs_number);
						if(Write(peer_socket, (void*)&buff, 2)!=2){
							printf("could not write succ\n");
							exit(1);
						}
				}			
				else if(strncmp(command, "data", 4)==0){
								char kc[10];
								int k;
								strcpy(kc, command+4);
								sscanf(kc, "%d", &k);
								
								if(k>pairs_number){
										char err[20];
										strcpy(err, "ERROR\n");
										if(Write(peer_socket, (void*)&err, sizeof(err))!=sizeof(err)){
											printf("could not write succ\n");
											exit(1);
										}
										continue;
								}
								k=abs(k-pairs_number);
								
								struct addrinfo hints, *res;
								memset(&hints, 0, sizeof(hints));
								
								hints.ai_family=AF_INET;
								hints.ai_socktype=SOCK_DGRAM;
								hints.ai_flags=AI_PASSIVE;
								Getaddrinfo(udp_pairss.pairs[k].ip, udp_pairss.pairs[k].port, &hints, &res);
								
								struct sockaddr_in res2;
								memcpy(&res2, res->ai_addr, sizeof(res2));
								
								//salji podatak STAT\n na adresu koja je upisana u resultu
								Sendto(udp_socket, "STAT\n", strlen("STAT\n"), 0, (struct sockaddr*) &res2, sizeof(struct sockaddr_in));
								
								if(setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, &cekaj, sizeof(struct timeval))<0){
										printf("socket error\n");
										exit(1);
								}else{
									char data[512];
									memset(data, '\0', sizeof(data));;
									
									int bytes_recieved = recvfrom(udp_socket, data, 512, 0, NULL, 0);
									if(bytes_recieved==-1){
										//printf("istek timeouta\n");
										if(Write(peer_socket, (void*)&error, sizeof(error))!=sizeof(error)){
											printf("could not write on sock tcp\n");
											exit(1);
										}
										
										continue;
									} else if (bytes_recieved<0){
											printf("err\n");
											exit(1);
									}
									if(Write(peer_socket, (void*)&data, bytes_recieved)!=bytes_recieved){
										printf("could not write succ\n");
										exit(1);
									}
									continue;
								}
							}
							else if(strcmp(command, "avg\n")==0){
								for(int j=0; j<pairs_number; j++){
				
									struct addrinfo hints, *res;
									memset(&hints, 0, sizeof(hints));
									
									hints.ai_family=AF_INET;
									hints.ai_socktype=SOCK_DGRAM;
									hints.ai_flags=AI_PASSIVE;
									Getaddrinfo(udp_pairss.pairs[j].ip, udp_pairss.pairs[j].port, &hints, &res);
									
									struct sockaddr_in res2;
									memcpy(&res2, res->ai_addr, sizeof(res2));
									
									//salji podatak STAT\n na adresu koja je upisana u resultu
									Sendto(udp_socket, "STAT\n", strlen("STAT\n"), 0, (struct sockaddr*) &res2, sizeof(struct sockaddr_in));
								}
								int total=0, djelim=0;
								
								for(int j=0; j<pairs_number; j++){
									struct addrinfo hints, *res;
									memset(&hints, 0, sizeof(hints));
									
									hints.ai_family=AF_INET;
									hints.ai_socktype=SOCK_DGRAM;
									hints.ai_flags=AI_PASSIVE;
									Getaddrinfo(udp_pairss.pairs[j].ip, udp_pairss.pairs[j].port, &hints, &res);
									
									struct sockaddr_in res2;
									memcpy(&res2, res->ai_addr, sizeof(res2));
									
									char datagram[512];
									memset(datagram, '\0', sizeof(datagram));
									int stat=0;
									int bytes_recieved = recvfrom(udp_socket, datagram, 512, 0, NULL, 0);
									if(bytes_recieved==-1){
										//printf("istek timeouta\n");
										
										if(Write(peer_socket, (void*)&error, sizeof(error))!=sizeof(error)){
											printf("could not write on sock tcp\n");
											exit(1);
										}
										continue;
									} else if (bytes_recieved<0){
											printf("err\n");
											exit(1);
									}
									sscanf(datagram, "%d", &stat); // char u int
									total+=stat;
									djelim++;
								}
								
								if(djelim!=0){
									total=total/djelim;	
								}
								djelim=0;;
								char dgram[512];
								memset(dgram, '\0', sizeof(dgram));
								sprintf(dgram, "%d\n", total);
								if(Write(peer_socket, (void*)&dgram, sizeof(dgram))!=sizeof(dgram)){
										printf("could not write on sock tcp\n");
										exit(1);
									}
								total=0;
							}
							else if(strcmp(command, "quit\n")==0){
								q=1;
								close(peer_socket);
								printf("Odspojio se: %s:%d\n", clientAddr, portNo);
								break;
							}
				}
		}
		return 0;
}
