#include<netinet/in.h>

typedef struct{
		char ip[INET_ADDRSTRLEN];
		char port[22];
}addresses_from_cc;

typedef struct{
		char command;
		addresses_from_cc pairs[20];
}message_from_cc;

typedef struct{
		addresses_from_cc bots[20];
}bots_cc;

typedef struct{
		struct sockaddr_in clients[20]; 
}clients_cc;
