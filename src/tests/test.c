#include <stdio.h>
#include <stdlib.h>

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

	return 0;
}
