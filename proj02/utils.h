#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include <netinet/in.h>

typedef struct LINK_INFO {
	char * user;
	char * password;
	char * host;
	char * ip; 
	char * path;
	char * filename;
	int port;
} link_info;
