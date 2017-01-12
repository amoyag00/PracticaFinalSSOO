#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#define TRUE 1
#define MAX_COCHES 5

pthread_t boxesWaitList[5]={0,0,0,0,0};
pthread_mutex_t mutexBox1;
pthread_mutex_t mutexBox2;


pthread_cond_t condCircuito=PTHREAD_COND_INITIALIZER;
pthread_mutex_t semaforoCochesEnCircuito;

void *boxesActions(void *arg);
int main(){

	

	pthread_t box_1,box_2;
	pthread_attr_t attrBox1;
	pthread_attr_t attrBox2;
	pthread_attr_init(&attrBox1);
	pthread_attr_init(&attrBox2);
	pthread_attr_setdetachstate(&attrBox1,PTHREAD_CREATE_JOINABLE);
	pthread_attr_setdetachstate(&attrBox2,PTHREAD_CREATE_JOINABLE);

	pthread_mutex_init(&mutexBox1, NULL);
	pthread_mutex_init(&mutexBox2, NULL);
	pthread_create (&box_1,	&attrBox1,boxesActions,&mutexBox1);
	pthread_create(&box_2,&attrBox2,boxesActions,&mutexBox2);
	pthread_join(box_1,NULL);
	pthread_join(box_2,NULL);

	pthread_mutex_init(&semaforoCochesEnCircuito,NULL);

	//Numero de coches en el circuito
	int cochesEnCircuito=0;
	
	//Tiene que colocarse donde anyadamos coches al circuito que es el main (Samuel)
	pthread_mutex_lock(&semaforoCochesEnCircuito);

	while(cochesEnCircuito>=MAX_COCHES){

		pthread_cond_wait(&condCircuito,&semaforoCochesEnCircuito);

	}
	pthread_mutex_unlock(&semaforoCochesEnCircuito);

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

