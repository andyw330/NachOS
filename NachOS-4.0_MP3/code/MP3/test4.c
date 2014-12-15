#include "syscall.h"

int
main()
{
    int i,j;
    Nice(105);
    for (i=0;i<10;i++){
	for(j=0;j<20;j++);
	Nice(105);
    }
}

