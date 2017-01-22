#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#define MAX_CARS 5
#define NUM_ROUNDS 5

void *racerAction(void *arg);
void *boxesActions(void *arg);
void *judgeActions(void *arg);

void racerCreation();
void boxesCreation();
void judgeCreation();
void endRace();
void writeLogMessage(char *id, char *msg);

int nRacer;
int racerNumber;
int openBoxes;
FILE *logFile;

//flags
int sanctionReceived;
int sanctionedAfterCheck;
int boxAssigned;


int boxesWaitList[5] = {-1,-1,-1,-1,-1};//Guarda la posición que ocupa el corredor en el array 'arrayCars' 

pthread_cond_t sanctionNoticed = PTHREAD_COND_INITIALIZER;
pthread_cond_t sanctionEnded = PTHREAD_COND_INITIALIZER;
pthread_cond_t condBoxAssigned=PTHREAD_COND_INITIALIZER;
pthread_cond_t condBox[2];



pthread_mutex_t mutexRacers;//Para acceder a arrayCars
pthread_mutex_t mutexBoxes;//Para comunicarse ellos
pthread_mutex_t mutexBoxesList;//Acceder a la lista de espera de mutex
pthread_mutex_t mutexAssign;//Asignar box al corredor
pthread_mutex_t mutexJudge;//Comunicar juez con Corredores
pthread_mutex_t mutexVictory;//Para acceder a winnerRacer
pthread_mutex_t mutexBox[2];//Comunicar corredores con boxes




typedef struct boxParameters{
	int boxID;
	int racerPos;
	int attendedCars;
	int isClosed;
}BoxParameters;

typedef struct racerParameters{
	int IDNumber;
	int posInArray;
	int sanctioned;
	int repared;//0 no necesita ser reparado; 1 lo necesita; 2 ha sido reparado; 3 no ha sido posible repararlo
	int rounds;
	int boxAssociated;
	time_t initialT;
	time_t finalT;
	time_t totalT;
}RacerParameters;

RacerParameters arrayCars[MAX_CARS];
BoxParameters arrayBoxes[2];
RacerParameters winnerRacer = {.totalT = 500};

int main(){
	struct sigaction sig;
	struct sigaction endSignal;
	
	pthread_mutex_init(&mutexRacers, NULL);
	pthread_mutex_init(&mutexBoxes, NULL);
	pthread_mutex_init(&mutexBoxesList,NULL);
	pthread_mutex_init(&mutexJudge, NULL);
	pthread_mutex_init(&mutexVictory,NULL);
	pthread_mutex_init(&mutexAssign,NULL);
	pthread_mutex_init(&mutexBox[0],NULL);
	pthread_mutex_init(&mutexBox[1],NULL);


	pthread_cond_init(&condBox[0],NULL);
	pthread_cond_init(&condBox[1],NULL);
	nRacer=1;
	racerNumber=0;
	openBoxes=0;
	sanctionReceived=0;
	boxAssigned=0;
	sanctionedAfterCheck=0;
	
	sig.sa_handler = racerCreation;
	
	if(sigaction(SIGUSR1,&sig,NULL)==-1){
		printf("Error: %s\n", strerror(errno));
	}

	endSignal.sa_handler = endRace;
	if(sigaction(SIGINT,&endSignal,NULL)==-1){
		printf("Error: %s\n", strerror(errno));
	}	
	boxesCreation();
	//judgeCreation();
	for(;;){
		pause();
	}
	
	return 0;
}

void endRace(){
	char msg[256], racerNum[256];

	sprintf(msg,"Es el ganador de la carrera con un tiempo de %lu segundos",winnerRacer.totalT);
	sprintf(racerNum,"Corredor %d",winnerRacer.IDNumber);
	writeLogMessage(racerNum, msg);
	exit(0);
}

void boxesCreation(){
	pthread_t box_1;
	pthread_t box_2;
	arrayBoxes[0].attendedCars=0;
	arrayBoxes[1].attendedCars=0;
	arrayBoxes[0].isClosed=0;
	arrayBoxes[1].isClosed=0;
	arrayBoxes[0].boxID=0;
	arrayBoxes[1].boxID=1;
	arrayBoxes[0].racerPos=-1;
	arrayBoxes[1].racerPos=-1;
	pthread_create (&box_1,	NULL,boxesActions,(void*)&arrayBoxes[0]);
	pthread_create(&box_2,NULL,boxesActions,(void*)&arrayBoxes[1]);

}

