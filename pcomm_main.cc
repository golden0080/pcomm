#include <stdio.h>
#include <stdlib.h>

// Other includes
#include "my_inet_utils.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
// ------------------
/*
 * Tool usage:
 * pcomm [-?] [-p tcp|udp] [-h <hostname>] [-f <datafile>] [-n] [-t <timeout>] <port> <packet data appended>
 */
#define MAX_DATALEN 2000

static int protocol = PROTO_UDP;
static int port;
static int echo = 1;
static char *hostname = NULL;

int read_to_buf(void *buf, int max_size, FILE *fp) {
  fseek(fp, 0, SEEK_END);
  int file_size = ftell(fp);
  rewind(fp);
  if (file_size > max_size) {
    fclose(fp);
    fprintf(stderr, "Error: data file is too large, %d(limit: %d)\n",
	    file_size, max_size);
    return -1;
  }
  char line[MAX_BUFLEN];
  int size, count = 0;
  while(!feof(fp)) {
    size = fread(line, 1, MAX_BUFLEN, fp);
    memcpy((char*)buf+count, line, size);
    count += size;
  }
  file_size = count;
  fclose(fp);
  return file_size;
}

int send_and_recv(void *buf, int size, int timeout) {
  int sfd, status = 0;
  struct addrinfo *info;
  struct addrinfo hints;
  if (protocol == PROTO_UDP) {
    sfd = socket(AF_INET, SOCK_DGRAM, protocol);
  }
  else if (protocol == PROTO_TCP) {
    sfd = socket(AF_INET, SOCK_STREAM, protocol);
  }

  if (sfd == -1) {
    perror("create socket");
    return -1;
  }
  hints.ai_family = AF_INET;
  hints.ai_protocol = protocol;
  if (hostname == NULL) {
    hints.ai_flags = AI_PASSIVE;
  }
  
  if (protocol == PROTO_UDP) {
    hints.ai_socktype = SOCK_DGRAM;
  } else {
    hints.ai_socktype = SOCK_STREAM;
  }
  char port_str[20];
  sprintf(port_str, "%d", port);
  if ( (status = getaddrinfo(hostname, port_str, &hints,
			     &info))) {
    fprintf(stderr, "Error: failed to get address info of %s - %s\n",
	    (hostname) ?hostname : "localhost",
	    gai_strerror(status));
    close(sfd);
    return -1;
  }
  int success = 0;
  for(struct addrinfo *ptr = info;
      ptr != NULL; ptr = ptr->ai_next) {
    if (connect(sfd, ptr->ai_addr, ptr->ai_addrlen)) {
      perror("connect(2)");
      continue;
    }
    if (send(sfd, buf, size, 0) == -1) {
      perror("send(2)");
      continue;
    }
    shutdown(sfd, SHUT_WR);
    char recv_buf[MAX_PACLEN];
    int recv_len = 0;
    if (echo) {
      fd_set waiting;
      FD_ZERO(&waiting);
      FD_SET(sfd, &waiting);
      struct timeval wait_time, *wait_ptr = NULL;
      if (timeout > 0) {
	wait_time.tv_sec = timeout;
	wait_time.tv_usec = 0;
	wait_ptr = &wait_time;
      }
      if ( (status = select(sfd+1, &waiting, NULL, NULL, wait_ptr)) == -1) {
	perror("waiting for incoming packet");
	continue;
      }
      if (FD_ISSET(sfd, &waiting)) {
	if ((recv_len = recv(sfd, recv_buf, MAX_PACLEN-1, 0)) == -1) {
	  perror("recv(2)");
	  continue;
	}
	recv_buf[recv_len] = 0;
	printf("Received %s Packet(%d): %s\n",
	       (protocol == PROTO_TCP) ? "TCP" : "UDP",
	       recv_len, recv_buf);
      } else {
	printf("No packet received.\n");
	freeaddrinfo(info);
	close(sfd);
	return 1;
      }
    }
    success = 1;
    break;
  }
  close(sfd);
  freeaddrinfo(info);
  if (!success) {
    fprintf(stderr, "Unsuccessfully send packet.\n");
    return -1;
  }
  return 0;
}

void print_usage() {
  printf("Usage:\n" "\tpcomm [-?] [-p tcp|udp] [-t <timeout in seconds>] [-n]"
	 " [-h <hostname>] [-f <datafile>] <port> <packet data appended>\n");
}

int main(int argc, char **argv) {
  char append_data[MAX_DATALEN+1];
  int file_size = 0, data_size = 0;
  int opt = 0;
  int ret = 0;
  int timeout = 5;
  while(ret == 0 && (opt = getopt(argc, argv, "?p:h:f:t:n")) != -1 ) {
    FILE *fp = NULL;
    switch(opt) {
    case 't':
      errno = 0;
      timeout = (int) strtol(argv[optind], NULL, 10);
      if (errno != 0) {
	fprintf(stderr, "Invalid port number %s.\n", argv[optind]);
	ret = -1;
	goto do_exit;
      }
      if (timeout < 0) {
	fprintf(stderr, "Timeout should be non-negative.\n");
	ret = -1;
	goto do_exit;
      }
      break;
    case 'n':
      echo = 0;
      break;
    case '?':
      print_usage();
      goto do_exit;
      break;
    case 'p':
      if (strcmp(optarg, "tcp") == 0) {
	protocol = PROTO_TCP;
      }
      break;
    case 'h':
      hostname = (char *) malloc(strlen(optarg)+1);
      memcpy(hostname, optarg, strlen(optarg) +1);
      break;
    case 'f':
      fp = fopen(optarg, "r");
      if (fp == NULL) {
	fprintf(stderr, "Error: open data file %s\n", optarg);
	ret = -1;
	goto do_exit;
      }
      if ( (file_size = read_to_buf(append_data, MAX_DATALEN, fp)) == -1) {
	ret = -1;
	goto do_exit;
      }
      break;
    default:
      print_usage();
      ret = -1;
    }
  }
  if (argc < optind + 2) {
    fprintf(stderr, "Require more parameters.\n");
    print_usage();
    goto do_exit;
  }
  errno = 0;
  port = (int) strtol(argv[optind], NULL, 10);
  if (errno != 0) {
    fprintf(stderr, "Invalid port number %s.\n", argv[optind]);
    ret = -1;
    goto do_exit;
  }
  ++optind;

  if (strlen(argv[optind]) + file_size > MAX_DATALEN) {
    fprintf(stderr, "Error: total data is too large, %lud(limit: %d)\n",
	    strlen(argv[optind])+file_size, MAX_DATALEN);
    ret = -1;
    goto do_exit;
  }
  memcpy(append_data+file_size, argv[optind], strlen(argv[optind]));
  data_size = file_size + strlen(argv[optind]);
  // Options parsing complete
  if (send_and_recv(append_data, data_size, timeout)) {
    ret = -1;
  }
 do_exit:
  if (hostname != NULL) {
    free(hostname);
  }
  exit(ret);
}
