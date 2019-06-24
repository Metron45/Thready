# Thready
Simple multithreading example project using ncurses and pthreads libraries. 
## Used Tools
I used Visual Studio: Code aslongside GCC compiler. I specifically wanted as simple tools as possible. VS:Code was meant to be solely alternative for text editor. I eneded up not requiring help of GDB during this project. 
## Running
To build the program simply run the:
```
make
```
command to start the makefile to build the program. Currently makefile itself includes solely:
```
gcc main.c -pthread -lncurses -o main
```
To run the program run the main file with command:
```
./main {amount of ball threads} {max amount of active threads} {amount of obstacles}
```
In case of any parameter missing it will use default values of 6,3,4. 