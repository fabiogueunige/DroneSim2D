# Compila il programma C
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

cc -o "drone" "drone.c"
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

cc -o "input" "input.c" "-lncurses"

if [ $? -eq 0 ]; then
        echo "Compilazione di input.c completata con successo"
    else
        echo "Errore durante la compilazione di input.c"
    fi

cc -o "window" "window.c" "-lncurses"
if [ $? -eq 0 ]; then
        echo "Compilazione di window.c completata con successo"
    else
        echo "Errore durante la compilazione di window.c"
    fi



