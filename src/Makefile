CC=gcc
CFLAGS=-Wall -Werror -std=c99 -c -g
LIBFLAGS=-lm -pg

all: jpeg

jpeg: jpgDriver.o jpgEncode.o block.o bitmap.o preprocess.o downsample.o dct.o
	$(CC) jpgEncode.o block.o bitmap.o preprocess.o downsample.o dct.o jpgDriver.o -o jpg $(LIBFLAGS)

jpgDriver.o: jpgDriver.c
	$(CC) $(CFLAGS) jpgDriver.c

jpgEncode.o: jpgEncode.c
	$(CC) $(CFLAGS) jpgEncode.c

block.o: block.c
	$(CC) $(CFLAGS) block.c

bitmap.o: bitmap.c
	$(CC) $(CFLAGS) bitmap.c

preprocess.o: preprocess.c
	$(CC) $(CFLAGS) preprocess.c

downsample.o: downsample.c
	$(CC) $(CFLAGS) downsample.c

dct.o: dct.c
	$(CC) $(CFLAGS) dct.c

clean:
	rm -f *.o jpg
