/* 
 * Source name     : lab0.c
 * Executable name : lab0
 * Version         : 1.0
 * Created date    : 9/18/03
 * Last update     : 9/18/03
 * Author          : Jason Fritts
 * Description     : Basic template for C programs
 * 
 * Build using this command:
 *   gcc -m32 lab0.c -o lab0
 */

#include <stdio.h>
#include "input.h"


#define MIN_ELEMENTS	1
#define MAX_ELEMENTS	1000000
#define FILENAME_LEN    6

unsigned int	n;
unsigned int	num_set[MAX_ELEMENTS];
unsigned int	i;

int main ()
{
  char filename[FILENAME_LEN] = "task0";
  FILE *filePtr = NULL;
  
  filePtr = fopen(filename, "w");  

  /* Get n from user at stdin */
  n = get_number (MIN_ELEMENTS, MAX_ELEMENTS, filePtr);
  
  i = 1;
  
  /* Determine n elements in set */
  for (i = 0; i < n; i += 1)
  {
    num_set[i] = 2 * i + 1;
  }

  /* Print out n elements of number pattern */
  fprintf(filePtr, "\nDisplaying %d elements of pattern:\n    ", n);

  for (i = 0; i < n; i++)
    fprintf (filePtr, "%d ", num_set[i]);
  
  fprintf (filePtr, "\n");

  fclose(filePtr);
}

