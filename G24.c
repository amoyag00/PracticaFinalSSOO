#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#define TRUE 1
#define MAX_CARS 5

void *racerAction(void *arg);
void *boxesActions(void *arg);
void racerCreation();

int racerNumber = 1;
pthread_t circuit[5];
pthread_t boxesWaitList[5] = {0,0,0,0,0};
pthread_mutex_t mutexCircuit;
pthread_cond_t condCircuit=PTHREAD_COND_INITIALIZER;
pthread_mutex_t semCircuit;


typedef struct boxParameters{
	pthread_mutex_t * mutex;
	pthread_t carID;
	int attendedCars;
	int *attCarsOtherBox;
}BoxParameters;

int main(){

	
	struct sigaction sig;
	pthread_t box_1,box_2;
	pthread_attr_t attrBox1;
	pthread_attr_t attrBox2;
	pthread_attr_init(&attrBox1);
	pthread_attr_init(&attrBox2);
	pthread_attr_setdetachstate(&attrBox1,PTHREAD_CREATE_JOINABLE);
	pthread_attr_setdetachstate(&attrBox2,PTHREAD_CREATE_JOINABLE);	

	pthread_mutex_init(&mutexCircuit, NULL);
	sig.sa_handler = racerCreation;
	if(sigaction(SIGUSR1,&sig,NULL)==-1){
		printf("Error: %s\n", strerror(errno));
	}

	pthread_mutex_t mutexBoxes;
	pthread_mutex_init(&mutexBoxes, NULL);
	BoxParameters paramsBox1;
	BoxParameters paramsBox2;
	paramsBox1.attendedCars=0;
	paramsBox2.attendedCars=0;
	paramsBox1.attCarsOtherBox=&(paramsBox2.attendedCars);
	paramsBox2.attCarsOtherBox=&(paramsBox1.attendedCars);
	paramsBox1.mutex=&mutexBoxes;
	paramsBox2.mutex=&mutexBoxes;

	pthread_create (&box_1,	&attrBox1,boxesActions,(void*)&paramsBox1);
	pthread_create(&box_2,&attrBox2,boxesActions,(void*)&paramsBox2);
	pthread_join(box_1,NULL);
	pthread_join(box_2,NULL);
//usar el while para que el programa no acabe y poder mandar la señal(Carlos)
//while(TRUE){
//}

	pthread_mutex_init(&semCircuit,NULL);

	//Numero de coches en el circuito
	int carsCircuit=0;
	
	//Tiene que colocarse donde anyadamos coches al circuito que es el main (Samuel)
	pthread_mutex_lock(&semCircuit);

	while(semCircuit>=MAX_CARS){

		pthread_cond_wait(&condCircuit,&semCircuit);

	}
	pthread_mutex_unlock(&semCircuit);

	//Se hará un signal cuando un coches salga del circuito
	
	

	return 0;
}

void *boxesActions(void *arg){
	BoxParameters * params=(BoxParameters*)arg;
	for(;;){
		pthread_mutex_lock(params->mutex);
		if(boxesWaitList[0]!=0){
			params->carID=boxesWaitList[0];
			int i=0;
			/*Desplaza la lista para que despues de atender
			 al primero el segundo sea el primero, el tercero el segundo, etc.*/
			for(i=0;i<MAX_CARS-1;i++){
				boxesWaitList[i]=boxesWaitList[i+1];
				boxesWaitList[i+1]=0;
			}
			pthread_mutex_unlock(params->mutex);
			//TODO Dormir tiempo de atención, de  1 a 3 segundos aleatorio
			//TODO Comprobar si hay problemas. 70% no tiene, 30% sí-> si hay abandonar carrera
			//=mandar señal de terminar thread al coche de threadID 'carID'
			//TODO Comprobar si hay que cerrar el box - sección crítica(?) (Alejandro)
			
			
		}else{
			sleep(1);
		}	
	}	
}

void racerCreation(){
	pthread_t racer;
	pthread_create(&racer,NULL,racerAction,&mutexCircuit);
}

void *racerAction(void *arg){
	int id = racerNumber;
	printf("El corredor número %d entra al circuito\n",racerNumber);
	racerNumber++;
}


