/*	$NetBSD: rtld_start.S,v 1.5 2001/08/14 22:17:48 eeh Exp $	*/

/*-
 * Copyright (c) 2000 Eduardo Horvath.
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas and Paul Kranenburg.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <machine/asm.h>
#define	_LOCORE
#include <machine/frame.h>

/*
 * ELF:
 *	On startup the stack should contain 16 extended word register save area,
 *	followed by the arg count, etc.
 *
 * _rtld() expects the stack pointer to point to two longwords for argument
 *	return followed by argc, etc.  We need to create a pointer to
 *	&argc + 16 and pass that in.  The return args will be in those locations.
 * 
 * NB:	We are violating the ELF spec by passing a pointer to the ps strings in
 * 	%g1 instead of a termination routine.
 */
	
/* Offset of ARGC from bottom of stack */
#define	ARGC	(16*8)			
	
	.section	".text"
	.align	16
	.global	_rtld_start
	.type	_rtld_start,@function
_rtld_start:
	mov	0, %fp			/* Erect a fence post for ourselves */
	mov	%g1, %l1		/* save ps_strings */
	sub	%sp, 16, %sp		/* Make room for return args */
	add	%sp, BIAS+(16*8), %l0	/* Now %l0 points at our return arg space. */

	call	_rtld
	 mov	%l0, %o0

	ldx	[%l0], %g3		/* arg: cleanup */
	ldx	[%l0 + 8], %g2		/* arg: obj */
	add	%sp, 16, %sp		/* restore stack pointer */

	jmp	%o0
	 mov	%l1, %g1		/* restore ps_strings */


	/*
	 * We have two separate entry points to the runtime linker.
	 * I'm implementing this following the SPARC v9 ABI spec.
	 *
	 * _rtld_bind_start_0(x, y) is called from .PLT0, and is used for
	 * PLT entries above 32768.
	 *
	 * _rtld_bind_start_1(x, y) is called from .PLT1, and is used for
	 * PLT entries below 32768.
	 *
	 * The first two entries of PLT2 contain the xword object pointer.
	 *
	 * These routines are called with two longword arguments, 
	 * x and y.  To calculate the address of the entry,
	 * _rtld_bind_start_1(x, y) does:
	 *
	 *	n = x >> 15;
	 *
	 * and _rtld_bind_start_0(x, y) does:
	 *
	 *	i = x - y + 1048596;
	 *	n = 32768 + (i/5120)*160 + (i%5120)/24;
	 *
	 * Neither routine needs to issue a save since it's already been
	 * done in the PLT entry.
	 */

	/* NOTE: _rtld_bind_start_0 is untested.  Hence the debug stuff */
	
	.section	".text"
	.align	4
	.global	_rtld_bind_start_0
	.type	_rtld_bind_start_0,@function
_rtld_bind_start_0:	# (x, y)
#if 0
	call	_rtld_bind_start_0_stub
	 ldx     [%o1 + (10*4)], %l7
	mov	%o0, %l5
	
	sllx	%l5, 1, %l6		/* Each element is an Elf_Rela which */
	add	%l6, %l5, %l5		/*  is 3 longwords or 24 bytes. */
	sllx	%l5, 3, %o1		/*  So multiply by 24. */
	
	call	_rtld_bind
	 mov	%l7, %o0
#else
	sethi	%hi(1048596), %l1
	sub	%o0, %o1, %l0		/* x - y */
	or	%l1, %lo(1048596), %l1
	add	%l0, %l1, %l0		/* x - y + 1048596 */

	sdivx	%l0, 5120, %l1		/* Calculate i/5120 */
	ldx	[%o1 + (10*4)], %o0	/* Load object pointer from PLT2 */
	sub	%l0, %l1, %l2		/* And i%5120 */
	
	/* Let the division churn for a bit. */
	sdivx	%l2, 14, %l4		/* (i%5120)/24 */

	/* 160 is (32 * 5) or (32 * (4 + 1)) */
	sllx	%l1, 2, %l3		/* 4 * (i/5120) */
	add	%l1, %l3, %l3		/* 5 * (i/5120) */
	sllx	%l3, 5, %l3		/* 32 * 5 * (i/5120) */

	sethi	%hi(32768), %l6
	add	%l3, %l4, %l5		/* %l5 = (i/5120)*160 + (i%5120)/24; */
	add	%l5, %l6, %l5
	
	sllx	%l5, 1, %l6		/* Each element is an Elf_Rela which */
	add	%l6, %l5, %l5		/*  is 3 longwords or 24 bytes. */
	sllx	%l5, 3, %l5		/*  So multiply by 24. */
	
	call	_rtld_bind		/* Call _rtld_bind(obj, offset) */
	 mov	%l5, %o1

	jmp	%o0			/* return value == function address */
	 restore			/* Dump our stack frame */
#endif	
	
	.section	".text"
	.align	4
	.global	_rtld_bind_start_1
	.type	_rtld_bind_start_1,@function
_rtld_bind_start_1:	# (x, y)
	srax	%o0, 15, %o2		/* %o0 is the index to our PLT slot */
	
	sllx	%o2, 1, %o3		/* Each element is an Elf_Rela which */
	add	%o3, %o2, %o2		/*  is 3 longwords or 24 bytes. */
	sllx	%o2, 3, %o2		/*  So multiply by 24. */
	
	ldx	[%o1 + 8], %o0		/* The object pointer is at [%o1 + 8] */
	
	call	_rtld_bind		/* Call _rtld_bind(obj, offset) */
	 mov	%o2, %o1

	jmp	%o0			/* return value == function address */
	 restore			/* Dump our stack frame */

