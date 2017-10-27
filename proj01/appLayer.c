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
	//printf("%lu\n", fileSize);
	/** Type
	 * 0 – tamanho do ficheiro,
	 * 1 – nome doficheiro
	 */
	int controlPacketSize = 5 + sizeof(fileSize) + sizeof(fileName);//sizeof(fileName) da sizeof unsigned char*, nao string
	unsigned char* controlPacket = malloc(controlPacketSize);
	controlPacket[0] = 0x02; //C
	controlPacket[1] = 0; //T: 0 = comprimento do ficheiro
	controlPacket[2] = sizeof(fileSize); //L
	size_t * locationSize = (size_t* )(&controlPacket[3]);
	*locationSize = fileSize;
	controlPacket[3+sizeof(size_t)] = 1; //T: 1 = nome do ficheiro
	controlPacket[4+sizeof(size_t)] = strlen(fileName); //L
	strcpy(&controlPacket[6],fileName);


	if(llwrite(fd,controlPacket,controlPacketSize) <= 0){
		printf("Erro no llwrite\n");
		return -1;
	}
	//Enviar inicial--^
	unsigned char n = 0;
	int pacotefdDataSize=/*Numero0*/ 255+4;
	unsigned char* pacotefdData = realloc(NULL,pacotefdDataSize);
	int size = -1;
	while(0<(size = read(fdData, &(pacotefdData[4]),255/*Numero*/))){
		//Enviar fdData
		pacotefdData[0] = 1;//C
		pacotefdData[1] = n;//N
		n++;
		pacotefdData[2]= 0;//LL //TODO ProvavellyWrong size/255(6)
		pacotefdData[3]= size;///*Numero*/ size % 256(5)
        int i=0;
        for(i=0;i<size+4;i++){
          //  printf("pacotefdData[%d] = %d\n",i,pacotefdData[i]);//C
        }
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
    close(fdData);
	return 0;
}
int appReceiver(){//
	//printf("\nfez open\n\n");
	int fd = llopen(0,RECEIVER);
	if(fd == -1){
		printf("Nao conseguiu abrir canal de comunicacao.\n");
		return -1;
	}
	//printf("saiu do llopen\n");
	unsigned char* pacotefdData= NULL;
	pacotefdData = realloc(pacotefdData,1000);//Alter
	int size=0;
	unsigned char n = 0;
	int controleSize = llread(fd, pacotefdData);
	char* fileName = malloc(controleSize); 
	int k = 0;
	int i = 0;
	if (pacotefdData[1] == 0) {
	    k = pacotefdData[2] + 5;
	}
	if (pacotefdData[1] == 1) {
	    controleSize = pacotefdData[2] + 3;
	    k = 3;
	}
	/** Type
	 * 0 – tamanho do ficheiro,
	 * 1 – nome doficheiro
	 */
	for( i = 0; k < controleSize;k++,i++){
	    fileName[i]=pacotefdData[k];
	}
	fileName[i] = '\0';
	printf("Nome do ficheiro :%s", fileName);
	int fdData = open(fileName,O_CREAT|O_EXCL|O_RDWR|O_APPEND,0666);
	if(fdData == -1){
		printf("Nao conseguiu abrir ficheiro de fdData");
		return -1;
	}
	
	while(0<(size = llread(fd, pacotefdData))) {//NOT FINAL
		if((pacotefdData[0] == 1) && (pacotefdData[1] == n)){
			n++;
			int sizeWrite = 4;
			int stillNeedToWrite=(size - sizeWrite);
			while((stillNeedToWrite=((size - sizeWrite)))> 0){
                //write(STDOUT_FILENO,pacotefdData + sizeWrite, stillNeedToWrite);
								// for(int k = 0; k < stillNeedToWrite; k++) {
								// 	printf("%X\n", pacotefdData[sizeWrite + k]);
								// }
				sizeWrite += write(fdData, pacotefdData + sizeWrite, stillNeedToWrite);
		}
		}else break;
	}
    llclose(fd);
    close(fdData);
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
