CC = gcc
CFLAGS = -O2 -Wall -I .

all:	eps

eps: eps.c
	$(CC) $(CFLAGS) -o eps eps.c -lpcap

clean:
	rm -f *.o eps *~

