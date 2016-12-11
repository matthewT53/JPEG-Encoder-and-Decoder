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
#define DEFAULT_QUALITY 50

// zig-zag stuff
#define NUM_COEFFICIENTS 64

// run length encoding constants
#define MAX_ZEROES 16
#define ZRL_VALUE 15

// huffman encoding constants
#define LUMINANCE 0 // indexes for the global arrays below
#define CHROMINANCE 1

// marker constants
#define FIRST_MARKER	0xFF
#define SOI_MARKER		0xD8
#define APP0_MARKER		0xE0
#define SOF0_MARKER		0xC0
#define HUFFMAN_MARKER	0xC4
#define QUAN_MARKER		0xDB
#define SCAN_MARKER		0xDA
#define COMMENT_MARKER	0xFE
#define EOI_MARKER		0xD9

// #define DEBUG // debugging constant
#define DEBUG_INFO
// #define DEBUG_PRE // debugging constant for the preprocessing code
// #define DEBUG_BLOCKS // debugging constant for the code that creates 8x8 blocks
// #define DEBUG_PADDING
// #define DEBUG_DOWNSAMPLE
// #define DEBUG_LEVEL_SHIFT
// #define DEBUG_DCT // debugging constant for the dct process
// #define DEBUG_QUAN // debugging constant for the quan process
// #define DEBUG_ZZ // debugging constant for zig-zag process
// #define DEBUG_DPCM // debugging constant for DPCM process
// #define DEBUG_RUN // debugging constant for run length coding
// #define DEBUG_HUFFMAN // debugging constant for huffman encoding

#define LEVEL_SHIFT

// #define DEBUG_DCT_Y

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

	// Chroma subsampling data
	int ratio;

	// luminance width + height is the same as the original image
	int YWidth;
	int YHeight;

	int CbWidth; // Resolution of the Cb component
	int CbHeight;

	int CrWidth; // Resolution of the Cr component
	int CrHeight;

	// number of MCU blocks in each component
	int numBlocksY;
	int numBlocksCb;
	int numBlocksCr;

	// DCT / Quantization step
	int **quanY;
	int **quanCb;
	int **quanCr;
	int quality; // 1 < q < 100, default = 50

	// quantization tables
	unsigned char DQT_TableY[QUAN_MAT_SIZE][QUAN_MAT_SIZE];
	unsigned char DQT_TableChr[QUAN_MAT_SIZE][QUAN_MAT_SIZE];

	unsigned char DQT_Y_zz[NUM_COEFFICIENTS]; // holds the quantization matrix for luminance in zig-zag order
	unsigned char DQT_CHR_zz[NUM_COEFFICIENTS]; // holds the chrominance quantization matrix in zig-zag order

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
void determineResolutions(JpgData jDat);
void findComponentResolution(int ratio, int *h, int *w);
void levelShiftComponent(char **component, int h, int w);
void addPadding(JpgData jDat); // determines if the width and height of each component are divisible by 8

// Chroma-subsampling
/*
	* Discards colour information to achieve better compression results

	Input:
	* jDat

	Output:
	* The Cb and Cr arrays have had their elements averaged resulting in only part the array being used in the compression.
*/
void chromaSubsample(JpgData jDat);

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
void quantise(double dctBlock[][BLOCK_SIZE], int **quantisedComp, unsigned char quanTable[][BLOCK_SIZE], int sx, int sy);

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
	* w = width of the component

	Output:
	* x[0] = startX, x[1] = endX
	* y[0] = startY, y[1] = endY

	Note: Don't forget to free() x and y

*/
void blockToCoords(int bn, int *x, int *y, unsigned int w);

/*
	This function creates the jpeg binary file
	Input: JpgData structure and JPEG filename
	Output: JPEG binary file on disk
*/
void writeToFile(JpgData jDat, const char *fileName);

/* ================== END of Encoding functions ==================== */

/* ================== Utility functions ========================= */

// changing width and height is ok since we can later modify it again in the header of the image

// Splits a colour component (Y, Cb or Cr) into 8 x 8 blocks
void form8by8blocksComponent(char **component, char *colourData, int width, int height, int paddingWidth, int paddingHeight);
int determinePaddingSize(int length); // determines how many lines to add for width or height to make dimensions divisible by 8
void fillHeight(char **colourComponent, int height, int width, int nEdges); // performs padding for the height
void fillWidth(char **colourComponent, int height, int width, int nEdges); // performs padding for the width

// DCT helper functions
void dctTransformBlock(char **component, int *x, int *y, double block[][BLOCK_SIZE]);

void loadCoordinates(Coordinate *c); // loads co-ordinates that indicate the order in which the quan coefficients should be analysed
void sanitise(Symbol *s); // removes random ZRL and places the EOB in the correct position
int numOfBits(int x); // determines the # bits required to represent a number x
char *intToHuffBits(int x); // convert an integer into huffman bits

// utility function for quantization
void buildQuanTables(JpgData jDat);

// utility functions for run length encoding
void zigZagComponent(int **zigZagBlocks, int **quantisedComponent, unsigned int width, int numBlocks);
void runLengthComponent(Symbol **componentToEncode, int **zigZaggedComponent, int numBlocks);

// utility functions for the huffman encoding process
void DCHuffmanEncode(Symbol encodedDC, HuffSymbol *block, int component);
void ACHuffmanEncode(Symbol *encodedBlock, HuffSymbol *block, int component);
void huffmanEncodeValue(HuffSymbol *huffCoeff, int value, int bitSize);

// functions for constructing the JPEG binary file
void writeSOI(FILE *fp); // start of image
void writeAPP0(FILE *fp); // JFIF standard
void writeDQT(FILE *fp, JpgData jDat); // store quantization tables
void writeDHT(FILE *fp, JpgData jDat); // store Huffman tables
void writeFrameHeader(FILE *fp, JpgData jDat); // header for everything between SOI and EOI
void writeScanHeader(FILE *fp, JpgData jDat); // header for the scan data
void writeScanData(FILE *fp, JpgData jDat); // entropy encoded data
void writeBlockData(FILE *fp, HuffSymbol *block, Byte *b, int *bitPos);
void writeComment(FILE *fp, char *comment, short size); // includes a comment in the binary file
void writeEOI(FILE *fp); // end of image

void writeByte(FILE *fp, Byte b); // writes a single byte into a file

// free resources
void disposeJpgData(JpgData jdat);

// utility functions for freeing resources
void dispose_YCbCr(JpgData jDat); // frees the YCbCr arrays
void dispose_blocks(JpgData jDat); // frees the 8x8 blocks
void dispose_quan(JpgData jDat); // frees the quantization matices for each component
void dispose_zz(JpgData jDat); // frees the zig-zag vectors for each component
void dispose_encode(JpgData jDat); // frees the encoded data for each component

// simple utility and debugging functions
static Pixel newPixBuf(int n); // creates a new pixel buffer with n pixels
static int determineFileSize(FILE *f); // determines the size of a file

// debugging output functions
#ifdef DEBUG
static void dbg_out_file(Pixel p, int size); // dumps the contents of buf into a file
#endif

// array of pointers that point to relevant huffman tables
// tables that indicate the # bits of the huffman code for each (run/size)
static const int *DC_nbits[2] = {numBitsDcLum, numBitsDcChr};
static const int (*AC_nbits[])[11] = {numBitsAcLum, numBitsAcChr};

// tables that contain the huffman codes for each (run/size)
static const uint8_t *DC_codes[2] = {DCLum_HuffCodes, DCChr_HuffCodes};
static const uint64_t *AC_codes[2] = {ACLum_HuffCodes, ACChr_HuffCodes};

// pattern for creating zig-zag vectors
static Coordinate c[NUM_COEFFICIENTS];

