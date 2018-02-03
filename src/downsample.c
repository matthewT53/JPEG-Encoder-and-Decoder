#include <stdio.h>
#include <stdlib.h>

#include "headers/downsample.h"

void subsample_420(JpgData j_data);
void subsample_422(JpgData j_data);

void chroma_subsample(JpgData j_data)
{
    if (j_data->sample_ratio == HORIZONTAL_SUBSAMPLING){
        subsample_422(j_data);
    }

    else if (j_data->sample_ratio == HORIZONTAL_VERTICAL_SUBSAMPLING){
        subsample_420(j_data);
    }

    else{
        printf("Not doing any Chroma Subsampling.\n");
    }
}

void subsample_422(JpgData j_data)
{
    printf("4:2:2 not ready yet.\n");
}

void subsample_420(JpgData j_data)
{
    printf("4:2:0 not ready yet.\n");
}
