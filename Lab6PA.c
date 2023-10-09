#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <semaphore.h>

#define SIZE 16

int main(int argc, char *argv[])
{
    int status;
    long int i, loop, temp, *sharedMemoryPointer;
    int sharedMemoryID;
    pid_t pid;
    sem_t semaphore;
    int initialValue = 1;

    loop = atoi(argv[1]);
    sharedMemoryID = shmget(IPC_PRIVATE, SIZE, IPC_CREAT|S_IRUSR|S_IWUSR);

    if (sem_init(&semaphore, 0, initialValue) != 0) {
        perror("Unable to initalize semaphore");
        exit(1);
    }
    semaphore = sem_open("/my_semaphore", O_CREAT, S_IRUSR | S_IWUSR, initialValue);
    if (semaphore == SEM_FAILED) {
        perror("Unable to create semaphore");
        exit(1);
    }


    if(sharedMemoryID < 0) {
        perror ("Unable to obtain shared memory\n");
        exit (1);
    }

    sharedMemoryPointer = shmat(sharedMemoryID, 0, 0);
    if(sharedMemoryPointer == (void*) -1) {
        perror ("Unable to attach\n");
        exit (1);
    }

    sharedMemoryPointer[0] = 0;
    sharedMemoryPointer[1] = 1;

    pid = fork();
    if(pid < 0){
        printf("Fork failed\n");
    }

    // this semaphore doesnt work....
    if(pid == 0) { // Child
        for(i=0; i<loop; i++) {
            // Wait for the semaphore (decrement)
            if (sem_wait(&semaphore) != 0) {
                perror("Semaphore wait failed");
                exit(1);
            }

            // swap the contents of sharedMemoryPointer[0] and sharedMemoryPointer[1]
            temp = sharedMemoryPointer[0];
            sharedMemoryPointer[0] = sharedMemoryPointer[1];
            sharedMemoryPointer[1] = temp;

            // Release the semaphore (increment)
            if (sem_post(&semaphore) != 0) {
                perror("Semaphore post failed");
                exit(1);
            }
        }
        if(shmdt(sharedMemoryPointer) < 0) {
            perror ("Unable to detach\n");
            exit(1);
        }
        exit(0);
    }
    else
        for(i=0; i<loop; i++) {
            // Wait for the semaphore (decrement)
            if (sem_wait(&semaphore) != 0) {
                perror("Semaphore wait failed");
                exit(1);
            }

            // swap the contents of sharedMemoryPointer[1] and sharedMemoryPointer[0]
            temp = sharedMemoryPointer[0];
            sharedMemoryPointer[0] = sharedMemoryPointer[1];
            sharedMemoryPointer[1] = temp;

            // Release the semaphore (increment)
            if (sem_post(&semaphore) != 0) {
                perror("Semaphore post failed");
                exit(1);
            }
        }

    wait(&status);

    printf("Values: %li\t%li\n", sharedMemoryPointer[0], sharedMemoryPointer[1]);

    if(shmdt(sharedMemoryPointer) < 0) {
        perror ("Unable to detach\n");
        exit (1);
    }

    if(shmctl(sharedMemoryID, IPC_RMID, 0) < 0) {
        perror ("Unable to deallocate\n");
        exit(1);
    }

    return 0;
}

/* 
The programmer's goal is to implement controlled, asynchronous access to shared memory; in
this lab that translates to properly synchronizing access to the critical sections
of sampleProgramOne. The main idea of this assignment is to demonstrate that with the use of
proper synchronization mechanisms, the expected value is always obtained from the program.
• Protect the critical sections in sampleProgramOne to prevent memory access conflicts
from causing inconsistencies in the output. Your solution should still maximize potential
parallelism.
o Insert the appropriate code to create and initialize a semaphore
o Use semaphore operations to synchronize the two processes
o Perform required cleanup operations
o Submit a screenshot of your program working properly
Note: Semaphore creation and initialization are two different, hence non-atomic
operations. Be sure they have both been completed before another process attempts to access
the semaphore.
*/