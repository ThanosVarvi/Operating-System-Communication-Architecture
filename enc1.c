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


#define SHMKEY (key_t)1600
#define SEMKEY (key_t)2600

#define SHMKEY2 (key_t)3333	// shm and sem between enc1 and channel
#define SEMKEY2 (key_t)4444

#define SHMSIZE 256
#define PERMS 0600



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

/* Semaphore Init - set a semaphore's value to val */

int sem_Init(int semid, int val) {

   union semnum arg;
   
   arg.val = val;
   if (semctl(semid,0,SETVAL,arg) == -1) {
       perror("# Semaphore setting value ");
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

const char *addchecksumtomessage(char *message)
{
    char *string =  malloc (sizeof (char) * MAXCHARACTERS);
	strcpy(string, message);
    char hash[MD5_DIGEST_LENGTH];
    char *buf1;
    MD5 (string, sizeof(string), hash);
    buf1 = MD5 (string, sizeof(string), NULL);

    printf("the message before the addition is: %s\n",string);
    printf("checksum is: %s\n", buf1);

    strcat(string, buf1);
   	printf("the merged message is: %s\n",string);
    return string;
}


int checksumisModified(char *message)
{
    printf("the input message is: %s\n",message);
    char *string1 =  malloc (sizeof (char) * MAXCHARACTERS);
	int g;
    
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
    g =  memcmp(buf, string1,n);
	return g;
}


const char *server(char *message)
{
    char *string =  malloc (sizeof (char) * MAXCHARACTERS);
	strcpy(string, message);

	char* returnmsg =  malloc (sizeof (char) * MAXCHARACTERS);

	int shmid, semid;
	char line[128], *shmem;
	union semnum arg;
	flag readfrominput = true,readfromshm = false;

	if ((shmid = shmget (SHMKEY2, SHMSIZE, PERMS | IPC_CREAT)) < 0) {
		perror("shmget");
		exit(1);
		}
	printf("Creating shared memory with ID: %d\n",shmid);

	/* create a semaphore */
	if ((semid = semget(SEMKEY2, 1, PERMS| IPC_CREAT)) <0) {
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

			printf("Writing to shared memory region: %s\n", line);
			
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

			strcpy(returnmsg, shmem);

			readfromshm = false;
			readfrominput = true;

			shmdt(shmem);

		}
		

		printf("Releasing shared memory region AND RETURNS %s\n", returnmsg);

		return returnmsg;

	}//while ends here

}




void 	main()
{

	server(""); //kalw tin server prin 3ekinisw tin diergasia gia na dhmiourgithoun kai ta epomena shm k sem segments ,
	//alliws molis 8a trexame to channel meta 8a evgaze la8os me tin shmget-semget afou de 8a eixan dimiourgi8ei ta segments 

	int shmid, semid, checksumerror=0, resendflag=0;
	char *shmem,line[128],message[MAXCHARACTERS], *modified, *msgfromchannel;
	struct sembuf oper[1]={0,-1,0};

	if ((shmid = shmget (SHMKEY, SHMSIZE, PERMS )) < 0) {
		perror("shmget");
		exit(1);
		}
	printf("Accessing shared memory with ID: %d\n",shmid);

	/* accessing a semaphore */
	if ((semid = semget(SEMKEY, 1, PERMS )) <0) {
		perror("semget");
		exit(1);
		}
	printf("Accessing semaphore with ID: %d \n",semid);



	while(1)
	{
		if(checksumerror !=1 && resendflag!=1)
																   //in case of coming here from continue, skip attach section
		{																					// we have alread attached shmem!!
		if ( (shmem = shmat(shmid, (char *) 0, 0)) == (char *) -1 ) {						 
			perror("shmat");																
			exit(1);
			}
		printf("Attaching shared memory segment\n");

		printf("Asking for access to shared memory region \n");
		sem_P(semid);
		printf("Reading from shared memory region: %s\n", shmem);
		strcpy(message, shmem);

		}

		

		if(checksumerror == 1)
		strcpy(modified, "ERROR-CODE: 1998");

		else if((strcmp(message , "TERM") == 0))
		{
			printf("TERM!!!!!!!!!!!!!!!!!!!!");
			strcpy(modified, message);
		}
		

		else
		{

			modified =  addchecksumtomessage(message);
			printf("modified message is %s\n", modified);
		}




		msgfromchannel =  server(modified);//steile to kai pare apantisi  ////////////////////////////////////////////////////////////////////////

			if(strcmp(modified, "TERM") == 0)
		{
			/* Clear recourses */
			free_resources(shmid,semid);
			printf("Shared memory and semaphore segments (P1<->ENC1) have been released and process enc1 is being terminated..\n");
			return;
		}
			

		printf(" MESSAGE FROM CHANNEL IS %s \n", msgfromchannel);

		///////////////////////////////////////////////////////////////////////////////////
			
		if(strcmp(msgfromchannel, "ERROR-CODE: 1998") !=0 && strcmp(msgfromchannel, "TERM") !=0)
		{

			if(checksumisModified(msgfromchannel) !=0)//ean uparxei la8os sto checksum //steile aitima la8ous
			{
				checksumerror = 1;
				printf("cointinue....resend the message"); //steile to ERROR-CODE: 1998 pisw, auto simatodotei to la8os sto checksum
				continue;
			}

			checksumerror = 0;
		
		}


			/* Write message in shared memory */
			strcpy(shmem, msgfromchannel); 

			printf("Writing to shared memory (send p1) region the message from channel: %s\n", msgfromchannel);

			
			/* Make shared memory available for reading for server  */
			sem_V(semid);


			
			

			/* detach shared memeory */
			shmdt(shmem); 
			sleep(1);


		if(strcmp(msgfromchannel, "TERM") == 0)
		{
			/* Clear recourses */
			free_resources(shmid,semid);
			printf("Shared memory and semaphore segments (P1<->ENC1) have been released and process enc1 is being terminated..\n");
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
