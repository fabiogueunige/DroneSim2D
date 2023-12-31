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

struct obstacle {
    int x;
    int y;
};

typedef struct {
    int x;
    int y;
    bool taken;
} targets;

pid_t wd_pid = -1;
bool sigint_rec = false;
float rho0 = 5; //m repulsive force ray of action
float rho1 = 8; //m attractive force ray of action
float rho2 = 2; //m target min dist to take it
float eta = 20; 
float csi = 1; 

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

float calculateAttractiveForcex(int x, int y, int xt, int yt){
    // calculate attractive force in x direction
    float rho = sqrt(pow(x-xt, 2) + pow(y-yt, 2));
    float theta = atan2(y-yt, x-xt);
    
    if(x<rho1)
        return csi * rho * cos(theta);
    else
        return 0;
}

bool isTargetTaken(int x, int y, int xt, int yt){
    float rho = sqrt(pow(x-xt, 2) + pow(y-yt, 2));
    if(rho < rho2)
        return true;
}

float calculateAttractiveForcey(int x, int y, int xt, int yt){
    // calculate attractive force in y direction
    float rho = sqrt(pow(x-xt, 2) + pow(y-yt, 2));
    float theta = atan2(y-yt, x-xt);
    if (rho < rho1)
        return csi * rho * sin(theta);
    else
        return 0;
    
}

float calculateRepulsiveForcex(int x, int y, int xo, int yo){
    // calculate repulsive force in x direction
    float rho = sqrt(pow(x-xo, 2) + pow(y-yo, 2));
    float theta = atan2(y-yo, x-xo);
    if (rho < rho0){
        return eta* (1/rho - 1/rho0) * (1/pow(rho, 2)) * cos(theta);
    }
    else
        return 0;
}

