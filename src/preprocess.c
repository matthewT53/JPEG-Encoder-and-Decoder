#include <stdio.h>
#include <stdlib.h>

#define "jpgEncode.h"
#include "preprocess.h"
#include "bitmap.h"
#include "block.h"

// #define DEBUG_PRE

// helper functions
void add_padding(JpgData j_data);
void convert_rgb_YCbCr(JpgData j_data);
void level_shift(JpgData j_data);

void determine_resolutions(JpgData j_data, int *num_extra_bytes_width, int *num_extra_bytes_height);
void store_rgb_values(JpgData j_data, Byte *pixels, int num_bytes, int extra_width, int extra_height);

void preprocess_jpeg(JpgData j_data)
{
    BmpImage b = NULL;
    Byte *pixels = NULL;
    int size = 0;
    int extra_width = 0, extra_height = 0;
    int i = 0;

    // get the array of pixels
    b = bmp_OpenBitmap(j_data->filename);

    if ( b != NULL ){
        pixels = bmp_GetColourData(b, &size);

        j_data->width = bmp_GetWidth(b);
        j_data->height = bmp_GetHeight(b);

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

        for (i = 0; i < j_data->num_blocks_Y; i++){
            j_data->Y[i]  = newBlock();
            j_data->Cb[i] = newBlock();
            j_data->Cr[i] = newBlock();
        }

        if ( pixels != NULL ){
            store_rgb_values(j_data, pixels, extra_width, extra_height);
            convert_rgb_YCbCr(j_data);
            level_shift(j_data);
        }
    }

    else{
        bmp_GetLastError(b);
    }
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

void store_rgb_values(JpgData j_data, Byte *pixels, int num_bytes, int extra_width, int extra_height)
{
    int i = 0;
    int original_width = 0, original_height = 0;

    original_width  = j_data->width - extra_width;
    original_height = j_data->height - extra_height;

    for ( i = 0; i < j_data->num_blocks_Y; i++ ){
        blockToCoords()
    }
}
