/*	$NetBSD: fpgetsticky.c,v 1.3 2002/01/13 21:45:45 thorpej Exp $	*/

/*
 * Written by J.T. Conklin, Apr 11, 1995
 * Public domain.
 */

#include <sys/cdefs.h>

#include "namespace.h"

#include <ieeefp.h>

#ifdef __weak_alias
__weak_alias(fpgetsticky,_fpgetsticky)
#endif

fp_except
fpgetsticky()
{
	int x;

	__asm__("cfc1 %0,$31" : "=r" (x));
	return (x >> 2) & 0x1f;
}
