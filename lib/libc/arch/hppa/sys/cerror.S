/*	$NetBSD: cerror.S,v 1.1 2004/07/01 06:50:04 skrll Exp $	*/

#include "SYS.h"

#ifdef _REENTRANT
ENTRY(__cerror,HPPA_FRAME_SIZE)
#else
LEAF_ENTRY(__cerror)
#endif
#ifdef _REENTRANT
	stw	%rp, HPPA_FRAME_CRP(%sp)
	ldo	HPPA_FRAME_SIZE(%sp),%sp
	bl	__errno, %rp
	nop
	stw	%t1, 0(%ret0)
	ldo	-HPPA_FRAME_SIZE(%sp), %sp
	ldw	HPPA_FRAME_CRP(%sp), %rp
#else
#ifdef PIC
	addil	LT%errno, %r19
	ldw	RT%errno(%r1), %r1
	stw	%t1, 0(%r1)
#else
	ldil	L%errno, %r1
	stw	%t1, R%errno(%r1)
#endif
#endif
	ldi	-1, %ret0
	bv	%r0(%rp)
	ldi	-1, %ret1
EXIT(__cerror)
