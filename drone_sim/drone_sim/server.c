#include <stdio.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include<stdbool.h>
#include <time.h>
#include <sys/file.h>
#include <sys/select.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>

# define MAX_MSG_LEN 1024
# define NUMPIPE 4
# define NUMCLIENT 2


pid_t wd_pid = -1;
pid_t window_pid;
bool sigint_rec = false;
float rho2 = 2;
int *pipeObfd[2]; 
int *pipeTafd[2]; 

typedef struct {
    int x;
    int y;
    float vx, vy;
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
void sig_handler(int signo, siginfo_t *info, void *context) {

    if (signo == SIGUSR1) {
        FILE *debug = fopen("logfiles/debug.log", "a");
        // SIGUSR1 received
        wd_pid = info->si_pid;
        fprintf(debug, "%s\n", "SERVER: signal SIGUSR1 received from WATCH DOG");
        kill(wd_pid, SIGUSR1);
        fclose(debug);
    }
    
    if (signo == SIGUSR2){
        FILE *debug = fopen("logfiles/debug.log", "a");
        fprintf(debug, "%s\n", "SERVER: terminating by WATCH DOG with return value 0...");
        printf("SERVER: terminating by WATCH DOG with return value 0...");
        fclose(debug);
        if (kill(window_pid, SIGTERM) == 0) {
            printf("SERVER: SIGTERM signal sent successfully to window");
        } 
        else {
            perror("Error sending SIGTERM signal");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_FAILURE);
    }
    if (signo == SIGINT){
        //pressed q or CTRL+C
        printf("SERVER: Terminating with return value 0...");
        FILE *debug = fopen("logfiles/debug.log", "a");
        FILE *errors = fopen("logfiles/errors.log", "a");
        fprintf(debug, "%s\n", "SERVER: terminating with return value 0...");
        fclose(debug);
        sigint_rec = true;
        if(write(*pipeObfd[1], "STOP", strlen("STOP")) == -1){
            perror("error in writing to pipe");
            writeToLog(errors, "SERVER: error in writing to pipe obstacles");
            exit(EXIT_FAILURE);
        }
        if(write(*pipeTafd[1], "STOP", strlen("STOP")) == -1){
            perror("error in writing to pipe");
            writeToLog(errors, "SERVER: error in writing to pipe targets");
            exit(EXIT_FAILURE);
        }
        fclose(errors);
        sleep(1);
        kill(window_pid, SIGTERM);
        
    }
}

bool isTargetTaken(int x, int y, int xt, int yt){
    /*
    Function to determine if th target is taken
    input: x, y: drone coordinates
           xt, yt: target coordinates
    output: true if the target is taken, false otherwise
    */
    float rho = sqrt(pow(x-xt, 2) + pow(y-yt, 2)); //distance between target and drone
    if(rho < rho2) //if it is in the "take area"
        return true;
}
// Use for the children processes (put an error string different each process)
pid_t spawn(const char * program, char ** arg_list) {
    FILE * errors = fopen("logfiles/errors.log", "a");
    pid_t child_pid = fork();
    if (child_pid != 0)
        return child_pid;
    else {
        execvp (program, arg_list);
        perror("exec failed");
        writeToLog(errors, "SERVER: execvp failed");
        exit(EXIT_FAILURE);
    }
    fclose(errors);
}

int main(int argc, char* argv[]){
    FILE * debug = fopen("logfiles/debug.log", "a");    // debug log file
    FILE * errors = fopen("logfiles/errors.log", "a");  // errors log file
    FILE * serdebug = fopen("logfiles/server.log", "w");    // server log file

    writeToLog(debug, "SERVER: process started");
    printf("SERVER : process started\n");
    Drone * drone;
    Drone dr;
    drone = &dr;
    int rows = 50;
    int cols = 100;

    // CREATING PIPE
    /*
        pipe 0: server -> ch1 re0 al server: wr0
        pipe 1: ch1 -> server wr1 al server: re1
        pipe 2: server -> ch2 re2 al server: wr2
        pipe 3: ch2 -> server wr3 al server: re3
    */
    int pipeDrfd[2];    // pipe for drone: 0 for reading, 1 for writing
    int ntargets = 0, nobstacles = 0;   // pipe for obstacles: 0 for reading, 1 for writing
    int targettaken = 0;
    char ti[] = "TI";
    char oi[] = "OI";
    char piperd[NUMPIPE][10];    // string that contains the readable fd of pipe_fd
    char pipewr[NUMPIPE][10];
    int pipe_fd[NUMPIPE][2]; 
    pid_t pidch[NUMCLIENT]; 
    pid_t window_pid;
    char fd_str[10];
    int port = 40000; // default port
    int client_sock;
    char msg [100];
    char sockmsg[MAX_MSG_LEN];
    char *token;
    char stop[] = "STOP";
    char start[] = "START";
    char ge[] = "GE";
    bool stopReceived = false;
    bool first_set_of_targets_arrived = false;
    int nobstacles_edge = 2 * (rows + cols);
    struct obstacle *edges[nobstacles_edge];
    int pipeWdfd[2];
    // these strings are for make the window knowing which type of data it will receive
    char *obs = "obs";
    char *tar = "tar";
    char *coo = "coo";

    if (debug == NULL || errors == NULL){
        perror("error in opening log files");
        exit(EXIT_FAILURE);
    }

    sscanf(argv[3], "%d", &port);

    
    // Window generation
    writeToLog(serdebug, "SERVER: generating the map");
    char *window_path[] = {"konsole", "-e", "./window", NULL};  // path of window process

    // pipe to window
    
    if(pipe(pipeWdfd) == -1){
        perror("error in pipe");
        writeToLog(errors, "SERVER: error in pipe opening");
        exit(EXIT_FAILURE);
    }

    sprintf(piperd[0], "%d", pipeWdfd[0]);
    char *argw[] = {"Konsole","-e","./window", piperd[0], NULL};  // path of window process
    window_pid = spawn("konsole", argw); // spawn the child process
    close(pipeWdfd[0]);

    // Drone pipe and select
    fd_set read_fds;
    fd_set write_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);

    sscanf(argv[1], "%d", &pipeDrfd[0]);
    sscanf(argv[2], "%d", &pipeDrfd[1]);

    writeToLog(serdebug, "SERVER: pipes opened");

    // Sending rows and cols to window and drone
    if((write(pipeWdfd[1], &rows, sizeof(int))) == -1){
        perror("error in writing to pipe");
        writeToLog(errors, "SERVER: error in writing to pipe");
        exit(EXIT_FAILURE);
    }
    if((write(pipeWdfd[1], &cols, sizeof(int))) == -1){
        perror("error in writing to pipe");
        writeToLog(errors, "SERVER: error in writing to pipe");
        exit(EXIT_FAILURE);
    }
    if((write(pipeDrfd[1], &rows, sizeof(int))) == -1){
        perror("error in writing to pipe");
        writeToLog(errors, "SERVER: error in writing to pipe");
        exit(EXIT_FAILURE);
    }
    if((write(pipeDrfd[1], &cols, sizeof(int))) == -1){
        perror("error in writing to pipe");
        writeToLog(errors, "SERVER: error in writing to pipe");
        exit(EXIT_FAILURE);
    }
    writeToLog(serdebug, "SERVER: rows and cols sent to window and drone");

    struct sigaction sa; //initialize sigaction
    sa.sa_flags = SA_SIGINFO; // Use sa_sigaction field instead of sa_handler
    sa.sa_sigaction = sig_handler;

    // Register the signal handler for SIGUSR1
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        writeToLog(errors, "SERVER: error in sigaction()");
        exit(EXIT_FAILURE);
    }

    if(sigaction(SIGUSR2, &sa, NULL) == -1){
        perror("sigaction");
        writeToLog(errors, "SERVER: error in sigaction()");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error setting up SIGINT handler");
        writeToLog(errors, "SERVER: error in sigaction()");
        exit(EXIT_FAILURE);
    }
    
    // SOCKET IMPLEMENTATION
    // generating socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return 1;
    }
    
    // Bind the socket
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;
    writeToLog(serdebug, "SERVER: binding...");
    
    if (bind(sock, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("bind");
        writeToLog(errors, "SERVER: error in bind()");
        return 1;
    }

    // Listen for connections
    if (listen(sock, 5) == -1) {  // 5 is the maximum length of the queue of pending connections
        perror("listen");
        return 1;
    }
    writeToLog(serdebug, "SERVER: listening...");

    // generates all the pipes
    for (int i = 0; i < NUMPIPE; i++){
        if (pipe(pipe_fd[i]) == -1){
            perror("error in pipe");
            writeToLog(errors, "SERVER: error opening pipe");
        }
    }
    writeToLog(serdebug, "SERVER: pipes opened");
    // converts the fd of the pipes in strings
    for (int i = 0; i < NUMPIPE; i++){
        sprintf(piperd[i], "%d", pipe_fd[i][0]);
        sprintf(pipewr[i], "%d", pipe_fd[i][1]);
    }
    writeToLog(serdebug, "SERVER: pipes converted in strings");

    // Generating all the server children for socket connection
    for (int i = 0; i < NUMCLIENT; i++){ // for make sure that obstacles and targets are ready to send data
        do{
        client_sock = accept(sock, NULL, NULL); // accept the connection
        }while(client_sock == -1 && errno == EINTR); // is a signal interrupt the accept, it will be repeated

        if(client_sock == -1){ // checks for other types of errors
            perror("accept");
            writeToLog(errors, "SERVER: error in accept()");
            return 1;
        }
        writeToLog(serdebug, "SERVER: connection accepted");
        sprintf(fd_str, "%d", client_sock);
        char id[5];
        sprintf(id, "%d", i);
        char rc[100];
        sprintf(rc, "%d.000,%d.000", rows,cols);

        char *args[] = {"./sockserver",fd_str, piperd[i*2], pipewr[i*2+1], id, rc, NULL};  // path of child process
        pidch[i] = spawn("./sockserver", args); // spawn the child process
        
        // Parent process: close the client socket and go back to accept the next connection
        writeToLog(serdebug, "SERVER: forked");
        close(client_sock);
        // close unused pipes
        close(pipe_fd[i*2][0]);
        close(pipe_fd[i*2+1][1]);
    }
    usleep(500000);
 
    writeToLog(serdebug, "SERVER: reading the pipe with sockets");
    for (int i= 0; i < NUMCLIENT; i++){
        char buffer[MAX_MSG_LEN];
        if ((read(pipe_fd[i*2+1][0], buffer, MAX_MSG_LEN)) == -1) { // reads from obstacles
            perror("error in reading from pipe from sockChild 1");
            writeToLog(errors, "SERVER: error in reading from pipe sockChild 1");
            exit(EXIT_FAILURE);
        }
        writeToLog(serdebug, buffer);
        if (strcmp(buffer, ti) == 0) // if targets are initialized
        {
            // sets the pipe between the child process that receives the targets
            pipeTafd[0] = &pipe_fd[i*2+1][0];
            pipeTafd[1] = &pipe_fd[i*2][1];
            writeToLog(serdebug, "SERVER: pipeTafd set");
        }
        if (strcmp(buffer, oi) == 0) // if obstacles are initialized
        {
            // sets the pipe between the child process that receives the obstacles
            pipeObfd[0] = &pipe_fd[i*2+1][0];
            pipeObfd[1] = &pipe_fd[i*2][1];
            writeToLog(serdebug, "SERVER: pipeObfd set");
        }
    }
    // writes to drone that the connection between client and server is accepted, so it can starts
    if ((write(pipeDrfd[1], start, strlen(start) + 1)) == -1) { 
        perror("error in writing to pipe");
        writeToLog(errors, "SERVER: error in writing to pipe drone to make him start");
        exit(EXIT_FAILURE);
    }

   // SIGNALS
    
    int count = 0;
    float num1, num2;
    int sel;
    targets *target[20];
    while(!sigint_rec){
        // select wich pipe to read from between drone and obstacles
        FD_SET(pipeDrfd[0], &read_fds); // include pipeDrfd[0] in read_fds
        FD_SET(*pipeObfd[0], &read_fds); // include pipeObfd[0] in read_fds
        FD_SET(*pipeTafd[0], &read_fds); // include pipeTafd[0] in read_fds
        // searching for the max fd to use in select
        int max_fd = -1;
        if(*pipeObfd[0] > max_fd) {
            max_fd = *pipeObfd[0];
        }
        if (*pipeTafd[0] > max_fd) {
            max_fd = *pipeTafd[0];
        }
        if(pipeDrfd[0] > max_fd) {
            max_fd = pipeDrfd[0];
        }
        
        do{
            sel = select(max_fd + 1, &read_fds, &write_fds, NULL, NULL);
        }while(sel == -1 && errno == EINTR && !sigint_rec);
        
        if(sel ==-1){
            perror("error in select");
            writeToLog(errors, "SERVER: error in select");
            exit(EXIT_FAILURE);
        }
        else if(sel>0){

            if(FD_ISSET(*pipeTafd[0], &read_fds)){
                writeToLog(serdebug, "SERVER: reading from pipe targets of socket server");
                targettaken = 0;
                first_set_of_targets_arrived = true;
                memset(sockmsg, '\0', MAX_MSG_LEN);
                if ((read(*pipeTafd[0], sockmsg, MAX_MSG_LEN)) == -1) { // reads from sockserver the string which contains targets
                    perror("error in reading from pipe");
                    writeToLog(errors, "SERVER: error in reading from pipe targets");
                    exit(EXIT_FAILURE);
                }
                writeToLog(serdebug, sockmsg);

                if (sockmsg[0] != 'T'){ // checks if the pipe fd is correctly set
                    writeToLog(errors, "SERVER: file descriptor of the pipe is not really the tagets one");
                    exit(EXIT_FAILURE);
                }
                else {
                    sscanf(sockmsg, "T[%d]", &ntargets);
                    
                    // Sending the number of targets to the drone and window
                    if((write(pipeDrfd[1], tar, strlen(tar))) == -1){
                        perror("error in writing to pipe");
                        writeToLog(errors, "SERVER: error in writing to pipe drone that it will sends targets");
                        exit(EXIT_FAILURE);
                    }                    
                    if ((write(pipeDrfd[1], &ntargets, sizeof(int))) == -1){  // writes to drone ntargets
                        perror("error in writing to pipe");
                        writeToLog(errors, "SERVER: error in writing to pipe number of targets");
                        exit(EXIT_FAILURE);
                    }
                    if((write(pipeWdfd[1], tar, strlen(tar))) == -1){
                        perror("error in writing to pipe");
                        writeToLog(errors, "SERVER: error in writing to pipe window that it will sends targets");
                        exit(EXIT_FAILURE);
                    }

                    if ((write(pipeWdfd[1], &ntargets, sizeof(int))) == -1){  // writes to window ntargets
                        perror("error in writing to pipe");
                        writeToLog(errors, "SERVER: error in writing to pipe number of targets");
                        exit(EXIT_FAILURE);
                    }

                    // Reading the targets from the pipe
                    token = strtok(sockmsg, "]");
                    for (token = strtok(NULL, "|"); token != NULL; token = strtok(NULL, "|")){
                        target[count] = malloc(sizeof(targets));
                        sscanf(token, "%f,%f", &num1, &num2);
                        // we used int because the drone can only move in integer coordinates
                        // so, because the protocol is to send the coordinates as float, we have to cast them to int
                        target[count]->x = (int)num1; 
                        target[count]->y = (int)num2;
                        target[count]->taken = false;

                        sprintf(msg,"SERVER: target %d position: (%d, %d)\n", count, target[count]->x, target[count]->y);
                        writeToLog(serdebug, msg);

                        if ((write(pipeDrfd[1], target[count], sizeof(targets))) == -1){  // writes to drone the targets position
                            perror("error in writing to pipe");
                            writeToLog(errors, "SERVER: error in writing to pipe drone the targets");
                            exit(EXIT_FAILURE);
                        }
                        if ((write(pipeWdfd[1], target[count], sizeof(targets))) == -1){  // writes to window the targets position
                            perror("error in writing to pipe");
                            writeToLog(errors, "SERVER: error in writing to pipe window the targets");
                            exit(EXIT_FAILURE);
                        }
                        count ++;
                    }
                    if (count != ntargets) // checks if it sent the right number of targets
                    {
                        writeToLog(serdebug, "SERVER: error in reading the targets (different numeber of targets)");
                    }
                    count = 0;
                }
            }
            

            if(FD_ISSET(*pipeObfd[0], &read_fds)){
                memset(sockmsg, '\0', MAX_MSG_LEN); // clear the string
                if ((read(*pipeObfd[0], sockmsg, MAX_MSG_LEN)) == -1) { // reads from the pipe the string which contains obstacles
                    perror("error in reading from pipe");
                    writeToLog(errors, "SERVER: error in reading from pipe obstacles");
                    exit(EXIT_FAILURE);
                }
                writeToLog(serdebug, sockmsg);
                if (sockmsg[0] != 'O'){ // checks if the pipe fd is correctly set
                    writeToLog(errors, "SERVER: file descriptor of the pipe is not really the obstacles one");
                    exit(EXIT_FAILURE);
                }
                else {
                    sscanf(sockmsg, "O[%d]", &nobstacles);

                    // Sending the number of obstacles to the drone and window
                    if((write(pipeDrfd[1], obs, strlen(obs))) == -1){
                        perror("error in writing to pipe");
                        writeToLog(errors, "SERVER: error in writing to pipe drone that it will sends obstacles");
                        exit(EXIT_FAILURE);
                    }
                    if ((write(pipeDrfd[1], &nobstacles, sizeof(int))) == -1){  // writes to drone nobstacles
                        perror("error in writing to pipe");
                        writeToLog(errors, "SERVER: error in writing to pipe number of obstacles");
                        exit(EXIT_FAILURE);
                    }
                    if ((write(pipeWdfd[1], obs, strlen(obs))) == -1){  // writes to window that it will sends obstacles
                        perror("error in writing to pipe");
                        writeToLog(errors, "SERVER: error in writing to pipe window that it will sends obstacles");
                        exit(EXIT_FAILURE);
                    }
                    if ((write(pipeWdfd[1], &nobstacles, sizeof(int))) == -1){  // writes to window nobstacles
                        perror("error in writing to pipe");
                        writeToLog(errors, "SERVER: error in writing to pipe number of obstacles");
                        exit(EXIT_FAILURE);
                    }

                    // Reading the obstacles coordinates from the string
                    struct obstacle *obstacles[nobstacles];
                    token = strtok(sockmsg, "]");
                    for (token = strtok(NULL, "|"); token != NULL; token = strtok(NULL, "|")){
                        obstacles[count] = malloc(sizeof(struct obstacle));
                        sscanf(token, "%f,%f", &num1, &num2);
                        // we used int because the drone can only move in integer coordinates
                        // so, because the protocol is to send the coordinates as float, we have to cast them to int
                        obstacles[count]->x = (int)num1;
                        obstacles[count]->y = (int)num2;
                        sprintf(msg,"SERVER: obstacle %d position: (%d, %d)\n", count, obstacles[count]->x, obstacles[count]->y);
                        writeToLog(serdebug, msg);
                        if ((write(pipeDrfd[1], obstacles[count], sizeof(struct obstacle))) == -1){  // writes to drone obstacles
                            perror("error in writing to pipe");
                            writeToLog(errors, "SERVER: error in writing to pipe drone the obstacles");
                            exit(EXIT_FAILURE);
                        }
                        if ((write(pipeWdfd[1], obstacles[count], sizeof(struct obstacle))) == -1){  // writes to window obstacles
                            perror("error in writing to pipe");
                            writeToLog(errors, "SERVER: error in writing to pipe window the obstacles");
                            exit(EXIT_FAILURE);
                        }
                        count ++;
                    }
                    if (count != nobstacles) // checks if it sent the right number of obstacles
                    {
                        writeToLog(serdebug, "SERVER: error in reading the obstacles (different numeber of obstacles)");
                    }
                    count = 0;
                }
            }
            
            if(FD_ISSET(pipeDrfd[0], &read_fds)){
                    if ((read(pipeDrfd[0], drone, sizeof(Drone))) == -1) { // reads from drone
                    perror("error in reading from pipe");
                    writeToLog(errors, "SERVER: error in reading from pipe drone");
                    exit(EXIT_FAILURE);
                    }
                    printf("SERVER: drone position: (%d, %d)\n", drone->x, drone->y);
                
            }
            if((write(pipeWdfd[1], coo, strlen(coo))) == -1){
                perror("error in writing to pipe");
                writeToLog(errors, "SERVER: error in writing to pipe window that it will sends coordinates");
                exit(EXIT_FAILURE);
            }
            if ((write(pipeWdfd[1], drone, sizeof(Drone))) == -1){  // writes to window drone
                perror("error in writing to pipe");
                writeToLog(errors, "SERVER: error in writing to pipe window the drone");
                exit(EXIT_FAILURE);
            }
        } 
        // if the first set of target is arrived (if not ntargets is 0) starts to check if the drone is taking targets
        if(first_set_of_targets_arrived){
            for (int i = 0; i< ntargets; i++){
                if (!(target[i]->taken) && isTargetTaken(drone->x, drone->y, target[i]->x, target[i]->y)){
                    target[i]->taken = true;
                    targettaken++;
                }
            }
            if(targettaken==ntargets){ // if took all targets sends GE to targets
                writeToLog(serdebug, "SERVER: all targets taken");
                if(write(*pipeTafd[1], ge, strlen(ge)) == -1){
                    perror("error in writing to pipe");
                    writeToLog(errors, "SERVER: error in writing to pipe targets");
                    exit(EXIT_FAILURE);
                }
                targettaken = 0;
            }
        }
    }

    // waits window to terminate
    for (int i = 0; i < NUMCLIENT; i++){
        if (waitpid(pidch[i], NULL, 0)==-1){
            perror("wait");
            writeToLog(errors, "SERVER: error in wait");
        }
    }
    if(waitpid(window_pid, NULL, 0)==-1){
        perror("wait");
        writeToLog(errors, "SERVER: error in wait");
    };
    // closing pipes
    for (int i = 0; i < 2; i++){
        if (close(pipeDrfd[i]) == -1){
            perror("error in closing pipe");
            writeToLog(errors, "SERVER: error in closing pipe Drone");
        }
    // close the right pipe with the socket child
    }
    for (int i = 0; i < NUMCLIENT; i++){
        if (close(pipe_fd[i*2+1][0]) == -1){
            perror("error in closing pipe");
            writeToLog(errors, "SERVER: error in closing pipe socket");
        }
        if (close(pipe_fd[i*2][1]) == -1){
            perror("error in closing pipe");
            writeToLog(errors, "SERVER: error in closing pipe socket");
        }
    }

    close(sock);
    close(pipeWdfd[1]);
    fclose(debug);
    fclose(errors);
    fclose(serdebug);
    return 0;
}