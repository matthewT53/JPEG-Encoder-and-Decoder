/*
	Name: Matthew Ta
	Date: 30/12/2015
	Description: Driver for the JPEG encoder and decoder
*/

#include <stdio.h>
#include <stdlib.h>

#include "include/jpgEncode.h"

int main(void)
{
	int size = 0;
	Pixel rgbBuf = imageToRGB("images/cam.bmp", &size);
	encodeRGBToJpgDisk("results/new.jpg", rgbBuf, size, 64, 64, 50, HORIZONTAL_VERTICAL_SUBSAMPLING);

	free(rgbBuf);
	return EXIT_SUCCESS;
}
