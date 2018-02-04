#ifndef QUANTISE_H
#define QUANTISE_H

#include "jpgEncode.h"
#include "block.h"

#define TABLE_SIZE 8

void quantise_lum(Block b);
void quantise_chr(Block b);

// perform quantization on he image data
void quantise(JpgData j_data);

#endif
