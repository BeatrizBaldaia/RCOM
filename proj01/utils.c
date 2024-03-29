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

unsigned char generateBCC(unsigned char * buf, int size) {
  unsigned char result = 0x00;
  int i = 0;
  for (; i < size; i++) {
    result = result ^ buf[i];
  }
  return result;
}

unsigned char * stuffing (unsigned char * dataBlock, int * dataBlockSize) {
  int size = *dataBlockSize;
  unsigned char * stuffedDataBlock = (unsigned char *)malloc((size * 2) * sizeof(unsigned char));
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

unsigned char * destuffing (unsigned char * dataBlock, int * dataBlockSize) {
  int size = *dataBlockSize;
  unsigned char * destuffedDataBlock = (unsigned char *)malloc((size * 2) * sizeof(unsigned char));
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
