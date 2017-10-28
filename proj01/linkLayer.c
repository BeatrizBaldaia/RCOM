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
unsigned char set[5] = {FLAG, TRANSMITER_SEND_ADDR, SET, TRANSMITER_SEND_ADDR ^ SET, FLAG};
unsigned char receiverUA[5] = {FLAG, TRANSMITER_SEND_ADDR, UA, TRANSMITER_SEND_ADDR ^ UA, FLAG};
unsigned char transmiterUA[5] = {FLAG, RECEIVER_SEND_ADDR, UA, RECEIVER_SEND_ADDR ^ UA, FLAG};
unsigned char rr[5] = {FLAG, TRANSMITER_SEND_ADDR, RR_FLAG, TRANSMITER_SEND_ADDR ^ RR_FLAG, FLAG};
unsigned char rej[5] = {FLAG, TRANSMITER_SEND_ADDR, REJ, TRANSMITER_SEND_ADDR ^ REJ, FLAG};
unsigned char transmiterDISC[5] = {FLAG, TRANSMITER_SEND_ADDR, DISC, TRANSMITER_SEND_ADDR ^ DISC, FLAG};
unsigned char receiverDISC[5] = {FLAG, RECEIVER_SEND_ADDR, DISC, RECEIVER_SEND_ADDR ^ DISC, FLAG};

struct termios oldtio,newtio;
int timeoutCount = 0, state = STATE_0;
int S = 0;
int task;

/**
    Handles the SIGALRM

    @param sig The signal number
*/
void alarm_handler(int sig) {
  timeoutCount++;
  if(timeoutCount == linkLayer.numTransmissions) {
    state = STATE_ABORT;
    timeoutCount = 0;
  } else {
    state = STATE_RETRY;
  }
}
/**
    State 0 of the state machine. Updates the current state (state)
    to STATE_1 if reads a FLAG

    @param bytePacket The read byte.
*/
void inSTATE_0 (unsigned char bytePacket) {
  if(bytePacket == FLAG){
    state = STATE_1;
  }
}

/**
    State 1 of the state machine. Updates the current state (state)
    to STATE_2 if reads the address field.
    Stays in state 1 if receives a FLAG.
    Returns to state 0, otherwise.

    @param bytePacket The read byte.
*/
void inSTATE_1 (unsigned char bytePacket) {
  if(bytePacket == FLAG) {
    state = STATE_1;
  } else if(bytePacket == TRANSMITER_SEND_ADDR || bytePacket == RECEIVER_SEND_ADDR) {
    state = STATE_2;
  } else {
    state = STATE_0;
  }
}

/**
    State 2 of the state machine. Updates the current state (state)
    to STATE_3 if reads the control field appropriate to the
    user task.
    Goes back to state 1 if receives a FLAG.
    Returns to state 0, otherwise.

    @param bytePacket The read byte.
*/
void inSTATE_2 (unsigned char bytePacket, int status) {
  if(bytePacket == FLAG) {
    state = STATE_1;
  } else if(status == TRANSMITER && bytePacket == UA) {
    state = STATE_3;
  } else if(status == RECEIVER && (bytePacket == SET || bytePacket == UA)) {
      state = STATE_3;
  } else if(status == TRANSMITER_RR_0 && bytePacket == RR_FLAG) {
      state = STATE_3;
  } else if(status == TRANSMITER_RR_1 && bytePacket == (RR_FLAG ^ 0x80)) {//TODO
    state = STATE_3;
  } else if((status == TRANSMITER_RR_0 || status == TRANSMITER_RR_1) && bytePacket == REJ) {
    state = STATE_3;
  } else if(bytePacket == DISC) {
    state = STATE_3;
  }
  else {
    state = STATE_0;
  }
}

