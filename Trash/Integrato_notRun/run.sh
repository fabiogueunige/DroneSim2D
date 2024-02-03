rm -f ./master ./drone ./input ./server ./wd ./inputcou ./description ./window

# compile library
cd library/
gcc -c win.c 
# gcc -shared -o  libwindow.so window.o
cd ..
# Compile the main files
gcc -c description.c 
gcc -c inputcou.c
# gcc -c map.c

# compile the master
gcc master.c -o master

# compile the server
gcc server.c -o server

# compile the drone
gcc drone.c -o drone

# compile the input
gcc -c input.c -o input

# compile the watchdog
gcc wd.c -o wd

# compile the window
gcc window.c -o window -lncurses

# Compile the descriptions files

gcc -o description description.o library/win.o -lncurses
gcc -o inputcou inputcou.o library/win.o -lncurses
#gcc -o map map.o library/window.o -lncurses

# Run the master
#./master
