/*
	Name: Matthew Ta
	Date: 30/12/2015
	Description: Driver for the JPEG encoder and decoder
*/

#include <stdio.h>
#include <stdlib.h>

#include "headers/jpg_encode.h"
#include "headers/bitmap.h"
#include "headers/block.h"
#include "headers/dct.h"
#include "headers/quantise.h"
#include "headers/zig_zag.h"

void test_bitmap(void);
void test_jpeg(void);
void test_dct(void);

int main(void)
{
	// test_bitmap();
	// test_jpeg();
	test_dct();
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
	encode_bmp_to_jpeg("images/redFlowers.bmp", "output/new.jpg", 50, NO_CHROMA_SUBSAMPLING);
}

void test_dct(void)
{
	Block b = new_block();
	int i = 0;

	// compare with the one in Wikipedia
	set_value_block(b, 0, 0, -76);
	set_value_block(b, 1, 0, -73);
	set_value_block(b, 2, 0, -67);
	set_value_block(b, 3, 0, -62);
	set_value_block(b, 4, 0, -58);
	set_value_block(b, 5, 0, -67);
	set_value_block(b, 6, 0, -64);
	set_value_block(b, 7, 0, -55);

	set_value_block(b, 0, 1, -65);
	set_value_block(b, 1, 1, -69);
	set_value_block(b, 2, 1, -73);
	set_value_block(b, 3, 1, -38);
	set_value_block(b, 4, 1, -19);
	set_value_block(b, 5, 1, -43);
	set_value_block(b, 6, 1, -59);
	set_value_block(b, 7, 1, -56);

	set_value_block(b, 0, 2, -66);
	set_value_block(b, 1, 2, -69);
	set_value_block(b, 2, 2, -60);
	set_value_block(b, 3, 2, -15);
	set_value_block(b, 4, 2, 16);
	set_value_block(b, 5, 2, -24);
	set_value_block(b, 6, 2, -62);
	set_value_block(b, 7, 2, -55);

	set_value_block(b, 0, 3, -65);
	set_value_block(b, 1, 3, -70);
	set_value_block(b, 2, 3, -57);
	set_value_block(b, 3, 3, -6);
	set_value_block(b, 4, 3, 26);
	set_value_block(b, 5, 3, -22);
	set_value_block(b, 6, 3, -58);
	set_value_block(b, 7, 3, -59);

	set_value_block(b, 0, 4, -61);
	set_value_block(b, 1, 4, -67);
	set_value_block(b, 2, 4, -60);
	set_value_block(b, 3, 4, -24);
	set_value_block(b, 4, 4, -2);
	set_value_block(b, 5, 4, -40);
	set_value_block(b, 6, 4, -60);
	set_value_block(b, 7, 4, -58);

	set_value_block(b, 0, 5, -49);
	set_value_block(b, 1, 5, -63);
	set_value_block(b, 2, 5, -68);
	set_value_block(b, 3, 5, -58);
	set_value_block(b, 4, 5, -51);
	set_value_block(b, 5, 5, -60);
	set_value_block(b, 6, 5, -70);
	set_value_block(b, 7, 5, -53);

	set_value_block(b, 0, 6, -43);
	set_value_block(b, 1, 6, -57);
	set_value_block(b, 2, 6, -64);
	set_value_block(b, 3, 6, -69);
	set_value_block(b, 4, 6, -73);
	set_value_block(b, 5, 6, -67);
	set_value_block(b, 6, 6, -63);
	set_value_block(b, 7, 6, -45);

	set_value_block(b, 0, 7, -41);
	set_value_block(b, 1, 7, -49);
	set_value_block(b, 2, 7, -59);
	set_value_block(b, 3, 7, -60);
	set_value_block(b, 4, 7, -63);
	set_value_block(b, 5, 7, -52);
	set_value_block(b, 6, 7, -50);
	set_value_block(b, 7, 7, -34);

	show_block(b);

	// test discrete cosine transformation
	dct_block(b);
	show_block(b);

	// test quantization
	quantise_lum(b);
	show_block(b);

	// test zig-zag ordering
	int a[64] = {0};
	zig_zag_block(b, a);

	printf("Testing zig-zag ordering: \n");
	for (i = 0; i < 64; i++){
		printf("%d\n", a[i]);
	}
}
