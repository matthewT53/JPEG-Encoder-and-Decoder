#ifndef DCT_H
#define DCT_H

#include "jpg_encode.h"
#include "block.h"

// performs a discrete cosine transformation on the YUV data
void dct(JpgData j_data);

void dct_block(Block b);

#endif