/**
    State 3 of the state machine. Updates the current state (state)
    to STATE_4 if the BCC1 is right (BCC1 = A ^ C).
    Goes back to state 1 if receives a FLAG.
    Returns to state 0, otherwise.

    @param bytePacket The read byte.
    @param status The user's task.
    @param retransmit Variable to warn if the transmiter will have to retransmit the I frame
*/
void inSTATE_3 (unsigned char bytePacket, int status, int * retransmit) {
  if(bytePacket == FLAG) {
    state = STATE_1;
  } else if(status == TRANSMITER && bytePacket == (TRANSMITER_SEND_ADDR ^ UA)) {//emissor esta a receber UA
    state = STATE_4;
  } else if(status == RECEIVER && (bytePacket == (TRANSMITER_SEND_ADDR ^ SET) || bytePacket == (RECEIVER_SEND_ADDR ^ UA))) {//recetor esta a receber SET
    state = STATE_4;
  } else if(status == TRANSMITER_RR_0 && bytePacket == (TRANSMITER_SEND_ADDR ^ RR_FLAG)) {//emissor esta a receber RR com seq. number = 0
    state = STATE_4;
  } else if(status == TRANSMITER_RR_1 && bytePacket == (TRANSMITER_SEND_ADDR ^ RR_FLAG ^ 0x80)) {//TODO//emissor esta a receber RR com seq. number = 1
    state = STATE_4;
  } else if((status == TRANSMITER_RR_0 || status == TRANSMITER_RR_1) && bytePacket == (TRANSMITER_SEND_ADDR ^ REJ)) {//emissor estava a espera de uma resposta e acaba por receber REJ
    state = STATE_4;
    *retransmit = 1;
  } else if((status == TRANSMITER && bytePacket == (RECEIVER_SEND_ADDR ^ DISC)) || (status == RECEIVER && bytePacket == (TRANSMITER_SEND_ADDR ^ DISC))) {//pedido para terminar comunicacao
    state = STATE_4;
  } else {
    state = STATE_0;
  }
}

/**
    State 4 of the state machine. Successful end of the state
    machine if a FLAG is now received.
    Returns to state 0, otherwise.

    @param bytePacket The read byte.
    @param status The user's task.
    @param retransmit Variable to warn if the transmiter will have to retransmit the I frame
    @return 0 if succeed, -1 otherwise (need to read the all frame again)
*/
int inSTATE_4 (unsigned char bytePacket, int * retransmit) {
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
/**
* Maquina de estados para o receiver receber tramas de supervisao (S) e nao numeradas (U)
*/
int receiverWaitingForPacket(int fd) {
  unsigned char bytePacket = 0;
  int retransmit = 0;
  state = STATE_0;
  while(1) {
    if(read(fd,&bytePacket,1) != 1) {
      printf("Error reading.\n");
      return -1;
    }
    switch (state) {
      case STATE_0:
        inSTATE_0(bytePacket);
        break;
      case STATE_1:
        inSTATE_1(bytePacket);
      break;
      case STATE_2:
        inSTATE_2(bytePacket, task);
      break;
      case STATE_3:
        retransmit = 0;//a variavel aqui nao tem valor pratico nenhum
        inSTATE_3(bytePacket, task, &retransmit);
      break;
      case STATE_4:
        retransmit = 0;
        int result = inSTATE_4(bytePacket, &retransmit);
        if(result == 0) {
          return 0;
        }
      break;
    }
  }
}

int transmiterWaitingForPacket(int fd, int status) {
  if(state == STATE_ABORT) {
    printf("TIMEOUT exceeded.\n");
    timeoutCount = 0;
    return -1;
  } else {
    state = STATE_0;
  }

  int retransmit = 0;
  signal(SIGALRM, alarm_handler);
  unsigned char bytePacket = 0;
  alarm(linkLayer.timeout);
  while(1) {
    if(state == STATE_ABORT) {
      printf("TIMEOUT exceeded.\n");
      timeoutCount = 0;
      return -1;
    } else if(state == STATE_RETRY) {
      if(status != RECEIVER) {
        printf("TIMEOUT %d\n\n", timeoutCount + 1);
        return -2;//emissor vai retransmitir (escrever) a trama
      }
    }
    if(read(fd,&bytePacket,1) == 0){
        continue;
    }

    switch (state) {
      case STATE_0:
        inSTATE_0(bytePacket);
        break;
      case STATE_1:
        inSTATE_1(bytePacket);
        break;
      case STATE_2:
        inSTATE_2(bytePacket, status);
        break;
      case STATE_3:
        inSTATE_3(bytePacket, status, &retransmit);
        break;
      case STATE_4:{
        int result = inSTATE_4(bytePacket, &retransmit);
        if(result == 0) {
          timeoutCount = 0;
          alarm(0);//"If seconds is 0, a pending alarm request, if any, is canceled."
          return result;
        } else if(result == -2) {
          printf("REJ- Transmiter needs to retransmit.\n");
          alarm(0);
              //sleep(1);//AnabelaSilva
          timeoutCount++;//a rececao de REJ conta como um tentativa de envio, por isso timeoutCount++
          //a recepcao de rej significa que esta aberta a comunicacao
          if(timeoutCount == linkLayer.numTransmissions) {
            state = STATE_ABORT;
            timeoutCount = 0;
          }
          return result;
        }
        break;
      }
    }
  }
}


/**
    Waits for a frame/packet of Information.

    @param fd port to read
    @param dataBlockSize returns the size of the packet read
    @return data read on success, NULL otherwise
*/
unsigned char * waitingForPacketI (int fd, int * dataBlockSize) {
  state = STATE_0;
  unsigned char bytePacket = 0;
  int nRead = 0;
  unsigned char data[1000];//string com o campo de dados e com o BCC2
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
      } else if((S == 0 && bytePacket == 0x40) || (S == 1 && bytePacket == 0x00)) { //trama e um duplicado; ignorar e enviar um RR_FLAG
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
        dataSize = 0;
      } else if((S == 0 && bytePacket == 0x40) || (S == 1 && bytePacket == 0x00)) { //trama e um duplicado; ignorar e enviar um RR_FLAG
          if(S) {
            rr[2] = RR_FLAG ^ 0x80;
            rr[3] = TRANSMITER_SEND_ADDR ^ RR_FLAG ^ 0x80;
          } else {
            rr[2] = RR_FLAG;
            rr[3] = TRANSMITER_SEND_ADDR ^ RR_FLAG;
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
		      unsigned char * destuffedData = destuffing(data, &dataSize);
          int destuffedDataSize = dataSize - 1; //nao consideramos o BCC2
          unsigned char bcc2 = generateBCC(destuffedData, destuffedDataSize);

          if(bcc2 == destuffedData[destuffedDataSize]) {//nao ha erros na trama I
            (*dataBlockSize) = destuffedDataSize;
            return destuffedData;
          } else {//cabecalho certo, mas dados errados; reenviar trama I
             free(destuffedData);
             printf("BCC errado\n");
  			     if(write(fd, rej, 5) != 5) {
               printf("Error writing to the serial port.\n");
             }
             state = STATE_0;
          }

      } else {//vai guardando bytes de dados
          data[dataSize] = bytePacket;
          dataSize++;
      }
    }
  }
	return NULL;
}


