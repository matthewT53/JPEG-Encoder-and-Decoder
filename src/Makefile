CC=gcc
CFLAGS=-Wall -Werror -std=c99 -c -g
LIBFLAGS=-lm -pg

all: jpeg

jpeg: jpg_driver.o jpg_encode.o block.o bitmap.o preprocess.o downsample.o dct.o quantise.o zig_zag.o
	$(CC) jpg_encode.o block.o bitmap.o preprocess.o downsample.o dct.o jpg_driver.o quantise.o zig_zag.o -o jpg $(LIBFLAGS)

jpg_driver.o: jpg_driver.c
	$(CC) $(CFLAGS) jpg_driver.c

jpg_encode.o: jpg_encode.c
	$(CC) $(CFLAGS) jpg_encode.c

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

quantise.o: quantise.c
	$(CC) $(CFLAGS) quantise.c

zig_zag.o: zig_zag.c
	$(CC) $(CFLAGS) zig_zag.c

clean:
	rm -f *.o jpg
