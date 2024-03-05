## ADVANCED AND ROBOT PROGRAMMING
----------------------------

# DRONE OPERATION INTERACTIVE SIMULATOR

This project consits in a drone operation interactive simulator.  
The drone is driven by the user and moves in an empty arena through the user's keyboard commands. 

!!!! Warning: After the window opens return to the terminal (press it) because, if not, the program will not take into account the keyboard iput !!!!

## Description

This is the first project for the assignment of Advanced and Robot Programming course of Robotics Engeneerig Master degree in Università degli studi di Genova.  
It consists in a 2Dimensional drone motion simulator with the goal of considering all the forces that act on the rigid body during the motion in the real world, such as the friction force.  
This simulation environment implents several different processes to manage all the possible application or complication that can happen during a drone flight. 
All the processes exchange data through shared memeory or pipes between them and all of them are chldren of the master process.   


## Repository Architecture

This github package contains **drone_sim** folder that is the main folder. Insided that there are:  
* logfiles: contains all the file opened during the software implementation to check the application state
* All processes.c
* the compiler compile.sh.


-----------------------
## Project Architecture

This program is composed by several different process:
* master: the main process and the father of all the other process. It `fork()` to generate the new child process and through an `exevp()` it overwrites the child with a new computation. It sends all the id-process to the watchdog
* [server:](https://github.com/leleviola/arp/blob/master/drone_sim/server.c) the blackboard  of the project, implemented through a shared memory with the drone that compute all the user's inputs
* [window:](https://github.com/leleviola/arp/blob/master/drone_sim/window.c) window that show the geometrical position of the drone in the arena and the position's numeeric values.
* [drone:](https://github.com/leleviola/arp/blob/master/drone_sim/drone.c) the computation side of the project, it takes the input, process it with the optimal exstimation of the drone position in the real world and share the data with the server.
* [input:](https://github.com/leleviola/arp/blob/master/drone_sim/input.c)  Takes the user's char input. It is important to allows the user to give the data input
* [watchdog:](https://github.com/leleviola/arp/blob/master/drone_sim/wd.c)  the process controller. Through the signal and the signal handler functions of the process it checks if all the process are still alive. If not, it ends the program.
  
![Project Architecture](https://github.com/leleviola/arp/blob/resources/resources/Ass1Architecture.png)
  
## Main features
The program starts with an introduction page to show all the commands to the user, and all the other processes initialize only when the description child of the master process has terminated through a `wait(NULL);`.  
It is important to remeber that the user iterface are connected to the parent process through unnamed pipes.
This several process are important to build a modular project and gives different tasks to the different pojects, as explained in the previouse chapter.
The balckboard receives information through the shared memory, but the axcess to the data is reguled and controlled by a semaphore.

The drone compute the position information with the second Newton Law considering as the sum of all the forces the value taken by the input, subtracting the friction forces and calculating the acceleration.  
So, with this parametr, aplliyng th Kinematics law for a rigid body, it computes the position of the robot in the arena. This position is given to the window process that draw the drone's position in the space.  
We need to specify that, even if the velocity is different from zero, if its module isn't > 5, the drone will not move. This is due to the fact that coordinates must be integer, while velocity and the other variables are floats (so it is because of an approximation). 

KEYS INSTRUCTIONS:
E: MOVE UP
C: MOVE DOWN 
S: MOVE LEFT 
F: MOVE RIGHT 
R: MOVE UP-RIGHT   
W: MOVE UP-LEFT
V: MOVE DOWN-RIGHT
X: MOVE DOWN-LEFT 
D: REMOVE ALL FORCES -> goes on by inertia
B: BRAKE -> generates force opposite to the velocity
U: RESET THE DRONE -> set the drone position to the starting values, and resets force amd velocity
Q: QUIT THE GAME -> terminate the program

In this video is showed the difference between the key D (the case in which the drone uses more time to stops) and B:

![emerency stop](https://github.com/leleviola/arp/blob/resources/resources/togli_acc_vs_frenata.gif)



---------------
## Installation  

To run this project is necessary to have installed on the computer this software:
* Konsole: application emulating KDE terminals, to run it copy that on the shell:
```bash
$ sudo apt install konsole
```
  * NCurses: C software library enabling the creation of graphical user interfaces. To install it:
```bash
$ sudo apt install libncurses-dev
``` 

To **run** this code you have to follow a few of steps:
* clone the repository
```bash
$ git clone https://github.com/leleviola/arp.git
```
* move inside drone_sim folder 
```bash
$ cd drone_sim/
```
* give the permession to the compiler
```bash
$ chmod +x compile.sh
```
* execute and run the code:
```bash
$ ./compile.sh
```
  then run the master process (./master)
## Usage

This project can be used to simulate the opeation of a drone only changing the values with the specific weight and strenght generated by the propellers in a planar motion. For now it is not yet a fully reliable simulator as it does not take into account many variables that may change over time(such as the wind or other external variables, but step by step, with new updates it will become increasingly reliable for the design of new drones

----------------
## Contributions

Until now this packege has been developed by:
* Project leaders: [Samuele Viola](https://github.com/leleviola) and [Fabio Guelfi](https://github.com/fabiogueunige).
* important collaborators:
  
There are several areas in which this project can be better. The most of them will be included with new fetures patches. Here a list of the most important ones:
- watchdog: implementing a watchdog that restore the process that falls saving in a file in the drone.c the last variables of the drone motion (position, velocity strength) and let the watchdog the axcess to this file to mark when the process falls.
- data exchange: change the data sharing from the shared memory to pipes that reads and writes on file descriptors. Useful because the shared memory is more easy to implement , but it is not good for the data safety and privacy
- Socket: to implement an open acess to the drone simulator from different point and machines
- Targets and obstacles: implement two processes that generate target and obstacle that the drone has to reach and avoid. It can be implemented also a scoreBoard.
- The drone gives problems when moving in coordinates that contains x = 0 or y = 0 (it stops if the velocity is too low).

## Licence

This is an open-source project, free for everyone.
Please commit your changes only after having completed the check of possible errors.


## Contacts

You can contact the project leaders throught their GitHub pages.

## References

For the Konsoles implementations it is mportant to know the ncurses libraries. [Here](https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/index.html) a web tutorial


