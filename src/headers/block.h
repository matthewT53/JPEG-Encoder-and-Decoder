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
Block new_block();

/*
	Returns the value at a specific position (x,y)
*/
double get_value_block(Block b, int x, int y);

/*
    Sets the value at a specific position (x,y)
*/
void set_value_block(Block b, int x, int y, double v);

/*
    Returns a copy of the specified block
*/
Block copy_block(Block b);

/*
    Displays the contents of the blocks
*/
void show_block(Block b);

/*
	Frees all memory used a block
*/
void destroy_block(Block b);

#endif
