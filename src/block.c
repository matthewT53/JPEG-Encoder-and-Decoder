/*
	Implementation of all the functions in block.h

	Written by: Matthew Ta
*/

#include <stdio.h>
#include <stdlib.h>

#include "include/block.h"

#define NUM_COEFFICIENTS 64
#define NUM_COEFFICIENTS_ROW 8

typedef struct _block{
	float values[NUM_COEFFICIENTS];
} block;

// returns a new block
Block newBlock()
{
	return malloc(sizeof(block));
}

float getValueBlock(Block b, int x, int y)
{
	return b->values[y * NUM_COEFFICIENTS_ROW + x];
}

void setValueBlock(Block b, int x, int y, float v)
{
	b->values[y * NUM_COEFFICIENTS_ROW + x] = v;
}

void showBlock(Block b)
{
	int i = 0;
	for (i = 0; i < NUM_COEFFICIENTS; i++){
		if (i % NUM_COEFFICIENTS_ROW == 0){
			printf("\n");
		}

		printf("%5.2f ", b->values[i]);
	}

	printf("\n");
}

void destroyBlock(Block b)
{
	free(b);
}
