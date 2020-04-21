/* $NetBSD: ffs.c,v 1.1.16.2 2020/04/21 19:37:46 martin Exp $ */

#include <sys/cdefs.h>

__strong_alias(ffs, __ffssi2);
int	__ffssi2(int);

int
__ffssi2(int i)
{
	return i == 0 ? 0 : __buildint_ctz(i) + 1;
}
