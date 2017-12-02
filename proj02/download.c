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

  changeRemoteHostDirectory(&ftp, info.path);

  enterPassiveMode(&ftp);

  startFileTransmission(&ftp, info.filename);

  saveFile(&ftp, info.filename);

  disconnectFromServer(&ftp);

  return 0;
}
