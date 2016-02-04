/*
	Name: Matthew Ta
	Date Created: 29/12/2015
	Date mod: 29/12/2015
	Description: Jpeg encoder based on the JFIF standard
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>

#include "include/jpgEncode.h"
#include "include/tables.h"

// boolean constants
#define TRUE 1
#define FALSE 0

// DCT macros and constants
#define A(x) ((x == 0) ? sqrt(0.5) : 1 )
#define PI 3.14159265358979323846 // c99 standard dropped M_PI

// quan matrix constants
#define BLOCK_SIZE 8
#define QUAN_MAT_SIZE 8
#define DEFAULT_QUALITY 50

// zig-zag stuff
#define NUM_COEFFICIENTS 64

// run length encoding constants
#define MAX_ZEROES 16
#define ZRL_VALUE 15

// #define DEBUG // debugging constant
// #define DEBUG_PRE // debugging constant for the preprocessing code
// #define DEBUG_BLOCKS // debugging constant for the code that creates 8x8 blocks
// #define DEBUG_DCT // debugging constant for the dct process
// #define DEBUG_QUAN // debugging constant for the quan process
#define DEBUG_ZZ // debugging constant for zig-zag process
#define DEBUG_DPCM // debugging constant for DPCM process
#define DEBUG_RUN // debugging constant for run length coding

// #define LEVEL_SHIFT

/*  
	To clear confusion, just in case.
	Width: X direction
	Height: Y direction
	Image is not down sampled for simplicity, will add this later

	Sources that assisted me:
	* https://en.wikipedia.org/wiki/JPEG
	* https://www.w3.org/Graphics/JPEG/itu-t81.pdf -> SPEC which is very convoluted
	* https://www.cs.auckland.ac.nz/courses/compsci708s1c/lectures/jpeg_mpeg/jpeg.html
	* http://www.dfrws.org/2008/proceedings/p21-kornblum_pres.pdf -> Quantization quality scaling
	* http://www.dmi.unict.it/~battiato/EI_MOBILE0708/JPEG%20%28Bruna%29.pdf
	* http://cnx.org/contents/eMc9DKWD@5/JPEG-Entropy-Coding -> Zig-zag: "Borrowed" the order matrix
	* http://www.pcs-ip.eu/index.php/main/edu/7 -> zig-zag ordering + entropy encoding 
	* http://users.ece.utexas.edu/~ryerraballi/MSB/pdfs/M4L1.pdf -> DPCM and Run length encoding
	* http://www.impulseadventure.com/photo/jpeg-huffman-coding.html -> Huffman encoding
	* http://www.cs.cf.ac.uk/Dave/MM/BSC_MM_CALLER/PDF/10_CM0340_JPEG.pdf -> good overview of jpeg
	* http://www.ctralie.com/PrincetonUGRAD/Projects/JPEG/jpeg.pdf -> mainly for decoding but also helps with encoding
	* https://www.opennet.ru/docs/formats/jpeg.txt -> good overview

	Algorithm: Quantization quality factor
	1 < Q < 100 ( quality range );
	s = (Q < 50) ? 5000/Q : 200 - 2Q;
	Ts[i] (element in new quan table) = ( S * Tb[i] ( element in original quan table ) + 50 ) / 100;
	
	Note: 
	* when Q = 50 there is no change
	* the higher q is, the larger the file size

	Things to do:
	* add down sampling (might need to), current setting is 4:4:4
	* store the quantisation table inside jDat
	* instead of freeing everything at once, free resources when we no longer require them
	
*/

// run length encoding as shown on wikipedia
typedef struct _Symbol{
	unsigned char s1; // Symbol 1: runlength (4 bits) | size (4 bits)
	int s2; // synbol 2: amplitude
} Symbol;

// represent huffman encoded bitstream for each DC and AC coefficient
typedef struct _HuffSymbol{
	int nBits; // number of bits to write to the file
	uint32_t bits; // huffman encoded bits
} HuffSymbol;

