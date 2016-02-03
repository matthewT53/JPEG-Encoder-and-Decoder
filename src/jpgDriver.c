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
	Pixel rgbBuf = imageToRGB("tiger.bmp", &size);
	encodeRGBToJpgDisk("new.jpg", rgbBuf, size, 320, 240, 50);

	free(rgbBuf);

	return EXIT_SUCCESS;
}
