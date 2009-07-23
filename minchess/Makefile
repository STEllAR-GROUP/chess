CC=gcc
CFLAGS=-Wall
OBJECTS=main.o board.o data.o eval.o search2.o
pxchess: $(OBJECTS)
	$(CC) -g -o pxchess $(CFLAGS) $(OBJECTS)
clean:
	rm -f pxchess pxchess.exe $(OBJECTS)
