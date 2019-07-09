CC=g++
CFLAGS=-I.

monitor: main.o parse.o myTimer.o
	$(CC) -o monitor main.o parse.o myTimer.o -lpcap -lpthread

main.o: main.c global.h networkHeader.h
	$(CC) -c main.c

parse.o: parse.c global.h
	$(CC) -c parse.c

myTimer.o: myTimer.c global.h
	$(CC) -c myTimer.c

clean:
	rm monitor main.o parse.o myTimer.o