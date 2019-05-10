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

int main(int argc, char *argv[]){
	int sf, pf, cf=0;
	int opcija;
	int mysock;
	
	struct sockaddr_in theiraddr;
	struct addrinfo hints, *res;
	
	char filename[250];
	char portnum[20];
	char hostname[100];
	char buff[1500];
	
	
	if((argc==1 || argc==2)){
				printf("Usage: ./tcpklijent [-s server] [-p port] [-c] filename\n");
				exit(1);
	}
	memset(filename, '\0', sizeof(filename));
	memset(buff, '\0', sizeof(buff));

	while((opcija=getopt(argc, argv, "p:s:c:"))!=-1){
				switch(opcija){
						case 'p':
							pf=1;
							strcpy(portnum, optarg);
							break;
						case 's':
							sf=1;
							strcpy(hostname, optarg);
							break;
						case 'c':
							cf=1;
							break;
						default:
							printf("Usage: ./tcpklijent [-s server] [-p port] [-c] filename\n");
							exit(1);
				}
	}
	strcpy(filename, argv[3]);
	//ako je zadan port, ali filename nije
	if(pf){
			if(strlen(filename)<1){
				printf("Usage: ./tcpklijent [-s server] [-p port] [-c] filename\n");
				exit(1);
	
			}
	}
	//ako se program pozove bez opcije -c, a file vec postoji, zavrsi
	if(!cf){
		if(access(filename, F_OK) == 0){
				printf("file already exists, and -c option was not given.\n");
				exit(1);
		}
	} else {
		if(access(filename, W_OK)!=0){
				printf("user is not authorized for writting in given file.\n");
				exit(1);
		}
	}
	//ako file ne postoji stvori mi ga, izracunaj koliko je dugacak, jer mozda vec postoji djelomicno prebacen tu
	//i stavi mi seek na pocetak 
	int filefd = open(filename, O_RDWR | O_APPEND | O_CREAT);
	int file_len = lseek(filefd, 0, SEEK_END);
	lseek(filefd, 0, SEEK_SET);
	
	char req_buff[200];
	int file_lenh = htonl(file_len);
	memcpy(req_buff, (void*)&file_lenh, 4);
	
	int namelen=strlen(filename);
	strcpy(req_buff + 4, filename);
	
	
	mysock=Socket(PF_INET, SOCK_STREAM, 0);
	printf("socket ok...\n");
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family=AF_INET;
	hints.ai_flags |= AI_CANONNAME;
	hints.ai_socktype=SOCK_STREAM;
	
	printf("argv[1]: %s", argv[1]);
	
	Getaddrinfo(NULL, argv[2], &hints, &res);
	printf("getaddrinfo ok...\n");
	
	theiraddr.sin_addr=((struct sockaddr_in *)res->ai_addr)->sin_addr;
	
	
	int port;
	sscanf(portnum, "%d", &port);
	printf("Portnum as int: %d\n", port);
	
	
	if(pf==0){
		theiraddr.sin_port=htons(PORT);
	} else {
		
		theiraddr.sin_port=htons(port);
	}
		
	theiraddr.sin_family=AF_INET;
	theiraddr.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
	memset(theiraddr.sin_zero, '\0', sizeof(theiraddr.sin_zero));
	
	
	
	printf("trying to connect...\n");
	if(connect(mysock, (struct sockaddr *)&theiraddr, sizeof(theiraddr)) == -1){
			printf("can not connect\n");
			exit(1);
	}
	printf("connected...\n");
	printf("File: %s\n", filename);


	//sending name of the file to server
	
	if(write(mysock, (void*) req_buff, namelen+5)!=namelen+5){
			printf("error while sending filename\n");
			exit(1);
	}
	
	printf("sent filename...\n");
	
	char server_code;
	
	if(read(mysock, (void*)&server_code, 1)<0){
			printf("couldnt recieve err msg from server\n");
			exit(1);
	}
	
	if(server_code != 0){
			const int max_error = 200;
			char error_msg[max_error];
			
			if(read(mysock, (void*)error_msg, max_error)<=0){
					printf("couldnt read err from server\n");
					exit(1);
			}
			
			switch(server_code){
					case 0x01:
					{
						errx(1, "%s", error_msg);
						break;
					}
					case 0x02:
					{
						errx(2, "%s", error_msg);
						break;
					}
					case 0x03:
					{
						errx(3, "%s", error_msg);
						break;
					}
					default:
					{
						errx(-1, "unknown code from server\n");
						}
			}
	}
	printf("server code: %c\n", server_code);
	const int max_loc_buffer = 10000;
	char recv_buffer[max_loc_buffer];
	int nb_read;
	
	while((nb_read = read(mysock, (void*)recv_buffer, max_loc_buffer))!=0){
			if(write(filefd, recv_buffer, nb_read)!=nb_read){
					printf("could not write in file\n");
					exit(1);
			}
			printf("%s", recv_buffer);
	}
	close(filefd);
	close(mysock);
		
	return 0; 
}
