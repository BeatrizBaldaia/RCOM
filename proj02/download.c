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

#define SERVER_PORT 21

typedef struct {
  char * username;
  char * password;
  char * host;
  char * url;
}ftp_info;

int cutString(const char * str, char ch, char * newStr) {
  int size = strlen((const char *)str);
  newStr[0] = str[0];
  int i = 1;
  for(; i < size; i++) {
    if(str[i] == ch) {
      return i + 1;
    }
    newStr = realloc(newStr, i + 1);
    newStr[i] = str[i];
  }
  return 0;
}

int parser(ftp_info* info, const char* st){
  int link_size = strlen(st);

  char * begin = malloc(7 * sizeof(char));
  memcpy(begin, st, 6);
  begin[6] = '\0';
  printf("Inicio: %s\n", begin);
  if(strcmp("ftp://\0", begin) != 0) {
    printf("Error: url didn't begin with 'ftp://'\n");
    exit(1);
  }

  link_size -= 6;
  st += 6;
  char * user = malloc(1);
  int user_size = cutString(st, ':', user);
  if(user_size == 0) {
    printf("Error on parsing user\n");
    exit(1);
  }
  printf("USER: %s\n", user);
  link_size -= user_size;
  st += user_size;

  char * password = malloc(1);
  int password_size = cutString(st, '@', password);
  if(password_size == 0) {
    printf("Error on parsing password\n");
    exit(1);
  }
  printf("PASSWORD: %s\n", password);
  link_size -= password_size;
  st += password_size;

  char * host = malloc(1);
  int host_size = cutString(st, '/', host);
  if(password_size == 0) {
    printf("Error on parsing host\n");
    exit(1);
  }
  printf("HOST: %s\n", host);
  link_size -= host_size;
  st += host_size;

  char * url  = malloc((link_size + 1) * sizeof(char));
  memcpy(url, st, link_size);
  url[link_size + 1] = '\0';
  printf("URL: %s\n", url);
  
  return 0;
}

int main(int argc, char** argv){
  if(argc != 2){
    printf("Usage: ./download ftp://[<user>:<password>@]<host>/<url-path>\n");
    exit(1);
  }
  ftp_info info;
  parser(&info, argv[1]);

  struct hostent *h;
  if ((h = gethostbyname(info.host)) == NULL) {
            herror("gethostbyname");
            exit(1);
  }
  printf("HOST%s\n", info.host);
  int	sockfd;
	struct	sockaddr_in server_addr;
	char	buf[] = "Mensagem de teste na travessia da pilha TCP/IP\n";
	int	bytes;


	//server address handling
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;

	server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *) h->h_addr)));	//32 bit Internet address network byte ordered
	server_addr.sin_port = htons(SERVER_PORT);		//server TCP port must be network byte ordered

	//open an TCP socket
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    		perror("socket()");
        exit(0);
  }
	//connect to the server
		printf("Triyng to cONNECT\n");
  if(connect(sockfd,
	           (struct sockaddr *)&server_addr,
		   sizeof(server_addr)) < 0){
    perror("connect()");
		exit(0);
	}
	printf("CONNECTED, GOING TO WRITE\n");
    	//send a string to the server
	bytes = write(sockfd, buf, strlen(buf));
  char * login = malloc(sizeof(char)*(strlen(info.username) + 5));
  login[0]='u';
  login[1]='s';
  login[2]='e';
  login[3]='r';
  login[4]=' ';
  memcpy(login+5,info.username,strlen(info.username));
  //bytes = write(sockfd, buf, strlen(buf));
  bytes = write(sockfd, login, strlen(login));
	printf("Bytes escritos %d\n", bytes);

	close(sockfd);
	exit(0);
}
