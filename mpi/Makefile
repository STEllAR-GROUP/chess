CC=gcc
CFLAGS=-Wall
OBJECTS=main.o board.o book.o data.o eval.o file.o search2.o network.o
chess: $(OBJECTS)
	$(CC) -g -o chess $(CFLAGS) $(OBJECTS)
clean:
	rm -f chess $(OBJECTS) rand loop.txt chess*.log
cleanlog:
	rm -f rand loop.txt chess*.log
