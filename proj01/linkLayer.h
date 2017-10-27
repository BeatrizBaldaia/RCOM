
struct linkLayer {
  char port[20]; /* /dev/ttySx   (x = 0, 1, ...) */
  int baudRate; /* velocidade de transmissão (bytes por segundo) */
  unsigned int sequenceNumber; /* numero de sequencia da trama (0 ou 1) */
  unsigned int timeout; /* valor do temporizador */
  unsigned int numTransmissions; /* numero de tentativas em caso de falha */
  unsigned char frame[MAX_SIZE]; /* trama (I, S ou U) */
};

void alarm_handler(int sig);
void initProtocol();
void inSTATE_0(unsigned char bytePacket);
void inSTATE_1(unsigned char bytePacket);
//void inSTATE_2 (unsigned char bytePacket);
void inSTATE_2(unsigned char bytePacket, int status);
//void inSTATE_3 (unsigned char bytePacket);
void inSTATE_3(unsigned char bytePacket, int status, int * retransmit);
//void inSTATE_4 (unsigned char bytePacket);
int inSTATE_4(unsigned char bytePacket, int * retransmit);
int setTerminalAttributes(int fd);
int resetTerminalAttributes(int fd);
int receiverWaitingForSET(int fd);
int waitingForPacket(int fd, int status);
unsigned char* waitingForPacketI(int fd, int * dataBlockSize);
int llopen(int port, int status);
int llwrite(int fd, unsigned char * buffer, int length);
int llread(int fd, unsigned char * buffer);
int llclose(int fd);
