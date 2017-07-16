/*
	This file contains implementations for the functions inside bitmap.h

	Written by: Matthew Ta
*/

#include <stdio.h>

#include "bitmap.h"

#define MAX_LEN 500

/*
	Helper functions
*/
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
	Byte *buffer = NULL;
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

		close(fp);
	}

	return b;
}

Byte *bmp_GetColourData(BmpImage b, int *size)
{
	FILE *fp = NULL;
	Byte *buffer = NULL;
	
	
}

void bmp_ShowBmpInfo(BmpImage b)
{
	printf("[BMP Info]:\n");
	printf("Filename: %s\n", b->filename);
	printf("File size: %d\n", b->fileSize);
	printf("Height: %d\n", b->height);
	printf("Width: %d\n", b->width);
	printf("Bit depth: %d\n", b->bitDepth);
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