typedef struct _jpegData{
	// YCbCr data
	char *Y; // luminance (black and white comp)
	char *Cb; // hue: Cb and Cr represent the chrominance
	char *Cr; // color saturation of the image
	
	// 8 x 8 blocks storage of each channel (YCbCr)
	char **YBlocks;
	char **CbBlocks;
	char **CrBlocks;
	
	// DCT / Quantization step
	int **quanY;
	int **quanCb;
	int **quanCr;
	int quality; // 1 < q < 100, default = 50
	int numBlocks; // counts the number of 8x8 blocks in the image

	// zig-zag ordering stuff
	int **zzY; // rows = # blocks, cols = # coefficients which is 64
	int **zzCb;
	int **zzCr;

	// encoding stuff
	Symbol **encodeY;
	Symbol **encodeCb;
	Symbol **encodeCr;

	HuffSymbol **huffmanY;
	HuffSymbol **huffmanCb;
	HuffSymbol **huffmanCr;
	
	// common image properties
	unsigned int width;
	unsigned int height;

	// new width and height that is divisible by 8
	unsigned int totalWidth;
	unsigned int totalHeight;
} jpegData;

// Coordinate structure used to predefine the order in which to process the quan coefficients in zig-zag()
typedef struct _coordinate{
	char x; // use char instead of int since we dont need huge storage
	char y;
} Coordinate;

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
/*
	Input: 
	* an 8x8 size matrix with DC and AC DCT coefs
	* jDat = JpgData structure contain the properties for the jpeg image
	* sx = startX for the quan matrices in the jDat
	* sy = same as for sx except in the y direction
	
	Output: Modified the jDat quan matrices
*/
void quantise(JpgData jDat, double **dctY, double **dctCb, double **dctCr, int sx, int sy); // takes in one 8x8 DCT block and converts it to a quantised form

// Zig-Zag ordering
/*
	Maps 8x8 blocks into 1x64 vectors
	Input: 
	* jDat containing quantised blocks

	Output:
	* adds to the JpgData struct the zig-zag ordering of coeffcients from each quan block
*/
void zigZag(JpgData jDat);

// Encoding functions
/*  DCPM
	Encodes the difference between DC values from current block and previous block
	Input: jDat structure
	Output: Updates the zig-zag vectors in the struct
*/
void dpcm(JpgData jDat); // encodes the DC values (upper-left values in the table)

/*
	Run Length Coding
	Compresses the AC values in the zig-zag vector. This is applied to the 1x63 vector.
*/
void runLength(JpgData jDat); // encodes the AC values in the block (the other values)

/*
	Huffman coding - compresses the remaining data

	Uses the example huffman tables presented in the jpeg spec.
	
*/
void huffmanEncoding(JpgData jDat);

/*
	Input:
	* bn = block number 0 < bn <= numBlocks

	Output:
	* x[0] = startX, x[1] = endX
	* y[0] = startY, y[1] = endY

	Note: Don't forget to free() x and y

*/
void blockToCoords(JpgData jDat, int bn, int *x, int *y); // more of a utility function, but i think its a critical part of the code
/* ================== END of Encoding functions ==================== */

/* ================== Utility functions ========================= */

// changing width and height is ok since we can later modify it again in the header of the image
void fillWidth(JpgData jDat, int nEdges); // extends the width of the image
void fillHeight(JpgData jDat, int nEdges); // extends the height of the image

void loadCoordinates(Coordinate *c); // loads co-ordinates that indicate the order in which the quan coefficients should be analysed
void sanitise(Symbol *s); // removes random ZRL and places the EOB in the correct position
int numOfBits(int x); // determines the # bits required to represent a number x
char *intToHuffBits(int x); // convert an integer into huffman bits

// utility functions for the huffman encoding process
void DCHuffmanEncodeLum(Symbol encodedDC, HuffSymbol *block);
void ACHuffmanEncodeLum(Symbol *encodedBlock, HuffSymbol *block);
void DCHuffmanEncodeChrom(Symbol encodedDC, HuffSymbol *block);
void ACHuffmanEncodeChrom(Symbol *encodedBlock, HuffSymbol *block);
void huffmanEncodeValue(HuffSymbol *block, int value, int bitPos);

// free resources
void disposeJpgData(JpgData jdat);

