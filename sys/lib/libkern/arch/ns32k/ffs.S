/*	$NetBSD: ffs.S,v 1.3 2005/02/26 22:58:56 perry Exp $	*/

/*
 * Written by Matthias Pfaller, 1996
 * Public domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: ffs.S,v 1.3 2005/02/26 22:58:56 perry Exp $")
#endif

KENTRY(ffs, 4)
	enter	[],0
	movqd	0,r0
	ffsd	B_ARG0,r0
	bfs	1f
	addqd	1,r0
1:	exit	[]
	ret	ARGS
