#include "syscall.h"

int
main()
{
    int result,i,a,b;
	result=0;
   	 b=0;
    for (i=0;i<1000;i++){
		a=b+i;
		a=b+i;
		a=b+i;
        	a=b+i;
                a=b+i;
                a=b+i;
	        result = Add(0, i);
	}
}

