#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SIZE 16

int main(int argc, char *argv[]) {
    int *sharedMemoryPointer;
    int sharedMemoryID;
    sem_t *semaphore;
    int initialValue = 1;
    pid_t pid;
    int temp;  // Declare temp variable to hold the temporary value for swapping
    int status;

    // Create shared memory
    sharedMemoryID = shmget(IPC_PRIVATE, SIZE, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (sharedMemoryID < 0) {
        perror("Unable to obtain shared memory\n");
        exit(1);
    }

    // Attach shared memory
    sharedMemoryPointer = (int *)shmat(sharedMemoryID, NULL, 0);
    if ((void *)sharedMemoryPointer == (void *)-1) {
        perror("Unable to attach\n");
        exit(1);
    }

    // Create semaphore and initialize
    semaphore = sem_open("/my_semaphore", O_CREAT, S_IRUSR | S_IWUSR, initialValue);
    if (semaphore == SEM_FAILED) {
        perror("Unable to create semaphore");
        exit(1);
    }

    // Set up initial values
    sharedMemoryPointer[0] = 0;
    sharedMemoryPointer[1] = 1;

    pid = fork();
    if (pid < 0) {
        printf("Fork failed\n");
        exit(1);
    }

    if (pid == 0) { // Child
        printf("Child process started.\n");
        for (int i = 0; i < 10; i++) {
            // Wait for the semaphore (decrement)
            if (sem_wait(semaphore) != 0) {
                perror("Semaphore wait failed");
                exit(1);
            }

            temp = sharedMemoryPointer[0];
            sharedMemoryPointer[0] = sharedMemoryPointer[1];
            sharedMemoryPointer[1] = temp;

            // Release the semaphore (increment)
            if (sem_post(semaphore) != 0) {
                perror("Semaphore post failed");
                exit(1);
            }

            printf("Child: Swapped values at iteration %d\n", i);
            sleep(1); // Simulate some processing time
        }
        printf("Child process done.\n");
        exit(0);
    } else { // Parent
        printf("Parent process started.\n");
        for (int i = 0; i < 10; i++) {
            // Wait for the semaphore (decrement)
            if (sem_wait(semaphore) != 0) {
                perror("Semaphore wait failed");
                exit(1);
            }

            temp = sharedMemoryPointer[1];
            sharedMemoryPointer[1] = sharedMemoryPointer[0];
            sharedMemoryPointer[0] = temp;

            // Release the semaphore (increment)
            if (sem_post(semaphore) != 0) {
                perror("Semaphore post failed");
                exit(1);
            }

            printf("Parent: Swapped values at iteration %d\n", i);
            sleep(1); // Simulate some processing time
        }
    }

    // Wait for child process to finish
    wait(&status);

    printf("Values: %d\t%d\n", sharedMemoryPointer[0], sharedMemoryPointer[1]);

    // Detach shared memory
    if (shmdt((void *)sharedMemoryPointer) < 0) {
        perror("Unable to detach\n");
        exit(1);
    }

    // Deallocate shared memory
    if (shmctl(sharedMemoryID, IPC_RMID, 0) < 0) {
        perror("Unable to deallocate\n");
        exit(1);
    }

    // Close and unlink the semaphore
    if (sem_close(semaphore) != 0) {
        perror("Unable to close semaphore");
        exit(1);
    }

    if (sem_unlink("/my_semaphore") != 0) {
        perror("Unable to unlink semaphore");
        exit(1);
    }

    return 0;
}