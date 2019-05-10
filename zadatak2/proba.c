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

int Socket(int family, int type, int protocol){
		int n;//nesto kao file descriptor
		char *error;
		n=socket(family, type, protocol);
		if(n==-1){
			error=strerror(errno);
			printf("%s\n", error);
		}else{
			return n;
		}
}

int Bind(int sockfd, const struct sockaddr *myaddr, int addrlen){
	char *error;
	int err;
	err=bind(sockfd, myaddr, addrlen);
	if(err==1){
		error=strerror(errno);
		printf("%s\n", error);
		exit(4);
	}else{
		return 0;
	}
}

int Getaddrinfo(const char *hostname, const char *service, const struct addrinfo *hints, struct addrinfo **result){
		char *error;
		int err;
		err=getaddrinfo(hostname, service, hints, result);
		if(err){
			error=strerror(errno);
			printf("%s\n", error);
			exit(5);
		}else{
			return 0;
		}
}

int Getnameinfo(const struct sockaddr *sockaddr, socklen_t addrlen, char *host, size_t hostlen, char *serv, size_t servlen, int flags){
		char *error;
		int err;
		err=getnameinfo(sockaddr, addrlen, host, hostlen, serv, servlen, flags);
		if(err){
			error=strerror(errno);
			printf("%s\n", error);
			exit(5);
		}else{
			return 0;
		}
}

int Inet_ntop(int family, const void *src, char *dst, socklen_t size){
		char *error;
		int err;
		if(inet_ntop(family, &src, dst, size)==NULL){
			perror("inet ntop err");
			exit(6);
		}else {
			return 0;
		}
		
}

int main(int argc, char *argv[]){
		int opcija;
		struct servent *server;
		int xf=0, rf=0, ip6f=0, uf=0, nf=0;
		int type;
		struct sockaddr_in myAddr;
		struct sockaddr_in6 myAddr6;
		struct addrinfo hints, *result;
		char addrstr[100];
		
		if(argc<3){
			printf("prog [-r] [-t|-u] [-x] [-h|-n] [-46] {hostname|IP_address} {servicename|port}\n");
			return 1;
		}
	
		while((opcija=getopt(argc, argv, "tuxhnr46"))!=-1){
			switch(opcija){
				case 'u':
					uf=1;
					break;
				case 'x':
					xf=1;
					break;
				case 'n':
					nf=1;
					break;
				case '6':
					ip6f=1;
					break;
				case 'r':
					rf=1;
					break;
				case 't':
					break;
				case 'h':
					break;
				case '4':
					break;
				default:
					printf("prog [-r] [-t|-u] [-x] [-h|-n] [-46] {hostname|IP_address} {servicename|port}\n");
					return 1;
			}
		}
		
		if(argc-optind!=2){
			printf("prog [-r] [-t|-u] [-x] [-h|-n] [-46] {hostname|IP_address} {servicename|port}\n");
			return 1;
		}

		memset(&hints, 0, sizeof(hints));
		
		if(uf){
			type=SOCK_DGRAM;
		}else{
			printf("sock stream\n");
			type=SOCK_STREAM;
			hints.ai_protocol=IPPROTO_TCP;
		}
		
		//int mySock;
		//mySock=Socket(PF_INET, type, 0);
		//myAddr.sin_port=htons(0);
		
	
		
		hints.ai_flags|=AI_CANONNAME;
		
		if(ip6f){
			myAddr6.sin6_family=AF_INET6;
			//myAddr6.sin6_addr.s6_addr=htonl(INADDR_ANY);
			hints.ai_family=AF_INET6;
			//Bind(mySock, (struct sockaddr *) &myAddr6, sizeof(myAddr6));
		}else{
			myAddr.sin_family=AF_INET;
			myAddr.sin_addr.s_addr=htonl(INADDR_ANY);
			hints.ai_family=AF_INET;
			//Bind(mySock, (struct sockaddr *) &myAddr, sizeof(myAddr));
		}
		
		char host[NI_MAXHOST]; //hostname za -r
		if(rf){
			if(ip6f){
				if(inet_pton(AF_INET6, argv[argc-2], &myAddr6.sin6_addr)!=1){
					errx(1, "%s nije valjana IPv6 adresa", argv[argc-2]);
				}
				Getnameinfo((struct sockaddr *)&myAddr6, sizeof(struct sockaddr_in6), host, sizeof(host), NULL, 0, NI_NAMEREQD);
			}else{
				if(inet_pton(AF_INET, argv[argc-2], &myAddr.sin_addr)!=1){
					errx(1, "%s nije uspio poziv", argv[argc-2]);
				}
				Getnameinfo((struct sockaddr *)&myAddr, sizeof(struct sockaddr_in), host, sizeof(host), NULL, 0, NI_NAMEREQD);
			}
		} else {
			Getaddrinfo(argv[argc-2], argv[argc-1], &hints, &result);//nadopisat
			if(ip6f){
				if(inet_ntop(result->ai_family, &((struct sockaddr_in6 *) result->ai_addr)->sin6_addr, addrstr, INET6_ADDRSTRLEN)==NULL){
					perror("inet6 ntop err");
					exit(6);
				}
				//Inet_ntop(result->ai_family, &((struct sockaddr_in6 *) result->ai_addr)->sin6_addr, addrstr, INET6_ADDRSTRLEN);
			}else {
				if(inet_ntop(result->ai_family, &((struct sockaddr_in *) result->ai_addr)->sin_addr, addrstr, INET_ADDRSTRLEN)==NULL){
					perror("inet ntop err");
					exit(6);
				}
				
				//Inet_ntop(result->ai_family, &((struct sockaddr_in *) result->ai_addr)->sin_addr, addrstr, INET_ADDRSTRLEN);
			}
		}
		
		int port;
		sscanf(argv[argc-1], "%d", &port);
		if(rf){
			int pnum=argv[argc-1];
			printf("port num: %s\n", argv[argc-1]);
			if(uf){
				if((server = getservbyport(port, "udp")) == NULL){
					perror("getservbyport err udp");
				}
			} else {
				if((server = getservbyport(port, "tcp")) == NULL){
					perror("getservbyport err tcp");	
				}
			}
		} else {
			if(uf){
				if((server= getservbyname(argv[argc-1], "udp"))== NULL){	// Pretvorba service name u port number
						perror("getservbyname err");
				}
			}else{
				if((server= getservbyname(argv[argc-1], "tcp"))== NULL){
						perror("getservbyname err");
				}	
			}
			myAddr.sin_port = server->s_port;								// Spremanje u strukturu
			myAddr6.sin6_port = server->s_port;
		}
		
		
		
		//ovi if elsseovi nisu potpuni nes fali
		
		if(rf){
			if(ip6f){
				printf("%s (%s) %d\n", myAddr6.sin6_addr, host, ntohs(myAddr6.sin6_port));
			}else {
				printf("%s (%s) %d\n", myAddr.sin_addr, host, ntohs(myAddr.sin_port));
			}
		}else{
			if(xf){
				printf("%s (%s) %04x\n", addrstr, result->ai_canonname, ntohs(myAddr.sin_port));
			}else{
				printf("%s (%s) %d\n", addrstr, result->ai_canonname,  ntohs(myAddr.sin_port));
			}
		}
		freeaddrinfo(result);
		//close(mySock);
		return 0;
		
}
