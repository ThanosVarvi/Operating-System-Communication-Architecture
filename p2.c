#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <sys/wait.h>

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

int sem_P(int sem_id) {

   struct sembuf sem_d;
   
   sem_d.sem_num = 0;
   sem_d.sem_op = -1;
   sem_d.sem_flg = 0;
   if (semop(sem_id,&sem_d,1) == -1) {
       perror("# Semaphore down (P) operation ");
       return -1;
   }
   return 0;

}

/* Semaphore V - up operation, using semop */

int sem_V(int sem_id) {

   struct sembuf sem_d;
   
   sem_d.sem_num = 0;
   sem_d.sem_op = 1;
   sem_d.sem_flg = 0;
   if (semop(sem_id,&sem_d,1) == -1) {
       perror("# Semaphore up (V) operation ");
       return -1;
   }
   return 0;

}



void free_resources(int shm_id, int sem_id) { 

   /* Delete the shared memory segment */
   shmctl(shm_id,IPC_RMID,NULL);
   
   /* Delete the semaphore */
   semctl(sem_id,0,IPC_RMID,0);

}





int main(void)
{

	int shmid, semid;
	char *shmem,line[128],message[MAXCHARACTERS], *modified, *msgfromchannel, *msgfromENC2;
	struct sembuf oper[1]={0,-1,0};

	if ((shmid = shmget (SHMKEY4, SHMSIZE, PERMS )) < 0) {
		perror("shmget");
		exit(1);
		}
	printf("Accessing shared memory with ID: %d\n",shmid);

	/* accessing a semaphore */
	if ((semid = semget(SEMKEY4, 1, PERMS )) <0) {
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
	//Reading from shared memory region: shmem


	strcpy(message, shmem);




	if(strcmp(message, "ERROR-CODE: 1998") == 0) //resend the message (it has been modified due to channel noice)
	printf("ERROR: Please resend your message (it has been modified due to channel noice)");
	else if(strcmp(message, "TERM") == 0)
	{
		/* Clear recourses */
   		free_resources(shmid,semid);
		printf("Shared memory and semaphore segments (ENC2<->P2) have been released and process P2 is being terminated..\n");
		return;
	}
	else
	printf("\nMessage from P1 is: %.*s\n\n", (int)(strlen(message) - 16), message); //I get rid rid of the checksum in the visualization


//////////////////////////////
	

//////////////////////////////




/////////////////////////////////


	printf("Enter a message to P1: ");
	fgets(line, sizeof(line), stdin);
	line[strlen(line)-1]='\0';

	/* Write message in shared memory */
	strcpy(shmem, line); 

	printf("Writing to shared memory region: %s\n", line);

	
	/* Make shared memory available for reading for server  */
	sem_V(semid);





	

	/* detach shared memeory */
	shmdt(shmem); 
	sleep(1);

	if(strcmp(line, "TERM") == 0)
	{
		printf("Process P2 is beeing terminated..\n");
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
	return 0;
}



