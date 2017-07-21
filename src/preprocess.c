#include <stdio.h>
#include <stdlib.h>

#define "jpgEncode.h"
#include "preprocess.h"
#include "bitmap.h"
#include "block.h"

// #define DEBUG_PRE

// helper functions

// determines resolutions of each colour component based on compression settings (e.g sampling, )
void determine_resolutions(JpgData j_data, int *num_extra_bytes_width, int *num_extra_bytes_height);

// converts the RGB values into YUV values
void convert_blocks(JpgData j_data, BmpImage bmp, int extra_width, int extra_height);

// levels shifts each block
void level_shift(JpgData j_data);

// converts block number to starting and ending coordinates (x,y)
void blockToCoords(int bn, int *x, int *y, unsigned int w);

void preprocess_jpeg(JpgData j_data, char *input_filename)
{
    BmpImage bmp = NULL;
    int extra_width = 0, extra_height = 0;
    int i = 0;

    // get the array of pixels
    bmp = bmp_OpenBitmap(input_filename);

    if ( bmp != NULL ){
        j_data->width = bmp_GetWidth(bmp);
        j_data->height = bmp_GetHeight(bmp);

        // determine resolutions
        determine_resolutions(j_data, &extra_width, &extra_height);

        // allocate memory for blocks (mcus)
        j_data->num_blocks_Y = (j_data->width / 8) * (j_data->height / 8);
        j_data->num_blocks_Cb = (j_data->width / 8) * (j_data->height / 8);
        j_data->num_blocks_Cr = (j_data->width / 8) * (j_data->height / 8);

        // store all the RGB information
        j_data->Y  = malloc(sizeof(Block) * j_data->num_blocks_Y);
        j_data->Cb = malloc(sizeof(Block) * j_data->num_blocks_Cb);
        j_data->Cr = malloc(sizeof(Block) * j_data->num_blocks_Cr);

        // create all the blocks, mcus
        for (i = 0; i < j_data->num_blocks_Y; i++){
            j_data->Y[i]  = newBlock();
            j_data->Cb[i] = newBlock();
            j_data->Cr[i] = newBlock();
        }

        convert_blocks(j_data, bmp, extra_width, extra_height);
        level_shift(j_data);
    }

    else{
        bmp_GetLastError(bmp);
    }

    bmp_DestroyBitmap(bmp);
}

void determine_resolutions(JpgData j_data, int *num_extra_bytes_width, int *num_extra_bytes_height)
{
    int w = 0, h = 0;
    int sample = 0;
    int extra_width = 0, extra_height = 0;

    w = j_data->width;
    h = j_data->height;
    sample = j_data->sample_ratio;

    if ( sample == NO_CHROMA_SUBSAMPLING ){
        *num_extra_bytes_width = extra_width = w % 8;
        *num_extra_bytes_height = extra_height = h % 8;
    }

    else if ( sample == HORIZONTAL_SUBSAMPLING ){
        *num_extra_bytes_width = extra_width = w % 16;
        *num_extra_bytes_height = extra_height = h % 8;
    }

    else if ( sample == HORIZONTAL_VERTICAL_SUBSAMPLING ){
        *num_extra_bytes_width = extra_width = w % 16;
        *num_extra_bytes_height = extra_height = h % 16;
    }

    j_data->width += extra_width;
    j_data->height += extra_height;
}

void convert_blocks(JpgData j_data, BmpImage bmp, int extra_width, int extra_height)
{
    int i = 0, j = 0;
    int original_width = 0, original_height = 0;
    Byte *r = NULL, *b = NULL, *g = NULL;
    Byte *r_new = NULL, *b_new = NULL, *g_new = NULL;
    int original_num_pixels = 0;
    int num_pixels_new = 0;
    int x_coords[2] = {0}, y_coords[2] = {0};
    int x = 0, y = 0;
    int offset = 0;
    float y_value = 0.0, cb_value = 0.0, cr_value = 0.0;

    // get the RGB colour channels
    r = bmp_GetRed(bmp);
    g = bmp_GetGreen(bmp);
    b = bmp_GetBlue(bmp);

    bmp_GetLastError(bmp);

    // convert RGB => YUV
    original_width      = j_data->width - extra_width;
    original_height     = j_data->height - extra_height;
    original_num_pixels = bmp_GetNumPixels(bmp);

    // create new RGB buffers to accomodate for resolution changes
    num_pixels_new = j_data->width * j_data->height;
    r_new = malloc(sizeof(Byte) * num_pixels_new);
    g_new = malloc(sizeof(Byte) * num_pixels_new);
    b_new = malloc(sizeof(Byte) * num_pixels_new);

    for ( i = 0; i < original_num_pixels; i++ ){
        r_new[i] = r[i];
        g_new[i] = g[i];
        b_new[i] = b[i];

        if ( extra_width ){
            for ( j = 1; j <= extra_width; j++ ){
                r_new[i + j] = r_new[i];
                g_new[i + j] = g_new[i];
                b_new[i + j] = b_new[i];
            }
        }
    }

    // fill in the remaining pixels
    if ( example_height ){
        for ( i = original_num_pixels; i < num_pixels_new; i++ ){
            r_new[i] = r_new[original_num_pixels - 1];
            g_new[i] = g_new[original_num_pixels - 1];
            b_new[i] = b_new[original_num_pixels - 1];
        }
    }

    // fill in the blocks
    for ( i = 1; i <= j_data->num_blocks_Y; i++ ){
        blockToCoords(i, x_coords, y_coords, j_data->width);
        for ( y = 0; y < 8; y++ ){
            for ( x = 0; x < 8; x++ ){
                offset   = ((y + y_coords[0]) * original_width) + (x_coords[0] + x);
                y_value  = 0.299 * r + 0.587 * g + 0.114 * b;
                cb_value = 128 - (0.168736 * r - 0.331264 * g + 0.5 * b);
                cr_value = 128 + (0.5 * r - 0.418688 * g - 0.081312 * b);

                setValueBlock(b, x, y, y_value);
                setValueBlock(b, x, y, cb_value);
                setValueBlock(b, x, y, cr_value);
            }
        }
    }

    free(r_new);
    free(g_new);
    free(b_new);
}

void levelShift(JpgData j_data)
{
    int i = 0, n = 0;
    int x = 0, y = 0;
    float new_Y = 0.0, new_Cb = 0.0, new_Cr = 0.0;

    n = j_data->num_blocks_Y
    for (i = 0; i < n; i++){
        for (y = 0; y < 8; y++){
            for (x = 0; x < 8; x++){
                new_Y  = getValueBlock(j_data->Y[i], x, y) - 128;
                new_Cb = getValueBlock(j_data->Cb[i], x, y) - 128;
                new_Cr = getValueBlock(j_data->Cr[i], x, y) - 128;

                setValueBlock(j_data->Y[i], x, y, new_Y);
                setValueBlock(j_data->Cb[i], x, y, new_Cb);
                setValueBlock(j_data->Cr[i], x, y, new_Cr);
            }
        }
    }
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
	// if (x[0] == -8) { x[0] = w - 8; } // in last column, maths doesn't make sense, need to do something weird
	x[1] = x[0] + 8;
}