/**
    Receiver waits for DISC

    @param fd port to open
    @return 0 on success, -1 otherwise
*/
int receiverWaitingForDISC(int fd) {
  state=STATE_0;
  unsigned char bytePacket = 0;//Iniciar com valor, LCOM
  while(1) {
    if(read(fd,&bytePacket,1) != 1) {
      printf("Error reading.\n");
      return -1;
    }
    int retransmit = 0;//a variavel aqui nao tem valor pratico nenhum

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
        inSTATE_3(bytePacket, task, &retransmit);
        break;
      }case STATE_4:{
        int result = inSTATE_4(bytePacket, &retransmit);
        if(result == 0) {
          return 0;
        }
       break;
      }
    }
  }
}


/**
    Opens the port of communication between Transmitter and Receiver

    @param fd port to open
    @param status receiver or transmitter
    @return number of bytes write, -1 if couldn't write
*/
int llopen(int port, int status) {
  linkLayer.baudRate = BAUDRATE;
  linkLayer.timeout = 3;
  linkLayer.numTransmissions = 3;
  if(port == 0) {
    strcpy(linkLayer.port, PORT_0);
  } else if(port == 1) {
    strcpy(linkLayer.port, PORT_1);
  } else {
    printf("Invalid port\n");
    return -1;
  }
  int fd = open(linkLayer.port, O_RDWR | O_NOCTTY );  //Opens the port
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
       if(write(fd,set,5) != 5) {		//Sends SET
          printf("Error writing to the serial port.\n");
          return -1;
        }
        result = transmiterWaitingForPacket(fd, TRANSMITER);		//Waits for UA
      } while (result != -1 && result != 0);
      if(result == -1) {
        printf("Coundn't get UA.\n");
        resetTerminalAttributes(fd);
        close(fd);
        return result;
      }
      break;
    case RECEIVER:
      task = RECEIVER;
      setTerminalAttributes(fd);
      if(receiverWaitingForPacket(fd) == -1) {		//Waits for SET
        resetTerminalAttributes(fd);
        return -1;
      }//fica em loop infinito ate receber o SET
      if(write(fd, receiverUA, 5) != 5) {		//Sends UA
        printf("Error writing to the serial port.\n");
        return -1;
      }
      break;
    default:
      printf("Invalid computer task/status.\n");
      return -1;
  }
  return fd;
}


