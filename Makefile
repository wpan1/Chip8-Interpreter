CC	= g++
CFLAGS 	= -Wall  
GLUT = -lglut -lGLU -lGL

## OBJ = Object files.
## SRC = Source files.
## EXE = Executable name.

## server
SRC =	chip8-MAIN.c chip8-CPU.c
OBJ =	chip8-MAIN.o chip8-CPU.o
EXE =	chip8

## Make
all:$(OBJ)
		$(CC) $(CFLAGS) -o $(EXE) $(OBJ) $(GLUT)

## Clean: Remove object files and core dump files.
clean:
		/bin/rm $(OBJ)
		/bin/rm $(OBJ) 

## Clobber: Performs Clean and removes executable file.
clobber: clean
		/bin/rm $(EXE) 
		/bin/rm $(EXE)
