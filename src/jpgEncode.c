/*
	Name: Matthew Ta
	Date Created: 29/12/2015
	Date mod: 29/12/2015
	Description: Jpeg encoder
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "headers/jpgEncode.h"

// boolean constants
#define TRUE 1
#define FALSE 0

// DCT macros and constants
#define A(x) ((x == 0) ? sqrt(0.5) : 1 )
#define PI 3.14159265358979323846 // c99 standard dropped M_PI

// #define DEBUG // debugging constant
// #define DEBUG_PRE // debugging constant for the preprocessing code
// #define DEBUG_BLOCKS // debugging constant for the code that creates 8x8 blocks
#define DEBUG_DCT // debugging constant for the dct process

/*  
	To clear confusion, just in case.
	Width: X direction
	Height: Y direction
	Image is not down sampled for simplicity, will add this later

	Sources that assisted me:
	* https://en.wikipedia.org/wiki/JPEG
	* http://www.dfrws.org/2008/proceedings/p21-kornblum_pres.pdf -> Quantization quality scaling

	Algorithm:
	1 < Q < 100 ( quality range );
	s = (Q < 50) ? 5000/Q : 200 - 2Q;
	Ts[i] (element in new quan table) = ( S * Tb[i] ( element in original quan table ) + 50 ) / 100;
	
	Note: 
	* when Q = 50 there is no change
	* the higher q is, the larger the file size

	Things to do:
	* add down sampling
	* add support for quality scaling (user specifies a quality)
*/

typedef struct _jpegData{
	// YCbCr data
	char *Y; // luminance (black and white comp)
	char *Cb; // hue 
	char *Cr; // color saturation of the image
	
	// 8 x 8 blocks storage of each channel (YCbCr)
	char **YBlocks;
	char **CbBlocks;
	char **CrBlocks;
	
	// DCT / Quantization step
	char **quanY;
	char **quanCb;
	char **quanCr;
	int numBlocks; // counts the number of 8x8 blocks in the image
	
	// common image properties
	unsigned int width;
	unsigned int height;

	// new width and height that is divisible by 8
	unsigned int totalWidth;
	unsigned int totalHeight;
} jpegData;

/* ===================== JPEG ENCODING FUNCTIONS ===================*/
// JPEG PREPROCESSING FUNCTION PROTOTYPES
void preprocessJpg(JpgData jDat, Pixel p, unsigned int numPixels);
void convertRGBToYCbCr(JpgData jDat, Pixel p, unsigned int numPixels);
void form8by8blocks(JpgData jDat); // only need the YCbCr stuff
void levelShift(JpgData jDat); // subtract 128 from YCbCr channels to make DCT efficient

// DCT
/*
	* Applies discrete cosine transformation to the YCbCr channels (DCT-II)
	* Also applies the quantization step to the DCT blocks
	* Note only part of the image wil be stored in a DCT buffer (memory efficient)
*/
void dct(JpgData jDat); 

// Quantization
char **quantise(double **dct); // takes in one 8x8 DCT block and converts it to a quantised form

/*
	Input:
	* bn = block number 0 < bn <= numBlocks

	Output:
	* x[0] = startX, x[1] = endX
	* y[0] = startY, y[1] = endY

	Note: Don't forget to free() x and y

*/
void blockToCoords(JpgData jDat, int bn, int *x, int *y);




/* ================== END of Encoding functions ==================== */

/* ================== Additional utility functions ========================= */

// get the x and y index of a certain # block
void getBlockIndex();

// changing width and height is ok since we can later modify it again in the header of the image
void fillWidth(JpgData jDat, int nEdges); // extends the width of the image
void fillHeight(JpgData jDat, int nEdges); // extends the height of the image

// free resources
void disposeJpgData(JpgData jdat);

// simple utility and debugging functions
static Pixel newPixBuf(int n); // creates a new pixel buffer with n pixels
static int determineFileSize(FILE *f); // determines the size of a file

