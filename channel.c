#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <openssl/md5.h>

#define MAXCHARACTERS 100

#define SHMKEY2 (key_t)3333// shm and sem between enc1 and channel
#define SEMKEY2 (key_t)4444

#define SHMKEY3 (key_t)5555	// shm and sem between  channel and enc2
#define SEMKEY3 (key_t)6666

#define SHMSIZE 256
#define PERMS 0600

float probability; //noise probability 

typedef enum { false, true } flag;  //flags for situtations ready to happen (1 = ready, 0 = not ready)


const char *addchecksumtomessage(char *message)
{
    char *string =  malloc (sizeof (char) * MAXCHARACTERS);
	strcpy(string, message);
    char hash[MD5_DIGEST_LENGTH];
    char *buf1;
    MD5 (string, sizeof(string), hash);
    buf1 = MD5 (string, sizeof(string), NULL);

    printf("the message without the checksum is: %s\n",string);
    printf("checksum is: %s\n", buf1);

    strcat(string, buf1);
   	printf("the merged message is: %s\n",string);
    return string;
}

union semnum{
	int val;
	struct semid_ds *buff;
	unsigned short *array;
	};



/* Semaphore P - down operation, using semop */

int sem_P(int semid) {

   struct sembuf sem_d;
   
   sem_d.sem_num = 0;
   sem_d.sem_op = -1;
   sem_d.sem_flg = 0;
   if (semop(semid,&sem_d,1) == -1) {
       perror("# Semaphore down (P) operation ");
       return -1;
   }
   return 0;

}

/* Semaphore V - up operation, using semop */

int sem_V(int semid) {

   struct sembuf sem_d;
   
   sem_d.sem_num = 0;
   sem_d.sem_op = 1;
   sem_d.sem_flg = 0;
   if (semop(semid,&sem_d,1) == -1) {
       perror("# Semaphore up (V) operation ");
       return -1;
   }
   return 0;

}



void free_resources(int shmid, int semid) { 

   /* Delete the shared memory segment */
   shmctl(shmid,IPC_RMID,NULL);
   
   /* Delete the semaphore */
   semctl(semid,0,IPC_RMID,0);

}


char *noisefunction(char *message)
{
	if(message == "ERROR-CODE: 1998")  //periptwsi resend, skip the noise
	return message;
    
    char *string =  malloc (sizeof (char) * MAXCHARACTERS);
	strcpy(string, message);
	int n = strlen(string);

    int i=0;
    while(i < n - 16)
    {
        int random_number = rand() % 100 + 1;
        if(random_number < (int)(probability*100)) //kanw cast se int, px ean probability = 0.05 tote: random_number < 5
        {   
            char c =  string[i];
            c = c + 1 ;
            string[i] = c;
        }
        i++;
    }

    return string;
}

const char *server(char *message)
{
    char *string =  malloc (sizeof (char) * MAXCHARACTERS);
	strcpy(string, message);

	char *rt =  malloc (sizeof (char) * MAXCHARACTERS);

	int shmid, semid;
	char line[128], *shmem;
	union semnum arg;
	flag readfrominput = true,readfromshm = false;

	if ((shmid = shmget (SHMKEY3, SHMSIZE, PERMS | IPC_CREAT)) < 0) {
		perror("shmget");
		exit(1);
		}
	printf("Creating shared memory with ID: %d\n",shmid);

	/* create a semaphore */
	if ((semid = semget(SEMKEY3, 1, PERMS| IPC_CREAT)) <0) {
		perror("semget");
		exit(1);
		}
	printf("Creating a semaphore with ID: %d \n",semid);

	arg.val=0;

	/* initialize semaphore for locking */
	if (semctl(semid, 0, SETVAL, arg) <0) {
		perror("semctl");
		exit(1);
		}
	printf("Initializing semaphore to lock\n");


	while(1)
	{
		//reding from p1
		if(readfrominput == true)
		{

	if ( (shmem = shmat(shmid, (char *)0, 0)) == (char *) -1) {
		perror("shmem");
		exit(1);
		}
	printf("Attaching shared memory segment \nEnters string: from P1 \n");



	printf("the string to be entered is: %s\n", string);

	/* Write message in shared memory */
	strcpy(shmem, string); 

	printf("Writing to shared memory region: %s\n", string);
	
	/* Make shared memory available for reading */
	sem_V(semid);

	readfrominput = false;
	readfromshm = true;
	shmdt(shmem);
	sleep(1);

	if(strcmp(string, "TERM") == 0)
	return "TERM";

		}



	
	if(readfromshm == true)
	{
	printf("Asking for access to shared memory region\n");
		sem_P(semid);
	if ( (shmem = shmat(shmid, (char *) 0, 0)) == (char *) -1 ) {
		perror("shmat");
		exit(1);
		}
	printf("Attaching shared memory segment\n");


	printf("Reading from shared memory region: %s\n", shmem);
	strcpy(rt, shmem);

	readfromshm = false;
	readfrominput = true;

	shmdt(shmem);

	}
	
	
	 //while ends here
	printf("Releasing shared memory region\n");

	return rt;
	}
    
}






