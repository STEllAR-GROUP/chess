CC=mpicc
CFLAGS=-Wall
OBJECTS=main.o board.o book.o data.o eval.o file.o search2.o mpi.o
mpichess: $(OBJECTS)
	$(CC) -g -o mpichess $(CFLAGS) $(OBJECTS)
clean:
	rm -f mpichess $(OBJECTS) rand loop chess*.log
cleanlog:
	rm -f rand loop chess*.log
