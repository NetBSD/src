/*	$NetBSD: memcpy.S,v 1.1 2005/12/20 19:28:50 christos Exp $	*/

/* 
 * Written by Matthias Pfaller, 1996
 * Public domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: memcpy.S,v 1.1 2005/12/20 19:28:50 christos Exp $")
# if defined(BCOPY)
	RCSID("$Masqueraded: as bcopy $")
# endif
# if defined(MEMMOVE)
	RCSID("$Masqueraded: as memmove $")
# endif
#endif

#if !defined(BCOPY) && !defined(MEMMOVE)
/*
 * void *
 * memcpy (void *d, const void *s, size_t len)
 *	copy len bytes from the string s to d.
 */

KENTRY(memcpy, 12)
#elif defined(BCOPY)
/*
 * void
 * bcopy (void *s, const void *d, size_t len)
 *	copy len bytes from the string s to d.
 *	s and d may overlap.
 */

# if defined(_KERNEL)
ALTENTRY(ovbcopy,bcopy)
# endif

KENTRY(bcopy, 12)
#else
/*
 * void *
 * memmove (void *d, const void *s, size_t len)
 *	copy len bytes from the string s to d.
 *	s and d may overlap.
 */

KENTRY(memmove, 12)
#endif
	enter	[r3],0
	movd	B_ARG2,r0
#if defined(BCOPY)
	movd	B_ARG0,r1
	movd	B_ARG1,r2
#else
	movd	B_ARG1,r1
	movd	B_ARG0,r2
#endif
#if defined(MEMMOVE) || defined(BCOPY)
	cmpd	r2,r1
	bls	0f
	movd	r1,r3
	addd	r0,r3
	cmpd	r2,r3
	bls	2f
#endif
0:	cmpqd	4,r0
	bhi	1f

	/*
	 * Align destination address.
	 */
	movd	3,r3
	andd	r2,r3
	movd	0(r1),tos	/* Delay write in case src-dst < 4 */
	negd	r3,r3
	addqd	4,r3
	addd	r3,r1
	addd	r3,r2
	subd	r3,r0

	movqd	3,r3
	andd	r0,r3
	lshd	-2,r0
	movsd

	movd	r3,r0
	movsb
#if !defined(BCOPY)
	movd	B_ARG0,r0
	movd	tos,0(r0)
#else
	movd	tos,0(B_ARG1)
#endif
	exit	[r3]
	ret	ARGS

1:	movsb
#if !defined(BCOPY)
	movd	B_ARG0,r0
#endif
	exit	[r3]
	ret	ARGS

#if defined(MEMMOVE) || defined(BCOPY)
2:	addd	r0,r1
	addd	r0,r2
	addqd	-1,r1
	addqd	-1,r2
	cmpqd	4,r0
	bhi	0f

	/*
	 * Align destination address.
	 */
	movd	r0,r3
	movqd	1,r0
	addd	r2,r0
	andd	3,r0
	subd	r0,r3
	movsb	b
	movd	r3,r0
	andd	3,r3

	addqd	-3,r1
	addqd	-3,r2
	lshd	-2,r0
	movsd	b

	movd	r3,r0
	addqd	3,r1
	addqd	3,r2
0:	movsb	b
#if !defined(BCOPY)
	movd	B_ARG0,r0
#endif
	exit	[r3]
	ret	ARGS
#endif
