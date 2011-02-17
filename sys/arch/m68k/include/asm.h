/*	$NetBSD: asm.h,v 1.27.4.1 2011/02/17 11:59:46 bouyer Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1994 Allen Briggs
 * All rights reserved.
 *
 * Gleaned from locore.s and sun3 asm.h which had the following copyrights:
 * locore.s:
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990 The Regents of the University of California.
 * sun3/include/asm.h:
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1990 The Regents of the University of California.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _M68K_ASM_H_
#define _M68K_ASM_H_

#if defined(__ELF__) && defined(PIC)
#define PIC_PLT(name)	name@PLTPC
#else
#define PIC_PLT(name)	name
#endif

#ifdef __ELF__
# if __STDC__
#  define _C_LABEL(name)	name
# else
#  define _C_LABEL(name)	name
#endif /* __STDC__ */
#else /* __ELF__ */
# if __STDC__
#  define _C_LABEL(name)	_ ## name
# else
#  define _C_LABEL(name)	_/**/name
# endif /* __STDC__ */
#endif /* __ELF__ */

#define	_ASM_LABEL(name)	name

#define	_ENTRY(name) \
	.text; .even; .globl name; .type name,@function; name:

#ifdef __ELF__
#define	MCOUNT_ENTRY	__mcount
#else
#define	MCOUNT_ENTRY	mcount
#endif

#ifdef GPROF
#define _PROF_PROLOG	link %a6,#0; jbsr MCOUNT_ENTRY; unlk %a6
#else
#define _PROF_PROLOG
#endif

#define ENTRY(name)		_ENTRY(_C_LABEL(name)) _PROF_PROLOG
#define	ASENTRY(name)		_ENTRY(_ASM_LABEL(name)) _PROF_PROLOG

#define	ENTRY_NOPROFILE(name)	_ENTRY(_C_LABEL(name))
#define	ASENTRY_NOPROFILE(name)	_ENTRY(_ASM_LABEL(name))

/*
 * The m68k ALTENTRY macro is very different than the traditional
 * implementation used by other NetBSD ports.  Usually ALTENTRY 
 * simply provides an alternate function entry point.  The m68k
 * definition takes a second argument and jumps inside the second
 * function when profiling is enabled.
 *
 * The m68k behavior is similar to the ENTRY2 macro found in
 * solaris' asm_linkage.h.
 *
 * Providing ENTRY2 and changing all the code that uses ALTENTRY
 * to use it would be a desirable change.
 */
#ifdef PROF
#define ALTENTRY(name, rname)	ENTRY(name); jra rname+12
#else
#define ALTENTRY(name, rname)	_ENTRY(_C_LABEL(name))
#endif

#define RCSID(x)	.pushsection ".ident"	;	\
			.asciz x		;	\
			.popsection

/*
 * Global variables of whatever sort.
 */
#define	GLOBAL(x)					\
		.globl	_C_LABEL(x)		;	\
	_C_LABEL(x):

#define	ASGLOBAL(x)					\
		.globl	_ASM_LABEL(x)		;	\
	_ASM_LABEL(x):

/*
 * ...and local variables.
 */
#define	LOCAL(x)					\
	_C_LABEL(x):

#define	ASLOCAL(x)					\
	_ASM_LABEL(x):

/*
 * Items in the BSS segment.
 */
#define	BSS(name, size)					\
	.comm	_C_LABEL(name),size

#define	ASBSS(name, size)				\
	.comm	_ASM_LABEL(name),size

#ifdef _KERNEL
/*
 * Shorthand for calling panic().
 * Note the side-effect: it uses up the 9: label, so be careful!
 */
#define	PANIC(x)					\
		pea	9f			;	\
		jbsr	_C_LABEL(panic)		;	\
	9:	.asciz	x			;	\
		.even

/*
 * Need a better place for these but these are common across
 * all m68k ports so let's define just once.
 */
#define INTERRUPT_SAVEREG	moveml	#0xC0C0,%sp@-
#define INTERRUPT_RESTOREREG	moveml	%sp@+,#0x0303

/* 64-bit counter increments */
#define CPUINFO_INCREMENT(n)					\
	lea	_C_LABEL(cpu_info_store)+(n)+4,%a1;		\
	addq.l	#1,(%a1);					\
	clr.l	%d0;		/* doesn't change CCR[X] */	\
	move.l	-(%a1),%d1;	/* doesn't change CCR[X] */	\
	addx.l	%d0,%d1;					\
	move.l	%d1,(%a1)

/* 64-bit counter increments */
#define CPUINFO_ADD(n, addend)					\
	lea	_C_LABEL(cpu_info_store)+(n)+4,%a1;		\
	add.l	addend,(%a1);					\
	clr.l	%d0;		/* doesn't change CCR[X] */	\
	move.l	-(%a1),%d1;	/* doesn't change CCR[X] */	\
	addx.l	%d0,%d1;					\
	move.l	%d1,(%a1)

#endif /* _KERNEL */

/*
 * Shorthand for defining vectors for the vector table.
 */
#define	VECTOR(x)					\
	.long	_C_LABEL(x)

#define	ASVECTOR(x)					\
	.long	_ASM_LABEL(x)

#define	VECTOR_UNUSED					\
	.long	0

#ifdef __ELF__
#define	WEAK_ALIAS(alias,sym)						\
	.weak alias;							\
	alias = sym
#endif
/*
 * STRONG_ALIAS: create a strong alias.
 */
#define STRONG_ALIAS(alias,sym)						\
	.globl alias;							\
	alias = sym

#ifdef __STDC__
#define	WARN_REFERENCES(sym,msg)					\
	.pushsection .gnu.warning. ## sym;				\
	.ascii msg;							\
	.popsection
#else
#define	WARN_REFERENCES(sym,msg)					\
	.pushsection .gnu.warning./**/sym;				\
	.ascii msg;							\
	.popsection
#endif /* __STDC__ */

/*
 * Macros to hide shortcomings in the 68010.
 */
#ifndef __mc68010__
#define	EXTBL(reg)					\
	extbl	reg
#else	/* __mc68010__ */
#define	EXTBL(reg)					\
	extw	reg		;			\
	extl	reg
#endif	/* __mc68010__ */

#endif /* _M68K_ASM_H_ */
