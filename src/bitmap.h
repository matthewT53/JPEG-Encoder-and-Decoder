/*
	This file contains functions for working with Bitmap images.

	This was created specifically to work with my JPEG encoder + decoder but can be used
	in your own projects if you like.

	Written by: Matthew Ta
*/

#ifndef BITMAP_H
#define BITMAP_H

// error codes
#define BMP_SUCCESS 0
#define BMP_FILE_DOESNT_EXIST 1
#define BMP_READ_FAILED 2

typedef struct _bitmap *BmpImage;
typedef struct _pixel *ColourData;
typedef unsigned char Byte;

/*
	Opens an existing bitmap image file and returns a handle to it.

	Takes in the path to the file. (This path should include the filename)

	If NULL is returned then there wasn't enough memory to allocate for the BmpImage
*/
BmpImage openBitmap(const char *file);

/*
	Retrives the RGB values from the BMP image

	A buffer that contains RGB values is returned, make sure to free this buffer when you are done.

	The order in the buffer is: R,G,B,R,G,B,R,G,B....R,G,B

	A pointer to an integer that contains the size of the buffer is passed back to the caller
	
*/
Byte *getColourData(BmpImage b, int *numPixels);

/*
	Dumps information about the BMP file
*/
void showBmpInfo(BmpImage b);

/*
	Determines the file size of the Bitmap image
*/
int getFileSize(BmpImage b);

/*
	Frees all memory used by the bitmap image
*/
void destroyBitmap(BmpImage b);

#endif
