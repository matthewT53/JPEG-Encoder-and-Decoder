// file to experiment with ideas

#include <stdio.h>
#include <stdlib.h>

// common jpeg segment markers
#define SOI_MARKER  0xD8FF
#define APP0_MARKER 0xE0FF
#define SOF0_MARKER 0xC0FF
#define DHT_MARKER  0xC4FF
#define DQT_MARKER  0xDBFF
#define SOS_MARKER  0xDAFF
#define COM_MARKER  0xFEFF
#define EOI_MARKER  0xD9FF

int main(void)
{
	FILE *fp = fopen("new.jpg", "wb");
	short c[8] = {SOI_MARKER, APP0_MARKER, SOF0_MARKER, DHT_MARKER, DQT_MARKER, SOS_MARKER, COM_MARKER, EOI_MARKER};
	int i = 0;

	if (fp != NULL){
		for (i = 0; i < 8; i++){
			fwrite(&c[i], 2, 1, fp);
		}
	}

	else{
		printf("Error opening file.\n");
	}

	return EXIT_SUCCESS;
}
