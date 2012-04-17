/*	$NetBSD: fpsetround.c,v 1.5.8.1 2012/04/17 00:05:13 yamt Exp $	*/

/*	$OpenBSD: fpsetround.c,v 1.3 2002/10/21 18:41:05 mickey Exp $	*/

/*
 * Written by Miodrag Vallat.  Public domain
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: fpsetround.c,v 1.5.8.1 2012/04/17 00:05:13 yamt Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <ieeefp.h>

fp_rnd
fpsetround(fp_rnd rnd_dir)
{
	uint64_t fpsr;
	fp_rnd old;

	__asm volatile("fstd %%fr0,0(%1)" : "=m" (fpsr) : "r" (&fpsr) : "memory");
	old = (fp_rnd)(fpsr >> 41) & 0x03;
	fpsr = (fpsr & 0xfffff9ff00000000LL) |
	    ((uint64_t)(rnd_dir & 0x03) << 41);
	__asm volatile("fldd 0(%0),%%fr0" : : "r" (&fpsr) : "memory");
	return (old);
}
