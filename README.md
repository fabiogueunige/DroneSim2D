## ADVACNCE ROBOT PROGRAMMING
----------------------------

# DRONE OPERATIONE INTERACTIVE SIMULATOR

This project consits in a drone operation interactive simulator.  
The drone is driven by the user and moves in an empty arena through the user's keyboard commands. 

## Description

This is the first project for the assignment of Advance Robotic Programming course of Robotics Engeneerig Master degree in Università degli studi di Genova.  
It consists in a 2Dimensional drone movement simulator with the goal to consider all the forces that act on the rigid body during the motion in the real world, such as the friction force.  
This simulation environment implents sevreal different processes to manage all the possible application or complication that can happen during a drone flight. 
All the  processes exchange data through shared memeory or pipes between them and all of them are chldren of the master process.  There are also two process that impements a friendly user interface.  

## Repository Architecture

This github package contains **drone_sim** folder that is the main folder. Insided that there are:  
* build: Contains the CMake file and the file list
* library: Contains all the libraies created by the developers for the progrms implementation 
* logfiles: contains all the file opened during the software implementation to check the application state
* All processes.c


-----------------------
## Project Architecture

This program is composed by several different process:
* master: the main process and the father of all the other process. It `fork()` to generate the new child process and through an `exevp()` it overwrites the child with a new computation.
* [description:](https://github.com/leleviola/arp/blob/master/drone_sim/description.c) Introductivewindow to show all the commnd to use for the input to the user.
* [server:](https://github.com/leleviola/arp/blob/master/drone_sim/server.c) the blackboard  of the project, implemented through a shared memory with the drone that compute all the user's inputs
* * [window:](https://github.com/leleviola/arp/blob/master/drone_sim/window.c) window that show the geometrical position of the drone in the arena and the position's numeeric values.
* [drone:](https://github.com/leleviola/arp/blob/master/drone_sim/drone.c) the computation side of the project, it takes the input, process it with the optimal exstimation of the drone position in the real world and share the data with the server.
* [input:](https://github.com/leleviola/arp/blob/master/drone_sim/input.c)  Takes the user's char input. It is important to notice that the user graphic interface is a child of the input. That's needed to have a $modular$ project. In the future it will be easier (if needed) to cange the graphic interface without changing any main process of the package.
* * [inputc:](https://github.com/leleviola/arp/blob/master/drone_sim/inputcou.c) the user graphic interface to take to user's inut only from the keyboard and only from the showed keys.
* [watchdog:](https://github.com/leleviola/arp/blob/master/drone_sim/wd.c)  the process controller. Through the signal and the signal handler functions of the process it checks if all the process are still alive. If not, it ends the program.
  
AGGIUNGI FOTO FATTA BENE

## Main features
The program starts with an introduction page to show all the commands to the user, and all the other processes initialize only when the description child of the master process has terminated through a `wait(NULL);`.  
It is important to remeber that tuìhe user iterface are conneced to the parent process through unnamed pipes.

---------------
## Installation  

AGGIUNGI TUTTA ROBA DA INSTALLARE konsole etc e link a file che descrive comeinstallare

To **run** this code you have to follow a few of steps:
* clone the repository
  ```bash
  $ git clone https://github.com/leleviola/arp.git
  ```
* move inside drone_sim folder 
  ```bash
  $ cd drone_sim/
  ```
* execute and run the code:
  ```bash
  $ ./compile.sh
  ```
  
## Usage
/*Spiega come utilizzare il progetto una volta installato. Fornisci esempi pratici, comandi da eseguire o istruzioni dettagliate su come sfruttare le funzionalità principali del progetto. Puoi includere anche esempi di codice, screenshot o diagrammi che illustrano l'utilizzo pratico del progetto.*/

----------------
## Contributions

Until now this packege has been developed by:
* Project leaders: [Samuele Viola](https://github.com/leleviola) and [Fabio Guelfi](https://github.com/fabiogueunige).
* important collaborators:
  
There are several areas in which this project can be better. The most of them will be included with new fetures patches. Here a list of the most important ones:
- watchdog: implementing a watchdog that restore the process that falls saving in a file in the drone.c the last variables of he drone motion (position, velocity strength) and let the watchdog the axcess to this file to mark when the process falls.
- data exchange: change the data sharing from the shared memory to pipes that rads and writes on file descriptors. Useful because the shared memory is more easy to implement , but it is not good for the data safety and privacy
- Socket: to implement an open acess to the drone simulator from different point and machines
- Targets and obstacles: implement two processes that generate target and obstacle that the drone has to reach and avoid. It can be implemented also a scoreBoard. 

## Licence

This is an open-source project, free for everyone.
Please commit your changes only after having completed the check of possible errors.


## Contacts

You can contact the project leaders throught their GitHub pages.

## References

For the Konsoles implementations it is mportant to know the ncurses libraries. [Here](METTI LINK) a web tutorial

DENTRO ALTRO README (SOLO SE VOGLIAMO) CHE SPEGA I VARI PROCESI LATO IMPLEMNTAZIONE
