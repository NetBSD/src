/*	$NetBSD: asm.h,v 1.7.14.1 2008/01/09 01:46:23 matt Exp $	*/

/*	$OpenBSD: asm.h,v 1.12 2001/03/29 02:15:57 mickey Exp $	*/

/* 
 * Copyright (c) 1990,1991,1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 *	Utah $Hdr: asm.h 1.8 94/12/14$
 */

#ifndef _HPPA_ASM_H_
#define _HPPA_ASM_H_

#include <machine/frame.h>
/*
 *	hppa assembler definitions
 */

#ifdef __STDC__
#define	__CONCAT(a,b)	a ## b
#else
#define	__CONCAT(a,b)	a/**/b
#endif

#define _C_LABEL(x)	x

#define	LEAF_ENTRY_NOPROFILE(x)				!\
	 ! .text ! .align 4				!\
	.export	x, entry ! .label x ! .proc		!\
	.callinfo frame=0, no_calls, save_rp		!\
	.entry

#define	ENTRY_NOPROFILE(x,n)				!\
	 ! .text ! .align 4				!\
	.export x, entry ! .label x ! .proc		!\
	.callinfo frame=n, calls, save_rp, save_sp	!\
	.entry

#ifdef GPROF

#define	_PROF_PROLOGUE				!\
1:						!\
	stw	%rp, HPPA_FRAME_CRP(%sp)	!\
	stw	%arg0, HPPA_FRAME_ARG(0)(%sp)	!\
	stw	%arg1, HPPA_FRAME_ARG(1)(%sp)	!\
	stw	%arg2, HPPA_FRAME_ARG(2)(%sp)	!\
	stw	%arg3, HPPA_FRAME_ARG(3)(%sp)	!\
	ldo	HPPA_FRAME_SIZE(%sp), %sp	!\
	copy	%rp, %arg0			!\
	bl	2f, %arg1			!\
	depi	0, 31, 2, %arg1			!\
2:						!\
	bl	_mcount, %rp			!\
	 ldo	1b - 2b(%arg1), %arg1		!\
	ldo	-HPPA_FRAME_SIZE(%sp), %sp	!\
	ldw	HPPA_FRAME_ARG(3)(%sp), %arg3	!\
	ldw	HPPA_FRAME_ARG(2)(%sp), %arg2	!\
	ldw	HPPA_FRAME_ARG(1)(%sp), %arg1	!\
	ldw	HPPA_FRAME_ARG(0)(%sp), %arg0	!\
	ldw	HPPA_FRAME_CRP(%sp), %rp	!\

#define LEAF_ENTRY(x) 				!\
	ENTRY_NOPROFILE(x,HPPA_FRAME_SIZE)	!\
	_PROF_PROLOGUE

#else /* GPROF */

#define _PROF_PROLOGUE

#define LEAF_ENTRY(x) 				!\
	LEAF_ENTRY_NOPROFILE(x)

#endif /* GPROF */

#define ENTRY(x,n) 				!\
	ENTRY_NOPROFILE(x,n)			!\
	_PROF_PROLOGUE

#define ALTENTRY(x) ! .export x, entry ! .label x
#define EXIT(x) ! .exit ! .procend ! .size x, .-x

#define RCSID(x)	.text				!\
			.asciz x			!\
			.align	4

#define WEAK_ALIAS(alias,sym)				\
	.weak alias !					\
	alias = sym

/*
 * STRONG_ALIAS: create a strong alias.
 */
#define STRONG_ALIAS(alias,sym)				\
	.globl alias !					\
	alias = sym

#define CALL(func,tmp)					!\
	ldil	L%func, tmp				!\
	ldo	R%func(tmp), tmp			!\
	.call						!\
	blr	%r0, %rp				!\
	bv,n	%r0(tmp)				!\
	nop

#ifdef PIC
#define PIC_CALL(func)					!\
	addil	LT%func, %r19				!\
	ldw	RT%func(%r1), %r1			!\
	.call						!\
	blr	%r0, %rp				!\
	bv,n	%r0(%r1)				!\
	nop
#else
#define PIC_CALL(func)					!\
	CALL(func,%r1)
#endif

#ifdef __STDC__
#define	WARN_REFERENCES(sym,msg)					\
	.stabs msg ## ,30,0,0,0 ;					\
	.stabs __STRING(sym) ## ,1,0,0,0
#else
#define	WARN_REFERENCES(sym,msg)					\
	.stabs msg,30,0,0,0 ;						\
	.stabs __STRING(sym),1,0,0,0
#endif

#define	BSS(n,s)	! .data ! .label n ! .comm s
#define	SZREG	4

#endif /* _HPPA_ASM_H_ */
