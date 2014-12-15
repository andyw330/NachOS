#include "syscall.h"

int
main()
{
    int i;
    Nice(80);
    for (i=0;i<2500;i++);
    Nice(20);
    for (i=0;i<500;i++);
}

