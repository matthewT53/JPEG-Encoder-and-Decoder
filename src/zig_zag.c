#include <stdio.h>
#include <stdlib.h>

#include "headers/zig_zag.h"

const int scan_order[8][8] = {
    {0, 1, 5, 6, 14, 15, 27, 28},
    {2, 4, 7, 13, 16, 26, 29, 42},
    {3, 8, 12, 17, 25, 30, 41, 43},
    {9, 11, 18, 24, 31, 40, 44, 53},
    {10, 19, 23, 32, 39, 45, 52, 54},
    {20, 22, 33, 38, 46, 51, 55, 60},
    {21, 34, 37, 47, 50, 56, 59, 61},
    {35, 36, 48, 49, 57, 58, 62, 63}
};

void zig_zag(JpgData j_data)
{
    int i = 0;

    printf("Performing zig zag ordering:\n");

    // construct the zig zag data structures
    j_data->zig_zag_Y = malloc(sizeof(int *) * j_data->num_blocks_Y);
    j_data->zig_zag_Cb = malloc(sizeof(int *) * j_data->num_blocks_Cb);
    j_data->zig_zag_Cr = malloc(sizeof(int *) * j_data->num_blocks_Cr);

    for (i = 0; i < j_data->num_blocks_Y; i++){
        j_data->zig_zag_Y[i] = calloc(64, sizeof(int));
        j_data->zig_zag_Cb[i] = calloc(64, sizeof(int));
        j_data->zig_zag_Cr[i] = calloc(64, sizeof(int));
    }

    // perform zig zag encoding on all blocks
    for (i = 0; i < j_data->num_blocks_Y; i++){
        zig_zag_block(j_data->Y[i], j_data->zig_zag_Y[i]);
    }

    for (i = 0; i < j_data->num_blocks_Cb; i++){
        zig_zag_block(j_data->Cb[i], j_data->zig_zag_Cb[i]);
    }

    for (i = 0; i < j_data->num_blocks_Cr; i++){
        zig_zag_block(j_data->Cr[i], j_data->zig_zag_Cr[i]);
    }
}

void zig_zag_block(Block b)
{
    printf("Zig zag ordering block:\n");
}