void *boxesActions(void *arg){
	BoxParameters * params=(BoxParameters*)arg;
	int prob=0;
	int i=0;
	char msg[256];
	char racerNum[256];
	srand(time(NULL));
	for(;;){
		
		pthread_mutex_lock(&mutexBoxesList);
		if(boxesWaitList[0]!=-1){
			params->racerPos=boxesWaitList[0];
			/*Desplaza la lista para que despues de atender
			 al primero el segundo sea el primero, el tercero el segundo, etc.*/
			for(i=0;i<MAX_CARS-1;i++){
				boxesWaitList[i]=boxesWaitList[i+1];
				boxesWaitList[i+1]=-1;
			}
		}	
		pthread_mutex_unlock(&mutexBoxesList);
		if(params->racerPos!=-1){
			pthread_mutex_lock(&mutexAssign);
			pthread_mutex_lock(&mutexRacers);
			arrayCars[params->racerPos].boxAssociated=params->boxID;

			pthread_cond_signal(&condBoxAssigned);
			/*while(boxAssigned==0){
				pthread_cond_wait(&condBoxAssigned,&mutexAssign);
			}
			boxAssigned=0;*/
			
			sprintf(racerNum,"Corredor %d",arrayCars[params->racerPos].IDNumber);
			sprintf(msg,"Entra en el box_%d",params->boxID);
			pthread_mutex_unlock(&mutexRacers);
			writeLogMessage(racerNum,msg);
			pthread_mutex_unlock(&mutexAssign);
			
			
			//Dormir tiempo de atención, de  1 a 3 segundos aleatorio
			sleep((rand()%3) +1);

			pthread_mutex_lock(&mutexBox[params->boxID]);
			//Comprobar si hay problemas. 70% no tiene, 30% ;sí-> si hay abandonar carrera
			prob=(rand()%10)+1;
			pthread_mutex_lock(&mutexRacers);
			if(prob>=7){
				arrayCars[params->racerPos].repared=2;
			}else{
				arrayCars[params->racerPos].repared=3;
			}
			pthread_mutex_unlock(&mutexRacers);
			pthread_cond_signal(&condBox[params->boxID]);
			sprintf(msg,"Sale del box_%d",(params->boxID));
			writeLogMessage(racerNum,msg);
			params->racerPos=-1;
			pthread_mutex_unlock(&mutexBox[params->boxID]);
		
			//Comprobar si hay que cerrar el box
			/*params->attendedCars++;
			if(params->attendedCars>=3){
				pthread_mutex_lock(&mutexBoxes);
				if(openBoxes>1){
					params->isClosed=1;
					params->attendedCars=0;
				}
				pthread_mutex_unlock(&mutexBoxes);
			}
			if(params->isClosed==1){
				sprintf(msg,"box_%d",(params->boxID));
				writeLogMessage(msg,"Se cierra temporalmente");
				sleep(20);
				params->isClosed=0;
				writeLogMessage(msg,"Se abre");
			}*/
		}else{
			sleep(1);
		}
	}	
}

void racerCreation(){

	if(racerNumber<5){	
		int pos=0;		
		while(arrayCars[pos].IDNumber!=0){
			pos++;
		}
		arrayCars[pos].IDNumber = nRacer++;
		arrayCars[pos].posInArray = pos;
		arrayCars[pos].repared=-1;
		arrayCars[pos].boxAssociated=-1;
		printf("%d\n",pos);
		
		pthread_t racer;
		pthread_create(&racer,NULL,racerAction,(void*)&arrayCars[pos]);
		
	}

}

