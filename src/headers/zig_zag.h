#ifndef ZIG_ZAG_H
#define ZIG_ZAG_H

#include "jpg_encode.h"
#include "block.h"

// groups pixel data in a zig-zag formation
void zig_zag(JpgData j_data);

// groups the pixel data of a single block in zig-zag formation
void zig_zag_block(Block b, int *zz);

#endif
