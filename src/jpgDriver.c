/*
	Name: Matthew Ta
	Date: 30/12/2015
	Description: Driver for the jpg encoder and decoder
*/

#include <stdio.h>
#include <stdlib.h>

#include "include/jpgEncode.h"

int main(void)
{
	int size = 0;
	Pixel rgbBuf = imageToRGB("test.bmp", &size);
	encodeRGBToJpgDisk("new.jpg", rgbBuf, size, 32, 32, 50, HORIZONTAL_VERTICAL_SUBSAMPLING);

	free(rgbBuf);
	return EXIT_SUCCESS;
}
