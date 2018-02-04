/*
	Implementation of all the functions in block.h

	Written by: Matthew Ta
*/

#include <stdio.h>
#include <stdlib.h>

#include "headers/block.h"

#define NUM_COEFFICIENTS 64
#define NUM_COEFFICIENTS_ROW 8

typedef struct _block{
	double values[NUM_COEFFICIENTS];
} block;

// returns a new block
Block new_block()
{
	return malloc(sizeof(block));
}

double get_value_block(Block b, int x, int y)
{
	return b->values[y * NUM_COEFFICIENTS_ROW + x];
}

void set_value_block(Block b, int x, int y, double v)
{
	b->values[ (y * NUM_COEFFICIENTS_ROW) + x ] = v;
}

Block copy_block(Block b)
{
	Block copy_block = new_block();

	int x = 0, y = 0;

	for (x = 0; x < NUM_COEFFICIENTS_ROW; x++){
		for (y = 0; y < NUM_COEFFICIENTS_ROW; y++){
			set_value_block(copy_block, x, y, get_value_block(b, x, y));
		}
	}

	return copy_block;
}

void show_block(Block b)
{
	int i = 0;
	for (i = 0; i < NUM_COEFFICIENTS; i++){
		if (i % NUM_COEFFICIENTS_ROW == 0){
			printf("\n");
		}

		printf("%8.2f ", b->values[i]);
	}

	printf("\n");
}

void destroy_block(Block b)
{
	free(b);
}