void *racerAction(void *arg){
	char racerNum [256], msg [256];
	int probBoxes,i,random;
	RacerParameters *params = (RacerParameters *) arg;
	sprintf(racerNum,"Corredor %d",params->IDNumber);
	writeLogMessage(racerNum,"Entra al circuito");
	racerNumber++;
	probBoxes=0;
	i=0;
	random=0;
	srand(time(NULL));
	
	for(;;){
		params->initialT=time(0);
		random=(rand()%4)+2;
		sleep(random);
		//Acaba la vuelta y mira si tiene que entrar a boxes 
		pthread_mutex_lock(&mutexBoxesList);
		probBoxes=rand()%10 ;
		if(probBoxes>5){
			params->repared=1;
			for(i=0;i<MAX_CARS;i++){
				if(boxesWaitList[i]==-1){
					boxesWaitList[i]=params->posInArray;
					writeLogMessage(racerNum,"Se pone en la entrada de boxes");
					break;
				}
			}	
		}else{
			params->repared=0;
		}
		pthread_mutex_unlock(&mutexBoxesList);
		if(params->repared==1){
			pthread_mutex_lock(&mutexAssign);
				while(params->boxAssociated==-1){
					pthread_cond_wait(&condBoxAssigned,&mutexAssign);
				}
			pthread_mutex_unlock(&mutexAssign);
			pthread_mutex_lock(&mutexBox[params->boxAssociated]);
				while(params->repared==1){
					pthread_cond_wait(&condBox[params->boxAssociated],&mutexBox[params->boxAssociated]);
				}	
				
			pthread_mutex_unlock(&mutexBox[params->boxAssociated]);
			params->boxAssociated=-1;
		}
		if(params->repared==3){
			pthread_mutex_lock(&mutexRacers);
			arrayCars[params->posInArray].IDNumber=0;
			arrayCars[params->posInArray].sanctioned = 0;
			arrayCars[params->posInArray].rounds = 0;
			arrayCars[params->posInArray].initialT = 0;
			arrayCars[params->posInArray].finalT = 0;
			arrayCars[params->posInArray].totalT = 0;
			arrayCars[params->posInArray].posInArray = 0;
			arrayCars[params->posInArray].repared = 0;
			arrayCars[params->posInArray].boxAssociated = -1;
			racerNumber--;
			writeLogMessage(racerNum,"Se dispone a abandonar la carrera");
			pthread_mutex_unlock(&mutexRacers);
			pthread_exit(0);
		}

		//y si tiene sanción del Juez
		
		if(params->sanctioned==1){
			pthread_mutex_lock(&mutexJudge);
			printf("%d esperó por juez\n",params->IDNumber);
			sanctionReceived=1;
			pthread_cond_signal(&sanctionNoticed);
			while(params->sanctioned==1){
				pthread_cond_wait(&sanctionEnded,&mutexJudge);
			}
			pthread_mutex_unlock(&mutexJudge);	
		}

		params->finalT=time(0);
		params->totalT += (params->finalT-params->initialT);
		params->rounds++;
		sprintf(msg,"Completa la vuelta número %d en %lu segundos",(params->rounds),(params->finalT-params->initialT));
		writeLogMessage(racerNum,msg);
		if(params->rounds==NUM_ROUNDS){
			break;
		}
		pthread_mutex_lock(&mutexJudge);
		if(params->sanctioned==1){//Por si el juez consigue sancionar en este fragmento de código
			sanctionedAfterCheck=1;
			pthread_cond_signal(&sanctionEnded);
		}
		pthread_mutex_unlock(&mutexJudge);
	}
	
	pthread_mutex_lock(&mutexVictory);
	if(params->totalT<winnerRacer.totalT){
		winnerRacer.totalT = params->totalT;
		winnerRacer.IDNumber = params->IDNumber;
	}
	pthread_mutex_unlock(&mutexVictory);
	pthread_mutex_lock(&mutexRacers);
	arrayCars[params->posInArray].IDNumber=0;
	arrayCars[params->posInArray].sanctioned = 0;
	arrayCars[params->posInArray].rounds = 0;
	arrayCars[params->posInArray].initialT = 0;
	arrayCars[params->posInArray].finalT = 0;
	arrayCars[params->posInArray].totalT = 0;
	arrayCars[params->posInArray].posInArray = 0;
	arrayCars[params->posInArray].repared = 0;
	arrayCars[params->posInArray].boxAssociated = -1;
	racerNumber--;
	pthread_mutex_unlock(&mutexRacers);
	pthread_mutex_lock(&mutexJudge);
	if(params->sanctioned==1){//Por si el juez consigue sancionar en el fragmento de código anterior
		sanctionedAfterCheck=1;
		pthread_cond_signal(&sanctionEnded);
	}
	pthread_mutex_unlock(&mutexJudge);
		
}


void judgeCreation(){
	pthread_t judge;
	pthread_create(&judge,NULL,judgeActions,NULL);
	
}


void *judgeActions(void *arg){
	//Sancionar cada 10 seg
	char msg [256];
	int sanctionedRacer=0;
	srand(time(NULL));

	for(;;){
		if(sanctionedAfterCheck==0){
			sleep(10);
		}
		sanctionedAfterCheck=0;
		
		if(racerNumber!=0){
			pthread_mutex_lock(&mutexRacers);
			do{
				sanctionedRacer = rand()% MAX_CARS;
			}while(arrayCars[sanctionedRacer].IDNumber==0);
			arrayCars[sanctionedRacer].sanctioned=1;
			pthread_mutex_unlock(&mutexRacers);
			
			pthread_mutex_lock(&mutexJudge);
			while(sanctionReceived==0){
				pthread_cond_wait(&sanctionNoticed,&mutexJudge);
			}
			if(sanctionedAfterCheck==0){
				sprintf(msg,"Sanciona al corredor %d",arrayCars[sanctionedRacer].IDNumber);
				writeLogMessage("Juez",msg);
				sleep(3);
			}
			sanctionReceived=0;
			arrayCars[sanctionedRacer].sanctioned=0;
			pthread_cond_signal(&sanctionEnded);
			pthread_mutex_unlock(&mutexJudge);
		}
	}

}


void writeLogMessage(char *id, char *msg) {
	time_t now = time (0);
	struct tm *tlocal = localtime (&now);
	char stnow [50];
	strftime(stnow , sizeof(stnow), "%Y-%m-%d %H:%M:%S ", tlocal);
	logFile = fopen("registroTiempos.log" , "a");
	fprintf(logFile , "[ %s] %s: %s\n", stnow , id, msg);
	fclose(logFile);
}
