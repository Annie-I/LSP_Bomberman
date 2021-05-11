CC=clang
CFLAGS=-Wall -g

all: program rename clean

program: client.o helper.o
	$(CC) $(CFLAGS) -lm client.o helper.o

client.o: client.c
	$(CC) $(CFLAGS) -c client.c


helper.o: ../functions/helper.c
	$(CC) $(CFLAGS) -c ../functions/helper.c

rename:
	mv a.out client

clean:
	rm *.o

