/*	$NetBSD: fpgetsticky.S,v 1.6.76.1 2014/08/10 06:51:49 tls Exp $	*/

/*
 * Written by J.T. Conklin, Apr 4, 1995
 * Public domain.
 */

#include <machine/asm.h>

#ifdef WEAK_ALIAS
WEAK_ALIAS(fpgetsticky, _fpgetsticky)
ENTRY(_fpgetsticky)
#else
ENTRY(fpgetsticky)
#endif
	subl $4,%esp
	fnstsw (%esp)
	movl (%esp),%eax
	andl $63,%eax
	addl $4,%esp
	ret
#ifdef WEAK_ALIAS
END(_fpgetsticky)
#else
END(fpgetsticky)
#endif
