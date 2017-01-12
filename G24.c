#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#define TRUE 1
#define MAX_CARS 5

int racerNumber = 1;
pthread_t circuit[5];
pthread_t boxesWaitList[5] = {0,0,0,0,0};
pthread_mutex_t mutexBox1;
pthread_mutex_t mutexBox2;
pthread_mutex_t mutexCircuit;
void *boxesActions(void *arg);
void racerCreation();
pthread_cond_t condCircuit=PTHREAD_COND_INITIALIZER;
pthread_mutex_t semCircuit;
void *racerAction(void *arg);
void *boxesActions(void *arg);
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
	pthread_mutex_init(&mutexBox1, NULL);
	pthread_mutex_init(&mutexBox2, NULL);
	pthread_create (&box_1,	&attrBox1,boxesActions,&mutexBox1);
	pthread_create(&box_2,&attrBox2,boxesActions,&mutexBox2);
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
	
	//while(TRUE){
		if(boxesWaitList[0]==0){
			printf("hola");
		}
		
		sleep(1);
	//}
	
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


