/*
	Name: Matthew Ta
	Date: 29/12/2015
	Description: Interface for the jpeg encoder
*/

#ifndef JPG_ENC
#define JPG_ENC

// Chroma Subsampling constants
#define NO_CHROMA_SUBSAMPLING 0 // no chroma subsampling
#define HORIZONTAL_SUBSAMPLING 1 // 4:2:2 chroma subsampling
#define HORIZONTAL_VERTICAL_SUBSAMPLING 2 // 4:2:0 chroma subsampling

typedef struct _jpeg_data *JpgData;

/*
	Takes a bmp filename as input and writes JPEG image to disk.

	Input:
		jpgFile: name of jpg file to create
		rgbBuffer: array of Pixels
		numPixels: number of pixels in rgbBuffer i.e sizeof(rgbBuffer)
		width: width of the jpg image to create (x-direction)
		height: height of the image to create (y-direction)
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
void encode_rgb_to_jpeg(Byte *colours, const char *output, int quality, int sample_ratio);


#endif
