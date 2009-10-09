CC=gcc
CFLAGS=-Wall
OBJECTS=main.o board.o data.o eval.o
microkernel: $(OBJECTS)
	$(CC) -g -o microkernel $(CFLAGS) $(OBJECTS)
clean:
	rm -f microkernel $(OBJECTS)
