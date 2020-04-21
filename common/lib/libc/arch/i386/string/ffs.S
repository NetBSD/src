/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: ffs.S,v 1.2.26.2 2020/04/21 19:37:43 martin Exp $")
#endif

ENTRY(ffs)
	bsfl	4(%esp),%eax
	jz	L1	 		/* ZF is set if all bits are 0 */
	incl	%eax			/* bits numbered from 1, not 0 */
	ret

	_ALIGN_TEXT
L1:	xorl	%eax,%eax		/* clear result */
	ret
END(ffs)
