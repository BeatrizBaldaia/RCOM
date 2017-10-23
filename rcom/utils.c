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

char generateBCC(char * buf, int size) {
  char result = 0x00;
  int i = 0;
  for (; i < size; i++) {
    result = result ^ buf[i];
  }
  return result;
}

char * stuffing (char * dataBlock, int * dataBlockSize) {
  int size = *dataBlockSize;
  char * stuffedDataBlock = (char *)malloc((size * 2) * sizeof(char));
  int stuffedDataBlock_it = 0, dataBlock_it = 0;

  for (; dataBlock_it < size; dataBlock_it++, stuffedDataBlock_it++) {
    if (dataBlock[dataBlock_it] == ESCAPE || dataBlock[dataBlock_it] == FLAG) {
        stuffedDataBlock[stuffedDataBlock_it] = ESCAPE;
        stuffedDataBlock_it++;
        stuffedDataBlock[stuffedDataBlock_it] = dataBlock[dataBlock_it] ^ BYTE_STUFFING;
    } else {
      stuffedDataBlock[stuffedDataBlock_it] = dataBlock[dataBlock_it];
    }
  }
  *dataBlockSize = stuffedDataBlock_it;
  return stuffedDataBlock;
}

char * destuffing (char * dataBlock, int * dataBlockSize) {
  int size = *dataBlockSize;
  char * destuffedDataBlock = (char *)malloc((size * 2) * sizeof(char));
  int destuffedDataBlock_it = 0, dataBlock_it = 0;

  for (; dataBlock_it < size; dataBlock_it++, destuffedDataBlock_it++) {
    if (dataBlock[dataBlock_it] == ESCAPE) {
      dataBlock_it++;
      destuffedDataBlock[destuffedDataBlock_it] = dataBlock[dataBlock_it] ^ BYTE_STUFFING;
    } else {
      destuffedDataBlock[destuffedDataBlock_it] = dataBlock[dataBlock_it];
    }
  }
  *dataBlockSize = destuffedDataBlock_it;
  return destuffedDataBlock;
}
/*
int main(int argc, char** argv){
  unsigned char data[1000];
  data[0] = 0x02;
  data[1] = 0x7D;
  data[2] = 0X5D;
  int size = strlen(data);

  //printf("tamanho: %d   string: %02X %02X\n", 2,  data[0], data[1]);
  char* newStr = destuffing(data, &size);
  printf("destuffed: %02X %02X    tamanho: %d\n", newStr[0], newStr[1], size);

}*/
