#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "utils.h"
#include "linkLayer.h"

//Eficiencia
#include <time.h>
//End eficiencia



int appTransmitter(char* fileName, unsigned int timeout, unsigned int numTransmissions, int bytesPerFrame){//
	//printf("%s\n %d\n",fileName, strlen(fileName));

//Eficiencia
struct timespec initialT, endT;
//  timespec_get(&initialT,TIME_UTC);
clock_gettime(CLOCK_MONOTONIC, &initialT);
//End eficiencia
	int fdFile = open(fileName,O_RDONLY);
	if(fdFile == -1) {
		printf("Can't open the file.\n");
		return -1;
	}

	initProtocol(timeout, numTransmissions);

	int fd = llopen(0, TRANSMITTER);
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


	if(llwrite(fd, controlPacket, controlPacketSize) == -1){
		printf("Error on llwrite\n");
		return -1;
	}

	unsigned char n = 0;
	int nDataBytes = bytesPerFrame;
	int dataPacketSize= nDataBytes + 4;
	unsigned char* fdDataPacket = realloc(NULL,dataPacketSize);
	int size = -1;
	while((size = read(fdFile, &(fdDataPacket[4]), nDataBytes)) > 0){
		//Enviar fdFile
		fdDataPacket[0] = 0;//C
		fdDataPacket[1] = n;//N
		n = (n + 1) % 256;
		fdDataPacket[2]= size / 256;//LL
		fdDataPacket[3]= size % 256;///*Numero*/ size % 256(5)

		if(llwrite(fd, fdDataPacket, size + 4) == -1) {
			printf("Error on llwrite\n");
			return -1;
		}
	}
	//Enviar final
	controlPacket[0] = 3;
	if(llwrite(fd, controlPacket, controlPacketSize) == -1) {
		printf("Error on llwrite\n");
		return -1;
	}



	llclose(fd);
  close(fdFile);
	//Eficiencia
 clock_gettime(CLOCK_MONOTONIC, &endT);
		//timespec_get(&endT,TIME_UTC);
printf("Time of transfer: %f\n", (endT.tv_sec+endT.tv_nsec*10.0E-9) - (initialT.tv_sec+initialT.tv_nsec*10.0E-9));
	//End eficiencia
	return 0;
}
int appReceiver() {
	initProtocol(3, 3);
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
			fileSize = (unsigned int*) (controlPacket + i + 2);

			i += 1 + controlPacket[i + 1];
		} else if(controlPacket[i] == 1) { //file size
			int fileNameSize = controlPacket[i + 1];
			i += 2;
			for (; j < fileNameSize; j++, i++) {
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
/*
1 - ./app
2 - file name
3 - timeout
4 - number of transmissions
5 - number of bytes per frame
*/
	unsigned int timeout = 0;
	unsigned int nTransmissions = 0;
	int bytesPerFrame = 0;
	if(argc == 5) {
		timeout = (unsigned int)atoi(argv[2]);
		nTransmissions = (unsigned int)atoi(argv[3]);
		bytesPerFrame = atoi(argv[4]);
		if(timeout > 500 || timeout < 1 || nTransmissions > 10 || nTransmissions < 1 || bytesPerFrame > 50000 || bytesPerFrame < 10) {
			printf("Invalid values.\n");
			return -1;
		}
		return appTransmitter(argv[1], timeout, nTransmissions, bytesPerFrame);
	} else if(argc == 4) {
		timeout = (unsigned int)atoi(argv[2]);
		nTransmissions = (unsigned int)atoi(argv[3]);
		if(timeout > 500 || timeout < 1 || nTransmissions > 10 || nTransmissions < 1 ) {
			printf("Invalid values.\n");
			return -1;
		}
		return appTransmitter(argv[1], timeout, nTransmissions, 256);
	} else if(argc == 3) {
		timeout = (unsigned int)atoi(argv[2]);
		if(timeout > 500 || timeout < 1) {
			printf("Invalid values.\n");
			return -1;
		}
		return appTransmitter(argv[1], timeout, 3, 256);
	} else if(argc == 2) {
		return appTransmitter(argv[1], 3, 3, 256);
	} else if(argc == 1){
		return appReceiver();
	}
	return -1;
}
