/*	$NetBSD: _context_u.S,v 1.1.2.1 2001/03/05 23:34:36 nathanw Exp $	*/

/*
 * Copyright
 */

#include <machine/asm.h>
	
#if defined(SYSLIBC_SCCS) && !defined(lint)
	RCSID("$NetBSD: _context_u.S,v 1.1.2.1 2001/03/05 23:34:36 nathanw Exp $")
#endif /* SYSLIBC_SCCS && !lint */

#define GETC \
	movl	%gs,	%eax	; \
	movl	%eax,  36(%ecx) ; \
	movl	%fs,	%eax	; \
	movl	%eax,  40(%ecx)	; \
	movl	%es,	%eax	; \
	movl	%eax,  44(%ecx)	; \
	movl	%ds,	%eax	; \
	movl	%eax,  48(%ecx)	; \
	movl	%edi,  52(%ecx)	; \
	movl	%esi,  56(%ecx)	; \
	movl	%ebp,  60(%ecx)	; \
	movl	$0,    64(%ecx) ; \
	movl	%ebx,  68(%ecx)	; \
	movl    $0,    72(%ecx) ; \
	movl    $0,    76(%ecx) ; \
	movl    $0,    80(%ecx) ; \
	movl    $0,    84(%ecx) ; \
	movl    $0,    88(%ecx) ; \
	movl	%edx,  92(%ecx)	; \
	movl	%cs,	%eax	; \
	movl	%eax,  96(%ecx) ; \
	pushfl			; \
	popl	%eax		; \
	movl	%eax, 100(%ecx)	; \
	movl	%ss,	%eax	; \
	movl	%eax, 108(%ecx)	; \
	movl	$4,	%eax	; \
 	movl	%eax,	0(%ecx)	

#define SETC \
	movl	36(%ecx), %gs	; \
	movl	40(%ecx), %fs	; \
	movl	44(%ecx), %es	; \
	movl	52(%ecx), %edi	; \
	movl	56(%ecx), %esi	; \
	movl	60(%ecx), %ebp	; \
	movl	68(%ecx), %ebx	; \
	movl	72(%ecx), %edx	; \
	movl	80(%ecx), %eax	; \
	pushl	100(%ecx)	; \
	popfl			; \
	movl	108(%ecx), %ss	; \
	movl	104(%ecx), %esp	; \
	push	96(%ecx)	; \
	pushl	92(%ecx)	; \
	push	48(%ecx)	; \
	movl	76(%ecx), %ecx	; \
	pop	%ds		; \
	lret

		
		
ENTRY(_getcontext_u)
	movl	4(%esp), %ecx
	movl	0(%esp), %edx
	GETC
	movl	%esp, %eax
	addl	$4, %eax
	movl	%eax, 104(%ecx)
	xorl	%eax,  %eax
	ret
	
ENTRY(_setcontext_u)					
	movl	4(%esp), %ecx
	SETC
	
ENTRY(_swapcontext_u)
	movl	4(%esp), %ecx
	movl	0(%esp), %edx
	GETC
	movl	%esp, %eax
	addl	$4, %eax
	movl	%eax, 104(%ecx)
	movl	8(%esp), %ecx
	SETC

