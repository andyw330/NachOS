/* add.c
 *	Simple program to test whether the systemcall interface works.
 *	
 *	Just do a add syscall that adds two values and returns the result.
 *
 */

#include "syscall.h"

int
main()
{
  int result,i;
	result=0;

	for (i=100;i<300;i++)  
		result = Add(0, i);
//  printf("result is %d\n", result);
 // Halt();
  /* not reached */
}
