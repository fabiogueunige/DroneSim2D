# Compila il programma C
cd library/
gcc -c win.c 
# gcc -shared -o  libwindow.so window.o
cd ..
# Compile the main files
gcc -c description.c 

cc  -o "master" "master.c"
if [ $? -eq 0 ]; then
        echo "Compilazione di master.c completata con successo"
    else
        echo "Errore durante la compilazione di master.c"
    fi

cc -o "server"  "server.c"
if [ $? -eq 0 ]; then
        echo "Compilazione di server.c completata con successo"
    else
        echo "Errore durante la compilazione di server.c"
    fi

cc -o "drone" "drone.c" -lm
if [ $? -eq 0 ]; then
        echo "Compilazione di drone.c completata con successo"
    else
        echo "Errore durante la compilazione di drone.c"
    fi

cc -o "wd" "wd.c"
if [ $? -eq 0 ]; then
        echo "Compilazione di wd.c completata con successo"
    else
        echo "Errore durante la compilazione di wd.c"
    fi


gcc -o description description.o library/win.o -lncurses

cc -o "input" "input.c" "-lncurses"

if [ $? -eq 0 ]; then
        echo "Compilazione di input.c completata con successo"
    else
        echo "Errore durante la compilazione di input.c"
    fi

cc -o "window" "window.c" "-lncurses" -lm
if [ $? -eq 0 ]; then
        echo "Compilazione di window.c completata con successo"
    else
        echo "Errore durante la compilazione di window.c"
    fi

cc -o "obstacles" "obstacles.c" 
if [ $? -eq 0 ]; then
        echo "Compilazione di obstacles.c completata con successo"
    else
        echo "Errore durante la compilazione di obstacles.c"
    fi
    
cc -o "targets" "targets.c" 
if [ $? -eq 0 ]; then
        echo "Compilazione di targets.c completata con successo"
    else
        echo "Errore durante la compilazione di targets.c"
    fi

export QT_QPA_PLATFORM=wayland
./master
