#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "headers/quantise.h"

// default jpeg quantization matrix for 50% quality (luminance)
int q_table_lum[TABLE_SIZE][TABLE_SIZE] = {{16, 11, 10, 16, 24, 40, 51, 61},
											 {12, 12, 14, 19, 26, 58, 60, 55},
											 {14, 13, 16, 24, 40, 57, 69, 56},
											 {14, 17, 22, 29, 51, 87, 80, 62},
											 {18, 22, 37, 56, 68, 109, 103, 77},
											 {24, 35, 55, 64, 81, 104, 113, 92},
											 {49, 64, 78, 87, 103, 121, 120, 101},
											 {72, 92, 95, 98, 112, 100, 103, 99}};

// matrix for chrominance
int q_table_chr[TABLE_SIZE][TABLE_SIZE] = {{17, 18, 24, 47, 99, 99, 99, 99},
											 {18, 21, 26, 66, 99, 99, 99, 99},
											 {24, 26, 56, 99, 99, 99, 99, 99},
											 {47, 66, 99, 99, 99, 99, 99, 99},
											 {99, 99, 99, 99, 99, 99, 99, 99},
											 {99, 99, 99, 99, 99, 99, 99, 99},
											 {99, 99, 99, 99, 99, 99, 99, 99},
									         {99, 99, 99, 99, 99, 99, 99, 99}};

void scale_table(int q_table[TABLE_SIZE][TABLE_SIZE], int quality);

void quantise(JpgData j_data)
{
    int i = 0;

	scale_table(q_table_lum, j_data->quality);
	scale_table(q_table_chr, j_data->quality);

    // quantise the luninance components
    for (i = 0; i < j_data->num_blocks_Y; i++){
        quantise_lum(j_data->Y[i]);
    }

    // quantise the chrominance components
    for (i = 0; i < j_data->num_blocks_Cb; i++){
        quantise_chr(j_data->Cb[i]);
    }

    for (i = 0; i < j_data->num_blocks_Cr; i++){
        quantise_chr(j_data->Cr[i]);
    }
}

void quantise_lum(Block b)
{
    int i = 0, j = 0;

    for (i = 0; i < 8; i++){
        for (j = 0; j < 8; j++){
            set_value_block(b, i, j, round( get_value_block(b, i, j) / q_table_lum[i][j] ));
        }
    }
}

void quantise_chr(Block b)
{
    int i = 0, j = 0;

    for (i = 0; i < 8; i++){
        for (j = 0; j < 8; j++){
            set_value_block(b, i, j, round( get_value_block(b, i, j) / q_table_chr[i][j] ));
        }
    }
}

void scale_table(int q_table[TABLE_SIZE][TABLE_SIZE], int quality)
{
	int i = 0, j = 0;
	int s = 0, Ts = 0;

	for (i = 0; i < TABLE_SIZE; i++){
		for (j = 0; j < TABLE_SIZE; j++){
			s = (quality < 50) ? 5000/quality : 200 - 2*quality;
			Ts = (int) floor( (s * q_table[i][j] + 50) / 100);
			q_table[i][j] = Ts;
		}
	}
}
