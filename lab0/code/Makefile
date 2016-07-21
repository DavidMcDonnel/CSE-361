all:  lab0

clean:
	rm -f lab0 *.o

input.o:  input.c
	gcc -g -c input.c -o input.o

lab0.o:  lab0.c input.h
	gcc -g -c lab0.c -o lab0.o

lab0:  lab0.o input.o
	gcc -g lab0.o input.o -o lab0
