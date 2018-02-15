/*
	Name: Matthew Ta
	Date: 29/12/2015
	Description: Interface for the jpeg encoder
*/

#ifndef JPG_ENC
#define JPG_ENC

#include "block.h"

// Chroma Subsampling constants
#define NO_CHROMA_SUBSAMPLING 0 // no chroma subsampling
#define HORIZONTAL_SUBSAMPLING 1 // 4:2:2 chroma subsampling
#define HORIZONTAL_VERTICAL_SUBSAMPLING 2 // 4:2:0 chroma subsampling

typedef struct _jpeg_data *JpgData;

typedef unsigned char JpgByte;

typedef struct _huffman_data{
	int freq[257];
	int code_len[257];
} HuffmanData;

typedef struct _jpeg_data{
	// output filename
	char *output_filename;
	char *input_filename;

	// properties of the JPEG image
	int width;
	int height;

	// these variables control the quality of the image
	int sample_ratio;
	int quality;

	// number of blocks in each colour channel
	int num_blocks_Y;
	int num_blocks_Cb;
	int num_blocks_Cr;

	// values of each colour channel
	Block *Y;
	Block *Cb;
	Block *Cr;

	// zig zag data
	int **zig_zag_Y;
	int **zig_zag_Cb;
	int **zig_zag_Cr;

	// huffman encoding data
	HuffmanData lum_DC;
	HuffmanData lum_AC;

	HuffmanData chrom_DC;
	HuffmanData chrom_AC;
} JpegData;

/*
	Takes a bmp filename as input and writes JPEG image to disk.

	Input:
		input: name of the BMP file
		output: name of the JPEG file to create
		quality: a value between 1 and 100 (inclusive). This specifies the quality of the image and effects compression and hence impacts size of the jpg image.
		sampleRatio: refers to how much colour information should be discarded from the JPEG image.
				 Pass in one of the chroma subsampling constants as defiend at the top of this header file.

	Output:
		JPEG image is written to disk.
*/
void encode_bmp_to_jpeg(const char *input, const char *output, int quality, int sample_ratio);

/*
	Takes in an array of RGB values in memory and writes it to a JPEG image on disk.

	Input:
	* colours: array of RGB values
	* output: jpeg file output
	* quality: quality of the image 1 - 100
	* sample_ratio: 4:4:4, 4:2:2, 4:2:0

	Output:
	* A jpeg image built from the RGB colours.
*/
void encode_rgb_to_jpeg(JpgByte *colours, const char *output, int quality, int sample_ratio);


#endif
