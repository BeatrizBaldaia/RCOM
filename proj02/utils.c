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

int parser() {
  return 0;
}

int getIpByHost(link_info* link) {
	struct hostent* h;

	if ((h = gethostbyname(link->host)) == NULL) {
		herror("gethostbyname");
		return 1;
	}

	char* ip = inet_ntoa(*((struct in_addr *) h->h_addr));
	strcpy(url->ip, ip);

	return 0;
}
