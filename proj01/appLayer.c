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
	printf("%s\n %d\n",fileName,strlen(fileName));
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
	int controlPacketSize = 5 + sizeof(fileSize) + strlen(fileName);//sizeof(fileName) da sizeof unsigned char*, nao string
	unsigned char* controlPacket = malloc(controlPacketSize);
	controlPacket[0] = 0x02; //C
	controlPacket[1] = 0; //T: 0 = comprimento do ficheiro
	controlPacket[2] = sizeof(fileSize); //L
	size_t * locationSize = (size_t* )(&controlPacket[3]);
	*locationSize = fileSize;
	controlPacket[3+sizeof(fileSize)] = 1; //T: 1 = nome do ficheiro
	controlPacket[4+sizeof(fileSize)] = strlen(fileName); //L
	strcpy(&controlPacket[5+sizeof(fileSize)],fileName);


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
	int controleSize = llread(fd, pacotefdData); //Leu pacote de controle

	char* fileName = malloc(controleSize);
  /*fileName tentar ver problema*/
	int i = 0;

	for(i = 0; i < controleSize; i++){
		printf("%X\n",pacotefdData[i] );
	}
	/** Type
	 * 0 – tamanho do ficheiro,
	 * 1 – nome doficheiro
	*/
	if (pacotefdData[1] == 0) {
		i = 3 + pacotefdData[2] + 2;
	} else {
		i = 3;
		controleSize = pacotefdData[2] + 3;
	}
	int k = 0;
	for ( k = 0; i < controleSize; i++, k++) {
		fileName[k]=pacotefdData[i];
	}

	int fdData = open(fileName,O_CREAT|O_EXCL|O_RDWR|O_APPEND,0666);
	if(fdData == -1){
		printf("Nao conseguiu abrir ficheiro de fdData\n");
		return -1;
	}

	while(0<(size = llread(fd, pacotefdData))) {//NOT FINAL
		if((pacotefdData[0] == 1) && (pacotefdData[1] == n)){
			n++;
			int sizeWrite = 4;
			int stillNeedToWrite=(size - sizeWrite);
			while((stillNeedToWrite=((size - sizeWrite)))> 0){
				sizeWrite += write(fdData, pacotefdData + sizeWrite, stillNeedToWrite);
		}
		}else break;
	}
    llclose(fd);
    close(fdData);
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
