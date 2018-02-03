#include <stdio.h>
#include <stdlib.h>

#include "headers/downsample.h"

void subsample_420();
void subsample_422();

void chroma_subsample(JpgData j_data)
{
    if (j_data->sample_ratio == HORIZONTAL_SUBSAMPLING){
        subsample_422();
    }

    else if (j_data->sample_ratio == HORIZONTAL_VERTICAL_SUBSAMPLING){
        subsample_420();
    }

    else{
        printf("Not doing any Chroma Subsampling.\n");
    }
}
