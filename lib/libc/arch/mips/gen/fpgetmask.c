/*	$NetBSD: fpgetmask.c,v 1.3 2002/01/13 21:45:45 thorpej Exp $	*/

/*
 * Written by J.T. Conklin, Apr 11, 1995
 * Public domain.
 */

#include <sys/cdefs.h>

#include "namespace.h"

#include <ieeefp.h>

#ifdef __weak_alias
__weak_alias(fpgetmask,_fpgetmask)
#endif

fp_except
fpgetmask()
{
	int x;

	__asm__("cfc1 %0,$31" : "=r" (x));
	return (x >> 7) & 0x1f;
}
