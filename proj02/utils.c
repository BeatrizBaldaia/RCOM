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

int parser(link_info * link, const char * str) {
  int specificUser = (str[6] == '[') ? 1 : 0;

  int link_size = strlen(st);

  char * begin = malloc(7 * sizeof(char));
  memcpy(begin, st, 6);
  begin[6] = '\0';
  printf("Inicio: %s\n", begin);
  if(strcmp("ftp://\0", begin) != 0) {
    printf("Error: url didn't begin with 'ftp://'\n");
    exit(1);
  }
  if(specificUser) {
    link_size -= 7;
    str += 7;

    char * user = malloc(1);
    int user_size = cutString(st, ':', user);
    if(user_size == 0) {
      printf("Error on parsing user\n");
      exit(1);
    }
    printf("USER: %s\n", user);
    link_size -= user_size;
    str += user_size;

    char * password = malloc(1);
    int password_size = cutString(st, '@', password);
    if(password_size == 0) {
      printf("Error on parsing password\n");
      exit(1);
    }
    printf("PASSWORD: %s\n", password);
    link_size -= password_size;
    str += password_size;

    link->user = user;
    link->password = password;
  } else {
    link_size -= 6;
    str += 6;

    link->user = "anonymous";
    link->password = "1234";
  }

  char * host = malloc(1);
  int host_size = cutString(str, '/', host);
  if(host_size == 0) {
    printf("Error on parsing host\n");
    exit(1);
  }
  printf("HOST: %s\n", host);
  link_size -= host_size;
  str += host_size;

  char * path = [""];
  char * path_aux = malloc(1);
  int path_size = 0, path_size_aux = 0;
  while((path_size = cutString(str, '/', path_aux)) != 0) {
    sprintf(path, "%s%s/\n", path, path_aux);
    link_size -= path_size_aux;
    path_size += path_size_aux;
    str += path_size_aux;
    free(path_aux);
    path_aux = malloc(1);
  }
  printf("PATH: %s\n", path);

  char * filename  = malloc(link_size + 1);
  memcpy(url, str, link_size);
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
	strcpy(link->ip, ip);

	return 0;
}
