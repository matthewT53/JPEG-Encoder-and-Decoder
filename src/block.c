/*
	Implementation of all the functions in block.h

	Written by: Matthew Ta
*/

#include <stdio.h>
#include <stdlib.h>

#include "block.h"

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

void destroyBlock(Block b)
{
	fee(b);
}
