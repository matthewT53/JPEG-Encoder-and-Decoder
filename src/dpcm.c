#include <stdio.h>
#include <stdlib.h>

#include "headers/dpcm.h"

void dpcm(JpgData j_data)
{
    int i = 0;

    for (i = 1; i < j_data->num_blocks_Y; i++){
        j_data->zig_zag_Y[i][0] = j_data->zig_zag_Y[i][0] - j_data->zig_zag_Y[i-1][0];
    }

    for (i = 1; i < j_data->num_blocks_Cb; i++){
        j_data->zig_zag_Cb[i][0] = j_data->zig_zag_Cb[i][0] - j_data->zig_zag_Cb[i-1][0];
    }

    for (i = 1; i < j_data->num_blocks_Cr; i++){
        j_data->zig_zag_Cr[i][0] = j_data->zig_zag_Cr[i][0] - j_data->zig_zag_Cr[i-1][0];
    }
}