// debugging output functions
#ifdef DEBUG
static void dbg_out_file(Pixel p, int size); // dumps the contents of buf into a file 
#endif



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
	if (bytesRead <= 0) { printf("Error reading from file.\n"); }
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
		dct(jD); // DCT	

		// entropy coding

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
	convertRGBToYCbCr(jDat, rgb, numPixels); // change the colour space
	form8by8blocks(jDat); // split the image into 8x8 blocks
	//levelShift(jDat); // subtract 127 from each value
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

// convert the image into 8 by 8 blocks
void form8by8blocks(JpgData jDat)
{
	int fillWEdges = FALSE;
	int fillHEdges = FALSE;
	int extraSpaceW = 0, extraSpaceH = 0;
	int totalH = 0, totalW = 0;
	int i = 0, j = 0, k = 0;
	
	// printf("Not yet implemented 8x8 blocks\n");
	// test width and height for divisibility by 8
	if (jDat->width % 8 != 0){ fillWEdges = TRUE; }
	if (jDat->height % 8 != 0) { fillHEdges = TRUE; }
	
	// determine how much we need to fill
	if (fillWEdges){
		extraSpaceW = (8 - (jDat->width % 8)) * jDat->width; // # pixels in width * number of columns (add to right of iamge)
	}
	
	if (fillHEdges){
		extraSpaceH = (8 - (jDat->height % 8)) * jDat->height; // add to bottom
	}
	
	// calculate the total height and width of the image that is divisible by 8
	totalH = jDat->height + extraSpaceH;
	totalW = jDat->width + extraSpaceW;

	#ifdef DEBUG_BLOCKS
		printf("fillWEdge = %d and fillHEdge = %d\n", fillWEdges, fillHEdges);
		printf("totalW = %d and totalH = %d\n", totalW, totalH);
	#endif
	
	// get memory to store our blocks
	jDat->YBlocks = malloc(sizeof(char *) * (totalH));
	jDat->CbBlocks = malloc(sizeof(char *) * (totalH));
	jDat->CrBlocks = malloc(sizeof(char *) * (totalH));
	
	if (jDat->YBlocks != NULL && jDat->CbBlocks != NULL && jDat->CrBlocks != NULL){
		for (i = 0; i < jDat->height; i++){
			jDat->YBlocks[i] = calloc(totalW, sizeof(char));
			jDat->CbBlocks[i] = calloc(totalW, sizeof(char));
			jDat->CrBlocks[i] = calloc(totalW, sizeof(char));
			if (jDat->YBlocks[i] == NULL || jDat->CbBlocks[i] == NULL || jDat->CrBlocks[i] == NULL){
				exit(1); // unable to get enough storage space for the 8x8 blocks so terminate program
			}
		}
	}
	
	// fill the data in 8x8 blocks
	for (i = 0; i < jDat->height; i++){
		for (j = 0; j < jDat->width; j++){
			jDat->YBlocks[i][j] = jDat->Y[k];
			jDat->CbBlocks[i][j] = jDat->Cb[k];
			jDat->CrBlocks[i][j] = jDat->Cr[k];
			k++;
		}
	}

	// add the necessary edges to the image
	if (fillWEdges){
		fillWidth(jDat, extraSpaceW);
	}

	if (fillHEdges){
		fillHeight(jDat, extraSpaceH);
	}

	jDat->totalWidth = totalW;
	jDat->totalHeight = totalH;
	jDat->numBlocks = (totalW / 8) * (totalH / 8);

	#ifdef DEBUG_BLOCKS
		printf("Showing luminance first: \n");
		for (i = 0; i < totalH; i++){
			for (j = 0; j < totalW; j++){
				printf("%4d", jDat->YBlocks[i][j]);
			}
			printf("\n");
		}
		
		printf("\n\nNow showing Cb comp: \n");
		for (i = 0; i < totalH; i++){
			for (j = 0; j < totalW; j++){
				printf("%4d", jDat->CbBlocks[i][j]);
			}
			printf("\n");
		}

		printf("\n\nNow showing Cr comp: \n");
		for (i = 0; i < totalH; i++){
			for (j = 0; j < totalW; j++){
				printf("%4d", jDat->CrBlocks[i][j]);
			}
			printf("\n");
		}
	#endif
}

