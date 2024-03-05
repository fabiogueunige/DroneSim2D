## ADVANCED AND ROBOT PROGRAMMING
---------------------------------
### Creotors:
Guelfi Fabio 

Samuele Viola

Group name: PurpleGuelphs

---------------------------------
  

# DRONE OPERATION INTERACTIVE SIMULATOR

This project consits in a drone operation interactive simulator.  In the flight area, the user will encounter obstacles and targets that will make the environment more similar to the real one, where for example trees or other obstructions may be encountered or a precise position may have to be reached
The drone is driven by the user through the user's keyboard commands. 
The movement of the drone is subject to the effect of air friction, while braking is subject to load transfer and the efficiency of the braking system

## Description

This is the third project for the assignment of Advanced and Robot Programming course of Robotics Engeneerig Master degree in Università degli studi di Genova.  
It consists in a 2Dimensional drone motion simulator with the goal of considering all the forces that act on the rigid body during the motion and the breaking in the real world, such as the friction force.  
This simulation environment implents several different processes to manage all the possible application or complication that can happen during a drone flight. 

All the processes are chldren of the master process and the exchange of the datas is developed by a socket to have the connection on web and pipes for the locals connection.
There is also the Watchdog process that check if all the other local processes are alive.


## Repository Architecture

This github package contains **drone_sim** folder that is the main folder. Inside that there are:  
* logfiles: contains all the file opened during the software implementation to check the application state
* All processes.c
* the compiler compile.sh.
The package conteains also **Docs** folder where there all the documents needed to understand the implementation.

-----------------------
## Project Architecture

