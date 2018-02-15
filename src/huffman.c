#include <stdio.h>
#include <stdlib.h>

#include "headers/huffman.h"
#include "headers/block.h"

// calculates frequencies for a block of image data
void calculate_freq_block_DC(HuffmanData *huffman_data, int *image_data);

// performs run length encoding on the AC coefficients
void calculate_freq_block_AC(HuffmanData *huffman_data, int *image_data);

// intializes huffman data structures
void initialize_huffman(JpgData j_data);

void huffman_encode(JpgData j_data)
{
    int i = 0;

    initialize_huffman(j_data);

    for (i = 0; i < j_data->num_blocks_Y; i++){
        calculate_freq_block_DC(&j_data->lum_DC, j_data->zig_zag_Y[i]);
        calculate_freq_block_AC(&j_data->lum_AC, j_data->zig_zag_Y[i]);
    }

    for (i = 0; i < j_data->num_blocks_Cb; i++){
        calculate_freq_block_DC(&j_data->chrom_DC, j_data->zig_zag_Cb[i]);
        calculate_freq_block_AC(&j_data->chrom_AC, j_data->zig_zag_Cb[i]);
    }

    for (i = 0; i < j_data->num_blocks_Cr; i++){
        calculate_freq_block_DC(&j_data->chrom_DC, j_data->zig_zag_Cr[i]);
        calculate_freq_block_AC(&j_data->chrom_AC, j_data->zig_zag_Cr[i]);
    }
}

void initialize_huffman(JpgData j_data)
{
    // reserve one code point
    j_data->lum_DC.freq[256] = j_data->lum_AC.freq[256] = j_data->chrom_DC.freq[256] = j_data->chrom_AC.freq[256] = 1;

    int i = 0;

    for (i = 0; i < 256; i++){
        j_data->lum_DC.freq[i] = j_data->lum_AC.freq[i] = j_data->chrom_DC.freq[i] = j_data->chrom_AC.freq[i] = 0;
    }

    for (i = 0; i <= 256; i++){
        j_data->lum_DC.code_len[i] = j_data->lum_AC.code_len[i] = j_data->chrom_DC.code_len[i] = j_data->chrom_AC.code_len[i] = 0;
        j_data->lum_DC.others[i] = j_data->lum_AC.others[i] = j_data->chrom_DC.others[i] = j_data->chrom_AC.others[i] = -1;
    }
}

void calculate_freq_block(HuffmanData *huffman_data, int *image_data)
{

}