/**
    Sends the information. Does the stuffing and prepares frame I to send 

    @param fd port to write
    @param buffer information to write
    @param length size of buffer
    @return number of bytes write, -1 if couldn't write
*/
int llwrite(int fd, unsigned char * buffer, int length) {
  unsigned char BCC2 = generateBCC(buffer, length);
  int stuffedBCC2Size = 1;
  unsigned char * stuffedBCC2 = stuffing (&BCC2, &stuffedBCC2Size);
  int stuffedBufferSize = length;
  unsigned char * stuffedBuffer = stuffing (buffer, &stuffedBufferSize);
  int packetILength = 5 + stuffedBCC2Size + stuffedBufferSize;
  unsigned char * packetI = (unsigned char *)malloc(packetILength * sizeof(unsigned char));

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
  free(stuffedBCC2);//AnabelaSilva
  packetI[4 + stuffedBufferSize + stuffedBCC2Size] = FLAG;//F
  int result = 0;//INICIAR LCOM
  do {
  //  int i=0;
    //for(i = 0; i < packetILength; i++){
      //printf("Packet :%x\n",packetI[i] );
    //}
    printf("Controlo %x\n",packetI[2] );
    if(write(fd, packetI, packetILength) != packetILength) {
    printf("Error writing to the serial port.\n");
    return -1;
    }
    int typeOfAnswer = S ? TRANSMITER_RR_1 : TRANSMITER_RR_0;
    result = transmiterWaitingForPacket(fd, typeOfAnswer);
  } while(result != -1 && result != 0);
  if(result == -1) {
    printf("Coundn't get RR.\n");
    resetTerminalAttributes(fd);
    close(fd);
    return -1;
  }
  free (packetI);//AnabelaSilva ealteracao
  return length;
}


/**
    Reads the information sent by the Transmitter

    @param fd port to read
    @param buffer information read 
    @return number of bytes read, -1 if couldn't read
*/
int llread(int fd, unsigned char * buffer) {
  int bufferSize = -1;
  unsigned char * aux = waitingForPacketI(fd, &bufferSize);
  buffer = memcpy(buffer, aux, bufferSize);
  free(aux);
    if(bufferSize != -1) {
      if(S==0) {
          rr[2] = RR_FLAG ^ 0x80;
          rr[3] = TRANSMITER_SEND_ADDR ^ RR_FLAG ^ 0x80;
	  S = 1;
      } else {
          rr[2] = RR_FLAG;
          rr[3] = TRANSMITER_SEND_ADDR ^ RR_FLAG;
	  S = 0;
      }

      if(write(fd, rr, 5) != 5) {	//Sends RR on success
	printf("Couldn't send RR\n");
      }
    }

  return bufferSize;
}

/**
    Closes the communication between the transmitter and the receiver

    @param fd port to close
    @return 0 if succeed, -1 otherwise (need to read the all frame again)
*/
int llclose(int fd) {
  int result=0;
  if(task == TRANSMITER) {
    do {
      if(write(fd, transmiterDISC, 5) != 5) {		//Sends DISC to receiver
        printf("Error writing to the serial port.\n");
        return -1;
      }
      printf("\nVai ler Disc\n");
      result = transmiterWaitingForPacket(fd, task);	//Waits for a DISC
    } while(result != -1 && result != 0);
    if(result == 0) {
      if(write(fd, transmiterUA, 5) != 5) {		//Sends UA to receiver
        printf("Error writing to the serial port.\n");
        return -1;
      }
      printf("Close successed!\n");
      sleep(1);
      printf("Close successed!\n");
      resetTerminalAttributes(fd);
      close(fd);
      return 0;
    }
    else {
      printf("Force the close...\n");
      resetTerminalAttributes(fd);
      return -1;
    }

  }
  else {//RECEIVER
    if(receiverWaitingForDISC(fd) != 0) {		//Waits for a DISC
        printf("Error closing the communication!\n");
        resetTerminalAttributes(fd);
        return -1;
      }

    if(write(fd, receiverDISC, 5) != 5) {		//Sends DISC to transmitter
        printf("Error writing to the serial port.\n");
        return -1;
      }

    if (receiverWaitingForPacket(fd) == -1) {		//Waits for UA
      resetTerminalAttributes(fd);
      return -1;
    }


    resetTerminalAttributes(fd);
    return 0;
  }
  return -1;
}
