#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define T 0.01 // Define a time constant

#define SEM_KEY 1234  // Chiave per i semafori
#define SHM_KEY 5678  // Chiave per la memoria condivisa
#define DEBUG 1
typedef struct Position { // se problemi togli Position e anche gli altri
    int x;
    int y;
} position;

typedef struct Strenght{
    float fx;
    float fy;
} strength;

typedef struct Velocity {
    float vx;
    float vy;
} velocity;

float weight = 0;
float screw = 0; // attrito

// metti strutture e sistema shared memory e fi watchdog
//#ifndef DEBUG
void droneMotion (int* x, int* y);

void sigusr1Handler(int signum, siginfo_t *info, void *context) {
    if (signum == SIGUSR1){
        /*send a signal SIGUSR2 to watchdog */
        kill(info->si_pid, SIGUSR2);
    }
}
// #endif
// Necessaria implementare una pipe per mandare il carattere al drone
// Necessario sistemare il watchdog

int main (int argc, char* argv[])
{
    // crea pieps da master e mantieni tra input e drone soltanto
    int shmid; 
    int semid;
    int tmpX = 0,tmpY = 0;
    strength force;
    velocity vel;
    position *pos;
    int pfd[2]; // posso sostituire con argc?
    FILE *dronefile;

    dronefile = fopen("drone.txt", "w");
    if (dronefile == NULL)
    {
        perror("Error opening file drone!\n");
        return 1;
        //exit(1);
    }
    fprintf(dronefile, "drone created\n");
    fclose(dronefile);
    
    //configure the handler for sigusr1
    struct sigaction sa_usr1;
    sa_usr1.sa_sigaction = sigusr1Handler;
    sa_usr1.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1){ 
        perror("sigaction");
        return -1;
    }

// #ifndef DEBUG

    // pipe computation (close writing)
    for (int i = 0; i < 2; i++) {
        pfd[i] = atoi(argv[i]); // converts from the string to the integer
    }
    close(pfd[1]);

    // Shared memory creation
    if ((shmid = shmget(SHM_KEY, sizeof(position), 0666)) < 0) {
        perror("shmget drone side");
        return 1;
    }
    // Shared memory attachment
    if ((pos = (position *) shmat(shmid, NULL, 0)) == (position *) -1) {
        perror("shmat drone side");
        return 1;
    }
    // Semaphore creation
    if ((semid = semget(SEM_KEY, 1, 0666)) < 0) {
        perror("semget drone side");
        return 1;
    }
    
    //main loop for functioning
    while (tmpX != -1000 && tmpY != -1000) 
    {
        // Reading forces frm the input
        // still to be computed

        // change the forces with function
        //droneMotion(&tmpX, &tmpY);
        // Writing the positiones in the shared memory 
        struct sembuf sb;
        sb.sem_num = 0;
        sb.sem_op = -1; // Acquisizione del semaforo
        sb.sem_flg = 0;
        if (semop(semid, &sb, 1) == -1) {
            perror("semop - lock");
            exit(EXIT_FAILURE);
        }

        // Lettura della variabile condivisa
        pos->x = pos->x + tmpX;
        pos->y = pos->y + tmpY;

        // Sblocco del semaforo
        struct sembuf su;
        su.sem_num = 0;
        su.sem_op = 1; // Rilascio del semaforo
        su.sem_flg = 0;
        if (semop(semid, &su, 1) == -1) {
            perror("semop - unlock");
            exit(EXIT_FAILURE);
        }

    }

    // closing pipe reading
    close (pfd[0]);

    
    // Deallocating the shared memory
    if ((shmdt(pos)) == -1) {
        perror("shmdt");
        return 5;
    }
    //shmdt(vel);
    //shmdt(force);
// #endif
    return 0;
}