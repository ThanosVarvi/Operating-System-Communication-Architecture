#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define SHMKEY (key_t)1600
#define SEMKEY (key_t)2600

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



void free_resources(int shmid, int semid) { 

   /* Delete the shared memory segment */
   shmctl(shmid,IPC_RMID,NULL);
   
   /* Delete the semaphore */
   semctl(semid,0,IPC_RMID,0);

}


int	main(void)
{
	int shmid, semid;
	char line[128], *shmem;
	union semnum arg;
	flag readfrominput = true,readfromshm = false;

	if ((shmid = shmget (SHMKEY, SHMSIZE, PERMS | IPC_CREAT)) < 0) {
		perror("shmget");
		exit(1);
		}
	printf("Creating shared memory with ID: %d\n",shmid);

	/* create a semaphore */
	if ((semid = semget(SEMKEY, 1, PERMS| IPC_CREAT)) <0) {
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
		//reading from command line
		if(readfrominput == true)
		{

			if ( (shmem = shmat(shmid, (char *)0, 0)) == (char *) -1) {
				perror("shmem");
				exit(1);
				}

			printf("Attaching shared memory segment \nEnter a message to P2: ");
			fgets(line, sizeof(line), stdin);
			line[strlen(line)-1]='\0';

			/* Write message in shared memory */
			strcpy(shmem, line); 

			printf("Writing to shared memory region: %s\n", line);
			
			/* Make shared memory available for reading */
			sem_V(semid);

			readfrominput = false;
			readfromshm = true;

			shmdt(shmem);
			sleep(1);

			if(strcmp(line, "TERM") == 0)
			{
				printf("Process P1 is beeing terminated..\n");
				return;
			}
		
		}



		if(readfromshm == true)
		{
			printf("Asking for access to shared memory region \n");
				sem_P(semid);
			if ( (shmem = shmat(shmid, (char *) 0, 0)) == (char *) -1 ) {
				perror("shmat");
				exit(1);
				}
			printf("Attaching shared memory segment\n");


			if(strcmp(shmem, "TERM") == 0)
			{
				/* Clear recourses */
				free_resources(shmid,semid);
				printf("Shared memory and semaphore segments (P1<->ENC1) have been released and process P1 is being terminated..\n");
				return;
			}
			

			if(strcmp(shmem, "ERROR-CODE: 1998") == 0) //resend the message (it has been modified due to channel noice)
				printf("ERROR: Please resend your message (it has been modified due to channel noice)");
			else
			{
				
				printf("\nMessage from P2 is: %.*s\n\n", (int)(strlen(shmem) - 16), shmem); //I get rid rid of the checksum in the visualization
			}
			
		
			
			readfromshm = false;
			readfrominput = true;

			shmdt(shmem);

			}
			
	
	} //while ends here

	return 0;
}
