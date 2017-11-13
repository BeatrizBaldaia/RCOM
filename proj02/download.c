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

int parser(ftp_info* info, const char* st){
  char* user = NULL;
  char* site = NULL;
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
  user = malloc((user_size+1) * sizeof(char));
  memcpy(user, st+7, user_size);
  user[user_size] = '\0';
  printf("USER: %s\n",user);

  char * username = NULL;
  if((username = strchr(user, ':'))==NULL)
    exit(1);  
  int username_size = (int) (username - user);  
  username = malloc((username_size + 1) * sizeof(char));
  memcpy(username, user, username_size);
  username[username_size] = '\0';
  printf("USERNAME: %s\n", username);
  
  char * password = malloc(strlen(user)-username_size-2);
  memcpy(password, user+username_size+1, strlen(user)-username_size-2);
  password[strlen(user)-username_size-1] = '\0'; 
  printf("PASSWORD: %s\n", password);
  
  if((site   = strchr(st,']')) == NULL)
    exit(1);
  site = malloc(strlen(st)-user_size-7);
  memcpy(site, st+8+user_size, strlen(st)-user_size-7);
  printf("URL: %s\n",site);

  char * host = NULL;
  if((host = strchr(site,'/')) == NULL)
    exit(1);
  int host_size = (int) (host - site); 
  host = malloc((host_size + 1) * sizeof(char));
  memcpy(host, site, host_size);
  host[host_size] = '\0';
  printf("HOST: %s\n",host);
  
  char * url = NULL;
  url = malloc((strlen(site)-host_size+1)* sizeof(char));
  memcpy(url, site+host_size, (strlen(site)-host_size));
  url[(strlen(site)-host_size)] = '\0';
  printf("URL: %s\n",url);
  
  //REFACTOR
  info->username = username;
  info->password = password;
  info->host = host;
  info->url = url;
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
  int	sockfd;
	struct	sockaddr_in server_addr;
	char	buf[] = "Mensagem de teste na travessia da pilha TCP/IP\n";  
	int	bytes;
	
	
	//server address handling
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;

	server_addr.sin_addr.s_addr = inet_addr(h->h_addr);	//32 bit Internet address network byte ordered
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
	printf("Bytes escritos %d\n", bytes);

	close(sockfd);
	exit(0);
}
