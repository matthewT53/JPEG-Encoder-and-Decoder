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
	Pixel rgbBuf = imageToRGB("images/redFlowers.bmp", &size);
	encodeRGBToJpgDisk("results/new_hv.jpg", rgbBuf, size, 1920, 1080, 50, HORIZONTAL_VERTICAL_SUBSAMPLING);
	encodeRGBToJpgDisk("results/new_h.jpg", rgbBuf, size, 1920, 1080, 50, HORIZONTAL_SUBSAMPLING);
	encodeRGBToJpgDisk("results/new_no.jpg", rgbBuf, size, 1920, 1080, 50, NO_CHROMA_SUBSAMPLING);

	free(rgbBuf);
	return EXIT_SUCCESS;
}
