/*
	Interface to interact with a block of 8x8 pixels

	Written by: Matthew Ta
*/

#ifndef BLOCK_H
#define BLOCK_H

typedef struct _block *Block;

/*
	Allocates memory for a new block
*/
Block newBlock();

/*
	Returns the value at a specific position (x,y)
*/
double getValueBlock(Block b, int x, int y);

/*
    Sets the value at a specific position (x,y)
*/
void setValueBlock(Block b, int x, int y, int v);

/*
    Displays the contents of the blocks
*/
double showBlock(Block b);

/*
	Frees all memory used a block
*/
void destroyBlock(Block b);

#endif
