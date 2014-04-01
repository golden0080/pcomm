#ifndef _MY_INET_UTILS_H_
#define _MY_INET_UTILS_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define _QUOTE(str) #str
#define QUOTE(str) _QUOTE(str)

#define PROTO_UDP 17
#define PROTO_TCP 6

#define REG_UDP_PORT 21365
#define CON_UDP_PORT 31365

#define APPL_UDP_PORT 21365
#define TRANS_TCP_PORT 40365

#define QUERY_UDP_PORT 31365

#define MAX_BUFLEN 1000
#define MAX_PACLEN 10000

#define MAX_NFSERVER 10

extern char packet_buf[MAX_PACLEN];
extern char host_addr[100];

int create_listen(int *sock, int family, int type, int protocol,
		  struct sockaddr *addr, socklen_t sa_size, int backlog);

int recv_udp_packet(int fd, struct sockaddr_storage *source, int *size);

int communicate(int protocol, int echo, void *buf, int size,
		struct sockaddr *addr, socklen_t sa_size,
		void *recv_buf, int max_size, int *recv_size);

int get_hostaddr();

#endif  // _MY_INET_UTILS_H_
