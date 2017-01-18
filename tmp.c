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
#define NUM_ROUNDS 5

void *racerAction(void *arg);
void *boxesActions(void *arg);
void *judgeActions(void *arg);

void racerCreation();
void boxesCreation();
void judgeCreation();
void writeLogMessage(char *id, char *msg);

int sanctions[5]={0,0,0,0,0};
int sanctionReceived=0;
int winner=0;
int racerNumber = 0;
pthread_t boxesWaitList[5] = {0,0,0,0,0};
pthread_t racers[5]={0,0,0,0,0};

pthread_cond_t condCreate=PTHREAD_COND_INITIALIZER;
pthread_cond_t sanctionNoticed = PTHREAD_COND_INITIALIZER;
pthread_cond_t sanctionEnded = PTHREAD_COND_INITIALIZER;
pthread_cond_t condBox1=PTHREAD_COND_INITIALIZER;
pthread_cond_t condBox2=PTHREAD_COND_INITIALIZER;

pthread_mutex_t mutexCreate;
pthread_mutex_t mutexBoxes;
pthread_mutex_t mutexJudge;
pthread_mutex_t mutexVictory;


typedef struct boxParameters{
	char  boxName [6];
	pthread_t carID;
	int attendedCars;
	int *otherIsClosed;
	int isClosed;
}BoxParameters;

typedef struct racerParameters{
	int IDNumber;
	//int sanction;
	int rounds;
}RacerParameters;

int main(){
	struct sigaction sig;
	
	pthread_mutex_init(&mutexCreate, NULL);
	pthread_mutex_init(&mutexBoxes, NULL);
	pthread_mutex_init(&mutexJudge, NULL);
	pthread_mutex_init(&mutexVictory,NULL);

	
	sig.sa_handler = racerCreation;
	if(sigaction(SIGUSR1,&sig,NULL)==-1){
		printf("Error: %s\n", strerror(errno));
	}
	pthread_mutex_lock(&mutexCreate);
	pthread_cond_wait(&condCreate,&mutexCreate);
	//boxesCreation();
	judgeCreation();
	pthread_mutex_unlock(&mutexCreate);	
//usar el while para que el programa no acabe y poder mandar la señal(Carlos)
	//hacer joinable
	while(TRUE){

	}
	return 0;
}

void boxesCreation(){
	pthread_t box_1,box_2;
	/*pthread_attr_t attrBox1;
	pthread_attr_t attrBox2;
	pthread_attr_init(&attrBox1);
	pthread_attr_init(&attrBox2);
	pthread_attr_setdetachstate(&attrBox1,PTHREAD_CREATE_JOINABLE);
	pthread_attr_setdetachstate(&attrBox2,PTHREAD_CREATE_JOINABLE);*/
	BoxParameters paramsBox1;
	BoxParameters paramsBox2;
	paramsBox1.attendedCars=0;
	paramsBox2.attendedCars=0;
	paramsBox1.isClosed=0;
	paramsBox2.isClosed=0;
	paramsBox1.otherIsClosed=&(paramsBox2.isClosed);
	paramsBox2.otherIsClosed=&(paramsBox1.isClosed);
	strcpy(paramsBox1.boxName,"box_1");
	strcpy(paramsBox2.boxName,"box_2");
	

	pthread_create (&box_1,	NULL,boxesActions,(void*)&paramsBox1);
	pthread_create(&box_2,NULL,boxesActions,(void*)&paramsBox2);
	//pthread_join(box_1,NULL);
	//pthread_join(box_2,NULL);
}

void *boxesActions(void *arg){
	BoxParameters * params=(BoxParameters*)arg;
	for(;;){
		pthread_mutex_lock(&mutexBoxes);
		if(boxesWaitList[0]!=0){
			params->carID=boxesWaitList[0];
			int i=0;
			/*Desplaza la lista para que despues de atender
			 al primero el segundo sea el primero, el tercero el segundo, etc.*/
			for(i=0;i<MAX_CARS-1;i++){
				boxesWaitList[i]=boxesWaitList[i+1];
				boxesWaitList[i+1]=0;
			}
			pthread_mutex_unlock(&mutexBoxes);
			//Dormir tiempo de atención, de  1 a 3 segundos aleatorio
			srand(time(NULL));
			
			sleep((rand()%3) +1);
			//Comprobar si hay problemas. 70% no tiene, 30% ;sí-> si hay abandonar carrera
			int probability=(rand()%10)+1;
			//mandar señal de terminar thread al coche de threadID 'carID'
			if(probability>=7){
				pthread_cancel(params->carID);
			}
			//Comprobar si hay que cerrar el box
			params->attendedCars++;
			if(params->attendedCars>=3){
				pthread_mutex_lock(&mutexBoxes);
				if(params->otherIsClosed==0){
					params->isClosed=1;
					params->attendedCars=0;
				}
				pthread_mutex_unlock(&mutexBoxes);
			}
			if(params->isClosed==1){
				writeLogMessage(params->boxName,"Se cierra temporalmente");
				sleep(20);
				params->isClosed=0;
				writeLogMessage(params->boxName,"Se abre");
			}

		}else{
			sleep(1);
		}	
		pthread_mutex_unlock(&mutexBoxes);
	}	
}

