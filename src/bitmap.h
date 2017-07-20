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
#define BMP_FAILED_ALLOCATE_BUFFER 3

typedef struct _bitmap *BmpImage;
typedef unsigned char Byte;

/*
	Opens an existing bitmap image file and returns a handle to it.

	Takes in the path to the file. (This path should include the filename)

	If NULL is returned then there wasn't enough memory to allocate for the BmpImage
*/
BmpImage bmp_OpenBitmap(const char *file);

/*
	Retrives the RGB values from the BMP image

	A buffer that contains RGB values is returned, make sure to free this buffer when you are done.

	The order in the buffer is: R,G,B,R,G,B,R,G,B....R,G,B

	A pointer to an integer that contains the size of the buffer is passed back to the caller
	
*/
Byte *bmp_GetColourData(BmpImage b, int *numPixels);

/*
	Returns the width of the image
*/
int bmp_GetWidth(BmpImage b);

/*
	Returns the height of the image
*/
int bmp_GetHeight(BmpImage b);

/*
	Dumps information about the BMP file
*/
void bmp_ShowBmpInfo(BmpImage b);

/*
	Determines the file size of the Bitmap image
*/
int bmp_GetFileSize(BmpImage b);

/*
	Displays the last error that occured
*/
void bmp_GetLastError(BmpImage b);

/*
	Frees all memory used by the bitmap image
*/
void bmp_DestroyBitmap(BmpImage b);

#endif
