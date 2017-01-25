//Code that was not int the final version
//Practca finalSSOO g24

/*Recordatorios:
*Compilar con -lpthread
*ps -ef -> Lista de procesos en ejecucion
*SIGUSR1 -> Crea corredor
*SIGINT -> Termina la carrera
*Si queremos usar la asignacion de recursos estaticos (Parte de la implementacion extra)  definimos la cantidad de corredores y la cantidad de boxes o solo la cantidad de corredores o niguna. Esto se logra pasando los argumentos en la ejecucion como: ./G24.c (MaxCars) (MaxBoxes)*/


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define TRUE 1
#define NUM_ROUNDS 5

//Thread Actions for Racers, Boxes and Judge
void *racerAction(void *arg);
void *boxesActions(void *arg);
void *judgeActions(void *arg);

//Thread creation for Racers, Boxes and Judege
void racerCreation();
void boxesCreation();
void judgeCreation();
//After SIGINT signal we use this method to end race
void endRace();
//We use this method to write into the .log file
void writeLogMessage(char *id, char *msg);

int finishExec=0;
//DEFINED BY ARGUMENTS
int maxCars;
int maxBoxes;

//Manejo de recursos estaticos
int paramCarrera[2]={0,0};
//Saves number of racers in the track
int nRacer;
//Saves the last racer id
int racerNumber;
//File for the logger
FILE *logFile;

//Variable that saves a 0 or 1 deppending if its time for the judge to give a sanction
int sanctionReceived;


int *boxesWaitList;//Guarda la posición que ocupa el corredor en el array 'arrayCars' 
//To manage the notification from judge to racers
pthread_cond_t sanctionNoticed;
//To manage judge sanction ending time
pthread_cond_t sanctionEnded;
//Racer car repaired
pthread_cond_t condRepared;


//Mutex
pthread_mutex_t mutexRacers;//Para acceder a arrayCars
pthread_mutex_t mutexBoxes;//Para comunicarse ellos
pthread_mutex_t mutexBoxesList;//acceder a la lista de entrada a boxes
pthread_mutex_t mutexJudge;//comunicar corredores con juez
pthread_mutex_t mutexVictory;//Para acceder a winnerRacer
pthread_mutex_t semaphore;


typedef struct boxParameters{
	char  boxNum;
	int racerPos;
	int attendedCars;
	int isClosed;
}BoxParameters;

typedef struct racerParameters{
	int IDNumber;
	int posInArray;
	int sanctioned;
	int repared;//-1 no necesita ser reparado; 0 lo necesita; 1 ha sido reparado; 2 no ha sido posible repararlo
	int rounds;
	time_t initialT;
	time_t finalT;
	time_t totalT;
}RacerParameters;
//Array for storing car positions
RacerParameters *arrayCars;
//Array for storing boxes positions
BoxParameters *arrayBoxes;
RacerParameters winnerRacer = {.totalT = 500};


