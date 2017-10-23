#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "utils.h"
#include "linkLayer.h"

struct linkLayer linkLayer;
char set[5] = {FLAG, TRANSMITER_SEND_ADDR, SET, TRANSMITER_SEND_ADDR ^ SET, FLAG};
char receiverUA[5] = {FLAG, TRANSMITER_SEND_ADDR, UA, TRANSMITER_SEND_ADDR ^ UA, FLAG};
char transmiterUA[5] = {FLAG, RECEIVER_SEND_ADDR, UA, RECEIVER_SEND_ADDR ^ UA, FLAG};
char rr[5] = {FLAG, TRANSMITER_SEND_ADDR, RR, TRANSMITER_SEND_ADDR ^ RR, FLAG};
char rej[5] = {FLAG, TRANSMITER_SEND_ADDR, REJ, TRANSMITER_SEND_ADDR ^ REJ, FLAG};
char transmiterDISC[5] = {FLAG, TRANSMITER_SEND_ADDR, TRANSMITER_SEND_ADDR ^ DISC, FLAG};
char receiverDISC[5] = {FLAG, RECEIVER_SEND_ADDR, RECEIVER_SEND_ADDR ^ DISC, FLAG};

struct termios oldtio,newtio;
int timeoutCount = 0, state = STATE_0;
int S = 0;
int task;
/**
* Vai apanhar e tratar o SIGALRM
*/
void alarm_handler(int sig) {
  printf("TIMEOUT!\n");
  timeoutCount++;
  if(timeoutCount == linkLayer.numTransmissions) {
    state = STATE_ABORT;
    timeoutCount = 0;
  } else {
    state = STATE_RETRY;
  }

}
/**
* Preenche a struct linkLayer
*/
void initProtocol() {

}
void inSTATE_0 (unsigned char bytePacket) {
  printf("ESTA NO ESTADO 0\n");
  if(bytePacket == FLAG){
    state = STATE_1;
    printf("MUDOU PARA ESTADO %d\n", state);
  }
}

void inSTATE_1 (char bytePacket) {
  printf("ESTA NO ESTADO 1\n");
  if(bytePacket == FLAG) {
    state = STATE_1;
  } else if(bytePacket == TRANSMITER_SEND_ADDR) {
    state = STATE_2;
  } else {
    printf("vai para estado 0...\n");
    state = STATE_0;
  }
  printf("MUDOU PARA ESTADO %d\n", state);
}

void inSTATE_2 (char bytePacket, int status) {
  printf("ESTA NO ESTADO 2\n");
  if(bytePacket == FLAG) {
    state = STATE_1;
  } else if(status == TRANSMITER && bytePacket == UA) {
    state = STATE_3;
  } else if(status == RECEIVER && bytePacket == SET) {
      state = STATE_3;
  } else if(status == TRANSMITER_RR_0 && bytePacket == RR) {
      state = STATE_3;
  } else if(status == TRANSMITER_RR_1 && bytePacket == (RR ^ 0x40)) {
    state = STATE_3;
  } else if((status == TRANSMITER_RR_0 || status == TRANSMITER_RR_1) && bytePacket == REJ) {
    state = STATE_3;
  } else if(bytePacket == DISC) {
    state = STATE_3;
  }
  else {
    state = STATE_0;
  }
  printf("MUDOU PARA ESTADO %d\n", state);
}

void inSTATE_3 (char bytePacket, int status, int * retransmit) {
  printf("ESTA NO ESTADO 3\n");
  if(bytePacket == FLAG) {
    state = STATE_1;
  } else if(status == TRANSMITER && bytePacket == (TRANSMITER_SEND_ADDR ^ UA)) {//emissor esta a receber UA
    state = STATE_4;
  } else if(status == RECEIVER && bytePacket == (TRANSMITER_SEND_ADDR ^ SET)) {//recetor esta a receber SET
    state = STATE_4;
  } else if(status == TRANSMITER_RR_0 && bytePacket == (TRANSMITER_SEND_ADDR ^ RR)) {//emissor esta a receber RR com seq. number = 0
    state = STATE_4;
  } else if(status == TRANSMITER_RR_1 && bytePacket == (TRANSMITER_SEND_ADDR ^ RR ^ 0x40)) {//emissor esta a receber RR com seq. number = 1
    state = STATE_4;
  } else if((status == TRANSMITER_RR_0 || status == TRANSMITER_RR_1) && bytePacket == (TRANSMITER_SEND_ADDR ^ REJ)) {//emissor estava a espera de uma resposta e acaba por receber REJ
    state = STATE_4;
    *retransmit = 1;
  } else if((status == TRANSMITER && bytePacket == (TRANSMITER_SEND_ADDR ^ DISC)) || (status == RECEIVER && bytePacket == (RECEIVER_SEND_ADDR ^ DISC))) {//pedido para terminar comunicacao
    state = STATE_4;
  } else {
    state = STATE_0;
  }
  printf("MUDOU PARA ESTADO %d\n", state);
}

