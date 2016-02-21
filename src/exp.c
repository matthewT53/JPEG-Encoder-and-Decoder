// this file shows that the RGB data starts from the bottom left of the bmp image
// this contradicts my assumption when writing the encoer that it starts from top left

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define HEADER_SIZE 54

int detFileSize(FILE *fp);

int main(void)
{
	FILE *fp = fopen("tiger.bmp", "rb");
	FILE *new = NULL;
	unsigned char *buffer = NULL;
	int fs = 0, bytesRead = 0, bytesWritten = 0;
	char r = 0;
	int i = 0;
		
	srand(time(NULL)); // random seed
	if (fp != NULL){
		fs = detFileSize(fp);
		buffer = malloc(sizeof(unsigned char) * (fs + 1));
		bytesRead = fread(buffer, sizeof(unsigned char), fs, fp);
		printf("Bytes read: %d\n", bytesRead);
		for (i = 1; i < 50; i++){
			r = rand() % 256;
			buffer[HEADER_SIZE + i] = r;
		}
		
		new = fopen("exp.bmp", "wb");
		if (new != NULL){
			fwrite(buffer, sizeof(unsigned char), fs, new);
			fclose(new);
		}
		fclose(fp);
		free(buffer);
	}
	
	else{
		printf("Cannot open image file.\n");
	}

	return 0;
}

int detFileSize(FILE *fp)
{
	int fs = 0;

	rewind(fp);
	fseek(fp, 0, SEEK_END);
	fs = (int) ftell(fp);
	fseek(fp, 0, SEEK_SET);
	return fs;
}
