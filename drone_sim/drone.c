#include <stdio.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <semaphore.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include <sys/file.h>
#define SEM_PATH_1 "/sem_drone1"
#define SEM_PATH_2 "/sem_drone2"

#define MASS 0.249    // kg
#define FRICTION_COEFFICIENT 0.1    // N*s*m

typedef enum {NO=0, OVER=1, UNDER=2} EDGE;

typedef struct {
    int x;
    int y;
    EDGE isOnEdgex; // if it is on the edges, for assignment 2
    EDGE isOnEdgey;
} Drone;    // Drone object

pid_t wd_pid = -1;

void writeToLog(FILE *logFile, const char *message) {
    time_t crtime;
    time(&crtime);
    int lockResult = flock(fileno(logFile), LOCK_EX);
    if (lockResult == -1) {
        perror("Failed to lock the log file");
        // Handle the error as needed (e.g., exit or return)
        return;
    }
    fprintf(logFile,"%s => ", ctime(&crtime));
    fprintf(logFile, "%s\n", message);
    fflush(logFile);

    int unlockResult = flock(fileno(logFile), LOCK_UN);
    if (unlockResult == -1) {
        perror("Failed to unlock the log file");
        // Handle the error as needed (e.g., exit or return)
    }
}

// Function to calculate viscous friction force
float calculateFrictionForce(float velocity) {
    return -FRICTION_COEFFICIENT * velocity;
}

// Function to update position and velocity based on applied force
void updatePosition(int *x, int *y, float *vx, float *vy, float dt, float forceX, float forceY) {
    // Calculate viscous friction forces
    float frictionForceX = calculateFrictionForce(*vx);
    float frictionForceY = calculateFrictionForce(*vy);

    // Calculate acceleration using Newton's second law (F = ma)
    float accelerationX = (forceX + frictionForceX) / MASS;
    float accelerationY = (forceY + frictionForceY) / MASS;
    float vx_1 = *vx;
    float vy_1 = *vy;
    // Update velocity and position using the kinematic equations
    *vx += accelerationX * dt;
    *vy += accelerationY * dt;
    *x += vx_1 * dt + 0.5 * accelerationX * dt * dt;
    *y += vy_1 * dt + 0.5 * accelerationY * dt * dt;
}

void sig_handler(int signo, siginfo_t *info, void *context) {

    if (signo == SIGUSR1) {
        FILE *debug = fopen("logfiles/debug.log", "a");
        // SIGUSR1 received
        wd_pid = info->si_pid;
        //received_signal =1;
        fprintf(debug, "%s\n", "DRONE: signal SIGUSR1 received from WATCH DOG");
        //printf("SERVER: Signal SIGUSR1 received from watch dog\n");
        //printf("SERVER: sending signal to wd\n");
        kill(wd_pid, SIGUSR1);
        fclose(debug);
    }

    if (signo == SIGUSR2){
        FILE *debug = fopen("logfiles/debug.log", "a");
        fprintf(debug, "%s\n", "DRONE: terminating by WATCH DOG");
        fclose(debug);
        exit(EXIT_FAILURE);
    }
}


// lock() per impedire che altri usino il log file -> unlock()
int main(int argc, char* argv[]){
    FILE * debug = fopen("logfiles/debug.log", "a");
    FILE * errors = fopen("logfiles/errors.log", "a");
    writeToLog(debug, "DRONE: process started");
    printf("i'm in drone\n");
    fd_set read_fds;
    FD_ZERO(&read_fds);
    
    int keyfd; //readable file descriptor for key pressed in input 
    sscanf(argv[1], "%d", &keyfd);
    char input;

    Drone * drone;    // drone object
    sem_t *sem_drone;  // semaphore for writing and reading drone
// OPENING SEMAPHORES
    sem_drone = sem_open(SEM_PATH_1, O_RDWR);  // Open existing semaphore

    if (sem_drone == SEM_FAILED) {
        perror("sem_open");
        writeToLog(errors, "DRONE: error in opening semaphore");
        exit(EXIT_FAILURE);
    }
// SIGNALS
    struct sigaction sa; //initialize sigaction
    sa.sa_flags = SA_SIGINFO; // Use sa_sigaction field instead of sa_handler
    sa.sa_sigaction = sig_handler;

    // Register the signal handler for SIGUSR1
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        writeToLog(errors, "DRONE: error in sigaction()");
        exit(EXIT_FAILURE);
    }

    if(sigaction(SIGUSR2, &sa, NULL) == -1){
        perror("sigaction");
        writeToLog(errors, "DRONE: error in sigaction()");
        exit(EXIT_FAILURE);
    }

