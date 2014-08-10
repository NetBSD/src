/*	$NetBSD: fpsetsticky.S,v 1.6.76.1 2014/08/10 06:51:49 tls Exp $	*/

/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public domain.
 */

#include <machine/asm.h>

#ifdef WEAK_ALIAS
WEAK_ALIAS(fpsetsticky, _fpsetsticky)
ENTRY(_fpsetsticky)
#else
ENTRY(fpsetsticky)
#endif
	subl $28,%esp

	fnstenv (%esp)
	movl 4(%esp),%eax
	movl %eax,%edx

	andl $63,%eax

	subl %eax,%edx
	movl 32(%esp),%ecx
	andl $63,%ecx
	addl %ecx,%edx
	movl %edx,4(%esp)
	fldenv (%esp)

	addl $28,%esp
	ret
#ifdef WEAK_ALIAS
END(_fpsetsticky)
#else
END(fpsetsticky)
#endif
