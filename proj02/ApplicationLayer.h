#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

typedef struct FTP_Ports
{
    int command_port; // file descriptor to control socket, where we send commands
    int data_port; // file descriptor to data socket, from where we receive data
} ftp_ports;

int connectToServer(ftp_ports * ftp, char * ip, int port);
int login(ftp_ports * ftp, const char * user, const char * password);
int changeRemoteHostDirectory(ftp_ports * ftp, const char * path);
int enterPassiveMode(ftp_ports * ftp);
int startFileTransmission(ftp_ports * ftp, const char * filename);
int saveFile(ftp_ports * ftp, const char * filename);
int disconnectFromServer(ftp_ports * ftp);
