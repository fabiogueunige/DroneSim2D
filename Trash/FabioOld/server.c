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
#include <sys/shm.h>
#include <sys/sem.h>


float weight = 0;
float screw = 0; // attrito

#define SEM_KEY 1234  // Chiave per i semafori
#define SHM_KEY 5678  // Chiave per la memoria condivisa
#define DEBUG 1

typedef struct Position { // se problemi togli Position e anche gli altri
    int x;
    int y;
} position;

// #ifndef DEBUG
void sigusr1Handler(int signum, siginfo_t *info, void *context) {
    if (signum == SIGUSR1){
        /*send a signal SIGUSR2 to watchdog */
        //printf("SERVER sig handler");
        kill(info->si_pid, SIGUSR2);
    }
}
// endif
/*
typedef struct Strenght{
    float fx;
    float fy;
} strength;

typedef struct Velocity {
    float vx;
    float vy;
} velocity;
*/


int main (int argc, char* argv[])
{
//#ifndef DEBUG
//#endif

    pid_t mpid;
    int i,children = 2;
    int mfd[children];
    char* pidstr[children];
    void* ptr;
    int shmid;
    int semid;
    FILE *serverfile;
    
    // strength *force;
    // velocity *vel;
    position *pos;
    // server con shared memory riceve tutto da drone.c e manda alla mappa 
    // per muovere il drone 
    serverfile = fopen("server.txt", "w");
    if (serverfile == NULL)
    {
        perror("Error opening file!\n");
        return 1;
        //exit(1);
    }
    fprintf(serverfile, "Server created\n");
    fclose(serverfile);

    //configure the handler for sigusr1
    struct sigaction sa_usr1;
    sa_usr1.sa_sigaction = sigusr1Handler;
    sa_usr1.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1){ 
        perror("sigaction");
        return -1;
    }


// #ifndef DEBUG
    if ((pipe(mfd)) < 0) {
        perror("pipe map ncurses");
        return 2;
    }
    
    if ((mpid = fork()) == -1) {
        perror("fork map");
        return 1;
    }
    for (i = 0; i < children; i++) {
        sprintf(pidstr[i], "%d", mfd[i]);
    }
    if (mpid == 0) {
        char* argvm[] = {"konsole", "-e","./map",pidstr[0],pidstr[1], NULL};
        // Add maybe some arguments to argvm
        execvp("konsole",argvm);
        printf("error in exec of map\n");
        return -2;
    }
    close(mfd[0]);

    // Shared memory

    shmid = shmget(SHM_KEY,sizeof(position), IPC_CREAT | 0666);
    if (shmid == -1) {
        printf("Shared memory failed\n");
        return 1;
    }

    // Semaphore
    semid = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("Semaphore failed");
        return 3;
    }
    if (semctl(semid, 0, SETVAL, 1) == -1) {
        perror("semctl");
        return 3;
    }
    // shared memory init
    pos = (position*) shmat(shmid, NULL, 0);
    if ((void*) pos == (void*)-1) {
        perror("shmat");
        return 2;
    }
    


    /*ptr = mmap(0, sizeof(message), PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        printf("Map failed\n");
        exit(-1);
    }*/

// this is a good condition for the loop, but probably x and y can be negative
    while (pos->x != -1000 && pos->y != -1000) {
        // axcess to the drone data
        struct sembuf sb; // Acquiring the semaphore
        sb.sem_num = 0;
        sb.sem_op = -1; // Acquiring the semaphore
        sb.sem_flg = 0;
        if (semop(semid, &sb, 1) == -1) {
            perror("semop - lock");
            return 4;
        }
        // changing all the variables
        // sends position to the map
        if ((write(mfd[1], &pos, sizeof(position))) < 0) {
            perror("write map");
            return 3;
        }
        // Releasing the semaphore
        struct sembuf sl;
        sl.sem_num = 0;
        sl.sem_op = 1; // Releasing the semaphore
        sl.sem_flg = 0;
        if (semop(semid, &sl, 1) == -1) {
            perror("semop - unlock");
            return 4;
        }
    }



    //shmdt(force);
    //shmdt(vel);
    shmdt(pos);
    shmctl(shmid, IPC_RMID, NULL);
    close(mfd[1]);
    wait(NULL);

// #endif

    return 0;
}