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
	int fdData = open(fileName,O_RDONLY);
	if(fdData == -1) {
		printf("Nao conseguio abrir ficheiro de fdData\n");
		return -1;
	}
	int fd = llopen(0,TRANSMITER);
	if(fd == -1) {
		printf("Nao conseguio abrir canal de comunicacao.\n");
		return -1;
	}
	struct stat st;
	if (stat(fileName, &st) != 0) {
		printf("Erro no stat\n");
		return -1;
	}
	size_t fileSize = st.st_size;
	printf("%lu\n", fileSize);
	/** Type
	 * 0 – tamanho do ficheiro,
	 * 1 – nome doficheiro
	 */
	int controlPacketSize = 5 + sizeof(fileSize) + sizeof(fileName);//sizeof(fileName) da sizeof char*, nao string
	char* controlPacket = malloc(controlPacketSize);
	controlPacket[0] = 0x02; //C
	controlPacket[1] = 0; //T: 0 = comprimento do ficheiro
	controlPacket[2] = sizeof(fileSize); //L
	size_t * locationSize = (size_t* )(&controlPacket[3]);
	*locationSize = fileSize;
	controlPacket[3+sizeof(size_t)] = 1; //T: 1 = nome do ficheiro
	controlPacket[4+sizeof(size_t)] = strlen(fileName); //L
	strcpy(&controlPacket[6],fileName);


	if(!llwrite(fd,controlPacket,controlPacketSize)){
		printf("Erro no llwrite\n");
		return -1;
	}
	//Enviar inicial--^
	unsigned char n=0;
	int pacotefdDataSize=/*Numero0*/ 255+4;
	char* pacotefdData = realloc(NULL,pacotefdDataSize);
	int size = -1;
	while(0<(size = read(fdData, &(pacotefdData[4]),255/*Numero*/))){
		//Enviar fdData
		pacotefdData[0] = 1;//C
		pacotefdData[1] = n;//N
		n++;
		pacotefdData[2]= 0;//LL //TODO ProvavellyWrong size/255(6)
		pacotefdData[3]= size;///*Numero*/ size % 256(5)
		if(!llwrite(fd,pacotefdData,size+4)){
			printf("Erro no llwrite\n");
			return -1;
		}
	}
	//Enviar final
	controlPacket[0]=3;
	if(!llwrite(fd,controlPacket,controlPacketSize)){
		printf("Erro no llwrite\n");
		return -1;
	}
	llclose(fd);
	return 0;
}
int appReceiver(char* fileName){//
	int fdData = open(fileName,O_CREAT|O_EXCL|O_WRONLY|O_APPEND);
	if(fdData == -1){
		printf("Nao conseguiu abrir ficheiro de fdData");
		return -1;
	}
	printf("\nfez open\n\n");
	int fd = llopen(0,RECEIVER);
	if(fd == -1){
		printf("Nao conseguiu abrir canal de comunicacao.\n");
		return -1;
	}
	printf("saiu do llopen\n");
	char* pacotefdData= NULL;
	pacotefdData = realloc(pacotefdData,1000);//Alter
	int size=0;
	int controleSize = llread(fd, pacotefdData);
	while(0<(size = llread(fd, pacotefdData))) {//NOT FINAL
		int sizeWrite = 0;
		while(size - sizeWrite> 0){
			sizeWrite += write(fdData, pacotefdData + sizeWrite, size);
		}
	}
	return -1;
}


int main(int argc, char** argv){//ENVIAR
	if (argc != 3){//TODO tpacotefdDataestar
		printf("Usage:\tapp receive fileName\n\tex: app 0 teste.txt\n");
	}
	if(strcmp(argv[1],"0") == 0){
		return appTransmiter(argv[2]);
	}
	if(strcmp(argv[1],"1") == 0){
		return appReceiver(argv[2]);
	}
	return -1;
}
