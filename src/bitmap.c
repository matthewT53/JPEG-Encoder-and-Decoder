/*
	This file contains implementations for the functions inside bitmap.h

	Written by: Matthew Ta
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/bitmap.h"

#define BMP_MAX_LEN 500

/*
	Helper functions
*/

// stores the RGB values in seperate channels
void bmp_GetColourData(BmpImage b);

// determines the size of a file
int determineFileSize(FILE *f);

typedef struct _bitmap {
	char filename[BMP_MAX_LEN]; // name of the bitmap file
	int fileSize;			// size of the bitmap file
	int offsetRGB; 			// offset to the RGB data
	int width;              // width of the image
	int height;             // height of the image
	short bitDepth;	        // bit depth of the image
	int numPixels; 			// number of pixels
	int error; 		        // error code associated with reading the file

	// data for each of the colour channels
	Byte *red;
	Byte *green;
	Byte *blue;
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
			memset(b->filename, 0, BMP_MAX_LEN);
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

				b->numPixels = b->width * b->height;

				bmp_GetColourData(b);
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

void bmp_GetColourData(BmpImage b)
{
	FILE *fp = NULL;
	Byte *buffer = NULL;
	int n = 0;
	int i = 0, j = 0; // index for the pixel array
	int fs = 0, offset = 0;
	int numPixels = 0;

	numPixels = b->numPixels;
	fs 		  = b->fileSize;
	buffer    = malloc(sizeof(Byte) * fs);

	// allocate memory for each of the colour channels
	b->red   = malloc(sizeof(Byte) * numPixels);
	b->green = malloc(sizeof(Byte) * numPixels);
	b->blue  = malloc(sizeof(Byte) * numPixels);


	if (buffer != NULL && b->red != NULL && b->green != NULL && b->blue != NULL){
		fp = fopen(b->filename, "rb");

		if (fp != NULL){
			// read the file into a buffer
			fread(buffer, sizeof(Byte), fs, fp);

			// store the pixel data RGB
			for (i = 1; i <= b->height; i++){
				offset = fs - (i * b->width * (b->bitDepth / 8));
				for (j = 0; j < (b->width * 3); j += 3){ // sometimes the ordering of the rgb values is different
					b->red[n]   = buffer[offset + j];     // r
				 	b->green[n] = buffer[offset + j + 1]; // g
					b->blue[n]  = buffer[offset + j + 2]; // b
					n++;
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
}

Byte *bmp_GetRed(BmpImage b)
{
	return b->red;
}

Byte *bmp_GetGreen(BmpImage b)
{
	return b->green;
}

Byte *bmp_GetBlue(BmpImage b)
{
	return b->blue;
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

int bmp_GetNumPixels(BmpImage b)
{
	return b->numPixels;
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

		case BMP_FAILED_ALLOCATE_BUFFER:
			printf("Failed to allocate a buffer.\n");
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

void bmp_DestroyBitmap(BmpImage b)
{
	int i = 0;

	for (i = 0; i < BMP_MAX_LEN; i++){
		b->filename[i] = 0;
	}

	free(b->red);
	free(b->green);
	free(b->blue);
	free(b);
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
