#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "headers/jpgEncode.h"
#include "headers/block.h"
#include "headers/dct.h"

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

#define ALPHA(x) (x == 0 ? 1/sqrt(2) : 1)

// performs the DCT-II algorithm to convert a block from the spatial domain
// into the frequency domain
// void dct_block(Block b);

void dct(JpgData j_data)
{
    int i = 0;

    for (i = 0; i < j_data->num_blocks_Y; i++){
        dct_block(j_data->Y[i]);
    }

    for (i = 0; i < j_data->num_blocks_Cb; i++){
        dct_block(j_data->Cb[i]);
    }

    for (i = 0; i < j_data->num_blocks_Cr; i++){
        dct_block(j_data->Cr[i]);
    }
}

void dct_block(Block b)
{
    int u = 0, v = 0;
    int x = 0, y = 0;
    double dct_value = 0.0;
    Block temp = copy_block(b);

    for (u = 0; u < 8; u++){
        for (v = 0; v < 8; v++){
            dct_value = 0.0;
            for (x = 0; x < 8; x++){
                for (y = 0; y < 8; y++){
                    dct_value += get_value_block(temp, x, y)
                                 * cos( ( (2*x + 1) * u * M_PI ) / 16 )
                                 * cos( ( (2*y + 1) * v * M_PI ) / 16 );
                }
            }

            set_value_block(b, u, v, 0.25 * ALPHA(u) * ALPHA(v) * dct_value);
        }
    }

    destroy_block(temp);
}
