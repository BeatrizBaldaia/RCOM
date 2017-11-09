#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef struct {
  char name[256];
  char password[256];
}user;

int parser(user* user, const char* st){
  char * begin = malloc(7 * sizeof(char));
  memcpy(begin, st, 6);
  begin[6] = '\0';
  if(strcmp("ftp://\0", begin) != 0) {
    printf("Error: url didn't begin with 'ftp://'\n");
    exit(1);
  }
  char * userInfo = malloc(strlen(st)-6);
  userInfo = strtok(&(st[6]),']');
  printf("User: %s\n", userInfo);
  return 0;
}

int main(int argc, char** argv){
  if(argc != 2){
    printf("Usage: ./download ftp://[<user>:<password>@]<host>/<url-path>\n");
    exit(1);
  }
  user user;
  parser(&user, argv[1]);
  exit(0);
}
