/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,/*c,*/ res;
    struct termios oldtio,newtio;
    char buf[255];

    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }
	//COMO USAR

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */
  
    
    fd = open(argv[1], O_RDWR | O_NOCTTY );//O_NOCTTY nao controla CTRL-C
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) *///ALTERAR?? NAO
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    //newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received *///ALTERAR??
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 1 chars received *///ALTERAR??



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

	char * buf_pt=buf;
    while (STOP==FALSE) {       /* loop for input */
	//printf("WHILE\n");      
	res = read(fd,buf_pt,1);   /* returns after 5 chars have been input *///BECOUSE "newtio.c_cc[VMIN] = 5;"
	//printf("READ\n");      
	//buf[res]=0;               /* so we can printf... *///WHY
      //printf(":%s:%d\n", buf, res);
      //if (buf[0]=='z') STOP=TRUE;//ALTERAR??
	
      if (buf_pt[res-1]=='\0') STOP=TRUE;
 	else
 	  buf_pt++;
    }
    printf(":%s:%d\n", buf,strlen(buf));

	write(fd,buf,strlen(buf)+1);
  /* 
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no guião 
  */



    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