void racerCreation(){
	if(racerNumber<5){
		pthread_t racers[racerNumber];
		RacerParameters paramsRacer;
		//pthread_attr_t atributeRacer;
		paramsRacer.IDNumber = ++racerNumber;
		//paramsRacer.sanction = 0;
		paramsRacer.rounds = 0;
		
		//pthread_attr_init(&atributeRacer);
	    //pthread_attr_setdetachstate(&atributeRacer,PTHREAD_CREATE_DETACHED);
		pthread_create(&racers[racerNumber-1],NULL,racerAction,(void*)&paramsRacer);
		
		pthread_mutex_lock(&mutexCreate);
		pthread_cond_signal(&condCreate);
		pthread_mutex_unlock(&mutexCreate);
	}
}

void *racerAction(void *arg){
	int probBoxes,i,random;
	probBoxes=0;
	i=0;
	random=0;
	srand(time(NULL));
	RacerParameters *params = (RacerParameters *) arg;
	char racerNum [256], msg [256];
	sprintf(racerNum,"Corredor %d",params->IDNumber);
	writeLogMessage(racerNum,"Entra al circuito");
	for(;;){
		
		random=(rand()%4)+2;
		sleep(random);
		sprintf(msg,"Completa la vuelta número %d en %d segundos",(params->rounds+1),random);
		writeLogMessage(racerNum,msg);

		//Acaba la vuelta y mira si tiene sanción:
		pthread_mutex_lock(&mutexJudge);
		
		if(sanctions[(params->IDNumber)-1]){
			sanctionReceived=1;
			pthread_cond_signal(&sanctionNoticed);
			while(sanctions[(params->IDNumber)-1]==1){
				pthread_cond_wait(&sanctionEnded,&mutexJudge);
			}
			
		}
		pthread_mutex_unlock(&mutexJudge);
		
		/*probBoxes=rand()%10 ;
		pthread_mutex_lock(&mutexBoxes);
		if(probBoxes>5){
			for(i=0;i<MAX_CARS;i++){
				if(boxesWaitList[i]!=0){
					boxesWaitList[i]=pthread_self();
					sprintf(racerNum,"Entra en boxes");
					break;
				}
			}
		}
		pthread_mutex_unlock(&mutexBoxes);*/
		pthread_mutex_lock(&mutexVictory);
		params->rounds++;
		if(params->rounds==NUM_ROUNDS){
			if(winner==0){
				winner=1;
				writeLogMessage(racerNum,"Ha ganado la carrera");
			}
			break;
		}
		
		pthread_mutex_unlock(&mutexVictory);

	}
	
}


void judgeCreation(){
	pthread_t judge;
	//pthread_attr_t attrJudge;
	//pthread_attr_init(&attrJudge);
	//pthread_attr_setdetachstate(&attrJudge,PTHREAD_CREATE_JOINABLE);
	pthread_create(&judge,NULL,judgeActions,NULL);
	
}


void *judgeActions(void *arg){
	//Sancionar cada 10 seg
	char msg [256];
	srand(time(NULL));
	sleep(10);

	pthread_mutex_lock(&mutexJudge);	
	int sanctionedRacer = rand()%racerNumber;
	
	sanctions[sanctionedRacer]=1;
	sprintf(msg,"Sanciona al corredor %d",sanctionedRacer+1);
	writeLogMessage("Juez",msg);
	while(sanctionReceived==0){
		pthread_cond_wait(&sanctionNoticed,&mutexJudge);
	}

	sleep(3);

	sanctions[sanctionedRacer]=0;
	pthread_cond_signal(&sanctionEnded);
	pthread_mutex_unlock(&mutexJudge);

}


void writeLogMessage(char *id, char *msg) {
	FILE *logFile;
	time_t now = time (0);
	struct tm *tlocal = localtime (&now);
	char stnow [50];
	strftime(stnow , sizeof(stnow), "%Y-%m-%d %H:%M:%S ", tlocal);
	logFile = fopen("registroTiempos.log" , "a");
	fprintf(logFile , "[ %s] %s: %s\n", stnow , id, msg);
	fclose(logFile);
	//"%d/ %m/ %y %H:%M:%S"
}