const char *client()
{

	int shmid, semid;
	char *shmem,line[128],message[MAXCHARACTERS], *modified, *msgfromchannel, *msgfromENC2;
	struct sembuf oper[1]={0,-1,0};

	if ((shmid = shmget (SHMKEY2, SHMSIZE, PERMS )) < 0) {
		perror("shmget");
		exit(1);
		}
	printf("Accessing shared memory with ID: %d\n",shmid);

	/* accessing a semaphore */
	if ((semid = semget(SEMKEY2, 1, PERMS )) <0) {
		perror("semget");
		exit(1);
		}
	printf("Accessing semaphore with ID: %d \n",semid);



while(1)
{
	if ( (shmem = shmat(shmid, (char *) 0, 0)) == (char *) -1 ) {
		perror("shmat");
		exit(1);
		}
	printf("Attaching shared memory segment\n");

	printf("Asking for access to shared memory region \n");
	sem_P(semid);
	printf("Reading from shared memory region: %s\n", shmem);


	strcpy(message, shmem);
	printf("	Message  from P1    is: %s\n", message);



char * t;
//////////////////////////////


if(message == "ERROR-CODE: 1998")  //ean o enc1 moy epistrefei la8os stlenw to error xwris noise
{
	t = message;
	printf("	Message without NOISE is %s\n", t);	
}	
else if(message == "TERM") 
{
	t = message;
	printf("	Message without NOISE is %s\n", t);	
}
else
{
	t = noisefunction(message);
	printf("	Message after NOISE is: %s\n", t);	
}	




//////////////////////////////////////////////////////

msgfromENC2 =  server(t);
printf("	      message to from enc2 is: %s \n", msgfromENC2);


	if(strcmp(t, "TERM") == 0)
{
	/* Clear recourses */
   	free_resources(shmid,semid);
	printf("Shared memory and semaphore segments (ENC1<->CHAN) have been released and process chan is being terminated..\n");
	return;
}
	

/////////////////////////////////
char *v;

if(msgfromENC2 == "ERROR-CODE: 1998") //ean o enc1 moy epistrefei la8os stlenw to error xwris noise
{
	v = msgfromENC2;
	printf("	message to p1 without noise is %s\n", v);	
}	
else if(msgfromENC2 == "TERM") 
{
	v = msgfromENC2;
	printf("	message to p1 without noise is %s\n", v);	
}
else
{
	v = noisefunction(msgfromENC2);
	printf("	message to p1  with   noise is %s\n", v);	
}	
	

	
	/* Write message in shared (without noise) memory */
	strcpy(shmem, v); 
	printf("Writing to shared memory region: %s\n", v);

	
	/* Make shared memory available for reading for server  */
	sem_V(semid);





	

	/* detach shared memeory */
	shmdt(shmem); 
	sleep(1);


	if(strcmp(v, "TERM") == 0)
{
	/* Clear recourses */
   	free_resources(shmid,semid);
	printf("Shared memory and semaphore segments (ENC1<->CHAN) have been released and process chan is being terminated..\n");
	return;
}


}//end of while

	/* destroy shared memory */
	if (shmctl(shmid, IPC_RMID, (struct shmid_ds *)0 ) <0) {
		perror("semctl");
		exit(1);
		}
	printf("Releasing shared segment with identifier %d\n", shmid);

	/* destroy semaphore set */
	if (semctl(semid, 0, IPC_RMID, 0) <0 ) { 
		perror("semctl");
		exit(1);
		}
	printf("Releasing semaphore with identifier %d\n", semid);
}





int main(void)
{
while(1)  //mesa sti while elegxw ean to probability input einai to swsto
{
	printf("\nHello, this is channel process, please enter the channel's noise Probability: ");
	char buffer[100];
    double value;
    char *endptr;
    if (fgets(buffer, sizeof(buffer), stdin) == NULL)
    {
		printf("\nPlease give an appropriate probability input. The value must be between 0 and 1. For example 0.05");
	  	continue;
	}
    value = strtod(buffer, &endptr);
    if ((*endptr == '\0') || (isspace(*endptr) != 0))
	{
		probability = value;
		if(probability >=0 && probability <=1)
		{
			printf("\nThank you!\n");
			break;
		}
	
	}
		
	printf("\nPlease give an appropriate probability input. The value must be between 0 and 1. For example 0.05");
	continue;

}

server(""); //kalw tin server prin 3ekinisw tin diergasia gia na dhmiourgithoun kai ta epomena shm k sem segments ,
//alliws molis 8a trexame to enc2 meta 8a evgaze la8os me tin shmget-semget afou de 8a eixan dimiourgi8ei ta segments 
client();
return 0;

}
