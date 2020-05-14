typedef struct{
		char ip[INET_ADDRSTRLEN];
		char port[22];
}addresses_from_cc;

typedef struct{
		char command;
		addresses_from_cc pairs[20];
}message_from_cc;
