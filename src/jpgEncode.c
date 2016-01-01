/*
	Name: Matthew Ta
	Date Created: 29/12/2015
	Date mod: 29/12/2015
	Description: Jpeg encoder
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "headers/jpgEncode.h"

// boolean constants
#define TRUE 1
#define FALSE 0

#define DEBUG // debugging constant
#define DEBUG_PRE // debugging constant for the preprocessing code

typedef struct _jpegData{
	// YCbCr data
	char *Y; // luminance (black and white comp)
	char *Cb; // 
	char *Cr;
	
	// common image properties
	unsigned int width;
	unsigned int height;


} jpegData;

/* ===================== JPEG ENCODING FUNCTIONS ===================*/
// JPEG PREPROCESSING FUNCTION PROTOTYPES
void preprocessJpg(JpgData jDat, Pixel p, unsigned int numPixels);
void convertRGBToYCbCr(JpgData jDat, Pixel p, unsigned int numPixels);
void form8by8blocks(JpgData jDat); // only need the YCbCr stuff

// free resources
void disposeJpgData(JpgData jdat);

// simple utility and debugging functions
static Pixel newPixBuf(int n); // creates a new pixel buffer with n pixels
static int determineFileSize(FILE *f); // determines the size of a file

// debugging output functions
static void dbg_out_file(Pixel p, int size); // dumps the contents of buf into a file 

// can easily be adapted to other image formats other than BMP - mainly used for testing
Pixel imageToRGB(const char *imageName, int *bufSize)
{
	Pixel pixBuf = NULL;
	int imageSize = 0, bytesRead = 0;
	FILE *fp = fopen(imageName, "rb");
	// unsigned char *pBuf = NULL; // store the pixel values (temp)
	// int i = 0, j = 0;
	int numberPixels = 0;
	
	if (fp == NULL){
		printf("Error opening image file.\n");
		exit(1); // terminate the process
	}

	imageSize = determineFileSize(fp) - 54; // det the size that our pixel buffer should be
	assert(imageSize > 0); // make sure the image size is > 0
	numberPixels = ceil(imageSize / 3); // # pixels required
	pixBuf = newPixBuf(numberPixels); // create an empty pixel buffer
	fseek(fp, 54, SEEK_SET); // seek 54 bytes from start of file
	// pBuf = malloc(sizeof(unsigned char) * imageSize); // extract raw pixel data (bytes)
	bytesRead = fread(pixBuf, sizeof(Colour), imageSize, fp); // read the RGB values into pixBuf
	// is the code above dangerous???
	*bufSize = numberPixels; // set the size of the buffer

	// O(n) copy since there is padding in the Pixel DAT struct
	/*for (i = 0; j < imageSize; i++){
		pixBuf[i].r = pBuf[j++]; // set the red
		pixBuf[i].g = pBuf[j++]; // set the green
		pixBuf[i].b = pBuf[j++]; // j % 3 == 2 // set the blue
	}*/
	
	#ifdef DEBUG
		dbg_out_file(pixBuf, numberPixels);
		printf("Bytes read: %d\n", bytesRead);
		printf("Sizeof(pixBuf): %d\n", imageSize);
		printf("# pixels: %d\n", numberPixels);
	#endif
	fclose(fp);
	return pixBuf;
}

// main encoding process
void encodeRGBToJpgDisk(const char *jpgFile, Pixel rgbBuffer, unsigned int numPixels, unsigned int width, unsigned int height)
{
	JpgData jD = malloc(sizeof(jpegData)); // create object to hold info about the jpeg

	jD->width = width;
	jD->height = height;
	
	if (jD != NULL){
		preprocessJpg(jD, rgbBuffer, numPixels); // preprocess the image data
		// DCT
		
		// Quantization

		// Huffman coding

		// write binary contents to disk
	}
	
	else{
		printf("Unable to allocate memory to store jpeg data.\n");
	}
	
}

/* ==================== JPG PREPROCESSING ======================*/
void preprocessJpg(JpgData jDat, Pixel rgb, unsigned int numPixels)
{
	convertRGBToYCbCr(jDat, rgb, numPixels);
	form8by8blocks(jDat);
	
	
}

void convertRGBToYCbCr(JpgData jDat, Pixel rgb, unsigned int numPixels)
{
	int i = 0;
	// printf("Not yet implemented conversion space.\n");
	jDat->Y = malloc(sizeof(char) * numPixels);
	jDat->Cb = malloc(sizeof(char) * numPixels);
	jDat->Cr = malloc(sizeof(char) * numPixels);

	if (jDat->Y == NULL || jDat->Cb == NULL || jDat->Cr == NULL){
		printf("Error allocating space for the jpeg data.\n");
		exit(1); // return some error
	}

	// do the conversion, RGB -> YCbCr
	for (i = 0; i < numPixels; i++){
		jDat->Y[i] = (0.299 * rgb[i].r) + (0.587 * rgb[i].g) + (0.114 * rgb[i].b); // should these arrays be char or double
		jDat->Cb[i] = 128 - (0.168736 * rgb[i].r) - (0.331264 * rgb[i].g) + (0.5 * rgb[i].b);
		jDat->Cr[i] = 128 + (0.5 * rgb[i].r) - (0.418688 * rgb[i].g) - (0.081312 * rgb[i].b);
	}

	#ifdef DEBUG_PRE
		printf("Printing YCbCr info: \n");
		for (i = 0; i < numPixels; i++){
			printf("[Pixel: %d]\n", i);
			printf("Y: %d\n", jDat->Y[i]);
			printf("Cb: %d\n", jDat->Cb[i]);
			printf("Cr: %d\n", jDat->Cr[i]);
		}
	#endif

}

// onvert the image into 8 by 8 blocks
void form8by8blocks(JpgData jDat)
{/*
	int fillWEdge = FALSE;
	int fillHEdge = FALSE;
	// test  width and height for divisibility by 8
	if (jDat->width % 8 != 0){ fillWEdge = TRUE; }
	if (jDat->height % 8 != 0) { fillHEdge = TRUE; }
	*/
	printf("Not yet implemented 8x8 blocks\n");
		


}

// free resources
void disposeJpgData(JpgData jdata)
{
	free(jdata->Y);
	free(jdata->Cb);
	free(jdata->Cr);
	free(jdata);
}

// static functions

static void dbg_out_file(Pixel p, int size)
{
	int i = 0;
	for (i = 0; i < size; i++){
		printf("[Pixel: %d]\n", i);
		printf("R: %d\n", p[i].r);
		printf("G: %d\n", p[i].g);
		printf("B: %d\n", p[i].b);
	}
}

static Pixel newPixBuf(int n)
{
	Pixel pBuf = malloc(sizeof(pixel) * n);
	return pBuf;
}

static int determineFileSize(FILE *f)
{
    int pos;
    int end;

    pos = ftell (f);
    fseek (f, 0, SEEK_END);
    end = ftell (f);
    fseek (f, pos, SEEK_SET);

    return end;
}