void fillWidth(JpgData jDat, int nEdges)
{
	//  extend the bottom width of the image
	int h = jDat->height;
	int i = 0;
	char *baseLineY = jDat->YBlocks[h - 1];
	char *baseLineCb = jDat->CbBlocks[h - 1];
	char *baseLineCr = jDat->CrBlocks[h - 1];	
	
	for (i = h; i < h + nEdges; i++){
		memcpy(jDat->YBlocks[i], baseLineY, jDat->width);
		memcpy(jDat->CbBlocks[i], baseLineCb, jDat->width);
		memcpy(jDat->CrBlocks[i], baseLineCr, jDat->width);
	} 
}

void fillHeight(JpgData jDat, int nEdges)
{
	// printf("Not yet implemented.\n");
	int w = jDat->width;
	int i = 0, j = 0;
	int baseIndexW = w - 1;

	for (i = w; i < w + nEdges; i++){
		for (j = 0; j < jDat->height; j++){
			jDat->YBlocks[j][i] = jDat->YBlocks[j][baseIndexW];
			jDat->CbBlocks[j][i] = jDat->CbBlocks[j][baseIndexW];
			jDat->CrBlocks[j][i] = jDat->CrBlocks[j][baseIndexW];
		}
	}
}

/*
// might not be required
void levelShift(JpgData jDat)
{
	int i = 0, j = 0;

	for (i = 0; i < jDat->totalHeight; i++){
		for (j = 0; j < jDat->totalWidth; j++){
			jDat->YBlocks[i][j] -= 128;
			jDat->CbBlocks[i][j] -= 128;
			jDat->CrBlocks[i][j] -= 128;
		}
	}
}
*/