// utility functions for freeing resources
void dispose_YCbCr(JpgData jDat); // frees the YCbCr arrays
void dispose_blocks(JpgData jDat); // frees the 8x8 blocks
void dispose_quan(JpgData jDat); // frees the quantiation matices for each component
void dispose_zz(JpgData jDat); // frees the zig-zag vectors for each component
void dispose_encode(JpgData jDat); // frees the encoded data for each component

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
void encodeRGBToJpgDisk(const char *jpgFile, Pixel rgbBuffer, unsigned int numPixels, unsigned int width, unsigned int height, int quality)
{
	JpgData jD = malloc(sizeof(jpegData)); // create object to hold info about the jpeg

	jD->width = width;
	jD->height = height;
	jD->quality = quality;
	
	if (jD != NULL){
		// preprocess the image data
		preprocessJpg(jD, rgbBuffer, numPixels);

		// DCT and quantization
		dct(jD);

		// zig-zag ordering: Convert 8x8 block into 1x64 vector
		zigZag(jD);

		// DPCM on DC coefficients
		dpcm(jD);

		// run length coding on AC coefficients
		runLength(jD);

		// huffman (entropy) coding
		huffmanEncoding(jD);

		// write binary contents to disk

		disposeJpgData(jD);
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
	#ifdef LEVEL_SHIFT
	levelShift(jDat); // subtract 127 from each value
	#endif
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

#ifdef LEVEL_SHIFT
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

#endif

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

	dctY = malloc(sizeof(double *) * BLOCK_SIZE); // OPT: Don't need to dynamically allocate, just create an 8x8 array on the stack
	dctCb = malloc(sizeof(double *) * BLOCK_SIZE);
	dctCr = malloc(sizeof(double *) * BLOCK_SIZE);

	for (i = 0; i < BLOCK_SIZE; i++){
		dctY[i] = calloc(BLOCK_SIZE, sizeof(double));
		dctCb[i] = calloc(BLOCK_SIZE, sizeof(double));
		dctCr[i] = calloc(BLOCK_SIZE, sizeof(double));
	}

	// create the quantization arrays
	jDat->quanY = malloc(sizeof(int *) * jDat->totalHeight);
	jDat->quanCb = malloc(sizeof(int *) * jDat->totalHeight);
	jDat->quanCr = malloc(sizeof(int *) * jDat->totalHeight);

	for (i = 0; i < jDat->totalHeight; i++){
		jDat->quanY[i] = calloc(jDat->totalWidth, sizeof(int));
		jDat->quanCb[i] = calloc(jDat->totalWidth, sizeof(int));
		jDat->quanCr[i] = calloc(jDat->totalWidth, sizeof(int));
	}

	#ifdef DEBUG_DCT
		printf("numBlocks: %d\n", jDat->numBlocks);
		printf("TW = %d and TH = %d\n", jDat->totalWidth, jDat->totalHeight);
	#endif
	// DCT main process: O(n^4) for each 8x8 block
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
		for (u = 0; u < BLOCK_SIZE; u++){ // calculate DCT for G(u,v)
			for (v = 0; v < BLOCK_SIZE; v++){
				dctYCoef = 0.0;
				dctCbCoef = 0.0;
				dctCrCoef = 0.0;
				for (i = startX; i < endX; i++){
					for (j = startY; j < endY; j++){
						dctYCoef += jDat->YBlocks[j][i] * cos(((2 * (i % 8) + 1) * u * PI) / 16) * cos(((2 * (j % 8) + 1) * v * PI) / 16); // i, j can only take values from 0 <= i,j < 8
						dctCbCoef += jDat->CbBlocks[j][i] * cos(((2 * (i % 8) + 1) * u * PI) / 16) * cos(((2 * (j % 8) + 1) * v * PI) / 16);
						dctCrCoef += jDat->CrBlocks[j][i] * cos(((2 * (i % 8) + 1) * u * PI) / 16) * cos(((2 * (j % 8) + 1) * v * PI) / 16);
					}
				}

				// finalise the dct coefficient
				dctYCoef = (0.25) * A(u) * A(v) * dctYCoef;
				dctCbCoef = (0.25) * A(u) * A(v) * dctCbCoef;
				dctCrCoef = (0.25) * A(u) * A(v) * dctCrCoef;

				// write the coefficient to the dct block
				dctY[v][u] = dctYCoef;
				dctCb[v][u] = dctCbCoef;
				dctCr[v][u] = dctCrCoef;
			}
		}

		#ifdef DEBUG_DCT
			// print the DCT matrix
			for (i = 0; i < 8; i++){
				for (j = 0; j < 8; j++){
					printf("%8.2f ", dctY[i][j]);
				}
				printf("\n");
			}
			printf("\n");
		#endif

		// quantise the 8x8 block
		quantise(jDat, dctY, dctCb, dctCr, startX, startY);
		// free the coordinates array
		free(sX);
		free(sY);
		sX = NULL;
		sY= NULL;
	}	

	// free resources
	for (i = 0; i < BLOCK_SIZE; i++){
		free(dctY[i]);
		free(dctCb[i]);
		free(dctCr[i]);
	}

	free(dctY);
	free(dctCb);
	free(dctCr);
}