// can easily be adapted to other image formats other than BMP - mainly used for testing
Pixel imageToRGB(const char *imageName, int *bufSize)
{
	Pixel pixBuf = NULL;
	int imageSize = 0;
	FILE *fp = fopen(imageName, "rb");
	int numberPixels = 0;
	char *buffer = NULL, *data = NULL;
	int fs = 0, imageDataOffset = 0, offset = 0;
	int bitDepth = 0;
	int width = 0, height = 0;
	int i = 0;

	if (fp == NULL){
		printf("Error opening image file.\n");
		exit(1); // terminate the process
	}

	fs = determineFileSize(fp);
	buffer = malloc(sizeof(char) * (fs + 1));
	fread(buffer, sizeof(char), fs, fp);
	assert(buffer != NULL);

	// read in bmp header info
	data = buffer + 10; // offset to RGB info
	memcpy(&imageDataOffset, data, 4);

	data = buffer + 18; // offset to width
	memcpy(&width, data, 4);

	data = buffer + 22; // offset to height
	memcpy(&height, data, 4);

	data = buffer + 28; // # bits / pixel
	memcpy(&bitDepth, data, 2);

	if (bitDepth != 24){
		printf("Images with 1, 4 or 8 bit depths are not supported.\n");
		exit(1);
	}

	#ifdef DEBUG
		printf("offset = %d, width = %d, height = %d and bitDepth = %d\n", imageDataOffset, width, height, bitDepth);
	#endif

	imageSize = determineFileSize(fp) - imageDataOffset; // det the size that our pixel buffer should be
	assert(imageSize > 0); // make sure the image size is > 0
	numberPixels = ceil(imageSize / 3); // # pixels required
	pixBuf = newPixBuf(numberPixels); // create an empty pixel buffer
	// fseek(fp, imageDataOffset, SEEK_SET); // seek 54 bytes from start of file

	// modify the order of the RGB values so the jpeg image can have the correct orientation
	int pixPos = 0, j = 0; // index for the pixel array
	for (i = 1; i <= height; i++){
		offset = fs - (i * width * (bitDepth / 8));
		for (j = 0; j < (width * 3); j += 3){ // sometimes the ordering of the rgb values is different
			pixBuf[pixPos].b = buffer[offset + j];
			pixBuf[pixPos].g = buffer[offset + j + 1];
			pixBuf[pixPos].r = buffer[offset + j + 2];
			pixPos++;
		}
	}

	if (pixPos != numberPixels){
		printf("We have a problem.\n");
	}

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
void encodeRGBToJpgDisk(const char *jpgFile, Pixel rgbBuffer, unsigned int numPixels, unsigned int width, unsigned int height, int quality, int sampleRatio)
{
	JpgData jD = malloc(sizeof(jpegData)); // create object to hold info about the jpeg

	// set up the struct
	jD->width = width;
	jD->height = height;
	jD->quality = quality;
	jD->ratio = sampleRatio;

	jD->YHeight = jD->CbHeight = jD->CrHeight = height;
	jD->YWidth = jD->CbWidth = jD->CrWidth = width;
	jD->numBlocksY = jD->numBlocksCb = jD->numBlocksCr = 0;

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
		writeToFile(jD, jpgFile);

		// free memory
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
	levelShift(jDat); // subtract 128 from each value
	#endif

	// user wants to chroma subsample the JPEG image
	if (jDat->ratio != NO_CHROMA_SUBSAMPLING){
		determineResolutions(jDat); // determines the resolution of each compoenent based on the sampling ratio
		chromaSubsample(jDat); // chroma subsample the JPEG image
	}

	// set the number of blocks in each component
	jDat->numBlocksY = ( (double) jDat->YHeight / 8 * (double) jDat->YWidth / 8 );
	jDat->numBlocksCb = ( (double) jDat->CbHeight / 8 * (double) jDat->CbWidth / 8 );
	jDat->numBlocksCr = ( (double) jDat->CrHeight / 8 * (double) jDat->CrWidth / 8 );

	addPadding(jDat);

	#ifdef DEBUG_INFO
		printf("YHeight: %d and YWidth: %d\n", jDat->YHeight, jDat->YWidth);
		printf("CbHeight: %d and CbWidth: %d\n", jDat->CbHeight, jDat->CbWidth);
		printf("CrHeight: %d and CrWidth: %d\n", jDat->CrHeight, jDat->CrWidth);
		printf("Y: numblocks = %d\n", jDat->numBlocksY);
		printf("Cb: numBlocks = %d\n", jDat->numBlocksCb);
		printf("Cr: numBlocks = %d\n", jDat->numBlocksCr);
	#endif
}

void convertRGBToYCbCr(JpgData jDat, Pixel rgb, unsigned int numPixels)
{
	int i = 0;
	// printf("Not yet implemented conversion space.\n");
	jDat->Y = malloc(sizeof(unsigned char) * numPixels);
	jDat->Cb = malloc(sizeof(unsigned char) * numPixels);
	jDat->Cr = malloc(sizeof(unsigned char) * numPixels);

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

void addPadding(JpgData jDat)
{
	int fillHeightY = determinePaddingSize(jDat->YHeight);
	int fillWidthY = determinePaddingSize(jDat->YWidth);
	int fillHeightCb = determinePaddingSize(jDat->CbHeight);
	int fillWidthCb = determinePaddingSize(jDat->CbWidth);
	int fillHeightCr = determinePaddingSize(jDat->CrHeight);
	int fillWidthCr = determinePaddingSize(jDat->CrWidth);

	#ifdef DEBUG_PADDING
		printf("fhY = %d and fwY = %d\n", fillHeightY, fillWidthY);
		printf("fhCb = %d and fwCb = %d\n", fillHeightCb, fillWidthCb);
		printf("fhCr = %d and fwCr = %d\n", fillHeightCr, fillWidthCr);
	#endif

	if (fillHeightY){
		fillHeight(jDat->YBlocks, jDat->YHeight, jDat->YWidth, fillHeightY);
		jDat->YHeight += fillHeightY;
	}

	if (fillWidthY){
		fillWidth(jDat->YBlocks, jDat->YHeight, jDat->YWidth, fillWidthY);
		jDat->YWidth += fillWidthY;
	}

	if (fillHeightCb){
		fillHeight(jDat->CbBlocks, jDat->CbHeight, jDat->CbWidth, fillHeightCb);
		jDat->CbHeight += fillHeightCb;
	}

	if (fillWidthCb){
		fillWidth(jDat->CbBlocks, jDat->CbHeight, jDat->CbWidth, fillWidthCb);
		jDat->CbWidth += fillWidthCb;
	}


	if (fillHeightCr){
		fillHeight(jDat->CrBlocks, jDat->CrHeight, jDat->CrWidth, fillHeightCr);
		jDat->CrHeight += fillHeightCr;
	}

	if (fillWidthCr){
		fillWidth(jDat->CrBlocks, jDat->CrHeight, jDat->CrWidth, fillWidthCr);
		jDat->CrWidth += fillWidthCr;
	}
}

// convert the image into 8 by 8 blocks
void form8by8blocks(JpgData jDat)
{
	int totalH = jDat->height, totalW = jDat->width;
	int i = 0, j = 0, k = 0;

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

	// set the resolution for the luminance component
	jDat->YWidth = totalW;
	jDat->YHeight = totalH;

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
	// printf("Finished output.\n");
}

int determinePaddingSize(int length)
{
	int paddingLength = 0;
	if (length % 8 != 0){
		paddingLength = 8 - (length % 8);
	}

	return paddingLength;
}

void fillHeight(char **colourComponent, int height, int width, int nEdges)
{
	//  extend the bottom width of the image
	int i = 0;
	char *bottomLine = colourComponent[height - 1];

	for (i = height; i < height + nEdges; i++){
		memcpy(colourComponent[i], bottomLine, width);
	}
}

void fillWidth(char **colourComponent, int height, int width, int nEdges)
{
	int i = 0, j = 0;
	int baseIndexW = width - 1;

	for (i = width; i < width + nEdges; i++){
		for (j = 0; j < height; j++){
			colourComponent[j][i] = colourComponent[j][baseIndexW];
		}
	}
}

// determines the resolution of each component based on sample rate
void determineResolutions(JpgData jDat)
{
	int ratio = jDat->ratio;

	// determine the resolution for the Cb component
	findComponentResolution(ratio, &jDat->CbHeight, &jDat->CbWidth);
	// determine the resolution for the Cr component
	findComponentResolution(ratio, &jDat->CrHeight, &jDat->CrWidth);

	#ifdef DEBUG_INFO
		printf("Luminance width + Height:\nHeight = %d and Width = %d.\n", jDat->YHeight, jDat->YWidth);
		printf("Cb:\nHeight = %d and Width = %d.\n", jDat->CbHeight, jDat->CbWidth);
		printf("Cr:\nHeight = %d and Width = %d.\n", jDat->CrHeight, jDat->CrWidth);
	#endif

}

void findComponentResolution(int ratio, int *h, int *w)
{
	switch (ratio){
		case NO_CHROMA_SUBSAMPLING: break; // keep the original resolutions i.e no chroma subsampling
		case HORIZONTAL_SUBSAMPLING: *w = *w / 2; break;
		case HORIZONTAL_VERTICAL_SUBSAMPLING: *h = *h / 2; *w = *w / 2; break;
		default: printf("%d chroma subsampling ratio not supported.\n", ratio); break;
	}
}

#ifdef LEVEL_SHIFT
// might not be required
void levelShift(JpgData jDat)
{
	#ifdef DEBUG_LEVEL_SHIFT
		printf("[LEVEL] Y:\n");
	#endif
	levelShiftComponent(jDat->YBlocks, jDat->YHeight, jDat->YWidth);
	#ifdef DEBUG_LEVEL_SHIFT
		printf("[LEVEL]: Cb:\n");
	#endif
	levelShiftComponent(jDat->CbBlocks, jDat->CbHeight, jDat->CbWidth);
	#ifdef DEBUG_LEVEL_SHIFT
		printf("[LEVEL]: Cr:\n");
	#endif
	levelShiftComponent(jDat->CrBlocks, jDat->CrHeight, jDat->CrWidth);
}

void levelShiftComponent(char **component, int h, int w)
{
	int i = 0, j = 0;

	for (i = 0; i < h; i++){
		for (j = 0; j < w; j++){
			component[i][j] -= 128;
			#ifdef DEBUG_LEVEL_SHIFT
				printf("%5d ", component[i][j]);
			#endif
		}
		#ifdef DEBUG_LEVEL_SHIFT
			printf("\n");
		#endif
	}
}
#endif

/*
	Chroma subsampling function
	Discards some colour information in the Cb and Cr components to achieve better file compression.
*/
void chromaSubsample(JpgData jDat)
{
	int i = 0, j = 0;
	int h = jDat->YHeight, w = jDat->YWidth;
	// printf("Height: %d and Width: %d\n", h, w);

	// Subsample 4:2:2
	if (jDat->ratio == HORIZONTAL_SUBSAMPLING){
		#ifdef DEBUG_DOWNSAMPLE
			printf("4:2:2 Subsampling.\n");
			// print the original first block
			for (i = 0; i < jDat->YHeight; i++){
				for (j = 0; j < jDat->YWidth; j++){
					printf("%5d", jDat->CbBlocks[i][j]);
				}
				printf("\n");
			}
		#endif

		// unsigned char value1 = 0, value2 = 0;
		// printf("Number of blocks: %d\n", jDat->numBlocksCb);
		for (i = 0; i < h; i++){
			for (j = 0; j < w; j += 2){
				jDat->CbBlocks[i][j/2] = (jDat->CbBlocks[i][j] + jDat->CbBlocks[i][j + 1]) / 2;
				jDat->CrBlocks[i][j/2] = (jDat->CrBlocks[i][j] + jDat->CrBlocks[i][j + 1]) / 2;
			}
		}

		#ifdef DEBUG_DOWNSAMPLE
			// print the subsampled block
			printf("After subsampling:\n");
			for (i = 0; i < jDat->CbHeight; i++){
				for (j = 0; j < jDat->CbWidth; j++){
					printf("%5d", jDat->CbBlocks[i][j]);
				}
				printf("\n");
			}
		#endif
	}

	// chroma subsample 4:2:0
	else if (jDat->ratio == HORIZONTAL_VERTICAL_SUBSAMPLING){
		#ifdef DEBUG_DOWNSAMPLE
			printf("4:2:0 Subsampling.\n");
			printf("Before subsampling: \n");
			for (i = 0; i < jDat->YHeight; i++){
				for (j = 0; j < jDat->YWidth; j++){
					printf("%5d ", jDat->CbBlocks[i][j]);
				}
				printf("\n");
			}
		#endif

		for (i = 0; i < h; i += 2){
			for (j = 0; j < w; j += 2){
				jDat->CbBlocks[i/2][j/2] = (jDat->CbBlocks[i][j] + jDat->CbBlocks[i][j+1] +
											jDat->CbBlocks[i+1][j] + jDat->CbBlocks[i+1][j+1]) / 4;
				jDat->CrBlocks[i/2][j/2] = (jDat->CrBlocks[i][j] + jDat->CrBlocks[i][j+1] +
											jDat->CrBlocks[i+1][j] + jDat->CrBlocks[i+1][j+1]) / 4;
			}
		}

		#ifdef DEBUG_DOWNSAMPLE
			printf("After subsampling: \n");
			for (i = 0; i < jDat->CbHeight; i++){
				for (j = 0; j < jDat->CbWidth; j++){
					printf("%5d ", jDat->CbBlocks[i][j]);
				}
				printf("\n");
			}
		#endif
	}

	else{
		printf("%d chroma subsampling ratio not supported.\n", jDat->ratio);
	}
}

// applies dicrete cosine transformation to the image
void dct(JpgData jDat)
{
	int curBlock = 0;
	int sX[2] = {0}, sY[2] = {0};
	int i = 0;

	double dctBlock[BLOCK_SIZE][BLOCK_SIZE];

	// create the quantization arrays
	jDat->quanY = malloc(sizeof(int *) * jDat->YHeight);
	jDat->quanCb = malloc(sizeof(int *) * jDat->CbHeight);
	jDat->quanCr = malloc(sizeof(int *) * jDat->CrHeight);

	for (i = 0; i < jDat->YHeight; i++){
		jDat->quanY[i] = calloc(jDat->YWidth, sizeof(int));
	}

	for (i = 0; i < jDat->CbHeight; i++){
		jDat->quanCb[i] = calloc(jDat->CbWidth, sizeof(int));
	}

	for (i = 0; i < jDat->CrHeight; i++){
		jDat->quanCr[i] = calloc(jDat->CrWidth, sizeof(int));
	}

	buildQuanTables(jDat); // build the quantization tables to use

	// perform DCT on the luminance component
	#ifdef DEBUG_DCT
		printf("Luminance DCT: \n");
	#endif
	for (curBlock = 1; curBlock <= jDat->numBlocksY; curBlock++){
		#ifdef DEBUG_DCT
			printf("Block number (Y): %d\n", curBlock);
		#endif
		blockToCoords(curBlock, sX, sY, jDat->YWidth); // convert block number into coordinates
		dctTransformBlock(jDat->YBlocks, sX, sY, dctBlock); // transform the block
		#ifdef DEBUG_QUAN
			printf("Block quantised: \n");
		#endif
		quantise(dctBlock, jDat->quanY, jDat->DQT_TableY, sX[0], sY[0]); // quantise the block
	}

	#ifdef DEBUG_DCT
		printf("\nCb DCT:\n");
	#endif
	// perform DCT on the Cb component
	for (curBlock = 1; curBlock <= jDat->numBlocksCb; curBlock++){
		#ifdef DEBUG_DCT
			printf("Block number (Cb): %d\n", curBlock);
		#endif
		blockToCoords(curBlock, sX, sY, jDat->CbWidth);
		dctTransformBlock(jDat->CbBlocks, sX, sY, dctBlock);
		#ifdef DEBUG_QUAN
			printf("Block quantised: \n");
		#endif
		quantise(dctBlock, jDat->quanCb, jDat->DQT_TableChr, sX[0], sY[0]);
	}

	#ifdef DEBUG_DCT
		printf("\nCr DCT:\n");
	#endif
	// perform DCT on the Cr component
	for (curBlock = 1; curBlock <= jDat->numBlocksCr; curBlock++){
		#ifdef DEBUG_DCT
			printf("Block number (Cr): %d\n", curBlock);
		#endif
		blockToCoords(curBlock, sX, sY, jDat->CrWidth);
		dctTransformBlock(jDat->CrBlocks, sX, sY, dctBlock);
		#ifdef DEBUG_QUAN
			printf("Block quantised: \n");
		#endif
		quantise(dctBlock, jDat->quanCr, jDat->DQT_TableChr, sX[0], sY[0]);
	}
}

void dctTransformBlock(char **component, int *x, int *y, double block[][BLOCK_SIZE])
{
	int u = 0, v = 0;
	int i = 0, j = 0;
	int startX = x[0], endX = x[1];
	int startY = y[0], endY = y[1];
	double dctCoef = 0;

	for (u = 0; u < BLOCK_SIZE; u++){ // calculate DCT for G(u,v)
		for (v = 0; v < BLOCK_SIZE; v++){
			dctCoef = 0.0;
			for (i = startX; i < endX; i++){
				for (j = startY; j < endY; j++){
					dctCoef += component[j][i] * cos(((2 * (i % 8) + 1) * u * PI) / 16) * cos(((2 * (j % 8) + 1) * v * PI) / 16);
				}
			}

			// finalise the dct coefficient
			block[v][u] = (0.25) * A(u) * A(v) * dctCoef;
		}
	}

	#ifdef DEBUG_DCT
	for (u = 0; u < BLOCK_SIZE; u++){
		for (v = 0; v < BLOCK_SIZE; v++){
			printf("%8.2f ", block[u][v]);
		}
		printf("\n");
	}
	printf("\n");
	#endif
}

// converts block number to starting and ending coordinates (x,y)
void blockToCoords(int bn, int *x, int *y, unsigned int w)
{
	unsigned int tw = 8 * (unsigned int) bn;

	// set the starting and ending y values
	y[0] = (int) (tw / w) * 8;
	y[1] = y[0] + 8;
	if (tw % w == 0) { y[0] -= 8; y[1] -= 8; } // reached the last block of a row
	// set the starting and ending x values
	x[0] = (tw % w) - 8;
	if (x[0] == -8) { x[0] = w - 8; } // in last column, maths doesn't make sense, need to do something weird
	x[1] = x[0] + 8;
}

void buildQuanTables(JpgData jDat)
{
	unsigned char q = (unsigned char) jDat->quality; // quality factor
	unsigned char s = 0; // scale factor
	int i = 0, j = 0;
	int x = 0, y = 0;

	// check if quality has the correct value
	if (q < 1 || q >= 100){
		q = jDat->quality = DEFAULT_QUALITY; // default quality = 50
	}

	s = ((q < DEFAULT_QUALITY) ? (5000 / q) : (200 - (2 * q))); // get the quality scaling factor

	for (i = 0; i < QUAN_MAT_SIZE; i++){
		for (j = 0; j < QUAN_MAT_SIZE; j++){
			jDat->DQT_TableY[i][j] = ((s*quanMatrixLum[i][j] + 50) / 100);
			jDat->DQT_TableChr[i][j] = ((s*quanMatrixChr[i][j] + 50) / 100);
		}
	}

	// store this table in vector to write to the file
	loadCoordinates(c);

	for (i = 0; i < NUM_COEFFICIENTS; i++){
		y = c[i].y;
		x = c[i].x;
		jDat->DQT_Y_zz[i] = jDat->DQT_TableY[y][x];
		jDat->DQT_CHR_zz[i] = jDat->DQT_TableChr[y][x];
	}

	#ifdef DEBUG_QUAN
		for (i = 0; i < QUAN_MAT_SIZE; i++){
			for (j = 0; j < QUAN_MAT_SIZE; j++){
				printf("%4d", jDat->DQT_TableY[i][j]);
			}
			printf("\n");
		}

		printf("Zig zag vector: ");
		for (i = 0; i < NUM_COEFFICIENTS; i++){
			printf("%d ", jDat->DQT_Y_zz[i]);
		}
		printf("\n");
	#endif
}

// quantises a DCT block
void quantise(double dctBlock[][BLOCK_SIZE], int **quantisedComp, unsigned char quanTable[][BLOCK_SIZE], int sx, int sy)
{
	int i = 0, j = 0, m = 0, n = 0;
	#ifdef DEBUG_QUAN
		printf("StartX = %d and startY = %d\n", sx, sy);
	#endif
	// quantise the matrices
	for (i = 0, m = sy; i < BLOCK_SIZE; i++, m++){
		for (j = 0, n = sx; j < BLOCK_SIZE; j++, n++){
			quantisedComp[m][n] = (int) round(dctBlock[i][j] / quanTable[i][j]);
			#ifdef DEBUG_QUAN
				printf("%5d", quantisedComp[m][n]);
			#endif
		}
		#ifdef DEBUG_QUAN
			printf("\n");
		#endif
	}
}

// re-orders the coefficients of the quantized matrices
void zigZag(JpgData jDat)
{
	int i = 0;

	// create the zig-zag vectors
	jDat->zzY = malloc(sizeof(int *) * jDat->numBlocksY);
	jDat->zzCb = malloc(sizeof(int *) * jDat->numBlocksCb);
	jDat->zzCr = malloc(sizeof(int *) * jDat->numBlocksCr);

	for (i = 0; i < jDat->numBlocksY; i++){
		jDat->zzY[i] = malloc(sizeof(int) * NUM_COEFFICIENTS);
	}

	for (i = 0; i < jDat->numBlocksCb; i++){
		jDat->zzCb[i] = malloc(sizeof(int) * NUM_COEFFICIENTS);
	}

	for (i = 0; i < jDat->numBlocksCr; i++){
		jDat->zzCr[i] = malloc(sizeof(int) * NUM_COEFFICIENTS);
	}

	// zig-zag order each colour component of the image (Y, Cb and Cr)
	zigZagComponent(jDat->zzY, jDat->quanY, jDat->YWidth, jDat->numBlocksY);
	zigZagComponent(jDat->zzCb, jDat->quanCb, jDat->CbWidth, jDat->numBlocksCb);
	zigZagComponent(jDat->zzCr, jDat->quanCr, jDat->CrWidth, jDat->numBlocksCr);
}

void zigZagComponent(int **zigZagBlocks, int **quantisedComponent, unsigned int width, int numBlocks)
{
	int j = 0;
	int curBlock = 0;

	int sX[2] = {0}, sY[2] = {0};
	int startX = 0, startY = 0;
	int cX = 0, cY = 0;

	for (curBlock = 1; curBlock <= numBlocks; curBlock++){
		blockToCoords(curBlock, sX, sY, width);
		startX = sX[0];
		startY = sY[0];
		for (j = 0; j < NUM_COEFFICIENTS; j++){
			cX = c[j].x;
			cY = c[j].y;
			zigZagBlocks[curBlock - 1][j] = quantisedComponent[startY + cY][startX + cX];
		}
		sX[0] = sX[1] = 0;
		sY[0] = sY[1] = 0;
	}
}

// Maps a 8x8 block into a 1x64 vector
void dpcm(JpgData jDat)
{
	int curBlock = 0;
	int i = 0;

	// allocate memory to store the encoded data
	jDat->encodeY = malloc(sizeof(Symbol *) * jDat->numBlocksY);
	jDat->encodeCb = malloc(sizeof(Symbol *) * jDat->numBlocksCb);
	jDat->encodeCr = malloc(sizeof(Symbol *) * jDat->numBlocksCr);

	for (i = 0; i < jDat->numBlocksY; i++){
		jDat->encodeY[i] = malloc(sizeof(Symbol) * NUM_COEFFICIENTS);
	}

	for (i = 0; i < jDat->numBlocksCb; i++){
		jDat->encodeCb[i] = malloc(sizeof(Symbol) * NUM_COEFFICIENTS);
	}

	for (i = 0; i < jDat->numBlocksCr; i++){
		jDat->encodeCr[i] = malloc(sizeof(Symbol) * NUM_COEFFICIENTS);
	}

	// perform DPCM on the DC coefficients of each block
	jDat->encodeY[0][0].s2 = jDat->zzY[0][0];
	jDat->encodeCb[0][0].s2 = jDat->zzCb[0][0];
	jDat->encodeCr[0][0].s2 = jDat->zzCr[0][0];

	// determine the # bits for the very first DC coefficients in each block
	jDat->encodeY[0][0].s1 |= (unsigned char) numOfBits(jDat->encodeY[0][0].s2);
	jDat->encodeCb[0][0].s1 |= (unsigned char) numOfBits(jDat->encodeCb[0][0].s2);
	jDat->encodeCr[0][0].s1 |= (unsigned char) numOfBits(jDat->encodeCr[0][0].s2);

	for (curBlock = 1; curBlock < jDat->numBlocksY; curBlock++){ // iterate in reverse to avoid incorrectly altering results of elements ahead
		jDat->encodeY[curBlock][0].s2 = jDat->zzY[curBlock][0] - jDat->zzY[curBlock - 1][0];

		// calculate the number of bits required to represent the values
		jDat->encodeY[curBlock][0].s1 |= (unsigned char) numOfBits(jDat->encodeY[curBlock][0].s2);
	}

	for (curBlock = 1; curBlock < jDat->numBlocksCb; curBlock++){
		jDat->encodeCb[curBlock][0].s2 = jDat->zzCb[curBlock][0] - jDat->zzCb[curBlock - 1][0];
		jDat->encodeCb[curBlock][0].s1 |= (unsigned char) numOfBits(jDat->encodeCb[curBlock][0].s2);
	}

	for (curBlock = 1; curBlock < jDat->numBlocksCr; curBlock++){
		jDat->encodeCr[curBlock][0].s2 = jDat->zzCr[curBlock][0] - jDat->zzCr[curBlock - 1][0];
		jDat->encodeCr[curBlock][0].s1 |= (unsigned char) numOfBits(jDat->encodeCr[curBlock][0].s2);
	}

	#ifdef DEBUG_DPCM
		unsigned char numBits = 0;
		printf("Debugging dpcm: \n");
		for (i = 0; i < jDat->numBlocksY; i++){
			numBits = jDat->encodeY[i][0].s1;
			printf("Block: %d = %d and numBits = %d\n", i + 1, jDat->encodeY[i][0].s2, numBits);
		}
	#endif
}

// encodes the 1x63 AC coefficients
void runLength(JpgData jDat)
{
	// run-length encode each component of the image
	runLengthComponent(jDat->encodeY, jDat->zzY, jDat->numBlocksY);
	runLengthComponent(jDat->encodeCb, jDat->zzCb, jDat->numBlocksCb);
	runLengthComponent(jDat->encodeCr, jDat->zzCr, jDat->numBlocksCr);

	#ifdef DEBUG_RUN
		unsigned char runL = 0;
		unsigned char size = 0;
		int i = 0, j = 0;
		printf("Debugging run length coding.\n");
		printf("Luminance: \n");
		for (i = 0; i < jDat->numBlocksY; i++){
			printf("Block: %d\n", i + 1);
			for (j = 1; j < NUM_COEFFICIENTS; j++){
				runL = jDat->encodeY[i][j].s1 >> 4;
				size = jDat->encodeY[i][j].s1 - (runL << 4);
				if (runL == 0 && size == 0 && jDat->encodeY[i][j].s2 == 0) { printf("EOB\n"); break; }
				printf("([r = %d, s = %d], A = %d)\n", runL, size, jDat->encodeY[i][j].s2);
			}
			printf("\n");
		}

		printf("Cb:\n");
		for (i = 0; i < jDat->numBlocksCb; i++){
			printf("Block: %d\n", i + 1);
			for (j = 1; j < NUM_COEFFICIENTS; j++){
				runL = jDat->encodeCb[i][j].s1 >> 4;
				size = jDat->encodeCb[i][j].s1 - (runL << 4);
				if (runL == 0 && size == 0 && jDat->encodeCb[i][j].s2 == 0) { printf("EOB\n"); break; }
				printf("([r = %d, s = %d], A = %d)\n", runL, size, jDat->encodeCb[i][j].s2);
			}
			printf("\n");
		}

		printf("Cr: \n");
		for (i = 0; i < jDat->numBlocksCr; i++){
			printf("Block: %d\n", i + 1);
			for (j = 1; j < NUM_COEFFICIENTS; j++){
				runL = jDat->encodeCr[i][j].s1 >> 4;
				size = jDat->encodeCr[i][j].s1 - (runL << 4);
				if (runL == 0 && size == 0 && jDat->encodeCr[i][j].s2 == 0) { printf("EOB\n"); break; }
				printf("([r = %d, s = %d], A = %d)\n", runL, size, jDat->encodeCr[i][j].s2);
			}
			printf("\n");
		}
	#endif
}

void runLengthComponent(Symbol **componentToEncode, int **zigZaggedComponent, int numBlocks)
{
	int i = 0, j = 0;
	int k = 0;
	unsigned int z = 0;

	for (i = 0; i < numBlocks; i++){
		k = 1;
		z = 0;
		for (j = 1; j < NUM_COEFFICIENTS; j++){ // skip the DC coefficient and process the AC coefs
			if (zigZaggedComponent[i][j] != 0 || z == MAX_ZEROES){
				if (z == MAX_ZEROES){
					componentToEncode[i][k].s1 |= ZRL_VALUE; // add the # zeroes in the correct part of the Symbol
				}
				else {
					componentToEncode[i][k].s1 |= z;
				}
				componentToEncode[i][k].s1 <<= 4; // shift by 4 bits to put the #zeroes in the run length part of the Symbol
				componentToEncode[i][k].s1 |= (unsigned char) numOfBits(zigZaggedComponent[i][j]);
				componentToEncode[i][k].s2 = zigZaggedComponent[i][j]; // don't forget about the amplitude
				z = 0;
				k++;
			}

			else{
				z++;
			}
		}

		// clean the run-length arrays
		sanitise(componentToEncode[i]);
	}
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
int numOfBits(int x) // this can be made more efficient with bit shifting
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
	int i = 0;

	// allocate memory to hold the huffman codes
	jDat->huffmanY = malloc(sizeof(HuffSymbol *) * jDat->numBlocksY);
	jDat->huffmanCb = malloc(sizeof(HuffSymbol *) * jDat->numBlocksCb);
	jDat->huffmanCr = malloc(sizeof(HuffSymbol *) * jDat->numBlocksCr);

	for (i = 0; i < jDat->numBlocksY; i++){
		jDat->huffmanY[i] = calloc(NUM_COEFFICIENTS, sizeof(HuffSymbol));
	}

	for (i = 0; i < jDat->numBlocksCb; i++){
		jDat->huffmanCb[i] = calloc(NUM_COEFFICIENTS, sizeof(HuffSymbol));
	}

	for (i = 0; i < jDat->numBlocksCr; i++){
		jDat->huffmanCr[i] = calloc(NUM_COEFFICIENTS, sizeof(HuffSymbol));
	}

	// huffman encode the 1x64 vectors
	for (i = 0; i < jDat->numBlocksY; i++){
		#ifdef DEBUG_HUFFMAN
			printf("Y component: \n");
		#endif
		DCHuffmanEncode(jDat->encodeY[i][0], jDat->huffmanY[i], LUMINANCE);
		ACHuffmanEncode(jDat->encodeY[i], jDat->huffmanY[i], LUMINANCE);
	}

	for (i = 0; i < jDat->numBlocksCb; i++){
		#ifdef DEBUG_HUFFMAN
		printf("Cb component: \n");
		#endif
		DCHuffmanEncode(jDat->encodeCb[i][0], jDat->huffmanCb[i], CHROMINANCE);
		ACHuffmanEncode(jDat->encodeCb[i], jDat->huffmanCb[i], CHROMINANCE);
	}

	for (i = 0; i < jDat->numBlocksCr; i++){
		#ifdef DEBUG_HUFFMAN
		printf("Cr component: \n");
		#endif
		DCHuffmanEncode(jDat->encodeCr[i][0], jDat->huffmanCr[i], CHROMINANCE);
		ACHuffmanEncode(jDat->encodeCr[i], jDat->huffmanCr[i], CHROMINANCE);
	}
}

// applies huffman encoding to a luminance DC coefficient
void DCHuffmanEncode(Symbol encodedDC, HuffSymbol *block, int component)
{
	int bitsToSearch = 0;
	int numBits = 0;
	int bitSize = 0;
	int codeIndex = 0, bitPos = 0;
	int i = 0, j = 0;
	uint8_t mask = 0;
	uint32_t res = 0, bit = 0;

	bitSize = (int) encodedDC.s1;
	for (i = 0; i < bitSize; i++){ // get the length of the # bits to pass until we reach the desired huffman code
		bitsToSearch += DC_nbits[component][i];
	}

	codeIndex = bitsToSearch / 8; // get the index that the code will be in -> bit pos to start extracting from
	bitPos = 7 - (bitsToSearch % 8); // bit position to start extracting the bits from

	#ifdef DEBUG_HUFFMAN
		printf("bitSize = %d\n", bitSize);
		printf("Code index: %d and bitPos = %d\n", codeIndex, bitPos);
	#endif

	// tells us the # bits needed to extract the correct code
	numBits = DC_nbits[component][bitSize];
	i = numBits;

	#ifdef DEBUG_HUFFMAN
		printf("numBits = %d\n", numBits);
	#endif
	j = 31;
	while (i > 0){ // extract the bits, bits are not alligned
		mask = 1;
		mask <<= bitPos; // extract the correct bit pos
		bit = (uint32_t) (DC_codes[component][codeIndex] & mask);
		bit = (bit) ? 1 : 0;
		bit <<= j;
		res |= bit;
		// check if we have to go to the next element
		bitPos--;
		if (bitPos < 0) { bitPos = 7; codeIndex++; }
		i--;
		j--;
	}

	block[0].nBits = numBits;
	block[0].bits = res;

	// encode the value
	huffmanEncodeValue(&block[0], encodedDC.s2, bitSize);
	#ifdef DEBUG_HUFFMAN
		printf("Huffman encoded DC: ");
		printf("(nBits = %d): ", block[0].nBits);
		uint32_t mask2 = 0;
		for (i = 32 - 1; i >= 0; i--){
			mask2 = 1;
			mask2 <<= i;
			bit = block[0].bits & mask2;
			printf("%d", (bit) ? 1 : 0);
		}
		printf("\n");
	#endif
}

// applies huffman encoding to luminance AC coefficients
void ACHuffmanEncode(Symbol *encodedBlock, HuffSymbol *block, int component)
{
	int i = 0, j = 0;
	int c = 0; // current coefficient
	int codeIndex = 0, bitPos = 0;
	int totalBits = 0, bitsToExtract = 0, numBits = 0;
	int runL = 0, size = 0;

	uint64_t mask = 0, bit = 0;
	uint32_t res = 0;

	for (c = 1; c < NUM_COEFFICIENTS; c++){ // loop through each coefficient
		// extract the run length and the bit size
		totalBits = 0;
		res = 0;
		runL = (unsigned int) (encodedBlock[c].s1 >> 4);
		size = (unsigned int) (encodedBlock[c].s1 - (runL << 4));
		#ifdef DEBUG_HUFFMAN
			printf("c = %d and runL = %u and size = %u\n", c, runL, size);
		#endif

		// calculate the # bits we have to loop through	to find the code we want
		for (i = 0; i < runL; i++){
			for (j = 0; j < 11; j++){
					totalBits += (AC_nbits[component])[i][j];
			}
		}

		for (i = 0; i < size; i++){
			totalBits += (AC_nbits[component])[runL][i];
		}

		// now extract the correct huffman code to represent this value
		codeIndex = totalBits / NUM_COEFFICIENTS;
		bitPos = (NUM_COEFFICIENTS - 1) - (totalBits % NUM_COEFFICIENTS);
		bitsToExtract = numBits = (AC_nbits[component])[runL][size]; // get the length of the huffman code
		#ifdef DEBUG_HUFFMAN
			if (runL == 0 && size == 0){
				printf("[DEBBUGING EOB]\n");
				printf("bitsToExtract = %d and totalBits = %d, bitPos = %d and codeIndex = %d\n", bitsToExtract, totalBits, bitPos, codeIndex);
			}
		#endif
		j = 31; // counter to keep track of the bits in the 32 bit storage space
		while (bitsToExtract > 0){
			mask = 1;
			mask <<= bitPos;
			bit = (mask & AC_codes[component][codeIndex]) ? 1 : 0;
			bit <<= j;
			res |= bit;
			bitPos--;
			if (bitPos < 0){
				bitPos = NUM_COEFFICIENTS - 1;
				codeIndex++;
			}
			j--;
			bitsToExtract--;
		}

		block[c].nBits = numBits;
		block[c].bits = res;
		if (!(runL == 0 && size == 0)){ // don't encode a zero
			huffmanEncodeValue(&block[c], encodedBlock[c].s2, size);
		}

		else{
			break; // reached EOB so just break
		}
	}

	#ifdef DEBUG_HUFFMAN
	for (c = 1; c < NUM_COEFFICIENTS; c++){
		res = block[c].bits;
		printf("Coefficient: %2d (nBits = %d): ", c, block[c].nBits);
		for (i = 32- 1; i >= 0; i--){
			mask = 1;
			mask <<= i;
			bit = mask & res;
			printf("%d", (bit) ? 1 : 0);
		}
		printf("\n");
	}
	#endif
}

void huffmanEncodeValue(HuffSymbol *huffCoeff, int value, int bitSize)
{
	int bitPos = 0, minValue = 0;
	int i = 0;
	uint32_t bit = 0, mask = 0, additionalBits = 0;
	uint32_t bit2 = 0, mask2 = 0, bitsToClear = 0, bitsToAdd = 0;

	if (value > 0){
		minValue = (int) pow(2, bitSize - 1);
		additionalBits |= (uint32_t) minValue;
	}

	else{
		minValue = (int) pow(2, bitSize) * (-1) + 1;
	}
	// determine the "additional bits" associated with the value
	mask = 1;
	#ifdef DEBUG_HUFFMAN
	printf("Size of value: %d and Min value: %d and value = %d\n", bitSize, minValue, value);
	#endif
	while (minValue < value){ // loop until additional bits represents our value
		bit = additionalBits & mask; // extract rightmost bit
		if (bit == 1){
			// shift the leftmost '1' from the right left and clear everything behind it
			mask2 = 1;
			bit2 = additionalBits & mask2;
			i = 0;
			bitsToClear = 0;
			while (bit2){ // loop until a '0' bit is reached
				mask2 <<= 1;
				bitsToClear += (uint32_t) pow(2, i);
				bit2 = additionalBits & mask2;
				i++;
			}
			bitsToAdd = (uint32_t) pow(2, i);
			additionalBits -= bitsToClear;
			additionalBits += bitsToAdd;
		}

		else if (bit == 0){
			// make the rightmost bit a '1'
			additionalBits |= mask;
		}

		else{
			printf("Error: Bit is neither 1 nor 0.\n");
		}

		minValue++;
	}

	// printf("Additionalbits = %u\n", additionalBits);

	// copy the additional bits into huffCoeff
	bitPos = huffCoeff->nBits + 1;
	i = bitSize - 1;
	while (i >= 0){
		mask = 1;
		mask <<= i;
		bit = (additionalBits & mask) ? 1 : 0;
		// printf("bit = %d\n", bit);
		bit <<= (32 - bitPos);
		huffCoeff->bits |= bit;
		i--;
		bitPos++;
	}
	huffCoeff->nBits += bitSize;
}

// produces a JPEG binary file
void writeToFile(JpgData jDat, const char *fileName)
{
	FILE *fp = fopen(fileName, "wb");
	//char *c = "MatthewT53's Jpeg encoder";

	// write JPEG data into
	if (fp != NULL){
		writeSOI(fp);
		writeAPP0(fp);
		writeDQT(fp, jDat);
		writeFrameHeader(fp, jDat);
		writeDHT(fp, jDat);
		writeScanHeader(fp, jDat);
		writeScanData(fp, jDat);
		// writeComment(fp, c, (short) strlen(c));
		writeEOI(fp);

		fclose(fp);
	}

	else{
		printf("Unable to create file for writing.\n");
	}
}

// write the SOI marker into the file
void writeSOI(FILE *fp)
{
	writeByte(fp, FIRST_MARKER);
	writeByte(fp, SOI_MARKER);
}

// write the JFIF standard APP0 into the jpeg image
void writeAPP0(FILE *fp)
{
	// standard 2-byte marker (0xFF, 0xE0)
	writeByte(fp, FIRST_MARKER);
	writeByte(fp, APP0_MARKER);

	// length of APP0
	writeByte(fp, 0x00);
	writeByte(fp, 0x10);

	// write JFIF
	writeByte(fp, 0x4A); // J
	writeByte(fp, 0x46); // F
	writeByte(fp, 0x49); // I
	writeByte(fp, 0x46); // F
	writeByte(fp, 0x00); // terminating null char

	// write the JFIF version, 1.02
	writeByte(fp, 0x01);
	writeByte(fp, 0x02);

	// dots/inch
	writeByte(fp, 0x01);

	// write thumbnail width
	writeByte(fp, 0x00);
	writeByte(fp, 0x01);

	// write thumbnail height
	writeByte(fp, 0x00);
	writeByte(fp, 0x01);

	// write # pixels for thumbnail in x direction
	writeByte(fp, 0x00);
	writeByte(fp, 0x00); // y direction
}

void writeDQT(FILE *fp, JpgData jDat)
{
	Byte PT = 0;
	Byte b = 0, des = 0;
	int i = 0;
	// write the marker for this segment
	writeByte(fp, FIRST_MARKER);
	writeByte(fp, QUAN_MARKER);

	// length of this segment (67 bytes)
	writeByte(fp, 0x00);
	writeByte(fp, 0x43);

	// PT
	b <<= 4;
	b += des;
	PT |= b;

	writeByte(fp, PT);

	// quantization coefficients for luminance
	for (i = 0; i < NUM_COEFFICIENTS; i++){
		writeByte(fp, jDat->DQT_Y_zz[i]);
	}

	// store the chrominance QT
	writeByte(fp, FIRST_MARKER);
	writeByte(fp, QUAN_MARKER);

	// length
	writeByte(fp, 0x00);
	writeByte(fp, 0x43);
	b = 0;
	b <<= 4;
	des = 1;
	b += des;
	PT |= b;

	// quantization coefficients for chrominance
	writeByte(fp, PT);
	for (i = 0; i < NUM_COEFFICIENTS; i++){
		writeByte(fp, jDat->DQT_CHR_zz[i]);
	}
}

void writeFrameHeader(FILE *fp, JpgData jDat)
{
	Byte h[2], w[2];
	short height = (short) jDat->height, width = (short) jDat->width; // should this be totalHeight i.e including the filling?
	int i = 0;
	Byte sampleLum = 0x00; // Hi: 0001, Vi: 0001

	/*	Only need to take into account the sampling factor for luminance since for
		all the supported subsampling rates, the chromiance components only occur once in each MCU */

	// determine the sampling factor for all components
	switch (jDat->ratio){
		case NO_CHROMA_SUBSAMPLING: sampleLum = 0x11; break;
		case HORIZONTAL_SUBSAMPLING: sampleLum = 0x21; break;
		case HORIZONTAL_VERTICAL_SUBSAMPLING: sampleLum = 0x22; break;
	}

	// frame header marker
	writeByte(fp, FIRST_MARKER);
	writeByte(fp, SOF0_MARKER);

	// frame length
	writeByte(fp, 0x00);
	writeByte(fp, 0x11);

	// bit precision
	writeByte(fp, 0x08);

	// height of image
	memcpy(h, &height, 2);
	writeByte(fp, h[1]);
	writeByte(fp, h[0]);

	// width of image
	memcpy(w, &width, 2);
	writeByte(fp, w[1]);
	writeByte(fp, w[0]);

	// # components
	writeByte(fp, 0x03);

	for (i = 0; i < 3; i++){
		writeByte(fp, (Byte) (i + 1)); // component ID
		writeByte(fp, (i == 0) ? sampleLum : 0x11); // sampling factors
		writeByte(fp, (i == 0) ? 0x00 : 0x01); // Selects which quantization table to use for a component
	}
}

void writeDHT(FILE *fp, JpgData jDat)
{
	short length = 0;
	Byte l[2], t = 0;
	int i = 0;
	int DCLum_nr_size = sizeof(DCHuffmanLum_nr), DCLum_values_size = sizeof(DCHuffmanLumValues);
	int ACLum_nr_size = sizeof(ACHuffmanLum_nr), ACLum_values_size = sizeof(ACHuffmanLumValues);
	int DCChr_nr_size = sizeof(DCHuffmanChr_nr), DCChr_values_size = sizeof(DCHuffmanChrValues);
	int ACChr_nr_size = sizeof(ACHuffmanChr_nr), ACChr_values_size = sizeof(ACHuffmanChrValues);

	// DHT marker
	writeByte(fp, FIRST_MARKER);
	writeByte(fp, HUFFMAN_MARKER);

	/* DC Luminance */
	// length
	length = 2 + DCLum_nr_size + DCLum_values_size;
	memcpy(l, &length, 2);
	writeByte(fp, l[1]);
	writeByte(fp, l[0]);

	// write T
	writeByte(fp, t);

	// write Lengths
	for (i = 1; i < DCLum_nr_size; i++){
		writeByte(fp, DCHuffmanLum_nr[i]);
	}

	// write values
	for (i = 0; i < DCLum_values_size; i++){
		writeByte(fp, DCHuffmanLumValues[i]);
	}

	/* AC Luminance */
	writeByte(fp, FIRST_MARKER);
	writeByte(fp, HUFFMAN_MARKER);
	length = 2 + ACLum_nr_size + ACLum_values_size;
	memcpy(l, &length, 2);
	writeByte(fp, l[1]);
	writeByte(fp, l[0]);

	t = 0x10;
	writeByte(fp, t);

	for (i = 1; i < ACLum_nr_size; i++){
		writeByte(fp, ACHuffmanLum_nr[i]);
	}

	for (i = 0; i < ACLum_values_size; i++){
		writeByte(fp, ACHuffmanLumValues[i]);
	}

	/* DC Chrominance */
	writeByte(fp, FIRST_MARKER);
	writeByte(fp, HUFFMAN_MARKER);

	length = 2 + DCChr_nr_size + DCChr_values_size;
	memcpy(l, &length, 2);
	writeByte(fp, l[1]);
	writeByte(fp, l[0]);

	// write T
	t = 0x01;
	writeByte(fp, t);

	// write Lengths
	for (i = 1; i < DCChr_nr_size; i++){
		writeByte(fp, DCHuffmanChr_nr[i]);
	}

	// write values
	for (i = 0; i < DCChr_values_size; i++){
		writeByte(fp, DCHuffmanChrValues[i]);
	}

	/* AC Chrominance */
	writeByte(fp, FIRST_MARKER);
	writeByte(fp, HUFFMAN_MARKER);
	length = 2 + ACChr_nr_size + ACChr_values_size;
	memcpy(l, &length, 2);
	writeByte(fp, l[1]);
	writeByte(fp, l[0]);

	t = 0x11;
	writeByte(fp, t);

	for (i = 1; i < ACChr_nr_size; i++){
		writeByte(fp, ACHuffmanChr_nr[i]);
	}

	for (i = 0; i < ACChr_values_size; i++){
		writeByte(fp, ACHuffmanChrValues[i]);
	}
}

void writeScanHeader(FILE *fp, JpgData jDat)
{
	int i = 0;
	// scan marker
	writeByte(fp, FIRST_MARKER);
	writeByte(fp, SCAN_MARKER);

	// length
	writeByte(fp, 0x00);
	writeByte(fp, 0x0c);

	// # components
	writeByte(fp, 0x03);

	// info for each component
	for (i = 1; i <= 3; i++){
		writeByte(fp, (Byte) i);
		writeByte(fp, (i == 1) ? 0x00 : 0x11);
	}

	// some other stuff
	writeByte(fp, 0x00); // Ss
	writeByte(fp, 0x3F); // Se
	writeByte(fp, 0x00); // Ah + Al
}

void writeScanData(FILE *fp, JpgData jDat)
{
	Byte b = 0xFF;
	int bitPos = 7;
	int curBlock = 0;
	// i = increment for the luminance index
	// j = increment for the # blocks written to the file
	int i = 0, j = 0, k = 2;
	int numBlocksWidth = jDat->YWidth / 8;
	int indexNextLine = 0;

	printf("Num blocks width: %d\n", numBlocksWidth);
	printf("Num blocks Cb: %d\n", jDat->numBlocksCb);

	// write all the MCU'S into the JPEG file
	while (curBlock < jDat->numBlocksCb){
		printf("Writing Lum Block (1): %d\n", i);
		writeBlockData(fp, jDat->huffmanY[i++], &b, &bitPos);
		// write this block if for 4:2:2 or 4:2:0 compression
		if (jDat->ratio >= HORIZONTAL_SUBSAMPLING){
			printf("Writing Lum Block (2): %d\n", i);
			writeBlockData(fp, jDat->huffmanY[i++], &b, &bitPos);
		}

		if (jDat->ratio == HORIZONTAL_VERTICAL_SUBSAMPLING){
			// write the MCU's from the same x position just from the next line
			j = i - 2;	

			if (j + numBlocksWidth >= jDat->numBlocksY){
				indexNextLine = j;
			}

			else{
				indexNextLine = j + numBlocksWidth;
			}
	
			printf("Writing Lum Block (3): %d\n", indexNextLine);
			writeBlockData(fp, jDat->huffmanY[indexNextLine], &b, &bitPos);
			printf("Writing Lum Block (4): %d\n", indexNextLine + 1);
			writeBlockData(fp, jDat->huffmanY[indexNextLine + 1], &b, &bitPos);
			if (i % numBlocksWidth == 0){
				printf("k = %d\n", k);
				i = numBlocksWidth * k; // need to fix this up
				k += 2;
			}
		}

		printf("Writing Cb Block: %d\n", curBlock);
		writeBlockData(fp, jDat->huffmanCb[curBlock], &b, &bitPos);
		printf("Writing Cr Block: %d\n", curBlock);
		writeBlockData(fp, jDat->huffmanCr[curBlock], &b, &bitPos);
		curBlock++;
	}

	// write last byte
	if (b != 0xFF){
		writeByte(fp, b);
	}
}

// bitPos is used to keep track of the bits we are trying to store
void writeBlockData(FILE *fp, HuffSymbol *block, Byte *b, int *bitPos)
{
	int length = 0, bitPos2 = 0; // bitPos2 is for the bits we are extracting
	uint32_t codeValue = 0, mask = 0, bit = 0, bitV = 0;
	int i = 0;

	while (block[i].nBits != 0){ // finish after writing EOB
		assert(i < 64);
		length = block[i].nBits;
		codeValue = block[i].bits;
		bitPos2 = 32 - 1;
		while (length > 0){
			mask = 1;
			mask <<= bitPos2;
			bit = (mask & codeValue) ? 1 : 0;
			if (bit == 0){
				bitV = 1;
				bitV <<= (*bitPos);
				bitV = ~bitV;
				(*b) &= bitV;
			}

			else{
				bitV = bit << (*bitPos);
				(*b) |= bitV;
			}

			length--;
			(*bitPos) = (*bitPos) - 1;
			bitPos2--;
			// determine whether to write the byte to file
			if (*bitPos < 0){
				*bitPos = 7;
				if ((*b) == 0xFF){ // byte stuffing
					writeByte(fp, 0xFF);
					writeByte(fp, 0x00);
				}

				else{
					writeByte(fp, (*b));
					#ifdef DEBUG_HUFFMAN
						Byte mask2 = 0, bit2 = 0;
						int j = 0;
						for (j = 7; j >= 0; j--){
							mask2 = 1;
							mask2 <<= j;
							bit2 = (mask2 & (*b));
							printf("%d", (bit2) ? 1 : 0);
						}
						printf("\n");
					#endif
				}
				(*b) = 0xFF;
			}
		}

		i++;
	}
}

void writeComment(FILE *fp, char *comment, short size)
{
	short i = 0;
	Byte length[2];

	writeByte(fp, FIRST_MARKER);
	writeByte(fp, COMMENT_MARKER);

	// length
	memcpy(length, &size, 2);
	writeByte(fp, length[1]);
	writeByte(fp, length[0]);

	// copy comment into jpeg
	for (i = 0; i < size; i++){
		writeByte(fp, comment[i]);
	}
}

void writeEOI(FILE *fp)
{
	writeByte(fp, FIRST_MARKER);
	writeByte(fp, EOI_MARKER);
}

// writes a single byte into a file
void writeByte(FILE *fp, Byte b)
{
	fwrite(&b, sizeof(Byte), 1, fp);
}

// free resources
void disposeJpgData(JpgData jdata)
{
	int i = 0;
	// free the YCbCr data
	free(jdata->Y);
	free(jdata->Cb);
	free(jdata->Cr);

	// free YCbCr and quantization data
	for (i = 0; i < jdata->YHeight; i++){
		free(jdata->YBlocks[i]);
		free(jdata->quanY[i]);
	}

	for (i = 0; i < jdata->CbHeight; i++){
		free(jdata->CbBlocks[i]);
		free(jdata->quanCb[i]);
	}

	for (i = 0; i < jdata->CrHeight; i++){
		free(jdata->CrBlocks[i]);
		free(jdata->quanCr[i]);
	}

	// free entropy encoding information
	for (i = 0; i < jdata->numBlocksY; i++){
		free(jdata->huffmanY[i]);
		free(jdata->encodeY[i]);
		free(jdata->zzY[i]);
	}

	for (i = 0; i < jdata->numBlocksCb; i++){
		free(jdata->huffmanCb[i]);
		free(jdata->encodeCb[i]);
		free(jdata->zzCb[i]);
	}

	for (i = 0; i < jdata->numBlocksCr; i++){
		free(jdata->huffmanCr[i]);
		free(jdata->encodeCr[i]);
		free(jdata->zzCr[i]);
	}

	// free everything else
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