int inSTATE_4 (char bytePacket, int * retransmit) {
  printf("ESTA NO ESTADO 4\n");
  if(bytePacket == FLAG) {
    state = STATE_0;
    if(*retransmit == 1) {//cabecalho bem, mas data mal => foi lido REJ
      return -2; //pedir para retransmitir a trama I
    } else {
      return 0;//recebeu resposta com sucesso
    }
  } else {
    state = STATE_0;
  }
  printf("MUDOU PARA ESTADO %d\n", state);
  return -1;//voltar a ler de inicio
}

int setTerminalAttributes(int fd) {
  if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
    printf("Erro getting the parameters associated with the terminal\n");
    close(fd);
    return -1;
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
  newtio.c_cc[VMIN]     = task == TRANSMITER ? 0 : 1;   /* blocking read until 1 chars received */

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

int receiverWaitingForSET(int fd) {
  char bytePacket = 0;//LCOM
  int retransmit = 0;
  state = STATE_0;
  while(1) {
    printf("vai ler um byte de set\n");
    if(read(fd,&bytePacket,1) != 1) {
      printf("Error reading.\n");
      return -1;
    }
    printf("%d\n", state);
    switch (state) {
      case STATE_0:
        printf("no estado 0\n\n");
        inSTATE_0(bytePacket);
        break;
      case STATE_1:
        printf("no estado 1\n\n");
        inSTATE_1(bytePacket);
      break;
      case STATE_2:
        printf("no estado 2\n\n");
        inSTATE_2(bytePacket, task);
      break;
      case STATE_3:
      printf("no estado 3\n\n");
      retransmit = 0;//a variavel aqui nao tem valor pratico nenhum
        inSTATE_3(bytePacket, task, &retransmit);
      break;
      case STATE_4:
      printf("no estado 4\n\n");
        retransmit = 0;
        int result = inSTATE_4(bytePacket, &retransmit);
        if(result == 0) {
          printf("Received an answer with success.\n");
          return 0;
        }
      break;
    }
  }
}

int waitingForPacket(int fd, int status) {
  state = STATE_0;
  int retransmit = 0;
  signal(SIGALRM, alarm_handler);
  char bytePacket = 0;
  alarm(linkLayer.timeout);
  while(1) {
    if(state == STATE_ABORT) {
      printf("TIMEOUT exceeded.\n");
      timeoutCount = 0;
      return -1;
    } else if(state == STATE_RETRY) {
      if(status == TRANSMITER) {
        printf("timeoutCount: %d\n\n", timeoutCount);
        return -2;//emissor vai retransmitir (escrever) a trama
      }
    }
    read(fd,&bytePacket,1);
    // if(read(fd,&bytePacket,1) != 1) {
    //   printf("Error reading.\n");
    //   return -1;
    // }

    switch (state) {
      case STATE_0:{
        inSTATE_0(bytePacket);
        printf("ESTA NO ESTADO %d\n", state);
        break;
      }case STATE_1:{
        inSTATE_1(bytePacket);
        printf("ESTA NO ESTADO %d\n", state);
      break;
      }case STATE_2:{
        inSTATE_2(bytePacket, status);
        printf("ESTA NO ESTADO %d\n", state);
      break;
      }case STATE_3:{
        inSTATE_3(bytePacket, status, &retransmit);
        printf("ESTA NO ESTADO %d\n", state);
      break;
      }case STATE_4:{
        int result = inSTATE_4(bytePacket, &retransmit);
        printf("ESTA NO ESTADO %d\n", state);
        if(result == 0) {
          printf("Received an answer with success.\n");
          timeoutCount = 0;
          alarm(0);//"If seconds is 0, a pending alarm request, if any, is canceled."
          return 0;
        } else if(result == -2) {
          printf("REJ- Need to retransmit.\n");
          alarm(0);
          timeoutCount++;//a rececao de REJ conta como um tentativa de envio, por isso timeoutCount++
          if(timeoutCount == linkLayer.numTransmissions) {
            state = STATE_ABORT;
            timeoutCount = 0;
          }
          return -2;
        }
      break;
    }}
  }
}

char * waitingForPacketI (int fd, int * dataBlockSize) {
  state = STATE_0;
  char bytePacket = 0;
  int nRead = 0;
  char data[100];//string com o campo de dados e com o BCC2
  int dataSize = 0;
  while((nRead = read(fd,&bytePacket,1)) > 0) {
    if(nRead != 1) {
      printf("Error reading.\n");
      return NULL;
    }
    switch (state) {
      case STATE_0:
      inSTATE_0(bytePacket);
      break;
      case STATE_1:
      inSTATE_1(bytePacket);
      break;
      case STATE_2:
      if(bytePacket == FLAG) {
        state = STATE_1;
      } else if((S == 0 && bytePacket == 0x00) || (S == 1 && bytePacket == 0x40)) { //caso em que recetor esta a receber uma trama I
        state = STATE_3;
      }  else if((S == 0 && bytePacket == 0x40) || (S == 1 && bytePacket == 0x00)) { //trama e um duplicado; ignorar e enviar um RR
        state = STATE_3;
      } else {
        state = STATE_0;
      }
      break;
      case STATE_3:
      if(bytePacket == FLAG) {
        state = STATE_1;
      } else if ((S == 0 && bytePacket == (TRANSMITER_SEND_ADDR ^ 0x00)) || (S == 1 && bytePacket == (TRANSMITER_SEND_ADDR ^ 0x40))) {
        //cabecalho foi lido com sucesso. enviar REJ cajo dados (D1 ... Dn) estejam mal
        state = STATE_4;
      } else if((S == 0 && bytePacket == 0x40) || (S == 1 && bytePacket == 0x00)) { //trama e um duplicado; ignorar e enviar um RR
        if(S) {
          rr[2] = RR ^ 0x80;
          rr[3] = TRANSMITER_SEND_ADDR ^ RR ^ 0x80;
        } else {
          rr[2] = RR;
          rr[3] = TRANSMITER_SEND_ADDR ^ RR;
        }
        if(write(fd,rr,5) != 5) {
        printf("Error writing to the serial port.\n");
      }
      return NULL;
      } else {
        state = STATE_0;
      }
      break;
      case STATE_4:
      if(bytePacket == FLAG) {//ja leu todos os dados + BCC2
        char * destuffedData = destuffing(data, &dataSize);
        int destuffedDataSize = dataSize - 1; //nao consideramos o BCC2
        char bcc2 = generateBCC(destuffedData, destuffedDataSize);
        if(bcc2 == destuffedData[destuffedDataSize]) {//nao ha erros na trama I
          (*dataBlockSize) = destuffedDataSize;
          return destuffedData;
        } else {//cabecalho certo, mas dados errados; reenviar trama I
          if(write(fd,rej,5) != 5) {
          printf("Error writing to the serial port.\n");
          }
          state = STATE_1;
        }

      } else {//vai guardando bytes de dados
        data[dataSize] = bytePacket;
        dataSize++;
      }
    }
  }
	return NULL;
}

int receiverWaitingForDISC(int fd) {
 char bytePacket = 0;//Iniciar com valor, LCOM
  while(1) {
    if(read(fd,&bytePacket,1) != 1) {
      printf("Error reading.\n");
      return -1;
    }

    switch (state) {
      case STATE_0:{
        inSTATE_0(bytePacket);
        break;
      }case STATE_1:{
        inSTATE_1(bytePacket);
      break;
      }case STATE_2:{
        inSTATE_2(bytePacket, task);
      break;
      }case STATE_3:{
      int retransmit = 0;//a variavel aqui nao tem valor pratico nenhum
        inSTATE_3(bytePacket, task, &retransmit);
      break;
      }case STATE_4:{
        int retransmit = 0;
        int result = inSTATE_4(bytePacket, &retransmit);
        if(result == 0) {
          printf("Received an answer with success.\n");
          return 0;
        }
      break;
    }}
  }
}

int llopen(int port, int status) {
  printf("entrou no llopen\n");
  linkLayer.baudRate = BAUDRATE;
  linkLayer.timeout = 1;
  linkLayer.numTransmissions = 3;
  if(port == 0) {
    strcpy(linkLayer.port, PORT_0);
  } else if(port == 1) {
    strcpy(linkLayer.port, PORT_1);
  } else {
    printf("Invalid port\n");
    return -1;
  }
  printf("vai fazer open, na porta : %s\n\n",linkLayer.port);
  int fd = open(linkLayer.port, O_RDWR | O_NOCTTY );
  printf("abriu porta de comunicacao\n");
  if(fd < 0) {
    printf("Fail opening %s\n", linkLayer.port);
    return -1;
  }

  switch (status) {
    case TRANSMITER:
    task = TRANSMITER;
    setTerminalAttributes(fd);
    int result = 0;
    do{
      if(write(fd,set,5) != 5) {
        printf("Error writing to the serial port.\n");
        return -1;
      }
      result = waitingForPacket(fd, TRANSMITER);
      //printf("Test 1 ---------------------------------- %d\n", result );
    } while (result != -1 && result != 0);
    if(result == -1) {
      printf("Coundn't get UA.\n");
      resetTerminalAttributes(fd);
      close(fd);
      return result;
    }
    break;
    case RECEIVER:
    printf("vai esperar receiverUApelo set\n");
    task = RECEIVER;
    setTerminalAttributes(fd);
    if(receiverWaitingForSET(fd) == -1) {
      resetTerminalAttributes(fd);
      return -1;
    }//fica em loop infinito ate receber o SET
    if(write(fd, receiverUA, 5) != 5) {
      printf("Error writing to the serial port.\n");
      return -1;
    }
    printf("mandou ua\n");
    break;
    default:
    printf("Invalid computer task/status.\n");
    return -1;
  }
  return fd;
}

/**
* Link Layer recebe da aplicação o campo de informacao (D1, ..., Dn),
* ou seja, o char * buffer, faz o stuffing e prepara a trama I
* para a enviar.
*
* retorna => 0 - sucesso; -1 - erro;
*/
int llwrite(int fd, char * buffer, int length) {
//protecao da trama (stuffing)
  char BCC2 = generateBCC(buffer, length);
  int stuffedBCC2Size = 1;
  char * stuffedBCC2 = stuffing (&BCC2, &stuffedBCC2Size);//Nao manda um char*??
  int stuffedBufferSize = length;
  char * stuffedBuffer = stuffing (buffer, &stuffedBufferSize);
//////////////////////////////
  int packetILength = 5 + stuffedBCC2Size + stuffedBufferSize;
  char * packetI = (char *)malloc(packetILength * sizeof(char));

  packetI[0] = FLAG;//F
  packetI[1] = TRANSMITER_SEND_ADDR;//A
  if(S == 0) {
    packetI[2] = 0x00;//C
    S = 1;
  } else {
    packetI[2] = 0x40;//C
    S = 0;
  }
  packetI[3] = packetI[1] ^ packetI[2];//BCC1
  memcpy(packetI + 4, stuffedBuffer, stuffedBufferSize);//campo de informacao (D1 ... Dn)
  memcpy(packetI + stuffedBufferSize + 4, stuffedBCC2, stuffedBCC2Size); //BCC2

  packetI[4 + stuffedBufferSize + stuffedBCC2Size] = FLAG;//F
  int result =0;//INICIAR LCOM
  do {
    if(write(fd, packetI, packetILength) != packetILength) {
    printf("Error writing to the serial port.\n");
    return -1;
    }
//Vai esperar por uma resposta
    int typeOfAnswer = S ? TRANSMITER_RR_1 : TRANSMITER_RR_0;
    result = waitingForPacket(fd, typeOfAnswer);
  } while(result != -1 && result != 0);
  if(result == -1) {
    printf("Coundn't get RR.\n");
    resetTerminalAttributes(fd);
    close(fd);
  }

//////////////////////////////
  return result;
}

int llread(int fd, char * buffer) {
  int bufferSize = -1;
  buffer = waitingForPacketI(fd, &bufferSize);
  return bufferSize;
}

int llclose(int fd) {
int result=0;
if(task == TRANSMITER) {
    do {
      if(write(fd, transmiterDISC, 5) != 5) {
      printf("Error writing to the serial port.\n");
      return -1;
      }
  //Vai esperar por uma resposta
      result = waitingForPacket(fd, task);
    } while(result != -1 && result != 0);
    if(result == 0) {
      printf("Close successed!\n");
      resetTerminalAttributes(fd);
      close(fd);
      return 0;
    } else {
      printf("Force the close...\n");
      resetTerminalAttributes(fd);
    //  close(fd);
      return -1;
    }

  } else {//RECEIVER
    if(receiverWaitingForDISC(fd) == 0) {
      printf("Close successed!\n");
    }
    if(write(fd, receiverDISC, 5) != 5) {
    printf("Error writing to the serial port.\n");
    return -1;
    }
    resetTerminalAttributes(fd);
    return 0;
  }
return -1;
}
