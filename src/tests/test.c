#include <stdio.h>
#include <stdlib.h>

void test(void);
void blockToCoords(int bn, int *x, int *y, unsigned int w);

int main()
{
	int a[4][4] = {
					{1,2,3,4},
					{1,2,3,4},
					{5,2,8,4},
					{5,2,8,4}
					};

	for (int i = 0; i < 4; i += 2){
		for (int j = 0; j < 4; j += 2){
			a[i/2][j/2] = (a[i][j] + a[i+1][j] + a[i][j+1] + a[i+1][j+1]) / 4;
		}
	}

	for (int c = 0; c < 4; c++){
		for (int b = 0; b < 4; b++){
			printf("%4d\t", a[c][b]);
		}

		printf("\n");
	}

	test();

	return 0;
}

void test()
{
	int x[2] = {0}, y[2] = {0};

	blockToCoords(480, x, y, 1920);

	printf("StartX: %d and endX: %d\n", x[0], x[1]);
	printf("StartY: %d and endY: %d\n", y[0], y[1]);
}

// converts block number to starting and ending coordinates (x,y)
void blockToCoords(int bn, int *x, int *y, unsigned int w)
{
	unsigned int tw = 8 * (unsigned int) bn;

	// set the starting and ending y values
	y[0] = (int) (tw / w) * 8;
	y[1] = y[0] + 8;
	if (tw % w == 0) { y[0] -= 8; y[1] -= 8; } // reached the last block of a row
	// set the starting and ending x values
	x[0] = (tw % w) - 8;
	if (x[0] == -8) { x[0] = w - 8; } // in last column, maths doesn't make sense, need to do something weird
	x[1] = x[0] + 8;
}
