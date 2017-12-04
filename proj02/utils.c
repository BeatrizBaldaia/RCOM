#include "utils.h"

int cutString(const char * str, char ch, char * newStr) {
  int size = strlen(str);
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

int parser(link_info * link, const char * str) {

  int specificUser = ((strchr(str, '@')) == NULL) ? 0 : 1;

  int link_size = strlen(str);

  char * begin = malloc(7 * sizeof(char));
  memcpy(begin, str, 6);
  begin[6] = '\0';
  printf("Inicio: %s\n", begin);
  if(strcmp("ftp://\0", begin) != 0) {
    printf("Error: url didn't begin with 'ftp://'\n");
    exit(1);
  }

  link_size -= 6;
  str += 6;
  if(specificUser) {
    char * user = malloc(1);
    int user_size = cutString(str, ':', user);
    if(user_size == 0) {
      printf("Error on parsing user\n");
      exit(1);
    }
    link_size -= user_size;
    str += user_size;

    char * password = malloc(1);
    int password_size = cutString(str, '@', password);
    if(password_size == 0) {
      printf("Error on parsing password\n");
      exit(1);
    }

    link_size -= password_size;
    str += password_size;

    link->user = user;
    link->password = password;
  } else {
    link->user = "anonymous";
    link->password = "1234";
  }
  printf("USER: %s\n", link->user);
  printf("PASSWORD: %s\n", link->password);
  char * host = malloc(1);
  int host_size = cutString(str, '/', host);
  if(host_size == 0) {
    printf("Error on parsing host\n");
    exit(1);
  }
  printf("HOST: %s\n", host);
  link_size -= host_size;
  str += host_size;

  char * path = malloc(100);
  char * path_aux = malloc(1);
  int path_size = 0, path_size_aux = 0;
  while((path_size_aux = cutString(str, '/', path_aux)) != 0) {
    sprintf(path, "%s%s/", path, path_aux);
    link_size -= path_size_aux;
    path_size += path_size_aux;
    str += path_size_aux;
    path_aux = realloc(path_aux, 1);
  }
  printf("PATH: %s\n", path);

  char * filename  = malloc(link_size + 1);
  memcpy(filename, str, link_size);
  filename[link_size + 1] = '\0';
  printf("FILENAME: %s\n", filename);

  link->host = host;
  link->path = path;
  link->filename = filename;

  return 0;
}

int getIpByHost(link_info* link) {
	struct hostent* h;

	if ((h = gethostbyname(link->host)) == NULL) {
		herror("gethostbyname");
		return 1;
	}
	char* ip = inet_ntoa(*((struct in_addr *) h->h_addr));

  if(ip == NULL) {
    printf("Error getting IP from Host name.\n");
    return -1;
  }
	link->ip = ip;

	return 0;
}
