CC=gcc
CFLAGS=-Wall -Werror -std=c99 -c

all: jpeg

jpeg: jpgDriver.o jpgEncode.o
	$(CC) jpgDriver.o jpgEncode.o -o jpg -lm -pg -g

jpgDriver.o: jpgDriver.c
	$(CC) $(CFLAGS) jpgDriver.c -g

jpgEncode.o: jpgEncode.c
	$(CC) $(CFLAGS) jpgEncode.c -g

clean:
	rm -f *.o jpg