// quantises the image
void quantise(JpgData jDat, double **dctY, double **dctCb, double **dctCr, int sx, int sy) // may have to modify for down sampling
{
	int q = jDat->quality;
	int s = 0;
	int i = 0, j = 0, m = 0, n = 0;
	int qMatY[QUAN_MAT_SIZE][QUAN_MAT_SIZE] = {0};
	int qMatCbCr[QUAN_MAT_SIZE][QUAN_MAT_SIZE] = {0};

	// check if quality has the correct value
	if (q < 1 || q > 100){
		q = jDat->quality = DEFAULT_QUALITY; // default quality = 50
	}

	s = ((q < DEFAULT_QUALITY) ? (5000 / q) : (200 - (2*q))); // get the quality scaling factor
	
	// change the quan matrix based on the scaling factor
	for (i = 0; i < BLOCK_SIZE; i++){
		for (j = 0; j < BLOCK_SIZE; j++){
			qMatY[i][j] = ((s*quanMatrixLum[i][j] + 50) / 100);
			qMatCbCr[i][j] = ((s*quanMatrixChr[i][j] + 50) / 100);
		}
	}
	
	// quantise the matrices
	for (i = 0, m = sy; i < BLOCK_SIZE; i++, m++){
		for (j = 0, n = sx; j < BLOCK_SIZE; j++, n++){
			jDat->quanY[m][n] = (int) round(dctY[i][j] / qMatY[i][j]);
			jDat->quanCb[m][n] = (int) round(dctCb[i][j] / qMatCbCr[i][j]);
			jDat->quanCr[m][n] = (int) round(dctCr[i][j] / qMatCbCr[i][j]);
			#ifdef DEBUG_QUAN
				printf("%4d ", jDat->quanY[m][n]);
			#endif
		}
		#ifdef DEBUG_QUAN
			printf("\n");
		#endif
	}	

	#ifdef DEBUG_QUAN
		printf("\n\n");
	#endif
}

// re-orders the coefficients of the quantized matrices
void zigZag(JpgData jDat)
{
	int i = 0, j = 0;
	int *sX = NULL, *sY = NULL;
	int startX = 0, startY = 0;
	int cX = 0, cY = 0;
	int curBlock = 0;

	Coordinate *c = malloc(sizeof(Coordinate) * NUM_COEFFICIENTS); // same here don't need to dynamically allocate

	// get space to hold the starting coordinates for each block
	sX = malloc(sizeof(int) * 2); // don't ned to dynamically allocate since the size is known
	sY = malloc(sizeof(int) * 2);

	jDat->zzY = malloc(sizeof(int *) * jDat->numBlocks);
	jDat->zzCb = malloc(sizeof(int *) * jDat->numBlocks);
	jDat->zzCr = malloc(sizeof(int *) * jDat->numBlocks);

	for (i = 0; i < jDat->numBlocks; i++){
		jDat->zzY[i] = calloc(NUM_COEFFICIENTS, sizeof(int));
		jDat->zzCb[i] = calloc(NUM_COEFFICIENTS, sizeof(int));
		jDat->zzCr[i] = calloc(NUM_COEFFICIENTS, sizeof(int));
	} 

	assert(sX != NULL && sY != NULL && c != NULL);
	loadCoordinates(c);
	// process earch block from the quantized matrices
	for (curBlock = 1; curBlock <= jDat->numBlocks; curBlock++){
		blockToCoords(jDat, curBlock, sX, sY);
		startX = sX[0];
		startY = sY[0];
		for (j = 0; j < NUM_COEFFICIENTS; j++){
			cX = c[j].x;
			cY = c[j].y;
			jDat->zzY[curBlock - 1][j] = jDat->quanY[startY+ cY][startX + cX];
			jDat->zzCb[curBlock - 1][j] = jDat->quanCb[startY + cY][startX + cX];
			jDat->zzCr[curBlock - 1][j] = jDat->quanCr[startY + cY][startX + cX];	
		}
	}

	#ifdef DEBUG_ZZ
		printf("dbg: zig-zag: \n");
		for (i = 0; i < jDat->numBlocks; i++){
			printf("Block num: %d.\n", i + 1);
			for (j = 0; j < NUM_COEFFICIENTS; j++){
				printf("%d ", jDat->zzY[i][j]);
			}

			printf("\n");
		}
	#endif

	free(c);
	free(sX);
	free(sY);
}

