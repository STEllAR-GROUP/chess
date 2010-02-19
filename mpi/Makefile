CC=mpicc
CFLAGS=-Wall
OBJECTS=main.o board.o data.o eval.o search2.o
pxchess: $(OBJECTS)
	$(CC) -g -o mpichess $(CFLAGS) $(OBJECTS)
clean:
	rm -f mpichess mpichess.exe $(OBJECTS)
