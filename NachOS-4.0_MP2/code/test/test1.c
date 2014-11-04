#include "syscall.h"

void
main()
{
	int n;
	for (n=9;n>5;n--) {
		PrintInt(n);
	}
// I add testcase
//	PrintInt(100000);
//	PrintInt(2147483647);
//	PrintInt(-2147483648);
//	PrintInt(0);
	Halt();
}
