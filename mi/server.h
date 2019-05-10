typedef struct{
		char ip[INET_ADDRSTRLEN];
		char port[22];
}udp_addresses;

typedef struct{
		udp_addresses pairs[5];
}udp_pairs;