float calculateRepulsiveForcey(int x, int y, int xo, int yo){
    // calclate repulsive force in y direction
    float rho = sqrt(pow(x-xo, 2) + pow(y-yo, 2));
    float theta = atan2(y-yo, x-xo);
    if (rho < rho0)
        return (eta * (1/rho - 1/rho0) * (1/pow(rho, 2)) * sin(theta));
    else
        return 0;
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
    fd_set read_fds;
    fd_set write_fds;
    FD_ZERO(&read_fds);
    //FD_ZERO(&write_fds);
    
    int keyfd; //readable file descriptor for key pressed in input 
    sscanf(argv[1], "%d", &keyfd);
    char input;
    int nobstacles = 0; // initializes nobstacles
    int ntargets = 0;
    Drone * drone;    // drone object
    Drone dr;
    drone = &dr;
    int pipeSefd[2];
    
    // FILE Opening
    if (debug == NULL || errors == NULL){
        perror("error in opening log files");
        exit(EXIT_FAILURE);
    }
    writeToLog(debug, "DRONE: process started");
    printf("DRONE: process started\n");
    
    // Pipe reading from arguments
    // pipe server and drone: 0 for reading from and 1 for writing to
    sscanf(argv[2], "%d", &pipeSefd[1]);
    sscanf(argv[3], "%d", &pipeSefd[0]);
    writeToLog(debug, "DRONE: pipes opened");
    
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
/*
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
    */

    int F[2]={0, 0};    // drone initially stopped
    int frx = 0, fry = 0;   // repulsive force in x and y direction

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

    

    float vx = 5, vy = 5;
    drone->vx = vx;
    drone->vy = vy;   
    // velocities initialized to (5,5) because:
    /*doing some test, we saw that the drone used to move to right when vx was >= 10, while 
    was used to move left when vx was <= 0. Because of this issue also the breaking, when
    it was moving left reported problems.
     We understood that this was because the point of equilibrium of the velocity was v = (5,5), 
     and so because of this we threated it like that: in breaking it stops breaking when it is 
     stopped (v = (5,5)), and then when we reset the drone we put the velocity in its point of 
     equilibrium.*/

    

    
    
    // reads obstacle position from server
    struct obstacle *obstacles[20]; //obstacles
    targets *target[20]; //targets
    // READS WINDOW DIMENSIONS
    int rows, cols;
    
    if ((read(pipeSefd[0], &rows, sizeof(int))) == -1){
        perror("error in reading from pipe");
        writeToLog(errors, "DRONE: error in reading from pipe rows");
        exit(EXIT_FAILURE);
    }
    if ((read(pipeSefd[0], &cols, sizeof(int))) == -1){
        perror("error in reading from pipe");
        writeToLog(errors, "DRONE: error in reading from pipe cols");
        exit(EXIT_FAILURE);
    }

    int x0 = cols/2;  //starting x
    int y0 = rows/2;  //starting y
    printf("DRONE:\n\n     Starting Position: \nx = %d; y = %d",x0, y0);
    printf("\n--------------------------------------\n");
    printf("     Parameters:\nM=%fkg; |F|= %dN, K=%fN*s*m", MASS, FORCE_MODULE, FRICTION_COEFFICIENT);
    printf("\n--------------------------------------\n\n");
    //initializes the drone's coordinates
    int x = x0;
    int y = y0;
    int fax = 0, fay = 0;
    printf("DRONE: rows = %d, cols = %d\n", rows, cols);
    int nedges = 2*(rows+cols); // number of edges
    struct obstacle *edges[nedges]; //edges

    for (int i = 0; i< rows; i++){
        edges[i] = malloc(sizeof(struct obstacle));
        edges[i]->x = 0;
        edges[i]->y = i;
        
        //write(pipeDrfd[1], edges[i], sizeof(struct obstacle));
        edges[i+rows+cols] = malloc(sizeof(struct obstacle));
        edges[i+rows+cols]->x = cols-1;
        edges[i+rows+cols]->y = i;
        
        //write(pipeDrfd[1], edges[i+rows+cols], sizeof(struct obstacle));
        //printf("DRONE: edge %d created at (%d, %d)\n", i, edges[i]->x, edges[i]->y);
        //printf("DRONE: edge %d created at (%d, %d)\n", i+rows+cols, edges[i+rows+cols]->x, edges[i+rows+cols]->y);
        // write to server with pipe ...
    }

    for (int i = 0; i< cols; i++){
        edges[i+rows] = malloc(sizeof(struct obstacle));
        edges[i+rows]->x = i;
        edges[i+rows]->y = rows-1;
        
        //write(pipeDrfd[1], edges[i], sizeof(struct obstacle));
        edges[i+2*rows+cols] = malloc(sizeof(struct obstacle));
        edges[i+2*rows+cols]->x = i;
        edges[i+2*rows+cols]->y = 0;
        
        //write(pipeDrfd[1], edges[i+rows+cols], sizeof(struct obstacle));
        //printf("DRONE: edge %d created at (%d, %d)\n", i, edges[i]->x, edges[i]->y);
        //printf("DRONE: edge %d created at (%d, %d)\n", i+rows+cols, edges[i+rows+cols]->x, edges[i+rows+cols]->y);
    }
    // reads edges
    /*
    for(int i = 0; i<nedges; i++){
            edges[i] = malloc(sizeof(struct obstacle));
            read(pipeSefd[0], edges[i], sizeof(struct obstacle));
            //printf("WINDOW: edge %d: x = %d, y = %d \n", i, edges[i]->x, edges[i]->y);
            printf("DRONE: edge %d: x = %d, y = %d \n", i, edges[i]->x, edges[i]->y);
    }*/

    
    while(!sigint_rec){
        
        bool brake = false;
        // t->t+1
        // select for skipping the switch if no key is pressed
        struct timeval timeout;
        timeout.tv_sec = 0;  
        timeout.tv_usec = 100000; // Set the timeout to 0.1 seconds
        FD_SET(keyfd, &read_fds);
        FD_SET(pipeSefd[0], &read_fds);
        //FD_SET(pipeSefd[1], &write_fds);    // adds the pipe to the set of fd to write to
        int ready;
        do {//because signal from watch dog stops system calls and read gives error, so if it gives it with errno == EINTR, it repeat the select sys call
            int maxfd = (keyfd > pipeSefd[0]) ? keyfd : pipeSefd[0];
            ready = select(maxfd + 1, &read_fds, NULL, NULL, &timeout);
        } while (ready == -1 && errno == EINTR);

        if (ready == -1){
            perror("select");
            writeToLog(errors, "DRONE: error in select");
            exit(EXIT_FAILURE);
        }
        if (ready == 0){
        }
        else{
            if(FD_ISSET(pipeSefd[0], &read_fds)){
                // reading obstacles and target
                char buffer[4];
                ssize_t numRead = read(pipeSefd[0], buffer, sizeof(buffer)-1);
                if (numRead == -1) {
                    perror("read");
                    return 1;
                }
                writeToLog(debug, buffer);
            //buffer[numRead] = '\0';
                if(strcmp(buffer, "obs") == 0){
                    if ((read(pipeSefd[0], &nobstacles, sizeof(int))) == -1){
                        perror("error in reading from pipe");
                        writeToLog(errors, "DRONE: error in reading from pipe number of obstacles");
                        exit(EXIT_FAILURE);
                    }
                    printf("DRONE: number of obstacles: %d\n", nobstacles);
                    //struct obstacle *obstacles[nobstacles];
                    for(int i=0; i<nobstacles; i++){
                        obstacles[i] = malloc(sizeof(struct obstacle));
                        if ((read(pipeSefd[0], obstacles[i], sizeof(struct obstacle))) == -1){
                            perror("error in reading from pipe");
                            writeToLog(errors, "DRONE: error in reading from pipe server for reading obstacles");
                            exit(EXIT_FAILURE);
                        }
                        printf("DRONE: obstacle %d at (%d, %d)\n", i, obstacles[i]->x, obstacles[i]->y);
                    }
                }
                else if(strcmp(buffer, "tar") == 0){
                    writeToLog(debug, "DRONE: reading targets");
                    if ((read(pipeSefd[0], &ntargets, sizeof(int))) == -1){
                        perror("error in reading from pipe");
                        writeToLog(errors, "DRONE: error in reading from pipe number of targets");
                        exit(EXIT_FAILURE);
                    }
                    printf("DRONE: number of targets: %d\n", ntargets);
                    for(int i=0; i<ntargets; i++){

                        target[i] = malloc(sizeof(targets));
                        if ((read(pipeSefd[0], target[i], sizeof(targets))) == -1){
                            perror("error in reading from pipe");
                            writeToLog(errors, "DRONE: error in reading from pipe server for reading targets");
                            exit(EXIT_FAILURE);
                        }
                        printf("DRONE: target %d at (%d, %d)\n", i, target[i]->x, target[i]->y);
                    }
                }
            }
            if(FD_ISSET(keyfd, &read_fds)){
                // reading key pressed
                if ((read(keyfd, &input, sizeof(char))) == -1){
                    perror("error in reading from pipe");
                    writeToLog(errors, "DRONE: error in reading from pipe the input");
                    exit(EXIT_FAILURE);

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
                    else if ((int)vx<0){
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
                    else if ((int)vy<0){
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
                    //reset drone
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
        }        
        for(int i= 0; i<2; i++){ // setting drone max input force
            if(F[i]<-6){
                F[i] = -6;
            }
            if(F[i]>6){
                F[i] = 6;
            }
        }
        
        // compute repulsive force of obstacles
        for (int i = 0; i < nobstacles; i++){
            frx += calculateRepulsiveForcex(x, y, obstacles[i]->x, obstacles[i]->y);
            fry += calculateRepulsiveForcey(x, y, obstacles[i]->x, obstacles[i]->y);
        }
        
        // compute attractive force of targets
        for(int i = 0; i<ntargets; i++){
            fax += calculateAttractiveForcex(x, y, target[i]->x, target[i]->y);
            fay += calculateAttractiveForcey(x, y, target[i]->x, target[i]->y);
            if(isTargetTaken(x, y, target[i]->x, target[i]->y)){
                target[i]->taken = true;
                printf("DRONE: target %d taken\n", i);
                writeToLog(debug, "DRONE: target taken");
            }
            else
                target[i]->taken = false;
        }

        // compute repulsive force of edges
        for(int i = 0; i < nedges; i++){
            frx += calculateRepulsiveForcex(x, y, edges[i]->x, edges[i]->y);
            fry += calculateRepulsiveForcey(x, y, edges[i]->x, edges[i]->y);
        }
        F[0]+=frx;
        F[1]+=fry;
        F[0]-=fax;
        F[1]-=fay;

        printf("frx: %d fry: %d\n", frx, fry);
        printf("fax: %d fay: %d\n", fax, fay);
        updatePosition(&x, &y, &vx, &vy, T, F[0], F[1]);
        //updatePosition(&x, &y, &vx, &vy, T, F[0]+frx+fax, F[1]+ fry + fay);
        frx = 0;
        fry = 0;
        fax = 0;
        fay = 0;
        drone->x = x;
        drone->y = y;
        drone->vx = vx;
        drone->vy = vy;
        drone->fx = F[0];
        drone->fy = F[1];
        
        if ((write(pipeSefd[1], drone, sizeof(Drone))) == -1){
            perror("error in writing to pipe");
            writeToLog(errors, "DRONE: error in writing to pipe x");
            exit(EXIT_FAILURE);
        }
        if(brake){
            F[0] = 0;
            F[1] = 0;
        }
    }
/*
    if (shm_unlink(shm_name) == 1) { // Remove shared memory segment.
        printf("Error removing %s\n", shm_name);
        exit(1);
    }
    if (close(shm_fd) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }
*/
    // closing pipes
    for (int i = 0; i < 2; i++){
        if (close(pipeSefd[i]) == -1){
            perror("error in closing pipe");
            writeToLog(errors, "DRONE: error in closing pipe Server");
        }
    }

    //munmap(drone, SIZE);
    
    fclose(debug);
    fclose(errors);
    return 0;
}