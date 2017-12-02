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

#define SERVER_PORT 21

typedef struct {
  char * user;
  char * password;
  char * host;
  char * url;
}ftp_info;

struct termios oldtio,newtio;

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

int setTerminalAttributes(int fd) {
  if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
    printf("Erro getting the parameters associated with the terminal\n");
    close(fd);
    return -1;
  }

  bzero(&newtio, sizeof(newtio));

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
  newtio.c_cc[VMIN]     = 0;   /* blocking read until 1 chars received */

  tcflush(fd, TCIOFLUSH);

  if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
    printf("Erro setting the parameters associated with the terminal\n");
    close(fd);
    return -1;
  }
  return 0;
}

int resetTerminalAttributes(int fd) {
  if (tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    printf("Erro setting the parameters associated with the terminal\n");
    return -1;
  }
  return 0;
}

int parser(ftp_info * info, const char* st){
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

  char * url  = malloc(link_size + 1);
  memcpy(url, st, link_size);
  url[link_size + 1] = '\0';
  printf("URL: %s\n", url);

  info->user = user;
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
  printf("fez gethostbyname\n");
  int	socket_fd;
	struct	sockaddr_in server_addr;
	char * dump = malloc(1000);
  char * dump_init_ptr = dump;


	//server address handling
	bzero((char*)&server_addr,sizeof(server_addr));
  printf("Fez bzero\n");
	server_addr.sin_family = AF_INET;

	server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *) h->h_addr)));	//32 bit Internet address network byte ordered
	server_addr.sin_port = htons(SERVER_PORT);		//server TCP port must be network byte ordered

	//open an TCP socket
	if ((socket_fd = socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK,0)) < 0) {
    		perror("socket()");
        exit(0);
  }
  printf("Abriu socket\n");
  setTerminalAttributes(socket_fd);
	//connect to the server
	if(connect(socket_fd,
	           (struct sockaddr *)&server_addr,
		   sizeof(server_addr)) < 0){
    perror("connect()");
		exit(0);
	}
  printf("Conectou ao socket\n");

  while(read(socket_fd, dump, 1) > 0) {
    printf("%c", dump[0]);
    dump += 1;
  }
  printf(":'(\n");

	/*
  Write user to socket
  */
	if(write(socket_fd, "user ", 5) != 5) {
    printf("Error writing user to socket");
    exit(1);
  }
  printf("1\n");

  if(write(socket_fd, info.user, strlen(info.user)) != strlen(info.user)) {
    printf("Error writing user to socket");
    exit(1);
  }
  printf("2\n");

  if(write(socket_fd, "\n", 1) != 1) {
    printf("Error writing user to socket");
    exit(1);
  }
  printf("3\n");

  dump = dump_init_ptr;
  while(read(socket_fd, dump, 1) > 0) {
    printf("%c", dump[0]);
    dump += 1;
  }
  /*
  Write password to socket
  */
  if(write(socket_fd, "pass ", 5) != 5) {
    printf("Error writing password to socket");
    exit(1);
  }
  if(write(socket_fd, info.password, strlen(info.password)) != strlen(info.password)) {
    printf("Error writing password to socket");
    exit(1);
  }
  if(write(socket_fd, "\n", 1) != 1) {
    printf("Error writing password to socket");
    exit(1);
  }
  dump = dump_init_ptr;
  while(read(socket_fd, dump, 1) > 0) {
    dump += 1;
  }
  printf("%s\n", dump_init_ptr);
  /*
  Write "pasv" to socket (Entering Passive Mode)
  */
  if(write(socket_fd, "pasv\n", 5) != 5) {
    printf("Error writing pasv to socket");
    exit(1);
  }
  dump = dump_init_ptr;
  int dump_size = 0;
  while(read(socket_fd, dump, 1) > 0) {
    dump_size++;
    dump += 1;
  }
  dump_init_ptr += 27;
  printf("%s\n", dump_init_ptr);
  int i = 0;
  for(; i < 4; i++) {
    char * aux = malloc(1);
    int n = cutString((const char *)dump_init_ptr, ',', aux);
    if(n == 0) {
      printf("Error!\n");
      exit(1);
    }
    dump_init_ptr += n;
    free(aux);
  }
  /*
  Read Port
  */
  char * first = malloc(1);
  int first_size = cutString((const char *)dump_init_ptr, ',', first);
  if(first_size == 0) {
    printf("Error getting the Port value\n");
    exit(1);
  }
  dump_init_ptr += first_size;
  char * second = malloc(1);
  if(cutString((const char *)dump_init_ptr, ')', second) == 0) {
    printf("Error getting the Port value\n");
    exit(1);
  }

  printf("port: %s %s\n", first, second);

  int port = atoi(first) * 256 + atoi(second);
	close(socket_fd);
	exit(0);
}
