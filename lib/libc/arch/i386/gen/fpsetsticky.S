/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public domain.
 */

#include <machine/asm.h>

ENTRY(fpsetsticky)
	subl $4,%esp

	fnstsw (%esp)
	movl (%esp),%eax
	movl %eax,%edx

	notl %eax
	andl $63,%eax

	orl %eax,%edx
	movl 8(%esp),%ecx
	andl $63,%ecx
	subl %ecx,%edx
	movl %edx,(%esp)
	fldsw (%esp)

	addl $4,%esp
	ret
