/*
	Name: Matthew Ta
	Date: 29/12/2015
	Description: Interface for the jpeg encoder
*/

#ifndef JPG_ENC
#define JPG_ENC

typedef unsigned char Colour;
typedef struct _pixel *Pixel;
typedef struct _jpegData *JpgData;
typedef char Byte;

typedef struct _pixel{ // RGB pixel
	Colour r; // red
	Colour g; // green
	Colour b; // blue
} pixel;

// converts a bmp image into RGB pixels
// remember to free the Pixel object returned
Pixel imageToRGB(const char *imageName, int *bufSize); 
void disposePixels(Pixel p); // use this function to free the Pixel data

// given an array of RGB pixels, this function creates a jpg image and writes it to the disk
void encodeRGBToJpgDisk(const char *jpgFile, Pixel rgbBuffer, unsigned int numPixels, unsigned int width, unsigned int height);

// given an array of RGB pixels, this function does the same as above instead holds the jpeg in memory
// remember to free the jpeg object
Byte *encodeRGBToJpgMem(Pixel rgbBuffer, unsigned int numPixels, unsigned int width, unsigned int height);
void disposeJpeg(); // use this function to free the jpeg image in memory

#endif
