#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "utils.h"
#include "linkLayer.h"

int appTransmiter(char* fileName){//
	printf("%s\n %d\n",fileName, strlen(fileName));
	int fdFile = open(fileName,O_RDONLY);
	if(fdFile == -1) {
		printf("Can't open the file.\n");
		return -1;
	}
	int fd = llopen(0, TRANSMITER);
	if(fd == -1) {
		printf("Can't open the serial port.\n");
		return -1;
	}
	struct stat st;
	if (stat(fileName, &st) != 0) {
		printf("Error on stat\n");
		return -1;
	}
	size_t fileSize = st.st_size;
	//printf("%lu\n", fileSize);
	/** Type
	 * 0 – tamanho do ficheiro,
	 * 1 – nome doficheiro
	 */
	int controlPacketSize = 5 + sizeof(fileSize) + strlen(fileName);//sizeof(fileName) da sizeof unsigned char*, nao string
	unsigned char* controlPacket = malloc(controlPacketSize);
	controlPacket[0] = 0x02; //C
	controlPacket[1] = 0; //T: 0 = comprimento do ficheiro
	controlPacket[2] = sizeof(fileSize); //L
	size_t * fileSizeOnArray = (size_t* )(&controlPacket[3]);
	*fileSizeOnArray = fileSize;
	controlPacket[3 + sizeof(fileSize)] = 1; //T: 1 = nome do ficheiro
	controlPacket[4 + sizeof(fileSize)] = strlen(fileName); //L
	memcpy(&controlPacket[5 + sizeof(fileSize)], fileName, strlen(fileName));


	if(llwrite(fd, controlPacket, controlPacketSize) <= 0){
		printf("Error on llwrite\n");
		return -1;
	}

	unsigned char n = 0;
	int nDataBytes = 256;
	int dataPacketSize= nDataBytes + 4;
	unsigned char* fdDataPacket = realloc(NULL,dataPacketSize);
	int size = -1;
	while((size = read(fdFile, &(fdDataPacket[4]), nDataBytes)) < 0){
		//Enviar fdFile
		fdDataPacket[0] = 0;//C
		fdDataPacket[1] = n;//N
		n = (n + 1) % 256;
		fdDataPacket[2]= size / 256;//LL
		fdDataPacket[3]= size % 256;///*Numero*/ size % 256(5)

		if(!llwrite(fd, fdDataPacket, size + 4)) {
			printf("Error on llwrite\n");
			return -1;
		}
	}
	//Enviar final
	controlPacket[0] = 3;
	if(!llwrite(fd, controlPacket, controlPacketSize)) {
		printf("Error on llwrite\n");
		return -1;
	}

	llclose(fd);
  close(fdFile);
	return 0;
}
int appReceiver() {
	int fd = llopen(0, RECEIVER);
	if(fd == -1){
		printf("Couldn't open the serial port.\n");
		return -1;
	}

	unsigned char* controlPacket = NULL;
	controlPacket = realloc(controlPacket, 1000);
	int size = 0;
	unsigned char n = 0;
	int controlPacketSize = llread(fd, controlPacket); //Leu pacote de controle

	if(controlPacketSize <= 0) {
		printf("Error on llread.\n");
		return -1;
	}

	int i = 0, j = 0;
	unsigned int* fileSize = NULL;
	char* fileName = malloc(controlPacketSize);

	for(; i < controlPacketSize; i++) {
		if(controlPacket[i] == 0) { //file size
			memcpy(fileSize, controlPacket + i + 2, controlPacket[i + 1]);
			i += 1 + controlPacket[i + 1];
		} else if(controlPacket[i] == 1) { //file size
			int fileNameSize = controlPacket[i + 1];
			i += 2;
			for (; j < fileNameSize; j++) {
				fileName[j] = controlPacket[i];
			}
		}
	}

  printf("%s\n",fileName );

	int fdFile = open(fileName, O_CREAT | O_RDWR | O_TRUNC, 0666);
	if(fdFile == -1){
		printf("Couldn't open the file.\n");
		return -1;
	}

	unsigned char* dataPacket = NULL;
	dataPacket = realloc(dataPacket, *fileSize);

	while((size = llread(fd, dataPacket)) > 0) {//NOT FINAL
		if((dataPacket[0] == 0) && (dataPacket[1] == n)) {
			n = (n + 1) % 256;
			int sizeWrite = 4;
			int stillNeedToWrite = (size - sizeWrite);
			while((stillNeedToWrite = (size - sizeWrite)) > 0) {
				sizeWrite += write(fdFile, dataPacket + sizeWrite, stillNeedToWrite);
		  }
		} else break;
	}
    llclose(fd);
    close(fdFile);
	return -1;
}

int main(int argc, char** argv){//ENVIAR

	if(argc == 2){
		return appTransmiter(argv[1]);
	}
	if(argc == 1){
		return appReceiver();
	}
	return -1;
}
