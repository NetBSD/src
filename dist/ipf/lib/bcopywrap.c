/*	$NetBSD: bcopywrap.c,v 1.1.1.1 2004/03/28 08:56:18 martti Exp $	*/

#include "ipf.h"

int bcopywrap(from, to, size)
void *from, *to;
size_t size;
{
	bcopy((caddr_t)from, (caddr_t)to, size);
	return 0;
}

