#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define MAX_NUM_OF_PIXELS (1000000UL)

typedef struct
{
	int x; /** horizontal position */
	int y; /** vertical position */
} pixelpos_t;


int main(void)
{
	pixelpos_t pixel;
	unsigned int num_of_pixels;
	FILE *fptr;

	// opening file in writing mode
	fptr = fopen("blackpixels.txt", "w");

	// exiting program 
	if (fptr == NULL) {
		printf("Error!");
		exit(1);
	}
	
	srand(time(NULL));   // Initialization, should only be called once.
  	num_of_pixels = (rand()*MAX_NUM_OF_PIXELS)/RAND_MAX;

	printf("\r\n %d pixels", num_of_pixels);

	for (int i = 0; i < num_of_pixels; i++)
	{
		printf("\r\n %d",i);
		pixel.x = rand();      // Returns a pseudo-random integer between 0 and RAND_MAX.
		pixel.y = rand();      // Returns a pseudo-random integer between 0 and RAND_MAX.

		fprintf(fptr, "%d %d\r\n", pixel.x, pixel.y);
	}

	fclose(fptr);
}