This program is composed by several different process:
* master: the main process and the father of all the other process. It `fork()` to generate the new child process and through an `exevp()` it overwrites the child with a new computation. It sends all the id-process to the watchdog and all the needed pipes to every process.
* [server:](https://github.com/leleviola/arp/blob/main/drone_sim/server.c) the blackboard  of the project, it manage all the information that arrives from the other processes to send them to the other procesesse that need them.
* [sockserver:](https://github.com/leleviola/arp/blob/main/drone_sim/sockserver.c) the child of the server genterated to implement the socket communication for more than one client (one for each client)
* [window:](https://github.com/leleviola/arp/blob/main/drone_sim/window.c) window that show the geometrical position of the drone in the arena and the position's numeeric values.
* [drone:](https://github.com/leleviola/arp/blob/main/drone_sim/drone.c) the computation side of the project, it takes the input, process it with the optimal exstimation of the drone position in the real world and share the data with the server.
* [input:](https://github.com/leleviola/arp/blob/main/drone_sim/input.c)  Takes the user's char input. It is important to allows the user to give the data input
* [obstacles:](https://github.com/leleviola/arp/blob/main/drone_sim/obstacles.c) the obstacles process.This is connect through the net. It generates, every 60 seconds, a random number of obstacles that will be inside the environment. Every obstacle generates a repulsive force everytime the drone will be near enough
* [targets:](https://github.com/leleviola/arp/blob/main/drone_sim/targets.c) the targets process. This is connect through the net. It generates, every time the user reach all the targets, a random number of targets that will be inside the environment. Every target generates an attractive force on the drone
* [watchdog:](https://github.com/leleviola/arp/blob/main/drone_sim/wd.c)  the process controller. Through the signal and the signal handler functions of the process it checks if all the local process are still alive. If not, it ends the program.
  
![Project Architecture](https://github.com/leleviola/arp/blob/main/Docs/1709068157920.png)
  
## Main features
The program starts with an introduction page to show all the commands to the user, and all the other processes initialize only when the description child of the master process has terminated through a `wait(NULL);`.  
The initial choice of the user will be if to use the local implementation or to connect through a TCP as the server (to give the input and move the drone) or as the target and obstacle generator. 
This several process are important to build a modular project and gives different tasks to the different pojects, as explained in the previouse chapter.
All processes manage the reading of the requested information sent in writing by the other processes. The server is the main information management node, receiving the obstacles, targets and position of the drone. It is only through the use of **select** and type identification codes that the server is able to read what is being read to the other nodes.
In every process has been implemented a select to manage the order between the reading of the pipes or the sockets

## Important topic to know before to use the simulator
### Description
KEYS INSTRUCTIONS:
```
-------------------------
|Q | W | E | R |... | U |
-------------------------
   | S | D | F |
   -----------------
   | X | C | V | B |
   -----------------
```

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

### Drone dynamics
The drone compute the position information with the second Newton Law considering as the sum of all the forces the value taken by the input, subtracting the friction forces and calculating the acceleration.  
So, with this parameter, appliyng the Kinematics law for a rigid body, it computes the position of the robot in the arena. This position is given to the window process that draw the drone's position in the space.  
We need to specify that, even if the velocity is different from zero, if its module isn't > 5, the drone will not move. This is due to the fact that coordinates must be integer, while velocity and the other variables are floats (so it is because of an approximation). 
When the drone is close to obstacle, they apply to it a repulsive force, modeled following the Lathombe-Kathib model. This for each obstacle in the window, including the edges, that are considered as obstacles. When the drone exits from the window (due to bug or other causes) it respawn automatically to the spawn point, which is set exatly at the centre of the window.
For better reaching target, a small attractive force has been setted, which is active when the drone is close to it.

In this video is showed an exemple of drone movement in the environment
![movimento del drone](https://github.com/leleviola/arp/blob/resources/resources/Video-del-15-01-2024-23_29_18.gif)

## Strong points
The points of strength of this simulator are:
* drone dynamics that mirror real physics in movement for a joystick-piloted drone
* speed of response and fluidity of movement to user input
* implementation of the socket
* stability of the system
* stability of the socket

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
* give the permession to the compiler and the folder cleaner
```bash
$ chmod +x *.sh
$ cd bash/
$ chmod +x *.sh
$ cd ..
```
* On the server computer take the IP address
```bash
$ hostname -I
```
* To run the target and the obstacle process:
* * Open the master.c file and put the IP address of the server computer in the variable `ipAddress`
* * Save the file

* You may change the port in the master process 

* execute and run the code:
```bash
$ ./compile.sh
```


## Usage

This project can be used to simulate the opeation of a drone only changing the values with the specific weight and strenght generated by the propellers in a planar motion. For now it is not yet a fully reliable simulator as it does not take into account many variables that may change over time (such as the wind or other external variables), but step by step, with new updates, it will become increasingly reliable for the design of new drones.

----------------
## Contributions

Until now this packege has been developed by:
* Project leaders: [Samuele Viola](https://github.com/leleviola) and [Fabio Guelfi](https://github.com/fabiogueunige).
* important collaborators:
  
There are several areas in which this project can be better. The most of them will be included with new fetures patches. Here a list of the most important ones:
- watchdog: implementing a watchdog that restore the process that falls saving in a file in the drone.c the last variables of the drone motion (position, velocity strength) and let the watchdog the axcess to this file to mark when the process falls.
- Socket: to implement a socket to send the input keyboard from the user to make only the input and the map seeble user side
- The drone gives problems when moving in coordinates that contains x = 0 or y = 0 (it stops if the velocity is too low).
- The user interface of input can be upgrade to a younger version 

## Licence

This is an open-source project, free for everyone.
Please commit your changes only after having completed the check of possible errors.

## Contacts

You can contact the project leaders throught their GitHub pages.

## References

For the Konsoles implementations it is mportant to know the ncurses libraries. [Here](https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/index.html) a web tutorial

## Warning and known error cases
**warning:** 
- After the window opens return to the terminal (press it) because, if not, the program will not take into account the keyboard iput.
- Rarely may happen that the drone escapes to the edges -> In this case it will be reset to the centre of the window

**problems:** When the parameters for the socket connection are incorrectly set, the program will not close, becouse it will wait the socket connection.
To close it the only way is to interrupt the processes with ctrl+c and close the terminal window.
Now (after the change of the connection setting) is possible to re-run the code.
