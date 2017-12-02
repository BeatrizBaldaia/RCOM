#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <strings.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

#include "utils.h"
#include "ApplicationLayer.h"
#define SERVER_PORT 21

typedef struct {
  char * user;
  char * password;
  char * host;
  char * url;
}ftp_info;


int main(int argc, char** argv){
  if(argc != 2){
    printf("Usage: ./download ftp://[<user>:<password>@]<host>/<url-path>\n");
    exit(1);
  }
  link_info info;
  parser(&info, argv[1]);

  getIpByHost(&info);
  ftp_ports ftp;

  connectToServer(&ftp, info.ip, 21);

  login(&ftp, info.user, info.password);
printf("FIZ LOGIN\n");
  changeRemoteHostDirectory(&ftp, info.path);
  // struct hostent *h;
  // if ((h = gethostbyname(info.host)) == NULL) {
  //           herror("gethostbyname");
  //           exit(1);
  // }
  // printf("fez gethostbyname\n");
  // int	socket_fd;
	// struct	sockaddr_in server_addr;
	// char * dump = malloc(1000);
  // char * dump_init_ptr = dump;
  //
  //
	// //server address handling
	// bzero((char*)&server_addr,sizeof(server_addr));
  // printf("Fez bzero\n");
	// server_addr.sin_family = AF_INET;
  //
	// server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *) h->h_addr)));	//32 bit Internet address network byte ordered
	// server_addr.sin_port = htons(SERVER_PORT);		//server TCP port must be network byte ordered
  //
	// //open an TCP socket
	// if ((socket_fd = socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK,0)) < 0) {
  //   		perror("socket()");
  //       exit(0);
  // }
  // printf("Abriu socket\n");
	// //connect to the server
	// if(connect(socket_fd,
	//            (struct sockaddr *)&server_addr,
	// 	   sizeof(server_addr)) < 0){
  //   perror("connect()");
	// 	exit(0);
	// }
  // printf("Conectou ao socket\n");
  //
  // while(read(socket_fd, dump, 1) > 0) {
  //   printf("%c", dump[0]);
  //   dump += 1;
  // }
  // printf(":'(\n");

	/*
  Write user to socket
  */
	// if(write(socket_fd, "user ", 5) != 5) {
  //   printf("Error writing user to socket");
  //   exit(1);
  // }
  // printf("1\n");
  //
  // if(write(socket_fd, info.user, strlen(info.user)) != strlen(info.user)) {
  //   printf("Error writing user to socket");
  //   exit(1);
  // }
  // printf("2\n");
  //
  // if(write(socket_fd, "\n", 1) != 1) {
  //   printf("Error writing user to socket");
  //   exit(1);
  // }
  // printf("3\n");
  //
  // dump = dump_init_ptr;
  // while(read(socket_fd, dump, 1) > 0) {
  //   printf("%c", dump[0]);
  //   dump += 1;
  // }
  /*
  Write password to socket
  */
  // if(write(socket_fd, "pass ", 5) != 5) {
  //   printf("Error writing password to socket");
  //   exit(1);
  // }
  // if(write(socket_fd, info.password, strlen(info.password)) != strlen(info.password)) {
  //   printf("Error writing password to socket");
  //   exit(1);
  // }
  // if(write(socket_fd, "\n", 1) != 1) {
  //   printf("Error writing password to socket");
  //   exit(1);
  // }
  // dump = dump_init_ptr;
  // while(read(socket_fd, dump, 1) > 0) {
  //   dump += 1;
  // }
  // printf("%s\n", dump_init_ptr);
  /*
  Write "pasv" to socket (Entering Passive Mode)
  */
  // if(write(socket_fd, "pasv\n", 5) != 5) {
  //   printf("Error writing pasv to socket");
  //   exit(1);
  // }
  // dump = dump_init_ptr;
  // int dump_size = 0;
  // while(read(socket_fd, dump, 1) > 0) {
  //   dump_size++;
  //   dump += 1;
  // }
  // dump_init_ptr += 27;
  // printf("%s\n", dump_init_ptr);
  // int i = 0;
  // for(; i < 4; i++) {
  //   char * aux = malloc(1);
  //   int n = cutString((const char *)dump_init_ptr, ',', aux);
  //   if(n == 0) {
  //     printf("Error!\n");
  //     exit(1);
  //   }
  //   dump_init_ptr += n;
  //   free(aux);
  // }
  /*
  Read Port
  */
  // char * first = malloc(1);
  // int first_size = cutString((const char *)dump_init_ptr, ',', first);
  // if(first_size == 0) {
  //   printf("Error getting the Port value\n");
  //   exit(1);
  // }
  // dump_init_ptr += first_size;
  // char * second = malloc(1);
  // if(cutString((const char *)dump_init_ptr, ')', second) == 0) {
  //   printf("Error getting the Port value\n");
  //   exit(1);
  // }
  //
  // printf("port: %s %s\n", first, second);
  //
  // int port = atoi(first) * 256 + atoi(second);
	// close(socket_fd);
	// exit(0);
  return 0;
}
