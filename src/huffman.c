#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "headers/huffman.h"
#include "headers/block.h"

// intializes huffman data structures
void initialize_huffman(JpgData j_data);

// calculates frequencies for a block of image data
void calculate_freq_block_DC(HuffmanData *huffman_data, int *image_data);

// performs run length encoding on the AC coefficients
void calculate_freq_block_AC(HuffmanData *huffman_data, int *image_data);

// returns the class of a value
int get_class(int value);

// constructs the huffman table for the image data
void construct_huffman_table(HuffmanData *huffman_data);

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

void construct_huffman_table(HuffmanData *huffman_data)
{
    int v1 = -1, v2 = -1;
    int i = 0;

    // find huffman code sizes
    while (1){
        // get an initial value for v1
        for (i = 0; i < 257; i++){
            if (huffman_data->freq[i] > 0){
                v1 = i;
                break;
            }
        }

        // find smallest v1 such that freq[v1] > 0
        for (i = 0; i < 257; i++){
            if (huffman_data->freq[i] > 0 && huffman_data->freq[i] < huffman_data->freq[v1]){
                v1 = i;
            }
        }

        // find next smallest value v2 such that freq[v2] > 0
        for (i = 0; i < 257; i++){
            if (huffman_data->freq[i] > 0 && huffman_data->freq[i] < huffman_data->freq[v2] && v1 != v2){
                v2 = i;
            }
        }

        // v2 doesn't exist so we are done
        if (v2 == -1){
            break;
        }

        huffman_data->freq[v1] += huffman_data->freq[v2];
        huffman_data->freq[v2] = 0;

        huffman_data->code_len[v1]++;

        while ( !huffman_data->others[v1] == -1 ){
            huffman_data->code_len[v1]++;
            v1 = huffman_data->others[v1];
        }

        huffman_data->others[v1] = v2;

        huffman_data->code_len[v2]++;

        while ( !huffman_data->others[v2] == -1 ){
            huffman_data->code_len[v2]++;
            v2 = huffman_data->others[v2];
        }
    }
}

void calculate_freq_block_DC(HuffmanData *huffman_data, int *image_data)
{
    huffman_data->freq[get_class(image_data[0])]++;
}

void calculate_freq_block_AC(HuffmanData *huffman_data, int *image_data)
{
    int i = 0;
    int num_zeroes = 0;
    int last_non_zero_index = 0;

    // find the last non-zero coefficient
    for (i = 63; i > 0; i--){
        if (image_data[i] != 0){
            last_non_zero_index = i;
            break;
        }
    }

    // perform run length encoding
    for (i = 1; i < 64; i++){
        if (image_data[i] == 0){
            num_zeroes++;
            if (num_zeroes == 16){
                huffman_data->freq[0xF0]++; // ZRL
                num_zeroes = 0;
            }
        }

        else if (i == last_non_zero_index + 1){ // +1 because we don't want the coefficient
            huffman_data->freq[0x00]++; // EOB
            break;
        }

        else{
            // run length | code size
            huffman_data->freq[num_zeroes | get_class(image_data[i])]++;
            num_zeroes = 0;
        }
    }
}

int get_class(int value)
{
    int class = 0;
    value = (int) abs(value);

    while (value > 0){
        value >>= 1;
        class++;
    }

    return class;
}
