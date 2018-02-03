/*
	Name: Matthew Ta
	Date: 30/12/2015
	Description: Driver for the JPEG encoder and decoder
*/

#include <stdio.h>
#include <stdlib.h>

#include "headers/jpgEncode.h"
#include "headers/bitmap.h"

void test_bitmap(void);
void test_jpeg(void);

int main(void)
{
	// test_bitmap();
	test_jpeg();
}

void test_bitmap(void)
{
	int i = 0;
	Byte *r = NULL, *b = NULL, *g = NULL;

	BmpImage bmp = bmp_OpenBitmap("images/redFlowers.bmp");
	bmp_ShowBmpInfo(bmp);
	bmp_GetLastError(bmp);

	r = bmp_GetRed(bmp);
	b = bmp_GetBlue(bmp);
	g = bmp_GetGreen(bmp);

	for (i = 0; i < bmp_GetNumPixels(bmp); i++){
		printf("[i] = %d, [R]: %d, [G]: %d [B]: %d\n", i, r[i], g[i], b[i]);
	}

	bmp_GetLastError(bmp);
	bmp_DestroyBitmap(bmp);
}

void test_jpeg(void)
{
	encode_bmp_to_jpeg("images/redFlowers.bmp", "output/new.jpg", 50, HORIZONTAL_VERTICAL_SUBSAMPLING);
}
