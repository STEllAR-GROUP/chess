CC=gcc
CFLAGS=-Wall
OBJECTS=main.o board.o data.o eval.o search2.o board_file.o
pxchess: $(OBJECTS)
	$(CC) -g -o minchess $(CFLAGS) $(OBJECTS)
clean:
	rm -f minchess minchess.exe $(OBJECTS)
