/*	$NetBSD: fpsetsticky.c,v 1.1 2004/07/24 19:09:29 chs Exp $	*/

/*	$OpenBSD: fpsetsticky.c,v 1.4 2004/01/05 06:06:16 otto Exp $	*/

/*
 * Written by Miodrag Vallat.  Public domain
 */

#include <sys/types.h>
#include <ieeefp.h>

fp_except
fpsetsticky(fp_except mask)
{
	uint64_t fpsr;
	fp_except old;

	__asm__ __volatile__("fstd %%fr0,0(%1)" : "=m" (fpsr) : "r" (&fpsr));
	old = (fpsr >> 59) & 0x1f;
	fpsr = (fpsr & 0x07ffffff00000000LL) | ((uint64_t)(mask & 0x1f) << 59);
	__asm__ __volatile__("fldd 0(%0),%%fr0" : : "r" (&fpsr));
	return (old);
}