// Maps a 8x8 block into a 1x64 vector
void dpcm(JpgData jDat)
{
	int curBlock = 0;
	int i = 0;

	// allocate memory to store the encoded data
	jDat->encodeY = malloc(sizeof(Symbol *) * jDat->numBlocks);
	jDat->encodeCb = malloc(sizeof(Symbol *) * jDat->numBlocks);
	jDat->encodeCr = malloc(sizeof(Symbol *) * jDat->numBlocks);

	assert(jDat->encodeY != NULL && jDat->encodeCb != NULL && jDat->encodeCr != NULL);
	
	for (i = 0; i < jDat->numBlocks; i++){
		jDat->encodeY[i] = calloc(NUM_COEFFICIENTS, sizeof(Symbol));
		jDat->encodeCb[i] = calloc(NUM_COEFFICIENTS, sizeof(Symbol));
		jDat->encodeCr[i] = calloc(NUM_COEFFICIENTS, sizeof(Symbol));
		assert(jDat->encodeY[i] != NULL && jDat->encodeCb[i] != NULL && jDat->encodeCr[i] != NULL);
	}

	// search through all the blocks
	jDat->encodeY[0][0].s2 = jDat->zzY[0][0];
	jDat->encodeCb[0][0].s2 = jDat->zzCb[0][0];
	jDat->encodeCr[0][0].s2 = jDat->zzCr[0][0];
	for (curBlock = 1; curBlock < jDat->numBlocks; curBlock++){ // iterate in reverse to avoid incorrectly altering results of elements ahead
		jDat->encodeY[curBlock][0].s2 = jDat->zzY[curBlock][0] - jDat->zzY[curBlock - 1][0];
		jDat->encodeCb[curBlock][0].s2 = jDat->zzCb[curBlock][0] - jDat->zzCb[curBlock - 1][0];
		jDat->encodeCr[curBlock][0].s2 = jDat->zzCr[curBlock][0] - jDat->zzCr[curBlock - 1][0];

		// calculate the number of bits required to represent the values
		jDat->encodeY[curBlock][0].s1 |= (unsigned char) numOfBits(jDat->encodeY[curBlock][0].s2);
		jDat->encodeCb[curBlock][0].s1 |= (unsigned char) numOfBits(jDat->encodeCb[curBlock][0].s2);
		jDat->encodeCr[curBlock][0].s1 |= (unsigned char) numOfBits(jDat->encodeCr[curBlock][0].s2);
	}

	#ifdef DEBUG_DPCM
		unsigned char numBits = 0;
		printf("Debugging dpcm: \n");
		for (i = 0; i < jDat->numBlocks; i++){
			numBits = jDat->encodeY[i][0].s1;
			printf("Block: %d = %d and numBits = %d\n", i + 1, jDat->encodeY[i][0].s2, numBits);
		}
	#endif 
}

