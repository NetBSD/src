/*	$NetBSD: fpgetround.c,v 1.2 1998/01/09 03:15:15 perry Exp $	*/

/*
 * Written by J.T. Conklin, Apr 10, 1995
 * Public domain.
 */

#include <ieeefp.h>

fp_rnd
fpgetround()
{
	int x;

	__asm__("st %%fsr,%0" : "=m" (*&x));
	return (x >> 30) & 0x03;
}
