CC=gcc
CFLAGS=-Wall
OBJECTS=main.o board.o data.o eval.o search2.o
pxchess: $(OBJECTS)
	$(CC) -g -o minchess $(CFLAGS) $(OBJECTS)
clean:
	rm -f minchess minchess.exe $(OBJECTS)
openmp:
	icc -Wall -openmp -g -o minchess main.c board.c data.c eval.c search2.c
