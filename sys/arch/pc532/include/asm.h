/*	$NetBSD: asm.h,v 1.19.6.1 2006/05/24 10:57:00 yamt Exp $	*/

/*
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * Copyright (c) 1992 Helsinki University of Technology
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON AND HELSINKI UNIVERSITY OF TECHNOLOGY ALLOW FREE USE
 * OF THIS SOFTWARE IN ITS "AS IS" CONDITION.  CARNEGIE MELLON AND
 * HELSINKI UNIVERSITY OF TECHNOLOGY DISCLAIM ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/*
 *	File: asm.h
 *	Author: Johannes Helander, Tero Kivinen, Tatu Ylonen
 *	Modified by Phil Nelson for NetBSD.
 *	Modified by Matthias Pfaller for PIC.
 *	Helsinki University of Technology 1992.
 */

#ifndef _MACHINE_ASM_H_
#define	_MACHINE_ASM_H_

#if __STDC__
#define	CAT(a, b)	a ## b
#else
#define	CAT(a, b)	a/**/b
#endif

#define	_C_LABEL(name)		CAT(_,name)
#define	_ASM_LABEL(name)	name

/* let kernels and others override entrypoint alignment */
#ifndef _ALIGN_TEXT
# define _ALIGN_TEXT .align 2,0xa2
#endif

#ifndef _ALIGN_DATA
# define _ALIGN_DATA .align 2
#endif

#define	S_ARG0	4(sp)
#define	S_ARG1	8(sp)
#define	S_ARG2	12(sp)
#define	S_ARG3	16(sp)

#define	B_ARG0	 8(fp)
#define	B_ARG1	12(fp)
#define	B_ARG2	16(fp)
#define	B_ARG3	20(fp)

#ifdef PIC
#define	PIC_PROLOGUE						\
		sprd	sb,tos				;	\
		addr	_C_LABEL(_GLOBAL_OFFSET_TABLE_)(pc),r1;	\
		lprd	sb,r1

#define	PIC_EPILOGUE						\
		lprd	sb,tos

#define	PIC_GOT(name)	0(name(sb))

#define	PIC_S_ARG0	8(sp)
#define	PIC_S_ARG1	12(sp)
#define	PIC_S_ARG2	16(sp)
#define	PIC_S_ARG3	20(sp)
#else
#define	PIC_PROLOGUE
#define	PIC_EPILOGUE
#define	PIC_GOT(name)	name(pc)

#define	PIC_S_ARG0	4(sp)
#define	PIC_S_ARG1	8(sp)
#define	PIC_S_ARG2	12(sp)
#define	PIC_S_ARG3	16(sp)
#endif

#if defined(_KERNEL) && defined(MRTD)
#define	_SETARGS(args)						\
		.set	ARGS,args
#else
#define	_SETARGS(args)						\
		.set	ARGS,0
#endif

#define	_ENTRY(name, args)					\
		.text					;	\
		_ALIGN_TEXT				;	\
		.globl name				;	\
		.type name,@function			;	\
		_SETARGS(args)				;	\
	name:

#ifdef GPROF
# ifndef _SAVELIST
#  define _SAVELIST
# endif
# define _PROF_PROLOGUE						\
		enter [_SAVELIST],0			;	\
		bsr _ASM_LABEL(mcount)			;	\
		exit [_SAVELIST]
#else
# define _PROF_PROLOGUE
#endif

#define	ENTRY(y)	_ENTRY(_C_LABEL(y), 0); _PROF_PROLOGUE
#define	KENTRY(y, n)	_ENTRY(_C_LABEL(y), n); _PROF_PROLOGUE
#define	ASENTRY(y)	_ENTRY(_ASM_LABEL(y), 0); _PROF_PROLOGUE

#define	ENTRY_NOPROFILE(name)	_ENTRY(_C_LABEL(name), 0)
#define	ASENTRY_NOPROFILE(name)	_ENTRY(_ASM_LABEL(name), 0)

#define	ALTENTRY(name, rname)					\
		.globl _C_LABEL(name)			;	\
		.type _C_LABEL(name),@function		;	\
		.set _C_LABEL(name),_C_LABEL(rname)

#define	ASMSTR							\
		.asciz

#define	RCSID(string)						\
		.text					;	\
		ASMSTR string

/*
 * Global variables of whatever sort.
 */
#define	GLOBAL(name)						\
		.globl	_C_LABEL(name)			;	\
	_C_LABEL(name):

#define	ASGLOBAL(name)						\
		.globl	_ASM_LABEL(name)		;	\
	_ASM_LABEL(name):

/*
 * ...and local variables.
 */
#define	LOCAL(name)						\
	_C_LABEL(name):

#define	ASLOCAL(name)						\
	_ASM_LABEL(name):

/*
 * Items in the DATA segment.
 */

#define	_DATA(name, pseudo, init)				\
		.data					;	\
		.globl	name				;	\
		.type	name,@object			;	\
		.size	name,9f - name			;	\
	name:	pseudo	init				;	\
	9:

#define	DATA_D(name, init) _DATA(_C_LABEL(name), .long, init)
#define	DATA_W(name, init) _DATA(_C_LABEL(name), .word, init)
#define	DATA_B(name, init) _DATA(_C_LABEL(name), .byte, init)
#define	DATA_S(name, init) _DATA(_C_LABEL(name), .asciz, init)

#define	ASDATA_D(name, init) _DATA(_ASM_LABEL(name), .long, init)
#define	ASDATA_W(name, init) _DATA(_ASM_LABEL(name), .word, init)
#define	ASDATA_B(name, init) _DATA(_ASM_LABEL(name), .byte, init)
#define	ASDATA_S(name, init) _DATA(_ASM_LABEL(name), .asciz, init)

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
#define	PANIC(message)						\
		addr	9f,tos				;	\
		bsr	_C_LABEL(panic)			;	\
	9:	.asciz	message

/*
 * Shorthand for defining vectors for the vector table.
 */
#define	VECTOR(name)						\
		.long	_C_LABEL(name)

#define	ASVECTOR(name)						\
		.long	_ASM_LABEL(name)

#define	VECTOR_UNUSED						\
		.long	0

#endif

#define	WEAK_ALIAS(alias,sym)						\
	.weak _C_LABEL(alias);						\
	_C_LABEL(alias) = _C_LABEL(sym)
/*
 * STRONG_ALIAS: create a strong alias.
 */
#define STRONG_ALIAS(alias,sym)						\
	.globl _C_LABEL(alias);						\
	_C_LABEL(alias) = _C_LABEL(sym)

#ifdef __STDC__
#define	__STRING(x)			#x
#define	WARN_REFERENCES(sym,msg)					\
	.stabs msg ## ,30,0,0,0 ;					\
	.stabs __STRING(_ ## sym) ## ,1,0,0,0
#else
#define	__STRING(x)			"x"
#define	WARN_REFERENCES(sym,msg)					\
	.stabs msg,30,0,0,0 ;						\
	.stabs __STRING(_/**/sym),1,0,0,0
#endif /* __STDC__ */

#endif