// encodes the 1x63 AC coefficients
void runLength(JpgData jDat)
{
	int i = 0, j = 0;
	int k1 = 0, k2 = 0, k3 = 0; // keep the next pos in the encode array to store the pairs
	unsigned char z1 = 0, z2 = 0, z3 = 0; // count the number of zeroes

	for (i = 0; i < jDat->numBlocks; i++){ // iterate through the blocks
		k1 = k2 = k3 = 1; // reset pos back to the start of the AC values
		z1 = z2 = z3 = 0; // reset the zero counters
	
		for (j = 1; j < NUM_COEFFICIENTS; j++){ // skip the DC coefficient and process the AC coefs
			if (jDat->zzY[i][j] != 0 || z1 == MAX_ZEROES){
				if (z1 == MAX_ZEROES){ 
					jDat->encodeY[i][k1].s1 |= ZRL_VALUE; // add the # zeroes in the correct part of the Symbol
				}
				else { jDat->encodeY[i][k1].s1 |= z1; }
				jDat->encodeY[i][k1].s1 <<= 4; // shift by 4 bits to put the #zeroes in the run length part of the Symbol
				jDat->encodeY[i][k1].s1 |= (unsigned char) numOfBits(jDat->zzY[i][j]);
				jDat->encodeY[i][k1].s2 = jDat->zzY[i][j]; // don't forget about the amplitude
				z1 = 0;
				k1++;
			}

			else{
				z1++;
			}

			if (jDat->zzCb[i][j] != 0 || z2 == MAX_ZEROES){
				if (z2 == MAX_ZEROES){ 
					jDat->encodeCb[i][k2].s1 |= ZRL_VALUE; // add the # zeroes in the correct part of the Symbol
				}
				else { jDat->encodeCb[i][k2].s1 |= z2; }
				jDat->encodeCb[i][k2].s1 <<= 4; // shift by 4 bits to put the #zeroes in the run length part of the Symbol
				jDat->encodeCb[i][k2].s1 |= (unsigned char) numOfBits(jDat->zzCb[i][j]);
				jDat->encodeCb[i][k2].s2 = jDat->zzCb[i][j]; // don't forget about the amplitude
				z2 = 0;
				k2++;
			}

			else{
				z2++;
			}

			if (jDat->zzCr[i][j] != 0 || z3 == MAX_ZEROES){
				if (z3 == MAX_ZEROES){ 
					jDat->encodeCr[i][k3].s1 |= ZRL_VALUE; // add the # zeroes in the correct part of the Symbol
				}
				else { jDat->encodeCr[i][k3].s1 |= z3; }
				jDat->encodeCr[i][k3].s1 <<= 4; // shift by 4 bits to put the #zeroes in the run length part of the Symbol
				jDat->encodeCr[i][k3].s1 |= (unsigned char) numOfBits(jDat->zzCr[i][j]);
				jDat->encodeCr[i][k3].s2 = jDat->zzCr[i][j]; // don't forget about the amplitude
				z3 = 0;
				k3++;
			}

			else{
				z3++;
			}
		}
	
		// clean the run-length arrays
		sanitise(jDat->encodeY[i]);
		sanitise(jDat->encodeCb[i]);
		sanitise(jDat->encodeCr[i]);
	}

	#ifdef DEBUG_RUN
		unsigned char runL = 0;
		unsigned char size = 0;
		printf("Debugging run length coding.\n");
		for (i = 0; i < jDat->numBlocks; i++){
			printf("Block: %d\n", i + 1);
			for (j = 1; j < NUM_COEFFICIENTS; j++){ 	
				runL = jDat->encodeY[i][j].s1 >> 4;
				size = jDat->encodeY[i][j].s1 - (runL << 4);
				if (runL == 0 && jDat->encodeY[i][j].s2 == 0) { printf("EOB\n"); break; }
				printf("([r = %d, s = %d], A = %d)\n", runL, size, jDat->encodeY[i][j].s2);
			}
			printf("\n");
		}
	#endif
}

// removes incorrect ZRLs and assigns EOB to the correct position
void sanitise(Symbol *s)
{
	int i = 0;
	for (i = NUM_COEFFICIENTS - 1; i >= 1; i--){
		if (s[i].s2 != 0){ break; } // leaves after the first non-zero coefficient is encountered
	}
	s[i + 1].s1 = 0;
}

// determines the number of bits requried to represent the number x
int numOfBits(int x)
{
	int n1 = 0, prevn = 0, nextn = 0;
	int i = 0; // represents the number of bits required to represent the number x

	prevn = n1; // first number in sequence
	while (x < ((-1) * nextn) || x > nextn){ // loop until we find the correct number of bits
		nextn = (2 * prevn) + 1;
		prevn = nextn;
		i++;
	}
	return i;
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
	y[1] = y[0] + 8;
	if (tw % w == 0) { y[0] -= 8; y[1] -= 8; } // reached the last block of a row
	// set the starting and ending x values
	x[0] = (tw % w) - 8;
	if (x[0] == -8) { x[0] = w - 8; } // in last column, maths doesn't make sense, need to do something weird
	x[1] = x[0] + 8;
}

