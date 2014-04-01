#include "my_inet_utils.h"
#include <stdio.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

char packet_buf[MAX_PACLEN] = {0};
char host_addr[100] = {0};

int create_listen(int *sock, int family, int type, int protocol,
		  struct sockaddr *addr, socklen_t sa_size, int backlog) {
  int status = 0;
  int temp_sock;
  if ((temp_sock = socket(family, type, protocol)) == -1) {
    perror("creating socket");
    return -1;
  }
  int yes = 1;
  if (setsockopt(temp_sock, SOL_SOCKET, SO_REUSEADDR, &yes,
		 sizeof(int)) == -1) {
    perror("setsockopt");
    return -1;
  }
  if (status = bind(temp_sock, addr, sa_size)) {
    perror("binding socket");
    close(temp_sock);
    return -1;
  }
  if (type == SOCK_STREAM) {
    if (status = listen(temp_sock, backlog)) {
      perror("listening the socket");
      close(temp_sock);
      return -1;
    }
  }

  *sock = temp_sock;
  return 0;
}

int recv_udp_packet(int fd, struct sockaddr_storage *source, int *size) {
  struct sockaddr_storage remote_host;
  socklen_t sa_size = sizeof(remote_host);
  int recv_size = 0;
  recv_size = recvfrom(fd, packet_buf, sizeof(packet_buf), 0,
		       (struct sockaddr *) &remote_host, &sa_size);
  if (recv_size < 0) {
    perror("Receiving data from socket");
    return -1;
  }
  packet_buf[recv_size] = 0;
  if (source != NULL) {
    *source = remote_host;
  }
  if (size != NULL) {
    *size = (int) sa_size;
  }
  return recv_size;
}

int communicate(int protocol, int echo, void *buf, int size,
		struct sockaddr *addr, socklen_t sa_size,
		void *recv_buf, int max_size, int *recv_size) {
  int sfd, status = 0;
  struct addrinfo *info;
  struct addrinfo hints;
  if (protocol == PROTO_UDP) {
    sfd = socket(AF_INET, SOCK_DGRAM, protocol);
  }
  else if (protocol == PROTO_TCP) {
    sfd = socket(AF_INET, SOCK_STREAM, protocol);
  } else {
    fprintf(stderr, "Unsupported protocol.\n");
    return -1;
  }

  if (sfd == -1) {
    perror("create socket");
    return -1;
  }
  if (connect(sfd, addr, sa_size)) {
    perror("connect(2)");
    close(sfd);
    return -1;
  }
  if (send(sfd, buf, size, 0) == -1) {
    perror("send(2)");
    close(sfd);
    return -1;
  }
  char temp_buf[MAX_PACLEN] = {0};
  int recv_len = 0;
  if (echo) {
    fd_set waiting;
    FD_ZERO(&waiting);
    FD_SET(sfd, &waiting);
    struct timeval wait_time;
    wait_time.tv_sec = 10; // 10s timeout
    wait_time.tv_usec = 0;

    if ( (status = select(sfd+1, &waiting, NULL, NULL, &wait_time)) == -1) {
      perror("waiting for incoming packet");
      close(sfd);
      return -1;
    }
    if (FD_ISSET(sfd, &waiting)) {
      if ((recv_len = recv(sfd, temp_buf, MAX_PACLEN-1, 0)) == -1) {
	perror("recv(2)");
	close(sfd);
	return -1;
      }
      temp_buf[recv_len] = 0;
      if (recv_buf && recv_size) {
	if (recv_len > max_size) {
	  fprintf(stderr, "Buffer is too small, smaller than packet received.\n");
	  close(sfd);
	  return -1;
	}
	memcpy(recv_buf, temp_buf, recv_len);
	*recv_size = recv_len;
      }
    } else {
      printf("No packet received.\n");
    }
  }
  close(sfd);
  return 0;
}

int get_hostaddr() {
  struct hostent *he;
  struct in_addr **list;

  if ((he = gethostbyname("localhost")) == NULL) {
    herror("gethostbyname");
    return 1;
  }

  list = (struct in_addr **) he->h_addr_list;
  if (list[0] != NULL) {
    inet_ntop(AF_INET, list[0],
	      host_addr, 99);
    return 0;
  }
  return 1;
}
