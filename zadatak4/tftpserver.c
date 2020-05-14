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
#include<signal.h>
#include<limits.h>
#include<stdint.h>
#include<syslog.h>
#include<stdarg.h>


#define NETASCII 1
#define OCTET 2

#define TRANS_TIMEOUT 1
#define TRANS_MAX 3

#define RRQ 1
#define DATA 3
#define ACK 4
#define ERROR 5

typedef union{
		uint16_t code;
		
		struct{
				uint16_t code;
				uint8_t filename[514];
		}request;
		
		struct{
				uint16_t code;
				uint16_t block_num;
				uint8_t data[512];
		}data;
		
		struct{
				uint16_t code;
				uint16_t block_num;
		}ack;
		
		struct{
				uint16_t code;
				uint16_t error_code;
				uint8_t error_string[512];
		}error;
}tftp_message;


void cld_handler(int sig){
		int status;
		wait(&status);
}
int demon;
/* DEMONIZACIJA PROCESA
int daemon_init(const char *pname, int facility){
				int i;
				pid_t pid;
				
				if((pid=fork())<0)
					return -1;
				else if (pid)
					exit(0); //parent izlazi
				//child nastavlja
				if(setsid()<0)
					return -1;
				signal(SIGHUP, SIG_IGN);
				
				if((pid=fork())<0)
					return -1;
				else if (pid)
					exit(0); //child 1 zavrsava
				//child 2 nastavlja
				chdir("/");
				for(int i=0; i<64, i++)
					close(i);
				openlog(pname, LOG_PID, facility);
				return 0;
		}
	*/	
ssize_t tftp_send_ack(int s, uint16_t block, struct sockaddr_in *sock, socklen_t len){
		tftp_message m;
		ssize_t l;
		m.code = htons(ACK);
		m.ack.block_num = htons(block);
		
		if((l=sendto(s, &m, sizeof(m.ack), 0, (struct sockaddr*)sock, len))<0){
				printf("sendto err\n");
		}
		return l;
}

ssize_t tftp_send_data(int s, uint16_t block, uint8_t *data, ssize_t dlen, struct sockaddr_in *sock, socklen_t slen){
		tftp_message m;
		ssize_t l;
		
		m.code = htons(DATA);
		m.data.block_num = htons(block);
		memcpy(m.data.data, data, dlen);
		
		if((l=sendto(s, &m, 4+dlen, 0, (struct sockaddr*)sock, slen))<0){
				printf("sendto err\n");
		}
		
		return l;
}

ssize_t tftp_send_err(int s, int err_code, char *err_msg, struct sockaddr_in *sock, socklen_t len){
		tftp_message msg;
		ssize_t l;
		/* errno EWOULDBLOCK*/
		if(demon){
				if(strlen(err_msg) >= 512){
					return -1;
				}
				openlog("as50080:MrePro tftpserver", LOG_PID, LOG_FTP);
				syslog(LOG_INFO,"TFTP ERROR %d %s\n", err_code, err_msg);
				closelog();
				if((l = sendto(s, &msg, 4 + strlen(err_msg) +1, 0, (struct sockaddr*)sock, len))<0){
					
				}
		} else {
				if(strlen(err_msg) >= 512){
					fprintf(stderr, "TFTP ERROR %d: error string too long\n", err_code);
					return -1;
				}
				msg.code = htons(ERROR);
				msg.error.error_code = err_code;
				strcpy(msg.error.error_string, err_msg);
				
				if((l = sendto(s, &msg, 4 + strlen(err_msg) +1, 0, (struct sockaddr*)sock, len))<0){
						perror("server: snedto");
				}
				printf("TFTP ERROR %d %s\n", msg.error.error_code, err_msg);
		}
		
		return l;
}

ssize_t tftp_recv_message(int s, tftp_message *m, struct sockaadr_in *sock, socklen_t len){
		ssize_t l;
		
		if((l=recvfrom(s, m, sizeof(*m), 0, (struct sockaddr*)sock, len))<0 && errno!=EAGAIN){
				printf("recv err\n");
				
		}
		return l;
}

