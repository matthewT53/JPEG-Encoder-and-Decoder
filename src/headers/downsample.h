/*
    This file contains functions for downsampling an image

    Written by: Matthew Ta
*/

#ifndef DOWN_SAMPLE_H
#define DOWN_SAMPLE_H

#include "jpg_encode.h"

/*
    Input:
    * j_data: JPEG data structure
*/
void chroma_subsample(JpgData j_data);

#endif
