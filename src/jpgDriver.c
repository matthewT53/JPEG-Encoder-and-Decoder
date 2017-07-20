/*
	Name: Matthew Ta
	Date: 30/12/2015
	Description: Driver for the JPEG encoder and decoder
*/

#include <stdio.h>
#include <stdlib.h>

// #include "include/jpgEncode.h"
#include "bitmap.h"

int main(void)
{
	int i = 0;
	int size = 0;
	Byte *data = NULL;
	
	BmpImage b = bmp_OpenBitmap("images/redFlowers.bmp");
	bmp_ShowBmpInfo(b);
	bmp_GetLastError(b);
	data = bmp_GetColourData(b, &size);
	
	for (i = 0; i < size; i++){
		printf("%d\n", data[i]); 
	}

	bmp_GetLastError(b);
	free(data);

	return EXIT_SUCCESS;
}
