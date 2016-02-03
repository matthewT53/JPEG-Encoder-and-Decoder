/*
	Name: Matthew Ta
	64 bit binary to long long unsigned int converter

	Description: Reads a file of binary strings and converts them into data type uint64_t (64 bits unsigned)
	Then outputs the numbers to an output file specified by the user.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> // helps make code more portable

#define MAX_BITSTRS 100
#define BIT_STR_LEN 66
#define NUM_BITS 64

// character constants
#define NULL_CHAR '\0'
#define NEW_LINE_CHAR '\n'

#define BIT_ONE '1'
#define BIT_ZERO '0'

// #define DEBUG // debugging constant
#define OUTPUT

void usage(void); // displays usage info
void convert(const char *inputFile); // converts the binary to uint64_t
void backToBin(const uint64_t *data, int size); // converts the numbers back into binary to check

static const uint64_t ACChromHuffTable[36] = {1826437377705359354U, 6863168485266997233U, 2301691190492363511U, 18302589295027578876U, 8069456558703806206U, 4580152023097250559U, 10664406268090750699U, 17149478679152112639U, 3890890172400614399U, 4458550433735983615U, 4899711882329278975U, 5476181330666443775U, 15312191453346649599U, 15456308840478998303U, 17726157275489367647U, 17762186622272534239U, 4609152705518583799U, 6915136454368149495U, 16138648714860297247U, 18172020373209480287U, 18190035046601050110U, 5764141323124631550U, 8070019517247293438U, 10369537368914919251U, 18399455573237038947U, 18403959241426581503U, 13402637723156594687U, 13979107271687260158U, 9221472052844281849U, 18445125568970801146U, 18445406934425010158U, 18442240409656754162U, 18443366326743727807U, 16285011304702278783U, 18230568542783143551U, 18374685380159995904U};

int main(int argc, char **argv)
{
	char *infile = NULL, *outfile = NULL;
	uint32_t bit = 1;
	bit <<= 31;
	printf("bit = %u\n", bit);
	if (argc != 2){
		usage();
		exit(1); 
	}

	// get the names of the files from the stdin
	infile = argv[1];
	convert(infile);
	printf("\n");
	backToBin(ACChromHuffTable, 36);
	
	return 0;
}

void usage(void)
{
	printf("Usage: ./conv <input file> <output file>\n");
}

void convert(const char *inputFile)
{
	FILE *infp = NULL;
	uint64_t data[MAX_BITSTRS] = {0}, mask = 0;
	char str[BIT_STR_LEN], *sPtr = NULL, *nlChr = NULL;
	int i = 0, j = 0, k = 0;

	infp = fopen(inputFile, "rb");
	if (infp != NULL){
		// read each line
		memset(str, 0, BIT_STR_LEN); 
		sPtr = fgets(str, BIT_STR_LEN, infp);
		#ifdef OUTPUT
			printf("{");
		#endif
		while (sPtr != NULL){
			#ifdef DEBUG
				printf("%s", str);
			#endif
			
			nlChr = strrchr(str, NEW_LINE_CHAR);
			if (nlChr != NULL) { *nlChr = NULL_CHAR; }
			#ifdef DEBUG
				printf("strlen = %lu\n", strlen(sPtr));
			#endif

			for (i = 63; i >= 0; i--){ // loop through the bits
				if (str[i] == BIT_ONE){
					mask = 1;
					mask <<= ((NUM_BITS - 1) - i);
					data[j] |= mask;
				}
			}
		
			#ifdef OUTPUT
				printf("%luU, ", data[j]);
			#endif
			j++;
			memset(str, 0, BIT_STR_LEN);
			sPtr = fgets(str, BIT_STR_LEN, infp);			
		}

		#ifdef OUTPUT
			printf("}\n\n");
		#endif

		// convert each bit string into uint64_t data
		backToBin(data, j);
		fclose(infp); 
	}
}

void backToBin(const uint64_t *data, int size)
{
	uint64_t mask = 0, bit = 0;
	int i = 0, j = 0;

	for (i = 0; i < size; i++){
		for (j = NUM_BITS - 1; j >= 0; j--){
			mask = 1;
			bit = 0;
			mask <<= j;
			bit = data[i] & mask;
			if (bit) { printf("1"); }
			else { printf("0"); }
		}
		printf("\n");
	}
}
