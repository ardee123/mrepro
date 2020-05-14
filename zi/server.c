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
		
		
		socklen_t len;
		int pf, tf;
		int tcp_port, udp_port, sec;
		int opcije;
		int counter_podataka;
		int ukupno_podataka;
		
		int data_arr[6]={-1};
		int udps[6];
		int listen_socket, peer_socket=0; //tcp sockets
		
		int udp_socket, udppeer_socket; //udp socket
		
		struct timeval tv={5,0};

		fd_set readfds;
		struct sockaddr_in myaddr, cliaddr;
		struct addrinfo hints;
		
		char clientAddr[100];

		char tcp_portnum[20]="1234";
		char udp_portnum[20]="1234";
		char seconds[20]="5";

		char zadani_timeout_char[20]="20";
		char error[10]="ERROR\n";
	
		if(argc%2==0 || argc==1 || argc>17){
			printf("Usage: server [-t tcp_port] [-u udp_port][-s sec] ip1 udp1 {ip2 udp2 ... ip5 udp5}\n");
				exit(1);	
		}
		while((opcije=getopt(argc, argv, "t:u:s:"))!=-1){
				switch(opcije){
						case 't':
							strcpy(tcp_portnum, optarg);
							pf=1;
							break;
						case 'u':
							strcpy(udp_portnum, optarg);
							tf=1;
							break;
						case 's':
							strcpy(seconds, optarg);
							tf=1;
							break;
						default:
							printf("Usage: server [-t tcp_port] [-u udp_port][-s sec] ip1 udp1 {ip2 udp2 ... ip5 udp5}\n");
							exit(1);
				}
		}
	
		
		tcp_port=atoi(tcp_portnum);
		if(tcp_port==0){
				struct servent *serv = getservbyname(tcp_portnum, "tcp");
				int pnum=htons(serv->s_port);
				tcp_port=pnum;
				printf("tcp service port: %d\n", pnum);
		}else{
			sscanf(tcp_portnum, "%d", &tcp_port);	
		}
		udp_port=atoi(udp_portnum);
		if(udp_port==0){
				struct servent *serv = getservbyname(udp_portnum, "udp");
				int pnum=htons(serv->s_port);
				udp_port=pnum;
				printf("udp service port: %d\n", pnum);
		}else{
			sscanf(udp_portnum, "%d", &udp_port);	
		}
		
		
		//sscanf(portnum, "%d", &port);
		sscanf(seconds, "%d", &sec);
		struct timeval cekaj={sec,0};
		
		//dodavanje parova udp adresa i portova u strukturu
		int pairs_number=0;
		udp_pairs udp_pairss;
		for(int i=argc-1; i>0; i=i-2){
				char udp_port[20];
				strcpy(udp_port, argv[i]);
				char udp_ip[100];
				strcpy(udp_ip, argv[i-1]);
				if(strcmp("-t", udp_ip)==0 || strcmp("-s", udp_ip)==0 || strcmp("-u", udp_ip)==0){
						break;
				}
				udp_addresses udp_address;
				strcpy(udp_address.ip, udp_ip);
				strcpy(udp_address.port, udp_port);
				udp_pairss.pairs[pairs_number++]=udp_address;
		}
		char pairs_char[20];
		
		
		
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
		const int flag=1;
		Bind(udp_socket, (struct sockaddr*) &udpaddr, sizeof(udpaddr));
		setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int));

		printf("udppeer sock listening\n");
		//=========================================================================	
		
		
		
		for(int i=0; i<pairs_number; i++){
			/*
			struct sockaddr_in udpaddr;
		memset(&udpaddr, 0, sizeof(udpaddr));

		udpaddr.sin_family=AF_INET;
		udpaddr.sin_addr.s_addr=INADDR_ANY;
		udpaddr.sin_port=htons(udp_pairss.pairs[i].port);
		memset(udpaddr.sin_zero, '\0', sizeof(udpaddr.sin_zero));
		
		udps[i+1]=Socket(AF_INET, SOCK_DGRAM, 0);

		Bind(udps[i+1], (struct sockaddr*) &udpaddr, sizeof(udpaddr));
			Sendto(udps[i+1], "INIT\n", strlen("INIT\n"), 0, (struct sockaddr*) &udpaddr, sizeof(struct sockaddr_in));


		printf("udppeer sock listening\n");
		*/
			
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
			printf("sending INIT...\n");
		}
		FD_ZERO(&readfds);
		int fd_tcp=0;
		int max=max(listen_socket, udppeer_socket);
		//max=max(max, udppeer_socket)
		printf("udp: %d, listen: %d\n", udp_socket, listen_socket);
		
		while(1){
			char command[1024];
			
			FD_ZERO(&readfds);
				
			FD_SET(listen_socket, &readfds);
			FD_SET(STDIN, &readfds);
			FD_SET(udp_socket, &readfds);
			if(peer_socket!=0){	
				printf("set my peer\n");
				FD_SET(peer_socket, &readfds);	
			}

			tv.tv_sec=3;
			int retval=select(max+1, &readfds, NULL, NULL, &tv);
				
			if(FD_ISSET(listen_socket, &readfds)){
				printf("lestenm\n");
				char clientAddr[100];
				socklen_t clilen = sizeof(myaddr);
				if(peer_socket==0){
					if((peer_socket=accept(listen_socket, (struct sockaddr *)&myaddr, &clilen))==-1){
						printf("Accept error\n");
						exit(-1);
					}
					inet_ntop(AF_INET, &(myaddr.sin_addr), clientAddr, 100);
					uint16_t portNo = ntohs(myaddr.sin_port);
					printf("Spojio se: %s:%d\n", clientAddr, portNo);
				}
				
				
				memset(command, '\0', sizeof(command));
				
				int nread=0;
					while(1){
						int bytes = read(peer_socket, (void*) (command+nread), 1000);
						if(bytes<=0){
								printf("error reading from tcp socket\n");
								close(peer_socket);
								//peer=1;
								break;
						}
						nread+=bytes;
						
						if(command[nread-1] =='\n' || command[nread-1] =='\0') break;
				}
				printf("Naredba: %s", command);
				if(strcmp(command, "status\n")==0){
							char buff[512];
						
							sprintf(buff, "%d\n", pairs_number);
							if(Write(peer_socket, (void*)&buff, 2)!=2){
								printf("could not write succ\n");
								exit(1);
							}

						} else if(strncmp(command, "data", 4)==0){
								int b;
								char kc[10];
								int k;
								strcpy(kc, command+4);
								sscanf(kc, "%d", &k);
								
								if(k>pairs_number){
										char err[20];
										memset(err, '\0', sizeof(err));
										strcpy(err, "ERROR\n");
										if(Write(peer_socket, (void*)&err, sizeof(err))!=sizeof(err)){
											printf("could not write succ\n");
											exit(1);
										};
										
										continue;
								}
								k=abs(k-pairs_number);
								
								struct addrinfo hints, *res;
								memset(&hints, 0, sizeof(hints));
								
								hints.ai_family=AF_INET;
								hints.ai_socktype=SOCK_STREAM;
								hints.ai_flags=AI_PASSIVE;
								Getaddrinfo(udp_pairss.pairs[k].ip, udp_pairss.pairs[k].port, &hints, &res);
								
								struct sockaddr_in res2;
								memcpy(&res2, res->ai_addr, sizeof(res2));
								
								//salji podatak STAT\n na adresu koja je upisana u resultu
								printf("try send stat\n");
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
									printf("data: %s\n", data);
									int d;
									sscanf(data, "%d", &d);//char u int
									data_arr[k]=d;

									if(Write(peer_socket, (void*)&data, bytes_recieved)!=bytes_recieved){
										printf("could not write succ\n");
										exit(1);
									}
									
									continue;
								}
						} else if(strcmp(command, "avg\n")==0){
								
								int total=0, djelim=0;
								for(int i=1; i<pairs_number+1; i++){
										if(data_arr[i]==-1){
											struct addrinfo hints, *res;
											memset(&hints, 0, sizeof(hints));
									
											hints.ai_family=AF_INET;
											hints.ai_socktype=SOCK_DGRAM;
											hints.ai_flags=AI_PASSIVE;
											Getaddrinfo(udp_pairss.pairs[i-1].ip, udp_pairss.pairs[i-1].port, &hints, &res);
											if(Write(peer_socket, (void*)&error, sizeof(error))!=sizeof(error)){
												printf("could not write on sock tcp\n");
												exit(1);
											}
											break;
										}
									total+=data_arr[i];	
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
								//Sendto(udppeer_socket, dgram, sizeof(dgram), 0, (struct sockaddr*) &cliaddr, sizeof(cliaddr));

								total=0;
						} else if(strncmp(command, "last", 4)==0){
								int b;
								char kc[10];
								int k;
								strcpy(kc, command+4);
								sscanf(kc, "%d", &k);
								
								if(k>pairs_number){
										char err[20];
										strcpy(err, "ERROR\n");
										if(Write(peer_socket, (void*)&error, sizeof(error))!=sizeof(error)){
											printf("could not write on sock tcp\n");
											exit(1);
										}
										//b=Sendto(udppeer_socket, err, sizeof(err), 0, (struct sockaddr*) &cliaddr, sizeof(cliaddr));
										//printf("Bytes sent: %d\n", b);
										
										continue;
								}
								k=abs(k-pairs_number);
						
								if(data_arr[k]==-1){
										//Sendto(udppeer_socket, "ERR\n", strlen("ERR\n"), 0, (struct sockaddr*) &cliaddr, sizeof(cliaddr));
										if(Write(peer_socket, (void*)&error, sizeof(error))!=sizeof(error)){
											printf("could not write on sock tcp\n");
											exit(1);
										}
	
								} else {
										char last[512];
										memset(last, '\0', sizeof(last));
										sprintf(last, "%d\n", data_arr[k]);
										if(Write(peer_socket, (void*)&last, sizeof(last))!=sizeof(last)){
											printf("could not write on sock tcp\n");
											exit(1);
										}
										//Sendto(udppeer_socket, last, sizeof(last), 0, (struct sockaddr*) &cliaddr, sizeof(cliaddr));
								}
																
									
						} else if(strcmp(command, "reset\n")==0){
								for(int i=0; i<6; i++)
									data_arr[i]=-1;
								for(int i=0; i<pairs_number; i++){
										
								}
						} else {
							if(Write(peer_socket, (void*)&error, sizeof(error))!=sizeof(error)){
											printf("could not write on sock tcp\n");
											exit(1);
							}
						}
				//close(listen_socket);
			}
			
			
				
			if(FD_ISSET(udp_socket, &readfds)){
						char command[1024];
						char por[20];
						char buff[1024];
						struct sockaddr_in cliaddr;
						
						memset(buff, '\0', sizeof(buff));
						len=sizeof(cliaddr);
						
						bzero(buff, sizeof(buff));
						int bytes_recieved = recvfrom(udp_socket, buff, sizeof(buff), 0, (struct sockaddr*)&cliaddr, &len);
						
						inet_ntop(AF_INET, &(cliaddr.sin_addr), clientAddr, 100);
						uint16_t portNo = ntohs(cliaddr.sin_port);
						printf("Spojio se: %s:%d\n", clientAddr, portNo);
						printf("Naredba: %s\n", command);
						
						if(strcmp(command, "status\n")==0){
							//printf("%s", buff);
							sprintf(buff, "%d\n", pairs_number);
							Sendto(udp_socket, buff, sizeof(buff), 0, (struct sockaddr*) &cliaddr, sizeof(cliaddr));

						} else if(strncmp(command, "data", 4)==0){
								int b;
								char kc[10];
								int k;
								strcpy(kc, command+4);
								sscanf(kc, "%d", &k);
								
								if(k>pairs_number){
										char err[20];
										strcpy(err, "ERROR\n");
										b=Sendto(udppeer_socket, err, sizeof(err), 0, (struct sockaddr*) &cliaddr, sizeof(cliaddr));
										printf("Bytes sent: %d\n", b);
										
										continue;
								}
								k=abs(k-pairs_number);
								
								struct addrinfo hints, *res;
								memset(&hints, 0, sizeof(hints));
								
								hints.ai_family=AF_INET;
								hints.ai_socktype=SOCK_STREAM;
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
										b=Sendto(udppeer_socket, error, sizeof(error), 0, (struct sockaddr*) &cliaddr, sizeof(cliaddr));
										printf("Bytes sent: %d\n", b);
										
										continue;
									} else if (bytes_recieved<0){
											printf("err\n");
											exit(1);
									}
									printf("data: %s\n", data);
									int d;
									sscanf(data, "%d", &d);//char u int
									data_arr[k]=d;

									b=Sendto(udppeer_socket, data, sizeof(data), 0, (struct sockaddr*) &cliaddr, sizeof(cliaddr));
									printf("Bytes sent: %d\n", b);
									
									continue;
								}
						} else if(strcmp(command, "avg\n")==0){
								
								int total=0, djelim=0;
								for(int i=1; i<pairs_number+1; i++){
										if(data_arr[i]==-1){
											struct addrinfo hints, *res;
											memset(&hints, 0, sizeof(hints));
									
											hints.ai_family=AF_INET;
											hints.ai_socktype=SOCK_DGRAM;
											hints.ai_flags=AI_PASSIVE;
											Getaddrinfo(udp_pairss.pairs[i-1].ip, udp_pairss.pairs[i-1].port, &hints, &res);
											Sendto(udppeer_socket, "ERR\n", strlen("ERR\n"), 0, (struct sockaddr*) &cliaddr, sizeof(cliaddr));
											break;
										}
									total+=data_arr[i];	
								}
								
								
								if(djelim!=0){
									total=total/djelim;	
								}
								djelim=0;;
								char dgram[512];
								memset(dgram, '\0', sizeof(dgram));
								sprintf(dgram, "%d\n", total);
								Sendto(udppeer_socket, dgram, sizeof(dgram), 0, (struct sockaddr*) &cliaddr, sizeof(cliaddr));

								total=0;
						} else if(strncmp(command, "last", 4)==0){
								int b;
								char kc[10];
								int k;
								strcpy(kc, command+4);
								sscanf(kc, "%d", &k);
								
								if(k>pairs_number){
										char err[20];
										strcpy(err, "ERROR\n");
										b=Sendto(udppeer_socket, err, sizeof(err), 0, (struct sockaddr*) &cliaddr, sizeof(cliaddr));
										printf("Bytes sent: %d\n", b);
										
										continue;
								}
								k=abs(k-pairs_number);
						
								if(data_arr[k]==-1){
										Sendto(udppeer_socket, "ERR\n", strlen("ERR\n"), 0, (struct sockaddr*) &cliaddr, sizeof(cliaddr));
	
								} else {
										char last[512];
										memset(last, '\0', sizeof(last));
										sprintf(last, "%d\n", data_arr[k]);
										Sendto(udppeer_socket, last, sizeof(last), 0, (struct sockaddr*) &cliaddr, sizeof(cliaddr));
								}
																
									
						} else if(strcmp(command, "reset\n")==0){
								for(int i=0; i<6; i++)
									data_arr[i]=-1;
								for(int i=0; i<pairs_number; i++){
										
								}
						} else {
								Sendto(udppeer_socket, "ERR\n", strlen("ERR\n"), 0, (struct sockaddr*) &cliaddr, sizeof(cliaddr));
						}
					
			}
			}
			/*if(FD_ISSET(STDIN, &readfds)){
						char msg[1024];
						char buff[
						memset(msg, 0, sizeof(msg));
						
						read(STDIN, msg, sizeof(msg));
						
						if(strcmp(msg, "status\n")==0){
							sprintf(buff, "%d\n", pairs_number);
							if(Write(peer_socket, (void*)&buff, 2)!=2){
								printf("could not write succ\n");
								exit(1);
							}
						}
						else if(strncmp(msg, "SET", 3)==0){
							strcpy(payload, msg+4);
							printf("new payload: %s", payload);
								
						}
						else if(strcmp(msg, "QUIT\n")==0){
								exit(0);
						}
			}*/
			
		return 0;
}