void loadCoordinates(Coordinate *c)
{
	// Ok to hard code since we will always process 8x8 blocks
	c[0].x = 0; c[0].y = 0;
	c[1].x = 1; c[1].y = 0;
	c[2].x = 0; c[2].y = 1;
	c[3].x = 0; c[3].y = 2;
	c[4].x = 1; c[4].y = 1;
	c[5].x = 2; c[5].y = 0;
	c[6].x = 3; c[6].y = 0;
	c[7].x = 2; c[7].y = 1;
	c[8].x = 1; c[8].y = 2;
	c[9].x = 0; c[9].y = 3;
	c[10].x = 0; c[10].y = 4;
	c[11].x = 1; c[11].y = 3;
	c[12].x = 2; c[12].y = 2;
	c[13].x = 3; c[13].y = 1;
	c[14].x = 4; c[14].y = 0;
	c[15].x = 5; c[15].y = 0;
	c[16].x = 4; c[16].y = 1;
	c[17].x = 3; c[17].y = 2;
	c[18].x = 2; c[18].y = 3;
	c[19].x = 1; c[19].y = 4;
	c[20].x = 0; c[20].y = 5;
	c[21].x = 0; c[21].y = 6;
	c[22].x = 1; c[22].y = 5;
	c[23].x = 2; c[23].y = 4;
	c[24].x = 3; c[24].y = 3;
	c[25].x = 4; c[25].y = 2;
	c[26].x = 5; c[26].y = 1;
	c[27].x = 6; c[27].y = 0;
	c[28].x = 7; c[28].y = 0;
	c[29].x = 6; c[29].y = 1;
	c[30].x = 5; c[30].y = 2;
	c[31].x = 4; c[31].y = 3;
	c[32].x = 3; c[32].y = 4;
	c[33].x = 2; c[33].y = 5;
	c[34].x = 1; c[34].y = 6;
	c[35].x = 0; c[35].y = 7;
	c[36].x = 1; c[36].y = 7;
	c[37].x = 2; c[37].y = 6;
	c[38].x = 3; c[38].y = 5;
	c[39].x = 4; c[39].y = 4;
	c[40].x = 5; c[40].y = 3;
	c[41].x = 6; c[41].y = 2;
	c[42].x = 7; c[42].y = 1;
	c[43].x = 7; c[43].y = 2;
	c[44].x = 6; c[44].y = 3;
	c[45].x = 5; c[45].y = 4;
	c[46].x = 4; c[46].y = 5;
	c[47].x = 3; c[47].y = 6;
	c[48].x = 2; c[48].y = 7;
	c[49].x = 3; c[49].y = 7;
	c[50].x = 4; c[50].y = 6;
	c[51].x = 5; c[51].y = 5;
	c[52].x = 6; c[52].y = 4;
	c[53].x = 7; c[53].y = 3;
	c[54].x = 7; c[54].y = 4;
	c[55].x = 6; c[55].y = 5;
	c[56].x = 5; c[56].y = 6;
	c[57].x = 4; c[57].y = 7;
	c[58].x = 5; c[58].y = 7;
	c[59].x = 6; c[59].y = 6;
	c[60].x = 7; c[60].y = 5;
	c[61].x = 7; c[61].y = 6;
	c[62].x = 6; c[62].y = 7;
	c[63].x = 7; c[63].y = 7;
}

// applies the huffman algorithm to compress the run length data + the DCT coefficients
void huffmanEncoding(JpgData jDat)
{
	int i = 0, n = 0;

	n = jDat->numBlocks; // total number of blocks

	// allocate memory to hold the huffman codes
	jDat->huffmanY = malloc(sizeof(HuffSymbol *) * n);
	jDat->huffmanCb = malloc(sizeof(HuffSymbol *) * n);
	jDat->huffmanCr = malloc(sizeof(HuffSymbol *) * n);

	for (i = 0; i < n; i++){
		jDat->huffmanY[i] = calloc(NUM_COEFFICIENTS, sizeof(HuffSymbol));
		jDat->huffmanCb[i] = calloc(NUM_COEFFICIENTS, sizeof(HuffSymbol));
		jDat->huffmanCr[i] = calloc(NUM_COEFFICIENTS, sizeof(HuffSymbol));
	}
	
	// huffman encode the 1x64 vectors
	for (i = 0; i < n; i++){
		// huffman encode a Y block
		DCHuffmanEncodeLum(jDat->encodeY[i][0], jDat->huffmanY[i]);
		ACHuffmanEncodeLum(jDat->encodeY[i], jDat->huffmanY[i]);
		
		// huffman encode a Cb block
		DCHuffmanEncodeChrom(jDat->encodeCb[i][0], jDat->huffmanCb[i]);
		ACHuffmanEncodeChrom(jDat->encodeCb[i], jDat->huffmanCb[i]);

		// huffman encode a Cr block
		DCHuffmanEncodeChrom(jDat->encodeCr[i][0], jDat->huffmanCr[i]);
		ACHuffmanEncodeChrom(jDat->encodeCr[i], jDat->huffmanCr[i]);
	}
}

