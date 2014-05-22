/*	$NetBSD: asm.h,v 1.27.8.1 2014/05/22 11:40:07 yamt Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *
 *	@(#)asm.h	5.5 (Berkeley) 5/7/91
 */

#ifndef _SH3_ASM_H_
#define	_SH3_ASM_H_

/*
 * The old NetBSD/sh3 ELF toolchain used underscores.  The new
 * NetBSD/sh3 ELF toolchain does not.  The C pre-processor
 * defines __NO_LEADING_UNDERSCORES__ for the new ELF toolchain.
 */

#if defined(__ELF__) && defined(__NO_LEADING_UNDERSCORES__)
# define _C_LABEL(x)	x
#else
#ifdef __STDC__
# define _C_LABEL(x)	_ ## x
#else
# define _C_LABEL(x)	_/**/x
#endif
#endif
#define	_ASM_LABEL(x)	x

/* let kernels and others override entrypoint alignment */
#ifndef _ALIGN_TEXT
# define _ALIGN_TEXT .align 2
#endif

#ifdef __ELF__
#define	_ENTRY(x)							\
	.text								;\
	_ALIGN_TEXT							;\
	.globl x							;\
	.type x,@function						;\
	x:
#else /* !__ELF__ */
#define	_ENTRY(x)							\
	.text								;\
	_ALIGN_TEXT							;\
	.globl x							;\
	x:
#endif /* !__ELF__ */

#ifdef GPROF
#define	_PROF_PROLOGUE				  \
	mov.l	1f,r1				; \
	mova	2f,r0				; \
	jmp	@r1				; \
	 nop					; \
	.align	2				; \
1:	.long	__mcount			; \
2:
#else  /* !GPROF */
#define	_PROF_PROLOGUE
#endif /* !GPROF */

#define	ENTRY(y)	_ENTRY(_C_LABEL(y)) _PROF_PROLOGUE
#define	NENTRY(y)	_ENTRY(_C_LABEL(y))
#define	ASENTRY(y)	_ENTRY(_ASM_LABEL(y)) _PROF_PROLOGUE

#define SET_ENTRY_SIZE(y) \
	.size	_C_LABEL(y), . - _C_LABEL(y)

#define SET_ASENTRY_SIZE(y) \
	.size	_ASM_LABEL(y), . - _ASM_LABEL(y)

#ifdef __ELF__
#define	ALTENTRY(name)				 \
	.globl _C_LABEL(name)			;\
	.type _C_LABEL(name),@function		;\
	_C_LABEL(name):
#else
#define	ALTENTRY(name)				 \
	.globl _C_LABEL(name)			;\
	_C_LABEL(name):
#endif


/*
 * Hide the gory details of PIC calls vs. normal calls.  Use as in the
 * following example:
 *
 *	sts.l	pr, @-sp
 *	PIC_PROLOGUE(.L_got, r0)	! saves old r12 on stack
 *	...
 *	mov.l	.L_function_1, r0
 * 1:	CALL	r0			! each call site needs a label
 *	 nop
 *      ...
 *	mov.l	.L_function_2, r0
 * 2:	CALL	r0
 *	 nop
 *	...
 *	PIC_EPILOGUE			! restores r12 from stack
 *	lds.l	@sp+, pr		!  so call in right order 
 *	rts
 *	 nop
 *
 *	.align 2
 * .L_got:
 *	PIC_GOT_DATUM
 * .L_function_1:			! if you call the same function twice
 *	CALL_DATUM(function, 1b)	!  provide call datum for each call
 * .L_function_2:
 * 	CALL_DATUM(function, 2b)
 */

#ifdef __PIC__

#define	PIC_PLT(x)	x@PLT
#define	PIC_GOT(x)	x@GOT
#define	PIC_GOTOFF(x)	x@GOTOFF

#define	PIC_PROLOGUE(got)			\
        	mov.l	r12, @-sp;		\
		PIC_PROLOGUE_NOSAVE(got)

/*
 * Functions that do non local jumps don't need to preserve r12,
 * so we can shave off two instructions to save/restore it.
 */
#define	PIC_PROLOGUE_NOSAVE(got)		\
        	mov.l	got, r12;		\
        	mova	got, r0;		\
        	add	r0, r12

#define	PIC_EPILOGUE				\
		mov.l	@sp+, r12

#define PIC_EPILOGUE_SLOT 			\
		PIC_EPILOGUE

#define PIC_GOT_DATUM \
		.long	_GLOBAL_OFFSET_TABLE_

#define CALL	bsrf
#define JUMP	braf

#define CALL_DATUM(function, lpcs) \
		.long	PIC_PLT(function) - ((lpcs) + 4 - (.))

/*
 * This will result in text relocations in the shared library,
 * unless the function is local or has hidden or protected visibility.
 * Does not require PIC prologue.
 */
#define CALL_DATUM_LOCAL(function, lpcs) \
		.long	function - ((lpcs) + 4)

#else  /* !__PIC__ */

#define	PIC_PROLOGUE(label)
#define	PIC_PROLOGUE_NOSAVE(label)
#define	PIC_EPILOGUE
#define	PIC_EPILOGUE_SLOT	nop
#define PIC_GOT_DATUM

#define CALL	jsr @
#define JUMP	jmp @

#define CALL_DATUM(function, lpcs) \
		.long	function

#define CALL_DATUM_LOCAL(function, lpcs) \
		.long	function

#endif /* !__PIC__ */


#define	ASMSTR		.asciz

#ifdef __ELF__
#define RCSID(x)	.pushsection ".ident"; .asciz x; .popsection
#else
#define	RCSID(x)	.text; .asciz x
#endif

#ifdef NO_KERNEL_RCSIDS
#define	__KERNEL_RCSID(_n, _s)	/* nothing */
#else
#define	__KERNEL_RCSID(_n, _s)	RCSID(_s)
#endif

#ifdef __ELF__
#define	WEAK_ALIAS(alias,sym)						\
	.weak _C_LABEL(alias);						\
	_C_LABEL(alias) = _C_LABEL(sym)
#endif

/*
 * STRONG_ALIAS: create a strong alias.
 */
#define STRONG_ALIAS(alias,sym)						\
	.globl _C_LABEL(alias);						\
	_C_LABEL(alias) = _C_LABEL(sym)

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

#endif /* !_SH3_ASM_H_ */
