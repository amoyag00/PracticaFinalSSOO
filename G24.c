#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#define TRUE 1
#define MAX_CARS 5

void *racerAction(void *arg);
void *boxesActions(void *arg);
void *judgeActions(void *arg);

void racerCreation();
void boxesCreation();
void judgeCreation();


int sanctions[5]={0,0,0,0,0};

int racerNumber = 1;
pthread_t circuit[5];
pthread_t boxesWaitList[5] = {0,0,0,0,0};

pthread_cond_t condCircuit=PTHREAD_COND_INITIALIZER;
pthread_cond_t sanctionNoticed = PTHREAD_COND_INITIALIZER;
pthread_cond_t sanctionEnded = PTHREAD_COND_INITIALIZER;
pthread_cond_t condBox1=PTHREAD_COND_INITIALIZER;
pthread_cond_t condBox2=PTHREAD_COND_INITIALIZER;

pthread_mutex_t mutexCircuit;
pthread_mutex_t semCircuit;
pthread_mutex_t mutexBoxes;
pthread_mutex_t mutexBox1;
pthread_mutex_t mutexBox2;
pthread_mutex_t semCircuit;
pthread_mutex_t mutexJudge;
pthread_mutex_t mutexCars;


typedef struct boxParameters{
	pthread_mutex_t * mutex;
	pthread_t carID;
	int attendedCars;
	int *otherIsClosed;
	int isClosed;
}BoxParameters;

typedef struct racerParameters{
	pthread_mutex_t *mutexRacer;
	int IDNumber;
	//int sanction;
	int rounds;
}RacerParameters;

int main(){
	struct sigaction sig;
	
	pthread_mutex_init(&mutexCircuit, NULL);
	pthread_mutex_init(&mutexBoxes, NULL);
	pthread_mutex_init(&mutexJudge, NULL);
	boxesCreation();
	judgeCreation();
	sig.sa_handler = racerCreation;
//	mutex_mutex_lock(&mutexCircuit);
	if(sigaction(SIGUSR1,&sig,NULL)==-1){
		printf("Error: %s\n", strerror(errno));
	}


	
	
	
//usar el while para que el programa no acabe y poder mandar la señal(Carlos)
while(TRUE){



}

	///pthread_mutex_init(&semCircuit,NULL);

	//Numero de coches en el circuito
	
	//Tiene que colocarse donde anyadamos coches al circuito que es el main (Samuel)
	///pthread_mutex_lock(&semCircuit);

/*	while(semCircuit>=MAX_CARS){

		pthread_cond_wait(&condCircuit,&semCircuit);

	}
	pthread_mutex_unlock(&semCircuit);*/

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
			//Dormir tiempo de atención, de  1 a 3 segundos aleatorio
			srand(time(NULL));
			
			sleep((rand()%3) +1);
			//Comprobar si hay problemas. 70% no tiene, 30% ;sí-> si hay abandonar carrera
			int probability=(rand()%10)+1;
			//mandar señal de terminar thread al coche de threadID 'carID'
			if(probability>=7){
				pthread_cancel(params->carID);
			}
			//TODO Comprobar si hay que cerrar el box
			params->attendedCars++;
			if(params->attendedCars>=3){
				pthread_mutex_lock(params->mutex);
				if(params->otherIsClosed==0){
					params->isClosed=1;
					params->attendedCars=0;
				}
				pthread_mutex_unlock(params->mutex);
			}
			if(params->isClosed==1){
				sleep(20);
				params->isClosed=0;
			}

		}else{
			sleep(1);
		}	
		pthread_mutex_unlock(params->mutex);
	}	
}

void racerCreation(){
	if(carsInCircuit<5){
		pthread_t circuit[5];
		RacerParameters paramsRacer;
		pthread_attr_t atributeRacer;
		paramsRacer.IDNumber = racerNumber;
		//paramsRacer.sanction = 0;
		paramsRacer.rounds = 0;
		paramsRacer.mutexRacer = &mutexCircuit;
		pthread_attr_init(&atributeRacer);
	    	pthread_attr_setdetachstate(&atributeRacer,PTHREAD_CREATE_DETACHED);
		int pos=0;
		while(circuit[pos]!=0){
			pos++;
		}
		pthread_create(&circuit[pos],&atributeRacer,racerAction,(void*)&paramsRacer);
		racerNumber++;
	}
}

void *racerAction(void *arg){
	int probBoxes;
	RacerParameters *params = (RacerParameters *) arg;
	
	//Acaba la vuelta y mira si tiene sanción:
	pthread_mutex_lock(&mutexCars);
	int pos=0;
	while(circuit[pos]!=pthread_self())){
		pos++;
	}
	if(sanction[pos]){
		pthread_cond_signal(&sanctionNoticed);
		pthread_cond_wait(&sanctionEnded,&mutexCars);
	}
	pthread_mutex_unlock(&mutexCars);


	srand(time(NULL));
	probBoxes=rand()%10 ;
	pthread_mutex_lock(&mutexBoxes);
	if(probBoxes>5){
		for(int i=0;i<MAX_CARS;i++){
			if(boxesWaitList[i]!=0){
				boxesWaitList[i]=pthread_self();
				continue;
			}
		}
	}
	printf("El corredor número %d entra al circuito\n",params->IDNumber);
	
}

void boxesCreation(){
	pthread_t box_1,box_2;
	pthread_attr_t attrBox1;
	pthread_attr_t attrBox2;
	pthread_attr_init(&attrBox1);
	pthread_attr_init(&attrBox2);
	pthread_attr_setdetachstate(&attrBox1,PTHREAD_CREATE_JOINABLE);
	pthread_attr_setdetachstate(&attrBox2,PTHREAD_CREATE_JOINABLE);
	BoxParameters paramsBox1;
	BoxParameters paramsBox2;
	paramsBox1.attendedCars=0;
	paramsBox2.attendedCars=0;
	paramsBox1.isClosed=0;
	paramsBox2.isClosed=0;
	paramsBox1.otherIsClosed=&(paramsBox2.isClosed);
	paramsBox2.otherIsClosed=&(paramsBox1.isClosed);
	paramsBox1.mutex=&mutexBoxes;
	paramsBox2.mutex=&mutexBoxes;

	pthread_create (&box_1,	&attrBox1,boxesActions,(void*)&paramsBox1);
	pthread_create(&box_2,&attrBox2,boxesActions,(void*)&paramsBox2);
	pthread_join(box_1,NULL);
	pthread_join(box_2,NULL);
}
void *judgeActions(void *arg){
	//Sancionar cada 10 seg
	sleep(10);

	pthread_mutex_lock(&mutexJudge);
	
	srand(time(NULL));		
	int corredorSancionado = (rand()%5);
	
	sanctions[corredorSancionado]=1;

	pthread_cond_wait(&sanctionNoticed,&mutexJudge)
	sleep(3);

	sanctions[corredorSancionado]=0;
	pthread_cond_signal(&sanctionEnded);

	pthread_mutex_unlock(&mutexJudge);

}

void judgeCreation(){

	pthread_t judge;
	pthread_attr_t attrJudge;
	pthread_attr_init(&attrJudge);
	pthread_attr_setdetachstate(&attrJudge,PTHREAD_CREATE_JOINABLE);

	pthread_create(&judge,&attrJudge,judgeActions,&mutexJudge);
	
	
}
