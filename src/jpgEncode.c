#include <stdio.h>
#include <stdlib.h>

#include "jpgEncode.h"
#include "preprocess.h"
#include "block.h"

typedef unsigned char Byte;

typedef struct _jpeg_data{
	// output filename
	char *output_filename;

	// properties of the JPEG image
	int width;
	int height;

	// these variables control the quality of the image
	int sampling_ratio;
	int quality;

	// number of blocks in each colour channel
	int num_blocks_Y;
	int num_blocks_Cb;
	int num_blocks_Cr;

	// values of each colour channel
	Block *Y;
	Block *Cb;
	Block *Cr;

	// blocks to store the chroma
	Block *downsample_Cb;
	Block *downsample_Cr;
} JpegData;

/* ======================================= Main JPEG compression functions ============================== */

// downsamples the image for more compression
void chroma_subsample(JpgData j_data);

// space to frequency transformation
void dct();

// removes redundant information
void quantise();

// reorders the coefficients in the block
void zig_zag();

// encoding of the DC coefficients
void dpcm();

// encoding of the AC coefficients
void run_length();

// main encoding process
void huffman_encoding();

/* ===================================== End of JPEG compression functions ======================= */

/* ===================================== Small helper functions ================================== */
JpgData create_jpeg_data(void);

/* ==================================== Function definitions ===================================== */

void encode_bmp_to_jpeg(const char *input, const char *output, int quality, int sample_ratio)
{
	JpgData j_data = NULL;

	j_data = create_jpeg_data

	if (j_data != NULL){
		j_data->sample_ratio = sample_ratio;
		j_data->quality = quality;
		j_data->output_filename = output;

		// convert RGb to YCbCr
		preprocess_jpeg(j_data, input);

		// downsample the image
		chroma_subsample(j_data);
	}
}