// SHARED MEMORY OPENING AND MAPPING
    const char * shm_name = "/dronemem";
    const int SIZE = 4096;
    int i, shm_fd;
    shm_fd = shm_open(shm_name, O_RDWR, 0666); // open shared memory segment for read and write
    if (shm_fd == 1) {
        perror("shared memory segment failed\n");
        writeToLog(errors, "DRONE:shared memory segment failed");
        exit(EXIT_FAILURE);
    }

    drone = (Drone *)mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0); // protocol write
    if (drone == MAP_FAILED) {
        perror("Map failed");
        writeToLog(errors, "Map Failed");
        return 1;
    }
    

    // initialization of parameters
    // these are the phisical characteristics of the drone, beacuse of this are not imported from the server
    // I have to make them imported from a parameter file
    float modF = 1;   //N
    
    float T = 0.1;  //s
    /*
    float a = m/(T*T);  // 25
    float b = k/T;  // 5*/

    int F[2]={0, 0};    // drone initially stopped

    // initialization of the command forces vectors
    int wf[] = {-1,1};
    int ef[] = {0,-1};
    int rf[] = {1,-1};
    int sf[] = {-1,0};
    int ff[] = {1,0};
    int xf[] = {-1,-1};
    int cf[] = {0,1};
    int vf[] = {1,1};
    
    //import drone initial position from the server
    sleep(2); // gives the time to the server to initialize staring values
    int x0 = drone->x;  //starting x
    int y0 = drone->y;  //starting y

    float vx = 0, vy = 0;   // velocities


    int x_1;    //xi-1
    
    int x_2;    //xi-2
    int y_1;    //yi-1
    int y_2;    //yi-2
    float vx_1, vy_1;
    float ax, ay;

    printf("DRONE: starting position: x = %d; y = %d \n",x0, y0);
    //initializes the drone's coordinates
    int x = x0;
    int y = y0;
    
    int re;
    //int edgx, edgy;
    while(1){
        bool brake = false;
        // t->t+1
        //edgx = drone->isOnEdgex;
        //edgy = drone->isOnEdgey;
        /*
        x_1 = x;
        
        x_2 = x_1;
        
        y_1 = y;
        y_2 = y_1;

        vx_1 = vx;
        vy_1 = vy;
        */
        // select for skipping the switch if no key is pressed
        struct timeval timeout;
        timeout.tv_sec = 0;  
        timeout.tv_usec = 100000; // Set the timeout to 0.1 seconds
        FD_SET(keyfd, &read_fds);
        int ready;
        do {//because signal from watch dog stops system calls and read gives error, so if it gives it with errno == EINTR, it repeat the select sys call
            ready = select(keyfd + 1, &read_fds, NULL, NULL, &timeout);
        } while (ready == -1 && errno == EINTR);

        
        if (ready == -1){
            perror("select");
            writeToLog(errors, "DRONE: error in select");
            exit(EXIT_FAILURE);
        }
        if (ready == 0){
        }
        else{
            
            re = read(keyfd, &input, sizeof(char));
            if (re== -1){
                perror("read");
            }
            printf("%d\n ", re);
        
            printf(" %c \n", input);

            // aggiungere select per far andare avanti
            switch (input) {
                case 'w':
                    for(int i=0; i<2; i++)
                        F[i] =F[i] + modF * wf[i];
                    break;
                case 's':
                    for(int i=0; i<2; i++)
                        F[i] =F[i] + modF * sf[i];
                    break;
                case 'd':
                    // goes on by inertia
                    F[0] = 0;
                    F[1] = 0;
                    break;
                case 'b':
                    // frena
                    
                    F[0] = 0;
                    
                    if ((int)vx>0){
                        F[0] += -1;
                        brake = true;
                    }
                    else if ((int)vx<0){
                        F[0] += 1;
                        brake = true;
                    }
                    else
                        F[0] = 0;
                    break;
                case 'f':
                    for(int i=0; i<2; i++)
                        F[i] += modF * ff[i];
                    break;
                case 'e':
                    for(int i=0; i<2; i++)
                        F[i] +=modF * ef[i];
                    break;
                case 'r':
                    for(int i=0; i<2; i++)
                        F[i] = F[i] + modF * rf[i];
                    break;
                case 'x':
                    for(int i=0; i<2; i++)
                        F[i] = F[i] + modF * xf[i];
                    break;
                case 'c':
                    for(int i=0; i<2; i++)
                        F[i] = F[i] + modF * cf[i];
                    break;
                case 'v':
                    for(int i=0; i<2;i++)
                        F[i] = F[i] + modF * vf[i];
                    break;
                case 'q':
                    //kill(wd_pid, SIGUSR2);
                    exit(EXIT_FAILURE);  // Exit the program
                case 'u':
                    //reset
                    
                    x = x0;
                    y = y0;
                    F[0] = 0;
                    F[1] = 0;
                    vx = 0;
                    vy = 0;
                    /*
                    x_1 = 0;
                    x_2 = 0;
                    y_1 = 0;
                    y_2 = 0;
                    vx_1 = 0;
                    vx_1 = 0;*/
                    break;
                default:
                    break;
            }
        }
        updatePosition(&x, &y, &vx, &vy, T, F[0], F[1]);
        if(brake){
            F[0] = 0;
        }

        /* //THIS REGARDS 2ND ASSIGNMENT
        if(edgx ==1){
            // forza repulsiva verso sx
        }
        else if(edgx == 2){
            //
        }

        if(edgy ==1){

        }
        else if(edgy == 2){

        }*/
        printf("x force = %d; y force = %d\n",F[0], F[1]);
        printf("vx = %f; vy = %f \n", vx, vy);
        printf("x = %d; y = %d \n",x, y);
        //printf("EDGE x = %d\n", edgx);
        //printf("EDGE y = %d\n", edgy);

        drone->x = x;
        drone->y = y;
    }

    if (shm_unlink(shm_name) == 1) { // Remove shared memory segment.
        printf("Error removing %s\n", shm_name);
        exit(1);
    }
    if (close(shm_fd) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }

    sem_close(sem_drone);
    munmap(drone, SIZE);
    fclose(debug);
    fclose(errors);
    return 0;
}