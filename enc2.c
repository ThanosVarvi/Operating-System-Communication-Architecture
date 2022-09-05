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


#define SHMKEY3 (key_t)5555	// shm and sem between  channel and enc2
#define SEMKEY3 (key_t)6666

#define SHMKEY4 (key_t)7777	// shm and sem between enc1 and channel
#define SEMKEY4 (key_t)8888

#define SHMSIZE 256
#define PERMS 0600

#include <openssl/md5.h>
#include <string.h>
#define MAXCHARACTERS 100


typedef enum { false, true } flag;  //flags for situtations ready to happen (1 = ready, 0 = not ready)


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



 char *addchecksumtomessage(char *message)
{
    char *string =  malloc (sizeof (char) * strlen(message));
	strcpy(string, message);
    char hash[MD5_DIGEST_LENGTH];
    char *buf1;
    MD5 (string, sizeof(string), hash);
    buf1 = MD5 (string, sizeof(string), NULL);

    printf("the message without checksum is: %s\n",string);
    printf("checksum is: %s\n", buf1);

    strcat(string, buf1);
   	printf("the merged message is: %s\n",string);
    return string;
}



int checksumisModified(char *message)
{
	
    printf("the input message is: %s\n",message);
    char *string1 =  malloc (sizeof (char) *strlen(message));
    
    strcpy(string1, message);

    int n = strlen(string1);
 
    char first[MAXCHARACTERS];
    char second[MAXCHARACTERS];


    strcpy(first, string1);
    first[n - 16] = '\0'; // IMPORTANT!


    printf("first is %s\n", first);

    strcpy(second, &string1[n - 16]);

    printf("second is %s\n", second);

    char *buf = addchecksumtomessage(first);
	
	printf("COMPARING THE TWO CHECKSUMS:\n");

    printf("%s\n",buf);
    printf("%s\n",string1);
	char str1[n];
	char str2[n];
	strcpy(str1, buf);
	strcpy(str2, string1);
    int rt =  strncmp(str1, str2, n);
	return rt;
}




const char *server(char *message)
{

    char *rt =  malloc (sizeof (char) * MAXCHARACTERS);
    char *string =  malloc (sizeof (char) * MAXCHARACTERS);
	strcpy(string, message);

	int shmid, semid;
	char line[128], *shmem;
    
	union semnum arg;
	flag readfrominput = true,readfromshm = false;

	if ((shmid = shmget (SHMKEY4, SHMSIZE, PERMS | IPC_CREAT)) < 0) {
		perror("shmget");
		exit(1);
		}
	printf("Creating shared memory with ID: %d\n",shmid);

	/* create a semaphore */
	if ((semid = semget(SEMKEY4, 1, PERMS| IPC_CREAT)) <0) {
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
	printf("Asking for access to shared memory regionNNNNNNN \n");
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

	int shmid, semid, errormsg;
	char *shmem,line[128],message[MAXCHARACTERS], *modified, *msgfromchannel, *msgfromENC2, *msgfromP2, *k;
	struct sembuf oper[1]={0,-1,0};

	if ((shmid = shmget (SHMKEY3, SHMSIZE, PERMS )) < 0) {
		perror("shmget");
		exit(1);
		}
	printf("Accessing shared memory with ID: %d\n",shmid);

	/* accessing a semaphore */
	if ((semid = semget(SEMKEY3, 1, PERMS )) <0) {
		perror("semget");
		exit(1);
		}
	printf("Accessing semaphore with ID: %d \n",semid);



while(1)
{
	int errormsg = 0;
	if ( (shmem = shmat(shmid, (char *) 0, 0)) == (char *) -1 ) {
		perror("shmat");
		exit(1);
		}
	printf("Attaching shared memory segment\n");

	printf("Asking for access to shared memory region \n");
	sem_P(semid);
	printf("Reading from shared memory region: %s\n", shmem);


	strcpy(message, shmem);
	printf("TERMINALL  message is %s\n", message);


//////////////////////////////




if( (strcmp(message, "TERM") ==0) || (strcmp(message, "ERROR-CODE: 1998") ==0) )
{
	printf("special occasion\n");
//do nothing 

}

else
{	//CHECKING IF CHANNEL - NOISE has modified our message
	
    int p = checksumisModified(message);   // if checksumisModified(message) == 0 it has not been modified  else it has been
	
	if(p != 0 ) //if it has been modified
    {	

	    errormsg = 1;
	    printf("exoun allaksei!!!!");
    }

}				




//////////////////////////////      

if(errormsg == 0) //if it hasn't been modified
{
	printf("den exoun allaksei!!!!");
	msgfromP2 =  server(message); 
	printf(" MESSAGE FROM P2 IS %s \n", msgfromP2);
}



	if(strcmp(message, "TERM") == 0)
{
	/* Clear recourses */
   	free_resources(shmid,semid);
	printf("Shared memory and semaphore segments (ENC2<->P2) have been released and process enc2 is being terminated..\n");
	return;
}





/////////////////////////////////


if(errormsg == 1)
{
	//ERROR resend the message: ERROR-CODE: 1998

	k = "ERROR-CODE: 1998";
	printf("ERROR resend the messages\n");
	errormsg=0;
}


else if(strcmp(msgfromP2, "TERM") ==0)
{
	k = "TERM";
	//do nothing (de 8elw checksum edw)
}

else
{
	/* Write message in shared memory */
	k = addchecksumtomessage(msgfromP2);   //vale to ckecksum afou einai kanoniko minima
	printf("Writing to shared memory region: %s\n", k);
}



	



strcpy(shmem, k);
	
	sem_V(semid);

	

	/* detach shared memeory */
	shmdt(shmem); 
	sleep(1);

	if(strcmp(k, "TERM") == 0)
{
	/* Clear recourses */
   	free_resources(shmid,semid);
	printf("Shared memory and semaphore segments (CHAN<->ENC2) have been released and process enc2 is being terminated..\n");

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
server(""); //kalw tin server prin 3ekinisw tin diergasia gia na dhmiourgithoun kai ta epomena shm k sem segments ,
//alliws molis 8a trexame to p2 meta 8a evgaze la8os me tin shmget-semget afou de 8a eixan dimiourgi8ei ta segments 
client();
return 0;

}