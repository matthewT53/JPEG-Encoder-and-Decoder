#include <stdio.h>
#include <stdlib.h>

// common jpeg segment markers
#define SOI  0xD8FF
#define SOF0 0xC0FF
#define DHT  0xC4FF
#define DQT  0xDBFF
#define SOS  0xDAFF
#define COM  0xFEFF
#define EOI  0xD9FF

int main(void)
{
	FILE *fp = fopen("new.jpg", "wb");
	short c[7] = {SOI, SOF0, DHT, DQT, SOS, COM, EOI};
	int i = 0;

	if (fp != NULL){
		for (i = 0; i < 7; i++){
			fwrite(&c[i], 2, 1, fp);
		}
	}

	else{
		printf("Error opening file.\n");
	}

	return EXIT_SUCCESS;
}