// applies dicrete cosine transformation to the image
void dct(JpgData jDat)
{
	// printf("Will implement tomorrow.\n");
	int u = 0, v = 0;
	int i = 0, j = 0;

	int curBlock = 0;
	int startX = 0, endX = 0;
	int startY = 0, endY = 0;
	int *sX = NULL , *sY = NULL;

	// DCT coefficient
	double dctYCoef = 0.0;
	double dctCbCoef = 0.0;
	double dctCrCoef = 0.0;

	// create the dct 8x8 arrays
	double **dctY = NULL;
	double **dctCb = NULL;
	double **dctCr = NULL;

	dctY = malloc(sizeof(double *) * jDat->totalHeight);
	dctCb = malloc(sizeof(double *) * jDat->totalHeight);
	dctCr = malloc(sizeof(double *) * jDat->totalHeight);

	for (i = 0; i < jDat->totalHeight; i++){
		dctY[i] = calloc(jDat->totalWidth, sizeof(double));
		dctCb[i] = calloc(jDat->totalWidth, sizeof(double));
		dctCr[i] = calloc(jDat->totalWidth, sizeof(double));
	}

	// create the quantization arrays
	jDat->quanY = malloc(sizeof(char *) * jDat->totalHeight);
	jDat->quanCb = malloc(sizeof(char *) * jDat->totalHeight);
	jDat->quanCr = malloc(sizeof(char *) * jDat->totalHeight);

	for (i = 0; i < jDat->totalHeight; i++){
		jDat->quanY[i] = calloc(jDat->totalWidth, sizeof(char));
		jDat->quanCb[i] = calloc(jDat->totalWidth, sizeof(char));
		jDat->quanCr[i] = calloc(jDat->totalWidth, sizeof(char));
	}

	#ifdef DEBUG_DCT
		printf("numBlocks: %d\n", jDat->numBlocks);
		printf("TW = %d and TH = %d\n", jDat->totalWidth, jDat->totalHeight);
	#endif
	// DCT main process:
	for (curBlock = 1; curBlock <= jDat->numBlocks; curBlock++){ // apply it to every block in the image
		sX = calloc(2, sizeof(int));
		sY = calloc(2, sizeof(int));
		blockToCoords(jDat, curBlock, sX, sY); // this function is seg faulting
		startX = sX[0]; endX = sX[1]; // get the starting + ending x coordinates 
		startY = sY[0]; endY = sY[1]; // get the starting + ending y coordinates
		#ifdef DEBUG_DCT
			printf("Block num: %d\n", curBlock);
			printf("sX = %p and sY = %p\n", sX, sY);
			printf("sX = %d, eX = %d\n", sX[0], sX[1]);
			printf("sY = %d, eY = %d\n", sY[0], sY[1]);
		#endif
		for (u = 0; u < 8; u++){ // calculate DCT for G(u,v)
			for (v = 0; v < 8; v++){
				dctYCoef = 0.0;
				dctCbCoef = 0.0;
				dctCrCoef = 0.0;
				for (i = startX; i < endX; i++){
					for (j = startY; j < endY; j++){
						dctYCoef += jDat->YBlocks[j][i] * cos(((2 * (i % 8) + 1) * u * PI) / 16) * cos(((2 * (j % 8) + 1) * v * PI) / 16); // i, j can only take values from 0 <= i, j < 8
						dctCbCoef += jDat->CbBlocks[j][i] * cos(((2 * (i % 8) + 1) * u * PI) / 16) * cos(((2 * (j % 8) + 1) * v * PI) / 16);
						dctCrCoef += jDat->CrBlocks[j][i] * cos(((2 * (i % 8) + 1) * u * PI) / 16) * cos(((2 * (j % 8) + 1) * v * PI) / 16);
					}
				}

				// finalise the dct coefficient
				dctYCoef = (0.25) * A(u) * A(v) * dctYCoef;
				dctCbCoef = (0.25) * A(u) * A(v) * dctCbCoef;
				dctCrCoef = (0.25) * A(u) * A(v) * dctCrCoef;

				#ifdef DEBUG_DCT
					printf("%6.2f ", dctYCoef);
				#endif

				// write the coefficient to the dct block
				dctY[v][u] = dctYCoef;
				dctCb[v][u] = dctCbCoef;
				dctCr[v][u] = dctCrCoef;
			}

			#ifdef DEBUG_DCT
				printf("\n");
			#endif
		}

		#ifdef DEBUG_DCT
			printf("\n\n");
		#endif

		// quantise the 8x8 block
		

		// free the coordinates array
		free(sX);
		free(sY);
		sX = NULL;
		sY= NULL;
	}	
}

// converts block number to starting and ending coordinates (x,y)
void blockToCoords(JpgData jDat, int bn, int *x, int *y)
{
	// unsigned int h = jDat->totalHeight;
	unsigned int w = jDat->totalWidth;
	
	unsigned int tw = 8 * (unsigned int) bn;
	
	// don't forget to check that bn < numBlocks

	// set the starting and ending y values
	y[0] = (int) (tw / w) * 8;
	y[1] = y[0] + 8; // seg fault here
	if (tw % w == 0) { y[0] -= 8; y[1] -= 8; } // reached the last block of a row
	// set the starting and ending x values
	x[0] = (tw % w) - 8;
	if (x[0] == -8) { x[0] = w - 8; } // in last column, maths doesn't make sense
	x[1] = x[0] + 8;
}


// free resources
void disposeJpgData(JpgData jdata)
{
	int i = 0;
	// free the YCbCr data
	free(jdata->Y);
	free(jdata->Cb);
	free(jdata->Cr);
	
	// free the 8x8 blocks
	for (i = 0; i < jdata->height; i++){
		free(jdata->YBlocks[i]);
		free(jdata->CbBlocks[i]);
		free(jdata->CrBlocks[i]);
	}
	
	free(jdata->YBlocks);
	free(jdata->CbBlocks);
	free(jdata->CrBlocks);
	// free the whole struct
	free(jdata);
}

// static functions
#ifdef DEBUG
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
#endif

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

