/*	$NetBSD: ffs.S,v 1.2 1997/05/08 13:38:48 matthias Exp $	*/

/* 
 * Written by Matthias Pfaller, 1996
 * Public domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: ffs.S,v 1.2 1997/05/08 13:38:48 matthias Exp $")
#endif

ENTRY(ffs)
	enter	[],0
	movqd	0,r0
	ffsd	B_ARG0,r0
	bfs	1f
	addqd	1,r0
1:	exit	[]
	ret	0
