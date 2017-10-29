#define BAUDRATE B38400
#define PORT_0 "/dev/ttyS0"
#define PORT_1 "/dev/ttyS1"
#define TRANSMITTER 0
#define RECEIVER 1
#define TRANSMITTER_RR_0 2
#define TRANSMITTER_RR_1 3
#define MAX_SIZE 100

#define FALSE 0
#define TRUE 1

#define FLAG 0x7E
#define ESCAPE 0x7D
#define BYTE_STUFFING 0x20

#define TRANSMITTER_SEND_ADDR 0x03 /* Comandos (trama I, SET e DISC) enviados pelo Emissor e Respostas (tramas UA, RR e REJ) enviadas pelo Recetor*/
#define RECEIVER_SEND_ADDR 0x01 /* Comandos enviados pelo Recetor e Respostas enviadas pelo Emissor */

#define SET 0x03
#define DISC 0x0B
#define UA 0x07
#define RR_FLAG 0x05
#define REJ 0x01

#define STATE_0 0
#define STATE_1 1
#define STATE_2 2
#define STATE_3 3
#define STATE_4 4
#define STATE_SUCCESS 5
#define STATE_RETRY 6
#define STATE_ABORT 7

/**
* retorna a informacao depois de sofrer stuffing
* e altera o argumento dataBlockSize para o valor
* do tamanho da string retornada
*/
unsigned char * stuffing (unsigned char * dataBlock, int * dataBlockSize);
unsigned char generateBCC(unsigned char * buf, int size);
/**
* retorna a informacao depois de sofrer destuffing
* e altera o argumento dataBlockSize para o valor
* do tamanho da string retornada
*/
unsigned char * destuffing (unsigned char * dataBlock, int * dataBlockSize);
