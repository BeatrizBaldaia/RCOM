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

#define SERVER_PORT 6000
#define SERVER_ADDR "192.168.28.96"

typedef struct {
  char name[256];
  char password[256];
}user;

int parser(const char* st){
  char* user = NULL;
  char* url = NULL;
  char * begin = malloc(7 * sizeof(char));
  memcpy(begin, st, 6);
  begin[6] = '\0';
  printf("Inicio: %s\n", begin);
  if(strcmp("ftp://\0", begin) != 0) {
    printf("Error: url didn't begin with 'ftp://'\n");
    exit(1);
  }

  if((user = strchr(st, ']'))==NULL)
    exit(1);
  int user_size = (int)(user - (st+7));
  printf("USER SIZE: %d\n", user_size);
  user = malloc((user_size+1) * sizeof(char));
  memcpy(user, st+7, user_size);
  user[user_size] = '\0';
  printf("USER: %s\n",user);
  
  if((url = strchr(st,']')) == NULL)
    exit(1);
  url = malloc(strlen(st)-user_size-7);
  memcpy(url, st+8+user_size, strlen(st)-user_size-7);
  printf("URL: %s\n",url);

  return 0;
}

int main(int argc, char** argv){
  if(argc != 2){
    printf("Usage: ./download ftp://[<user>:<password>@]<host>/<url-path>\n");
    exit(1);
  }
  //user user;
  parser(argv[1]);
  /*
  int	sockfd;
	struct	sockaddr_in server_addr;
	char	buf[] = "Mensagem de teste na travessia da pilha TCP/IP\n";  
	int	bytes;
	
	//server address handling
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);	//32 bit Internet address network byte ordered
	server_addr.sin_port = htons(SERVER_PORT);		//server TCP port must be network byte ordered 
    
	//open an TCP socket
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    		perror("socket()");
        exit(0);
  }
	//connect to the server
  if(connect(sockfd, 
	           (struct sockaddr *)&server_addr, 
		   sizeof(server_addr)) < 0){
    perror("connect()");
		exit(0);
	}
    	//send a string to the server
	bytes = write(sockfd, buf, strlen(buf));
	printf("Bytes escritos %d\n", bytes);

	close(sockfd);
	*/exit(0);
}
