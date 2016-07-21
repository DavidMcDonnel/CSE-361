/* 
 * Source name     : input.c
 * Executable name : -
 * Version         : 1.0
 * Created date    : 9/21/03
 * Last update     : 9/21/03
 * Author          : Jason Fritts
 * Description     : Function to read an input (between min and max) from stdin
 * 
 * Build using this command:
 *   gcc -c input.c -o input.o
 */

#include <stdio.h>

#define TRUE		1
#define FALSE		0
#define NUM_DIGITS	10

unsigned int get_number (unsigned int min, unsigned int max, FILE *filePtr)
{
    unsigned int valid, num;
    char numstr[NUM_DIGITS];

    valid = FALSE;

    while (valid != TRUE)
    {
	printf ("How many elements of the pattern do you want to display " \
		"(%d to %d)? ", min, max);

	fgets (numstr, NUM_DIGITS, stdin);
	num = atoi (numstr);

	if ((num >= min) && (num <= max))
	  valid = TRUE;
	else
	  fprintf(filePtr, "\n  Not a valid number\n\n");
    }

    return (num);
}


