CC = gcc

all: seaShell.c 
	gcc -Wall seaShell.c -o seaShell

clean: 
	$(RM) seaShell