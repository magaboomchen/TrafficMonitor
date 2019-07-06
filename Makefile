CC=g++
CFLAGS=-I.

main: main.o
	$(CC) -o main main.o

clean:
	rm main main.o
