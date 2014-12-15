#include "syscall.h"

int
main()
{
    int i,j;
    Nice(100);
    for (i=0;i<10;i++){
	for(j=0;j<10;j++);
	Nice(100);
    }
}

