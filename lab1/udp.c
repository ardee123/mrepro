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

#define MAX_PORT 22
#define MAX_MESSAGE_LENGTH 512

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
				printf("sendto error: only %d bytes sent\n", succ);
				errx(2, "%s", strerror(errno));
		}
		return succ;
}	

ssize_t Recvfrom(int sock_fd, void *buff, size_t nbytes, int flags, struct sockaddr* from, socklen_t *fromaddrlen){
		int recieved = recvfrom(sock_fd, buff, nbytes, flags, from, fromaddrlen);
		if(recieved <0){
				printf("recvfrom error: did not recieve any message\n");
				exit(2);
		}
		return recieved;
}

int main(int argc, char* argv[]){
	char payload[MAX_MESSAGE_LENGTH]="payload\n"; //default
	char port[MAX_PORT]="1234"; //default
	char option;
	
	int portic;
	int sock_fd;
	
	struct sockaddr_in addr;
	
	
	//parsiranje ulaza
	while((option=getopt(argc, argv, "l:p:"))!=-1){
			switch(option){
					case 'l':
						if(strlen(optarg) <= MAX_PORT){
								strcpy(port, optarg);
						}else {
								printf("port number is too long\n");
								exit(-1);
						}
						break;
					case 'p':
						if(strlen(optarg) <= MAX_MESSAGE_LENGTH){
								strcpy(payload, optarg);
						} else {
								printf("payload is too long\n");
								exit(-1);
						}
						break;
					default:
						printf("Usage %s: [-l port] [-p payload]\n", argv[0]);
						exit(-1);
			}
				
	}
	
	//pretvorba stringa u integer
	sscanf(port, "%d", &portic);
	
	memset(&addr, 0, sizeof(addr));
	
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=INADDR_ANY;
	addr.sin_port=htons(portic); // broj porta koji zadam, npr 7777
	
	//socket
	sock_fd=Socket(AF_INET, SOCK_DGRAM, 0);
	
	//bindanje socketa na lokalnu ip adresu
	Bind(sock_fd, (struct sockaddr*) &addr, sizeof(addr));
	
	
	//cekaj na zahtjev od bota
	while(1){
			char buff[100], payload_recieved[MAX_MESSAGE_LENGTH];
			
			struct sockaddr_in source; // izvorna poruka
			unsigned int adress_len=sizeof(source);
			printf("waiting for bot...\n");
			
			int reclen=Recvfrom(sock_fd, buff, 100, 0, (struct sockaddr*) &source, &adress_len);
			buff[reclen]='\0'; // init buffera
			
			//ako je manje od 100 bajtova primljeno
			if(adress_len != sizeof(source)){
					printf("invalid source len %d\n", adress_len);
					exit(-1);
			}
			
			if(strcmp(buff, "HELLO")!=0){
					printf("did not recieve HELLO from bot\n");
					exit(-1);
			}
			sprintf(payload_recieved, "PAYLOAD:%s\n", payload);
			
			//posalji botu kolko si primio
			Sendto(sock_fd, payload_recieved, strlen(payload_recieved), 0, (struct sockaddr*) &source, sizeof(source));
			
			printf("sent %s\n", payload_recieved);	
	}
	
	return 0;	
}
