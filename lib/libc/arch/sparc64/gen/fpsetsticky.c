/*	$NetBSD: fpsetsticky.c,v 1.1.12.1 2002/01/28 20:50:19 nathanw Exp $	*/

/*
 * Written by J.T. Conklin, Apr 10, 1995
 * Public domain.
 */

#include <sys/cdefs.h>

#include "namespace.h"

#include <ieeefp.h>

#ifdef __weak_alias
__weak_alias(fpsetsticky,_fpsetsticky)
#endif

fp_except
fpsetsticky(sticky)
	fp_except sticky;
{
	fp_except old;
	fp_except new;

	__asm__("st %%fsr,%0" : "=m" (*&old));

	new = old;
	new &= ~(0x1f << 5); 
	new |= ((sticky & 0x1f) << 5);

	__asm__("ld %0,%%fsr" : : "m" (*&new));

	return (old >> 5) & 0x1f;
}
