#include <stdio.h>
#include <stdlib.h>

#include "headers/jpgEncode.h"
#include "headers/preprocess.h"
#include "headers/block.h"

/* ======================================= Checklist ============================== */

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

/* ===================================== End of checklist ======================= */

/* ===================================== Small helper functions ================================== */
JpgData create_jpeg_data(void);

/* ==================================== Function definitions ===================================== */

void encode_bmp_to_jpeg(const char *input, const char *output, int quality, int sample_ratio)
{
	JpgData j_data = NULL;

	j_data = create_jpeg_data();

	if (j_data != NULL){
		j_data->sample_ratio = sample_ratio;
		j_data->quality = quality;
		j_data->output_filename = (char *) output;
		j_data->input_filename =  (char *) input;

		// convert RGb to YCbCr
		preprocess_jpeg(j_data);

		// downsample the image
		// chroma_subsample(j_data);
	}
}

JpgData create_jpeg_data(void)
{
	JpgData j_data = malloc(sizeof(JpegData));
	return j_data;
}
