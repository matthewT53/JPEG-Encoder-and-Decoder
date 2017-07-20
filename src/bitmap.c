/*
	This file contains implementations for the functions inside bitmap.h

	Written by: Matthew Ta
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitmap.h"

#define MAX_LEN 500

/*
	Helper functions
*/

// determines the size of a file
int determineFileSize(FILE *f);

typedef struct _bitmap {
	char filename[MAX_LEN]; // name of the bitmap file
	int fileSize;			// size of the bitmap file
	int offsetRGB; 			// offset to the RGB data
	int width;              // width of the image
	int height;             // height of the image
	short bitDepth;	        // bit depth of the image
	int error; 		        // error code associated with reading the file
} Bitmap;

BmpImage bmp_OpenBitmap(const char *filename)
{
	FILE *fp = NULL;
	BmpImage b = NULL;
	Byte *buffer = NULL, *data = NULL;
	int fs = 0; // filesize local

	// create the bitmap structure
	b = malloc(sizeof(Bitmap));
	b->error = BMP_SUCCESS;

	if (b != NULL){
		// open the bitmap
		fp = fopen(filename, "rb");

		// check if we got a handle to a file
		if (fp != NULL){
			fs = determineFileSize(fp);
			buffer = malloc(sizeof(Byte) * (fs + 1));

			b->fileSize = fs;
			memset(b->filename, 0, MAX_LEN);
			strncpy(b->filename, filename, strlen(filename));

			// check if the buffer for reading has been created properly
			if (buffer != NULL){
				fread(buffer, sizeof(Byte), fs, fp);

				// read in bmp header info
				data = buffer + 10; // offset to RGB info
				memcpy(&b->offsetRGB, data, sizeof(int));

				data = buffer + 18; // offset to width
				memcpy(&b->width, data, sizeof(int));

				data = buffer + 22; // offset to height
				memcpy(&b->height, data, sizeof(int));

				data = buffer + 28; // # bits / pixel
				memcpy(&b->bitDepth, data, sizeof(short));
			}

			else{
				b->error = BMP_READ_FAILED;
			}
		}

		else{
			b->error = BMP_FILE_DOESNT_EXIST;
		}

		fclose(fp);
	}

	return b;
}

Byte *bmp_GetColourData(BmpImage b, int *size)
{
	FILE *fp = NULL;
	Byte *buffer = NULL;
	Byte *pixels = NULL;
	int n = 0;
	int i = 0, j = 0; // index for the pixel array
	int fs = 0, offset = 0;
	int numPixels = 0;

	numPixels = b->width * b->height * 3;
	fs = b->fileSize;
	buffer = malloc(sizeof(Byte) * fs);
	pixels = malloc(sizeof(Byte) * numPixels);

	*size = numPixels;

	if (buffer != NULL && pixels != NULL){
		fp = fopen(b->filename, "rb");

		if (fp != NULL){
			// read the file into a buffer
			fread(buffer, sizeof(Byte), fs, fp);

			// store the pixel data RGB
			for (i = 1; i <= b->height; i++){
				offset = fs - (i * b->width * (b->bitDepth / 8));
				for (j = 0; j < (b->width * 3); j += 3){ // sometimes the ordering of the rgb values is different
					pixels[n++] = buffer[offset + j];     // r
					pixels[n++] = buffer[offset + j + 1]; // g
					pixels[n++] = buffer[offset + j + 2]; // b
				}
			}
		}

		else{
			b->error = BMP_READ_FAILED;
		}

		fclose(fp);
	}

	else{
		b->error = BMP_FAILED_ALLOCATE_BUFFER;
	}

	free(buffer);

	return pixels;
}

void bmp_ShowBmpInfo(BmpImage b)
{
	printf("[BMP Info]:\n");
	printf("Filename: %s\n", b->filename);
	printf("File size: %d kb\n", b->fileSize / 1000);
	printf("Height: %d\n", b->height);
	printf("Width: %d\n", b->width);
	printf("Bit depth: %d\n", b->bitDepth);
}

int bmp_GetWidth(BmpImage b)
{
	return b->width;
}

int bmp_GetHeight(BmpImage b)
{
	return b->height;
}

void bmp_GetLastError(BmpImage b)
{
	switch (b->error){
		case BMP_FILE_DOESNT_EXIST:
			printf("%s doesn't exist\n", b->filename);
			break;

		case BMP_READ_FAILED:
			printf("Reading BMP image failed\n");
			break;

		default:
			printf("No error.\n");
			break;
	}
}

int bmp_GetFileSize(BmpImage b)
{
	return b->fileSize;
}

// helper function that determines the size of a file
int determineFileSize(FILE *f)
{
    int end;

    fseek (f, 0, SEEK_END);
    end = ftell (f);
    fseek (f, 0, SEEK_SET);

    return end;
}
