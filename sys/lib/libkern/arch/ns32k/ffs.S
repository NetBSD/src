/*	$NetBSD: ffs.S,v 1.2 1998/04/03 22:59:29 matthias Exp $	*/

/* 
 * Written by Matthias Pfaller, 1996
 * Public domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: ffs.S,v 1.2 1998/04/03 22:59:29 matthias Exp $")
#endif

KENTRY(ffs, 4)
	enter	[],0
	movqd	0,r0
	ffsd	B_ARG0,r0
	bfs	1f
	addqd	1,r0
1:	exit	[]
	ret	ARGS