void tftp_handle_request(tftp_message *msg, ssize_t len, struct sockaddr_in *client_sock, socklen_t slen){
		int s;
		struct timeval tv;
		struct protoent *pp;
		char *filename, *mode_s, *end;
		FILE *file;
		
		int mode;
		uint16_t code;
		
		if((pp = getprotobyname("udp"))==0){
				fprintf(stderr, "server: getprotobyname\n");
				exit(1);
		}
		
		int sck = Socket(AF_INET, SOCK_DGRAM, pp->p_proto);
		
		tv.tv_sec = TRANS_TIMEOUT;
		tv.tv_usec =0;
		
		if(setsockopt(sck, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))<0){
				printf("server: setsockopt\n");
				exit(1);
		}
		
		
		filename = msg->request.filename;
		end = &filename[len -2 -1];
		
				
		mode_s = strchr(filename, '\0') +1;
		
		code = ntohs(msg->code);
		file = fopen(filename, code == RRQ ? "r" : "0");
		if(file==NULL){
				tftp_send_err(sck, errno, strerror(errno), client_sock, slen);
				exit(1);
		}
		mode_s = tolower(mode_s); // bilo koja kombinacija velikih i malih slova
		if(strcmp(mode_s, "netascii")==0){
				mode = NETASCII;
		} else if(strcmp(mode_s, "octet")==0){
				mode=OCTET;
		} else {
				mode=0;
		}
		
		mode = strcasecmp(mode_s, "netascii") ? NETASCII : strcasecmp(mode_s, "octet") ? OCTET : 0;
		if(mode == 0){
				tftp_send_err(sck, 0, "invalid transfer mode", client_sock, slen);
				exit(1);
		}
		
		if(code ==  RRQ){
				tftp_message m;
				
				uint8_t data[512];
				ssize_t dlen, c;
				uint16_t block =0;
				
				int trans=0;
				int to_close=0;
				
				while(!to_close){
						dlen = fread(data, 1, sizeof(data), file);
						block++;
						
						if(dlen<512){
							to_close=1;
						}
						
						for(trans = 0; trans<= TRANS_MAX; trans++){
								c = tftp_send_data(sck, block, data, dlen, client_sock, slen);
								if(c<0){
										printf("bad transfer\n");
										exit(1);
								}
								c = tftp_recv_message(sck, &m, client_sock, &slen);
								
								if(c >=0 && c<4){
										printf("invalid size of messafe\n");
										tftp_send_err(sck, 0, "invalid request size", client_sock, slen);
										exit(1);
								}
								
								if(c >= 4) break;
								
						}
						
						if(trans > TRANS_MAX){
								printf("TFTP ERROR %d max trinsmissions passed, timed out\n", m.code);
								exit(1);
						}
						
						if(ntohs(m.code)== ERROR){
								printf("TFTP ERROR %d error msg recieved\n", m.code);
								exit(1);
						}
						if(ntohs(m.code)!=ACK){
								printf("TFTP ERROR %d didnt recieve ack\n", m.code);
								exit(1);
						}
						if(ntohs(m.ack.block_num)!=block){
								printf("TFTP ERROR %d wrong ack num recieved\n", m.code);
								exit(1);
						}
				}
				
				printf("transfer succes\n");
				fclose(file);
				close(sck);
				exit(0);
		} else {
				tftp_send_err(sck, errno, strerror(errno), client_sock, slen);

		}
}

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





int main(int argc, char *argv[]){
	
		int port;
		char portn[10];
		
		struct protoent *pp;
		struct sockaddr_in tftp_server;
		
		if(argc < 2 || argc>3){
				printf("Usage: ./tftpserver [-d] port_name_or_number\n");
				exit(1);
		}
		if(argc==3){
			if(strcmp(argv[1], "-d")==0)
				demon=1;
			
			strcpy(portn, argv[2]);
		} else {
			strcpy(portn, argv[1]);
		}
		char workdir[1024];
		strcpy(workdir, "/tftpboot/");
		//change directory working
		if(chdir(workdir) <0){
				printf("err change dir\n");
				exit(1);
		}
		
		char cwd[PATH_MAX];
		getcwd(cwd, sizeof(cwd));
		printf("working dir: %s\n", cwd);
		
		
		port=atoi(portn);
		if(port==0){
				struct servent *serv = getservbyname(portn, "udp");
				int pnum=htons(serv->s_port);
				port=pnum;
		}else{
			sscanf(portn, "%d", &port);	
		}
		printf("listening on port %d\n", port);
		
		if((pp = getprotobyname("udp")) == 0){
				printf("err on getprotobyname\n");
				exit(1);
		}
		
		int sock = Socket(AF_INET, SOCK_DGRAM, pp->p_proto);
		printf("sock ok...\n");
		
		tftp_server.sin_family = AF_INET;
		tftp_server.sin_addr.s_addr = INADDR_ANY;
		tftp_server.sin_port = htons(port);
		memset(tftp_server.sin_zero, '\0', sizeof(tftp_server.sin_zero));
		
		Bind(sock, (struct sockaddr*)&tftp_server, sizeof(tftp_server));
		printf("bind ok...\n");
		
		(void)signal(SIGCHLD, (void*)cld_handler);
		
		printf("tftp ready and listening on: %d\n", port);
		
		
		while(1){
				struct sockaddr_in client_sock;
				socklen_t slen= sizeof(client_sock);
				ssize_t len;
				
				tftp_message message;
				uint16_t code;
				char filen[1000];
				
				int bytes_recieved = recvfrom(sock, &message, sizeof(message), 0, (struct sockaddr*)&client_sock, &slen);
				if(bytes_recieved < 0) continue;
				char clientAddr[100];
				inet_ntop(AF_INET, &(client_sock.sin_addr), clientAddr, 100);

				strcpy(filen, message.request.filename);
				
				if(!demon){
						printf("bytes %d\n", bytes_recieved);
						//tko se spojio===========================

						printf("Spojio se: %s->%s\n", clientAddr, filen);
						//=================
						if(bytes_recieved<4){
								printf("req with invalid size\n");
								tftp_send_err(sock, 0, "invalid req size", &client_sock, slen);
								continue;
						}
						code = ntohs(message.code);
						printf("code %d\n", code);
						
						if(code == RRQ){
							printf("code is RRQ\n");
							if(fork()==0){
									tftp_handle_request(&message, bytes_recieved, &client_sock, slen);
									//exit(0);
							}
						} else {
								printf("invalid request\n");
								tftp_send_err(sock, 0, "invalid code", &client_sock, slen);
						}
				} else {
						pid_t pid;
						int i;
						
						signal(SIGHUP, SIG_IGN);
						
						code = ntohs(message.code);
						
						if(code == RRQ){
							int f = fork();
							if(f ==0){
								for(i=0; i<64 ; i++)
							close(i);
									openlog("as50080:MrePro tftpserver", LOG_PID, LOG_FTP);
									syslog(LOG_INFO,"%s->%s\n", clientAddr, filen);
									tftp_handle_request(&message, bytes_recieved, &client_sock, slen);
									//exit(0);
							} else if(f==-1){
									//exit(1);
							} else {
									//exit(0);
							}
						} else {
								tftp_send_err(sock, 0, "invalid code", &client_sock, slen);
						}
						closelog();
				}
		}
		close(sock);
		
		

		return 0;
}
