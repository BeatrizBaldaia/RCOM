#include "ApplicationLayer.h"

int writeToSocket(ftp_ports* ftp, const char* str, size_t size) {
	int bytes;

	if ((bytes = write(ftp->command_port, str, size)) <= 0) {
		printf("Error writing to socket.\n");
		return -1;
	}

	return 0;
}

int readFromSocket(ftp_ports* ftp, char* str, size_t size) {
	FILE* fd;
  if((fd = fdopen(ftp->command_port, "r")) == NULL) {
    printf("Error tring to open file.\n");
    return -1;
  }

	do {
		memset(str, 0, size);//empty string
		str = fgets(str, size, fd);//fill string
    if(str == NULL) {
      break;
    }
    printf("%s", str);
	} while (1);

	return 0;
}

int establishConnection(const char * ip, int port) {
  int socket_fd;
  struct sockaddr_in server_addr;
  char buff[1000];

  // server address handling
	bzero((char*) &server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip); /*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(port); /*server TCP port must be network byte ordered */

	// open an TCP socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket()");
		return -1;
	}

	// connect to the server
	if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr))
			< 0) {
		perror("connect()");
		return -1;
	}

  return socket_fd;
}

int connectToServer(ftp_ports * ftp, char * ip, int port) {
  int socket_fd = establishConnection(ip, port);
  if(socket_fd < 0) {
    printf("Error tring to connect to socket.\n");
    return -1;
  }

  ftp->command_port = socket_fd;

  if (readFromSocket(ftp, buff, 1000)) {
		printf("Couldn't read from socket.\n");
		return -1;
	}

	return 0;
}

int login(ftp_ports * ftp, const char * user, const char * password) {
  char buff[1000];

  sprintf(buff, "USER %s\n", user);
  if (writeToSocket(ftp, buff, 1000)) {
    printf("Couldn't write to socket\n");
    return -1;
  }
  if (readFromSocket(ftp, buff, 1000)) {
		printf("Couldn't read from socket.\n"),
    return -1;
	}
  memset(buff, 0, 1000);

  sprintf(buff, "PASS %s\n", password);
  if (writeToSocket(ftp, buff, 1000)) {
    printf("Couldn't write to socket\n");
    return -1;
  }
  if (readFromSocket(ftp, buff, 1000)) {
		printf("Couldn't read from socket.\n"),
    return -1;
	}

  return 0;
}

int changeRemoteHostDirectory(ftp_ports * ftp, const char * path) {
  char buff[1000];

  sprintf(buff, "CWD %s\n", path);
  if (writeToSocket(ftp, buff, 1000)) {
    printf("Couldn't write to socket\n");
    return -1;
  }
  if (readFromSocket(ftp, buff, 1000)) {
		printf("Couldn't read from socket.\n"),
    return -1;
	}

  return 0;
}

int enterPassiveMode(ftp_ports ftp) {
  char buff[1000];

  sprintf(buff, "PASV\n");
  if (writeToSocket(ftp, buff, 1000)) {
    printf("Couldn't write to socket\n");
    return -1;
  }
  if (readFromSocket(ftp, buff, 1000)) {
		printf("Couldn't read from socket.\n"),
    return -1;
	}

  int a1, a2, a3, a4;//IP address
  int p1, p2;//Port number

  if ((sscanf(pasv, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &a1, &a2, &a3, &a4, &p1, &p2)) < 0) {
		printf("Error scaning IP and Port.\n");
		return -1;
	}

  memset(buff, 0, 1000);
  if ((sprintf(buff, "%d.%d.%d.%d", a1, a2, a3, a4)) < 0) {
		printf("Error getting IP address.\n");
		return -1;
	}

  int port = p1 * 256 + p2;
  int fd = establishConnection(buff, port);
  if(fd < 0) {
    printf("Error tring to connect to socket\n");
    return -1;
  }

  ftp.data_port = fd;

  return 0;
}

int startFileTransmission(ftp_ports * ftp, const char * filename) {
  char buff[1000];

  sprintf(buff, "RETR %s\n", filename);
  if (writeToSocket(ftp, buff, 1000)) {
    printf("Couldn't write to socket\n");
    return -1;
  }
  if (readFromSocket(ftp, buff, 1000)) {
		printf("Couldn't read from socket.\n"),
    return -1;
	}

  return 0;
}

int saveFile(ftp_ports * ftp, const char * filename) {
  FILE* file;
	int bytes;

	if (!(file = fopen(filename, "w"))) {
		printf("Error on opening file.\n");
		return -1;
	}

	char buff[1000];
	while ((bytes = read(ftp->data_port, buff, 1000))) {
		if (bytes < 0) {
			printf("Error reading the file.\n");
			return -1;
		}

		if ((bytes = fwrite(buf, 1, bytes, file)) <= 0) {//size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
			printf("Error writing to file.\n");
			return -1;
		}
	}

	fclose(file);
	close(ftp->data_port);

	return 0;
}

int disconnectFromServer(ftp_ports * ftp) {
  char buff[1000];

  if (readFromSocket(ftp, buff, 1000)) {
		printf("Couldn't read from socket.\n"),
    return -1;
	}

  sprintf(buff, "QUIT\n");
  if (writeToSocket(ftp, buff, 1000)) {
    printf("Couldn't write to socket\n");
    return -1;
  }

  close(ftp->command_port);

  return 0;
}