//Entry point for the entire practice
int main(int argc, char *argv[]){
	struct sigaction sig;
	struct sigaction endSignal;
	
	pthread_mutex_init(&mutexRacers, NULL);
	pthread_mutex_init(&mutexBoxes, NULL);
	pthread_mutex_init(&mutexBoxesList,NULL);
	pthread_mutex_init(&mutexJudge, NULL);
	pthread_mutex_init(&mutexVictory,NULL);
	pthread_mutex_init(&semaphore,NULL);
	pthread_cond_init(&sanctionNoticed,NULL);
	pthread_cond_init(&sanctionEnded,NULL);
	pthread_cond_init(&condRepared,NULL); 

	nRacer=1;
	racerNumber=0;
	sanctionReceived=0;
	
	sig.sa_handler = racerCreation;
	
	if(sigaction(SIGUSR1,&sig,NULL)==-1){
		printf("Error: %s\n", strerror(errno));
	}


	endSignal.sa_handler = endRace;
	if(sigaction(SIGINT,&endSignal,NULL)==-1){
		printf("Error: %s\n", strerror(errno));
	}	
	judgeCreation();
	if (argc>=4){
		printf("To many params\n");

	}else if(argc==1){
		printf("Default option \n");
		maxCars=5;
		maxBoxes = 2;
	}else if(argc==2){//Pass max car 
		printf("Racer max amount defined\n");
		maxCars=atoi(argv[1]);
		paramCarrera[0]=maxCars;
		maxBoxes=2;
	}else if(argc==3){//PASS MAX CARS AND BOXES
		printf("Racer an boxes max amount defined\n");
		maxCars=atoi(argv[1]);
		paramCarrera[0]=maxCars;
		maxBoxes=atoi(argv[2]);
		paramCarrera[1]=maxBoxes;
	}

	
	boxesCreation();
	//hacer joinable
//Waiting for the signals to arrive
	while(finishExec==0){
		endSignal.sa_handler=endRace;
		if (sigaction(SIGINT,&endSignal,NULL)==-1){
			printf("Error: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		if(sigaction(SIGUSR1,&sig,NULL)==-1){
			printf("Error: %s\n", strerror(errno));
		}
		pause();
	}
	if (pthread_mutex_destroy (&mutexRacers)!=0){
		printf("Error mutex\n");
	}
	if (pthread_mutex_destroy (&mutexBoxes)!=0){
		printf("Error mutex\n");
	}
	if (pthread_mutex_destroy (&mutexBoxesList)!=0){
		printf("Error mutex\n");
	}
	if (pthread_mutex_destroy (&mutexJudge)!=0){
		printf("Error mutex\n");
	}
	if (pthread_mutex_destroy (&mutexVictory)!=0){
		printf("Error mutex\n");
	}
	if (pthread_mutex_destroy (&semaphore)!=0){
		printf("Error mutex\n");
	}
	return 0;
}

void endRace(){
	char msg[256], msg2[256] , racerNum[256];
	
	writeLogMessage("Circuito","La carrera ha finalizado");

	sprintf(msg,"El total de corredores que han participado es: %d",totalRacers);
	writeLogMessage("Circuito", msg);
	if(winnerRacer.IDNumber>0){
		sprintf(msg2,"Es el ganador de la carrera con un tiempo de %lu segundos",winnerRacer.totalT);
		sprintf(racerNum,"Corredor_%d",winnerRacer.IDNumber);
		writeLogMessage(racerNum, msg2);
	}else{
		sprintf(msg2,"Ningún corredor ha completado la carrera");
		writeLogMessage("Circuito", msg2);
	}
	free(arrayCars);
	free(arrayBoxes);
	exit(0);
}

void boxesCreation(){
	BoxParameters * params=(BoxParameters*)arg;
	char msgBox[256], sBox[256], aBox[256];
	int prob=0;
	int i=0;
	srand(time(NULL));
	sprintf(msgBox,"Box_%d",(params->boxID));
	for(;;){
		sleep(1);

		pthread_mutex_lock(&mutexBoxesList);
		if(boxesWaitList[0]>0){
			params->racerPos=boxesWaitList[0]-1;
			
			sprintf(aBox,"Accede a este box el corredor_%d",arrayCars[params->racerPos].IDNumber);
			writeLogMessage(msgBox,aBox);
			/*Desplaza la lista para que despues de atender
			 al primero el segundo sea el primero, el tercero el segundo, etc.*/
			
			for(i=0;i<maxCars-1;i++){
				boxesWaitList[i]=boxesWaitList[i+1];
				boxesWaitList[i+1]=0;
			}
			pthread_mutex_unlock(&mutexBoxesList);
			//Dormir tiempo de atención, de  1 a 3 segundos aleatorio
			
			
			sleep((rand()%3) +1);
			//Comprobar si hay problemas. 70% no tiene, 30% ;sí-> si hay abandonar carrera
			pthread_mutex_lock(&mutexRepare);
			prob=(rand()%10)+1;
			if(prob>7){
				arrayCars[params->racerPos].repared=2;
			}else{
				arrayCars[params->racerPos].repared=1;
			}
			//mandar señal de terminar thread al racer
			
			sprintf(sBox,"Sale del box el corredor_%d",arrayCars[params->racerPos].IDNumber);
			writeLogMessage(msgBox,sBox);
			pthread_cond_signal(&condRepared);
			pthread_mutex_unlock(&mutexRepare);
			//Comprobar si hay que cerrar el box
			params->attendedCars++;
			if(params->attendedCars>=3){
				pthread_mutex_lock(&mutexBoxes);
				if(openBoxes>1){
					openBoxes--;
					params->isClosed=1;
					params->attendedCars=0;
					
				}

				pthread_mutex_unlock(&mutexBoxes);
			}
			if(params->isClosed==1){
				writeLogMessage(msgBox,"Se cierra temporalmente");
				sleep(20);
				params->isClosed=0;
				params->attendedCars=0;

				pthread_mutex_lock(&mutexBoxes);
				openBoxes++;
				writeLogMessage(msgBox,"Se abre");
				pthread_mutex_unlock(&mutexBoxes);
			}

		}
		pthread_mutex_unlock(&mutexBoxesList);	
	}	
}

void *boxesActions(void *arg){
	BoxParameters * params=(BoxParameters*)arg;
	//mechanical issues probability.
	int prob=0;
	srand(time(NULL));
	//Infinit loop so boxes never stop working even after the 5 racers cross the finish line
	for(;;){
		pthread_mutex_lock(&mutexBoxes);
		if(boxesWaitList[0]!=0){
			params->racerPos=boxesWaitList[0];
			int i=0;
			/*Desplaza la lista para que despues de atender
			 al primero el segundo sea el primero, el tercero el segundo, etc.*/
			//Replace the list 1# position for the next so another racer can be fixed
			for(i=0;i<maxCars-1;i++){
				boxesWaitList[i]=boxesWaitList[i+1];
				boxesWaitList[i+1]=0;
			}
			pthread_mutex_unlock(&mutexBoxes);
			//Sleep box time (between 1 and 3 seconds)
			sleep((rand()%3) +1);
			//Check for mechanical issues.
			//If probability is bigger than 70% then because of mechanical issues the racer leaves the race (cancel signal to thread)
			prob=(rand()%10)+1;
			//mandar señal de terminar thread al racer
			
				pthread_mutex_lock(&semaphore);
				if(prob>=7){
					arrayCars[params->racerPos].repared=2;
				}else{
					arrayCars[params->racerPos].repared=1;
				}
				pthread_cond_signal(&condRepared);
				pthread_mutex_unlock(&semaphore);
			
			//Check that the box is closed (after attending 3 cars the box closes)
			params->attendedCars++;
			if(params->attendedCars>=3){
				pthread_mutex_lock(&mutexBoxes);
				//Can close only if the other box is not.
				if(params->otherIsClosed==0){
//Close the box.
					params->isClosed=1;
//Reset number of attended cars.
					params->attendedCars=0;
				}
				pthread_mutex_unlock(&mutexBoxes);
			}
			//If the box already fixed 3 cars, the box is closed for 20 seconds and the it opens again
			if(params->isClosed==1){
				writeLogMessage(params->boxName,"Se cierra temporalmente");
				sleep(20);
				params->isClosed=0;
				writeLogMessage(params->boxName,"Se abre");
			}
		//Else if o
		}else{
			sleep(1);
		}	
		pthread_mutex_unlock(&mutexBoxes);
	}	
}
//Here we initialize the attributes and create the racer threads
void racerCreation(){
//If the amount of cars in the circuit is more then five, then we still cannot create another
	if(racerNumber<5){
		/*pthread_t racers[racerNumber];
		RacerParameters paramsRacer;
		//pthread_attr_t atributeRacer;
		paramsRacer.IDNumber = ++racerNumber;
		//paramsRacer.sanction = 0;
		paramsRacer.rounds = 0;
		
		//pthread_attr_init(&atributeRacer);
	    //pthread_attr_setdetachstate(&atributeRacer,PTHREAD_CREATE_DETACHED);
		pthread_create(&racers[racerNumber-1],NULL,racerAction,(void*)&paramsRacer);*/
		int pos=0;		
		while(arrayCars[pos].IDNumber!=0){
			pos++;
		}
		arrayCars[pos].IDNumber = nRacer++;
		arrayCars[pos].posInArray = pos;
		arrayCars[pos].repared=-1;
		printf("%d\n",pos);
		
		pthread_t racer;
		pthread_create(&racer,NULL,racerAction,(void*)&arrayCars[pos]);
		
	}

}
//Method that defines the racer threads actions
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
	//time_t roundTime;
	srand(time(NULL));
	
	for(i=0;i<NUM_ROUNDS;i++){
		params->initialT=time(0);
		random=(rand()%4)+2;
		sleep(random);
		//Acaba la vuelta y mira si tiene que entrar a boxes 
		pthread_mutex_lock(&mutexBoxesList);
		probBoxes=rand()%10 ;
		if(probBoxes>5){
			
			params->repared=0;
			for(i=0;i<maxCars;i++){
				if(boxesWaitList[i]==0){
					boxesWaitList[i]=params->posInArray;
					writeLogMessage(racerNum,"Entra en boxes");
					break;
				}
			}
			
		}
		pthread_mutex_unlock(&mutexBoxesList);
		pthread_mutex_lock(&semaphore);
		while(params->repared==0){
			pthread_cond_wait(&condRepared,&semaphore);
		}
		pthread_mutex_unlock(&semaphore);
		if(params->repared==2){
			//pirarse
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
		/*roundTime = params->finalT-params->initialT;
		params->totalT = params->totalT + roundTime;*/
		params->totalT += (params->finalT-params->initialT);
		params->rounds++;
		sprintf(msg,"Completa la vuelta número %d en %lu segundos",(params->rounds),/*(roundTime)*/(params->finalT-params->initialT));
		writeLogMessage(racerNum,msg);
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
	racerNumber--;
	/*arrayCars[params->posInArray].sanctioned = 0;
	arrayCars[params->posInArray].IDNumber = 0;
	arrayCars[params->posInArray].rounds = 0;*/
	pthread_mutex_unlock(&mutexRacers);
		
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
	int sanctionedRacer=0;
	srand(time(NULL));

	for(;;){
		sleep(10);
		
		if(racerNumber!=0){
			pthread_mutex_lock(&mutexRacers);
			do{
				sanctionedRacer = rand()% racerNumber;
			}while(arrayCars[sanctionedRacer].IDNumber==0);
			pthread_mutex_unlock(&mutexRacers);
		/*while(arrayCars[sanctionedRacer].IDNumber==0){
			sanctionedRacer = rand()%racerNumber;
		}*/
			
			pthread_mutex_lock(&mutexJudge);
			arrayCars[sanctionedRacer].sanctioned=1;		
			sprintf(msg,"Sanciona al corredor %d",arrayCars[sanctionedRacer].IDNumber);
			writeLogMessage("Juez",msg);
			while(sanctionReceived==0){
				pthread_cond_wait(&sanctionNoticed,&mutexJudge);
			}
			sleep(3);
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
	//"%d/ %m/ %y %H:%M:%S"
}
