#include <stdlib.h>
#include <stdio.h>
#include <time.h>


typedef struct
{
	int x; /** horizontal position */
	int y; /** vertical position */
} pixelpos_t;


int main(void)
{
	pixelpos_t pixel;
	unsigned int num_of_pixels = 100;
	FILE *fptr;

	// opening file in writing mode
	fptr = fopen("blackpixels.txt", "w");

	// exiting program 
	if (fptr == NULL) {
		printf("Error!");
		exit(1);
	}
	
	srand(time(NULL));   // Initialization, should only be called once.

	for (int i = 0; i < num_of_pixels; i++)
	{
		printf("\r\n %d",i);
		pixel.x = rand();      // Returns a pseudo-random integer between 0 and RAND_MAX.
		pixel.y = rand();      // Returns a pseudo-random integer between 0 and RAND_MAX.

		fprintf(fptr, "%07d %07d\r\n", pixel.x, pixel.y);
	}

	fclose(fptr);
}