// applies huffman encoding to a luminance DC coefficient
void DCHuffmanEncodeLum(Symbol encodedDC, HuffSymbol *block)
{
	int bitsToSearch = 0;
	int numBits = 0;
	unsigned int rl = 0;
	int bitSize = 0;
	int codeIndex = 0, bitPos = 0;
	int cb = 0;
	int i = 0, j = 0;
	uint8_t mask = 0;
	uint32_t res = 0, bit = 0;
	
	bitSize = (int) encodedDC->s1;

	for (i = 0; i < bitSize; i++){ // get the length of the # bits to pass until we reach the desired huffman code
		bitsToSearch += numBitsDCLum[i];
	}

	codeIndex = bitsToSearch / 8; // get the index that the code will be in -> bit pos to start extracting from
	bitPos = bitsToSearch % 8; // bit position to start extracting the bits from

	numBits = numBitsDCLum[bitSize]; // tells us the # bits available for extraction in the current element
	i = numBits;
	j = 31;
	while (i > 0){ // extract the bits
		mask = 1;
		mask <<= bitPos; // extract the correct bit pos
		bit = (uint32_t) (DCLum_HuffCodes[codeIndex] & mask); // '1' or '0'
		bit <<= j;
		res |= bit;
		// check if we have to go to the next element
		if (bitPos % 8 == 0) { bitPos = 0; codeIndex++; }
		bitPos++;
		i--;
		j--;
	}

	block[0].nBits = numBits;
	block[0].bits = res;
	
	// encode the value
	huffmanEncodeValue(block, encodedDC.s2, bitSize);
}

// applies huffman encoding to luminance AC coefficients
void ACHuffmanEncodeLum(Symbol *encodedBlock, HuffSymbol *block)
{
	printf("Not implemented yet buddy.\n");
}

// huffman encodes a chrominance DC coefficient
void DCHuffmanEncodeChrom(Symbol encodedDC, HuffSymbol *block)
{
	printf("Not implemented yet bud.\n");
}

// huffman encodes chrominance AC coefficients
void ACHuffmanEncodeChrom(Symbol *encodedBlock, HuffSymbol *block)
{
	printf("Not implemented yet bud.\n");
}

void huffmanEncodeValue(HuffSymbol huffCoeff, int value, int bitSize)
{
	int bitPos = 0;
	int i = 0, j = 0;
	uint32_t bit = 0, mask = 0, minValue = 0, additionalBits = 0;
	uint32_t bit2 = 0, mask2 = 0, bitsToClear = 0, bitsToAdd = 0;
	
	minValue = ((uint32_t) pow(2, bitSize)) * (-1);
	// determine the "additional bits" associated with the value
	mask = 1;
	while (minValue < value){ // loop until additional bits represents our value
		bit = additionalBits & mask; // extract rightmost bit
		if (bit == 1){
			// shift the leftmost '1' from the right left and clear everything behind it
			mask2 = 1;
			bit2 = additionalBits & mask2;
			i = 0;
			while (bit2){ // loop until a '0' bit is reached
				i++; 
				mask2 <<= 1;
				bit2 = additionalBits & mask2;
			}
			bitsToClear = (uint32_t) pow(2, i);
			bitsToAdd = (uint32_t) pow(2, i + 1);
			additionalBits -= bitsToClear;
			additionalBits += bitsToAdd;
		}

		else if (bit == 0){
			// make the rightmost bit a '1'
			additionaBits |= mask;
		}

		else{
			printf("Error: Bit is neither 1 nor 0.\n");
		}
		
		minValue++;
	}	

	// copy the additional bits into huffCoeff
	bitPos = huffCoeff.numBits;
	i = bitSize - 1;
	while (i > 0){
		mask = 1;
		mask <<= i;
		bit = additionalBits & mask;
		bit <<= (32 - bitPos);
		huffCoeff.bits |= bit;
		i--;
	}
	huffCoeff.nBits += bitSize;
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
		free(jdata->quanY[i]);
		free(jdata->quanCb[i]);
		free(jdata->quanCr[i]);
		free(jdata->zzY[i]);
		free(jdata->zzCb[i]);
		free(jdata->zzCr[i]);
		free(jdata->encodeY[i]);
		free(jdata->encodeCb[i]);
		free(jdata->encodeCr[i]);
		free(jdata->huffmanY[i]);
		free(jdata->huffmanCb[i]);
		free(jdata->huffmanCr[i]);
	}
	
	free(jdata->YBlocks);
	free(jdata->CbBlocks);
	free(jdata->CrBlocks);
	free(jdata->quanY);
	free(jdata->quanCb);
	free(jdata->quanCr);
	free(jdata->zzY);
	free(jdata->zzCb);
	free(jdata->zzCr);
	free(jdata->encodeY);
	free(jdata->encodeCb);
	free(jdata->encodeCr);
	free(jdata->huffmanY);
	free(jdata->huffmanCb);
	free(jdata->huffmanCr);
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

