/*	$NetBSD: fpgetround.c,v 1.2.16.1 2002/01/28 20:49:56 nathanw Exp $	*/

/*
 * Written by J.T. Conklin, Apr 11, 1995
 * Public domain.
 */

#include <sys/cdefs.h>

#include "namespace.h"

#include <ieeefp.h>

#ifdef __weak_alias
__weak_alias(fpgetround,_fpgetround)
#endif

fp_rnd
fpgetround()
{
	int x;

	__asm__("cfc1 %0,$31" : "=r" (x));
	return x & 0x03;
}
