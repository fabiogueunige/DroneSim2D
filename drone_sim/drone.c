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

#define MASS 0.249    // kg -> 0.249 kg is the maximum mass of a drone that doesn't require license
#define FRICTION_COEFFICIENT 0.1    // N*s*m
#define FORCE_MODULE 1 //N
#define T 0.1 //s   time instants' duration

typedef struct {
    int x;
    int y;
    float vx;
    float vy;
    int fx, fy;
} Drone;    // Drone object

pid_t wd_pid = -1;
bool sigint_rec = false;

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
    return -FRICTION_COEFFICIENT * (velocity-5);
}

// Function to update position and velocity based on applied force
void updatePosition(int *x, int *y, float *vx, float *vy, float dt, float forceX, float forceY) {
    // Calculate viscous friction forces
    float frictionForceX = calculateFrictionForce(*vx);
    float frictionForceY = calculateFrictionForce(*vy);

    // Calculate acceleration using Newton's second law (F = ma)
    float accelerationX = (forceX + frictionForceX) / MASS;
    float accelerationY = (forceY + frictionForceY) / MASS;

    // Update velocity and position using the kinematic equations
    *vx += accelerationX * dt;
    *vy += accelerationY * dt;
    *x += *vx * dt + 0.5 * accelerationX * dt * dt;
    *y += *vy * dt + 0.5 * accelerationY * dt * dt;
}

void sig_handler(int signo, siginfo_t *info, void *context) {

    if (signo == SIGUSR1) {
        FILE *debug = fopen("logfiles/debug.log", "a");
        // SIGUSR1 received
        wd_pid = info->si_pid;
        printf("DRONE: signal SIGUSR1 received from WATCH DOG\n");
        fprintf(debug, "%s\n", "DRONE: signal SIGUSR1 received from WATCH DOG");
        kill(wd_pid, SIGUSR1);
        fclose(debug);
    }

    if (signo == SIGUSR2){
        FILE *debug = fopen("logfiles/debug.log", "a");
        fprintf(debug, "%s\n", "DRONE: terminating by WATCH DOG with return value 1");
        fclose(debug);
        exit(EXIT_FAILURE);
    }
}

// lock() per impedire che altri usino il log file -> unlock()
int main(int argc, char* argv[]){
    FILE * debug = fopen("logfiles/debug.log", "a");
    FILE * errors = fopen("logfiles/errors.log", "a");
    writeToLog(debug, "DRONE: process started");
    printf("DRONE: process started\n");
    fd_set read_fds;
    FD_ZERO(&read_fds);
    
    int keyfd; //readable file descriptor for key pressed in input 
    sscanf(argv[1], "%d", &keyfd);
    char input;

    Drone * drone;    // drone object

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

    int F[2]={0, 0};    // drone initially stopped

    // initialization of the command forces vectors
    int wf[] = {-1,-1};
    int ef[] = {0,-1};
    int rf[] = {1,-1};
    int sf[] = {-1,0};
    int ff[] = {1,0};
    int xf[] = {-1,1};
    int cf[] = {0,1};
    int vf[] = {1,1};
    
    //import drone initial position from the server
    sleep(2); // gives the time to the server to initialize starting values

    int x0 = drone->x;  //starting x
    int y0 = drone->y;  //starting y

    float vx = 5, vy = 5;
    drone->vx = vx;
    drone->vy = vy;   // velocities initialized to (5,5) because:
    /*doing some test, we saw that the drone used to move to right when vx was >= 10, while 
    was used to move left when vx was <= 0. Because of this issue also the breaking, when
    it was moving left reported problems.
     We understood that this was because the point of equilibrium of the velocity was v = (5,5), 
     and so because of this we threated it like that: in breaking it stops breaking when it is 
     stopped (v = (5,5)), and then when we reset the drone we put the velocity in its point of 
     equilibrium.*/

    printf("DRONE:\n\n     Starting Position: \nx = %d; y = %d",x0, y0);
    printf("\n--------------------------------------\n");
    printf("     Parameters:\nM=%fkg; |F|= %dN, K=%fN*s*m", MASS, FORCE_MODULE, FRICTION_COEFFICIENT);
    printf("\n--------------------------------------\n\n");
    //initializes the drone's coordinates
    int x = x0;
    int y = y0;
    int re; //return of the read function
    while(!sigint_rec){
        bool brake = false;
        // t->t+1

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
            
            re = read(keyfd, &input, sizeof(char)); //reads input
            if (re== -1){
                perror("read");
                writeToLog(errors, "DRONE: error in read");
            }
            switch (input) {
                case 'w':
                    for(int i=0; i<2; i++)
                        F[i] =F[i] + FORCE_MODULE * wf[i];
                    break;
                case 's':
                    for(int i=0; i<2; i++)
                        F[i] =F[i] + FORCE_MODULE * sf[i];
                    break;
                case 'd':
                    // goes on by inertia
                    F[0] = 0;
                    F[1] = 0;
                    break;
                case 'b':
                    // brake
                    
                    F[0] = 0;
                    F[1] = 0;
                    if ((int)vx>5){
                        F[0] += -FORCE_MODULE;
                        brake = true;
                    }
                    else if ((int)vx<5){
                        F[0] += FORCE_MODULE;
                        brake = true;
                    }
                    else
                        F[0] = 0;
                        brake = false;

                    if ((int)vy>5){
                        F[1] += -FORCE_MODULE;
                        brake = true;
                    }
                    else if ((int)vy<5){
                        F[1] += +FORCE_MODULE;
                        brake = true;
                    }
                    else
                        F[1] = 0;
                        brake = false;
                    break;
                case 'f':
                    for(int i=0; i<2; i++)
                        F[i] += FORCE_MODULE * ff[i];
                    break;
                case 'e':
                    for(int i=0; i<2; i++)
                        F[i] += FORCE_MODULE * ef[i];
                    break;
                case 'r':
                    for(int i=0; i<2; i++)
                        F[i] = F[i] + FORCE_MODULE * rf[i];
                    break;
                case 'x':
                    for(int i=0; i<2; i++)
                        F[i] = F[i] + FORCE_MODULE * xf[i];
                    break;
                case 'c':
                    for(int i=0; i<2; i++)
                        F[i] = F[i] + FORCE_MODULE * cf[i];
                    break;
                case 'v':
                    for(int i=0; i<2;i++)
                        F[i] = F[i] + FORCE_MODULE * vf[i];
                    break;
                case 'q':
                    exit(EXIT_SUCCESS);  // Exit the program
                case 'u':
                    //reset
                    
                    x = x0;
                    y = y0;
                    F[0] = 0;
                    F[1] = 0;
                    vx = 5;
                    vy = 5;
                    break;
                default:
                    break;
            }
        }
        updatePosition(&x, &y, &vx, &vy, T, F[0], F[1]);
        drone->x = x;
        drone->y = y;
        drone->vx = vx;
        drone->vy = vy;
        drone->fx = F[0];
        drone->fy = F[1];

        if(brake){
            F[0] = 0;
            F[1] = 0;
        }
    }

    if (shm_unlink(shm_name) == 1) { // Remove shared memory segment.
        printf("Error removing %s\n", shm_name);
        exit(1);
    }
    if (close(shm_fd) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }

    munmap(drone, SIZE);
    fclose(debug);
    fclose(errors);
    return 0;
}