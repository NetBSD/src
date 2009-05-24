/*	$NetBSD: copy.S,v 1.12 2009/05/24 09:17:59 skrll Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthew Fredette.
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

/*	$OpenBSD: locore.S,v 1.46 2001/09/20 18:33:03 mickey Exp $	*/

/*
 * Copyright (c) 1998-2001 Michael Shalayeff
 * All rights reserved.
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
 *      This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Portitions of this file are derived from other sources, see
 * the copyrights and acknowledgements below.
 */
/*
 * Copyright (c) 1990,1991,1992,1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * THE UNIVERSITY OF UTAH AND CSL PROVIDE THIS SOFTWARE IN ITS "AS IS"
 * CONDITION, AND DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM ITS USE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 *	Utah $Hdr: locore.s 1.62 94/12/15$
 */
/*
 *  (c) Copyright 1988 HEWLETT-PACKARD COMPANY
 *
 *  To anyone who acknowledges that this file is provided "AS IS"
 *  without any express or implied warranty:
 *      permission to use, copy, modify, and distribute this file
 *  for any purpose is hereby granted without fee, provided that
 *  the above copyright notice and this notice appears in all
 *  copies, and that the name of Hewlett-Packard Company not be
 *  used in advertising or publicity pertaining to distribution
 *  of the software without specific, written prior permission.
 *  Hewlett-Packard Company makes no representations about the
 *  suitability of this software for any purpose.
 */

/*
 * This file contains the functions for user-space access:
 * copyin/copyout, fuword/suword, etc.
 */

	.align	NBPG	/* let's fit 'em on a single page */

#define	FUSUX(name)				  \
LEAF_ENTRY_NOPROFILE(name)			! \
	ldil	L%VM_MAXUSER_ADDRESS, %t1	! \
	comb,>>= %arg0, %t1, fusubadaddr	! \
	mfctl	CR_CURLWP, %t1			! \
	ldw	L_ADDR(%t1), %t1		! \
	ldil	L%fusufault, %t2		! \
	ldo	R%fusufault(%t2), %t2		! \
	ldw	U_PCB+PCB_ONFAULT(%t1), %t3	! \
	stw	%t2, U_PCB+PCB_ONFAULT(%t1)	! \
	ldw	U_PCB+PCB_SPACE(%t1), %t2	! \
	mtsp	%t2, %sr1

#define	FUX(name,insn)				  \
	FUSUX(name)				! \
	insn	0(%sr1, %arg0), %ret0		! \
	bv	%r0(%rp)			! \
	stw	%r0, U_PCB+PCB_ONFAULT(%t1)	! \
EXIT(name)

#define	SUX(name,insn)				  \
	FUSUX(name)				! \
	insn	%arg1, 0(%sr1, %arg0)		! \
	bv	%r0(%rp)			! \
	stw	%r0, U_PCB+PCB_ONFAULT(%t1)	! \
EXIT(name)

LEAF_ENTRY_NOPROFILE(fusufault)
	stw	%r0, U_PCB+PCB_ONFAULT(%t1)
ALTENTRY(fusubadaddr)
	bv	0(%rp)
	ldi	-1, %ret0
EXIT(fusuexit)

FUX(fubyte,   ldb)
FUX(fusword,  ldh)
FUX(fuword,   ldw)
FUX(fuswintr, ldh)
SUX(subyte,   stb)
SUX(susword,  sth)
SUX(suword,   stw)
SUX(suswintr, sth)

	.align	64

LEAF_ENTRY_NOPROFILE(_copy_on_fault)
	/* reset fault handler */
	stw	%r0, PCB_ONFAULT+U_PCB(%r31)
ALTENTRY(copy_on_fault)
	bv	0(%rp)
	ldi	EFAULT, %ret0
EXIT(_copy_on_fault)

/*
 * int spstrcpy (pa_space_t ssp, const void *src, pa_space_t dsp, void *dst,
 *     size_t size, size_t *rsize)
 *
 * Do a space to space size bounded string copy and return actual copy size in
 * rsize. If the copy ran out of space before '\0' is seen return ENAMETOOLONG
 *
 */
LEAF_ENTRY(spstrcpy)
	/* setup fault handler */
	mfctl	CR_CURLWP, %r31
	ldw	L_ADDR(%r31), %r31
	ldil	L%_copy_on_fault, %t2
	ldo	R%_copy_on_fault(%t2), %t2
	stw	%t2, PCB_ONFAULT+U_PCB(%r31)

	ldw	HPPA_FRAME_ARG(4)(%sp), %ret1		/* size */
	mfsp	%sr2, %ret0	/* XXX need this? */
	mtsp	%arg0, %sr1
	mtsp	%arg2, %sr2
	copy	%arg1, %arg0	/* save src */

L$spstrcpy_loop:
	comb,=,n %r0, %ret1, L$spstrcpy_exit
	ldbs,ma	1(%sr1, %arg1), %t1
	stbs,ma	%t1, 1(%sr2, %arg3)
	comb,<>	%r0, %t1, L$spstrcpy_loop
	ldo	-1(%ret1), %ret1

L$spstrcpy_exit:
	/* reset fault handler */
	stw	%r0, PCB_ONFAULT+U_PCB(%r31)
	mtsp	%r0, %sr2	/* XXX need this? */
	sub	%arg1, %arg0, %arg1
	ldw	HPPA_FRAME_ARG(5)(%sp), %arg0		/* rsize */
	comiclr,= 0, %arg0, %r0
	stw	%arg1, 0(%arg0)
	comiclr,= 0, %t1, %ret0
	ldi	ENAMETOOLONG,%ret0
	bv,n	0(%rp)
EXIT(spstrcpy)

/*
 * This macro expands into one function that calls spcopy
 * or strspcpy.  The function name is name, the copy function
 * is copyfn, and the assembly to set up the space arguments
 * is in spaceargs.
 */
#define _SPCOPY(name, fncall, spaceargs)	  \
ENTRY(name, HPPA_FRAME_SIZE * 2)		! \
						! \
	/* Start stack calling convention. */	! \
	stw	%rp, HPPA_FRAME_CRP(%sp)	! \
	copy	%r3, %r1			! \
	copy	%sp, %r3			! \
	stw,ma	%r1, (HPPA_FRAME_SIZE * 2)(%sp)	! \
						! \
	/* 					! \
	 * Set up all arguments for the copy	! \
	 * function.  spcopy only takes five	! \
	 * arguments, but the sixth argument	! \
	 * we set up is harmless.		! \
	 */					! \
	stw	%arg3, HPPA_FRAME_ARG(5)(%sp)	! \
	stw	%arg2, HPPA_FRAME_ARG(4)(%sp)	! \
	copy	%arg1, %arg3			! \
	copy	%arg0, %arg1			! \
	spaceargs				! \
						! \
	/* Call the copy function. */		! \
	fncall					! \
						! \
	/* End stack calling convention. */	! \
	ldw	HPPA_FRAME_CRP(%r3), %rp	! \
	ldo	HPPA_FRAME_SIZE(%r3), %sp	! \
	ldw,mb	-HPPA_FRAME_SIZE(%sp), %r3	! \
	bv,n	%r0(%rp)			! \
EXIT(name)

/* This loads curlwp's space into the given register. */
#define SPACE_CURLWP(reg)			  \
	mfctl	CR_CURLWP, reg			! \
	ldw	L_ADDR(reg), reg		! \
	ldw	U_PCB+PCB_SPACE(reg), reg

/* This loads the kernel's space into the given register. */
#define SPACE_KERNEL(reg)			  \
	ldi	HPPA_SID_KERNEL, reg

/* This calls the spcopy function. */
#define	CALL_SPCOPY				  \
	ldil	L%spcopy, %r1			! \
	ldo	R%spcopy(%r1), %r1		! \
	blr	%r0, %rp			! \
	bv,n	%r0(%r1)			! \
	nop

/* This calls the spstrcpy function. */
#define CALL_SPSTRCPY				  \
	bl,n	spstrcpy, %rp			! \
	nop

/*
 * This function expands to one pair of copyin or copyout
 * functions - one non-string copier and one string copier.
 */
#define SPCOPY(name, namestr, spaceargs)	  \
	_SPCOPY(name, CALL_SPCOPY, spaceargs)	! \
	_SPCOPY(namestr, CALL_SPSTRCPY, spaceargs)

/*
 * int copystr(const void *src, void *dst, size_t size, size_t *lenp);
 */
_SPCOPY(copystr, CALL_SPSTRCPY,
	SPACE_KERNEL(%arg0) ! SPACE_KERNEL(%arg2))

/*
 * int copyin(const void *src, void *dst, size_t size);
 * int copyinstr(const void *src, void *dst, size_t size, size_t *lenp);
 */
SPCOPY(copyin, copyinstr,
	SPACE_CURLWP(%arg0) ! SPACE_KERNEL(%arg2))

/*
 * int copyout(const void *src, void *dst, size_t size);
 * int copyoutstr(const void *src, void *dst, size_t size, size_t *lenp);
 */
SPCOPY(copyout, copyoutstr,
	SPACE_KERNEL(%arg0) ! SPACE_CURLWP(%arg2))

/*
 * For lack of a better place to put them, these functions
 * write the kernel text.  The kernel text is normally mapped
 * read/execute, but since it is mapped directly, we can use
 * absolute accesses on it.
 */
ENTRY(hppa_ktext_stw, HPPA_FRAME_SIZE)

	/* Start stack calling convention. */
	stw	%rp, HPPA_FRAME_CRP(%sp)
	copy	%r3, %r1
	copy	%sp, %r3
	stw,ma	%r1, HPPA_FRAME_SIZE(%sp)

	/* Do the store. */
	stwas	%arg1, 0(%arg0)

	/* Call fcacheall(). */
	.import	fcacheall, code
	ldil	L%fcacheall, %r1
	ldo	R%fcacheall(%r1), %r1
	blr	%r0, %rp
	.call
	bv,n	%r0(%r1)
	nop

	/* Add some nops for good measure. */
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	
	/* End stack calling convention. */
	ldw	HPPA_FRAME_CRP(%r3), %rp
	ldo	HPPA_FRAME_SIZE(%r3), %sp
	ldw,mb	-HPPA_FRAME_SIZE(%sp), %r3
	bv,n	%r0(%rp)
EXIT(hppa_ktext_stw)

ENTRY(hppa_ktext_stb, HPPA_FRAME_SIZE)

	/*
	 * Make the deposit location for the byte in the
	 * aligned word.
	 */
	ldi     7, %t2
	extru	%arg0, 31, 2, %t1
	sh3add	%t1, %t2, %t1
	mtsar	%t1

	/* Load the aligned word and deposit the byte. */
	dep	%r0, 31, 2, %arg0
	ldwas	0(%arg0), %t2
	vdep	%arg1, 8, %t2

	/* Jump to hppa_ktext_stw to finish. */
	b	hppa_ktext_stw
	copy	%t2, %arg1
EXIT(hppa_ktext_stb)

STRONG_ALIAS(ucas_int,ucas_32)
STRONG_ALIAS(ucas_ptr,ucas_32)

	.global ucas_ras_start
	.global ucas_ras_end

/*
 * Compare-and-swap the pointer in the user-space.
 *
 * int	ucasword(volatile void *uptr, void *old, void *new, void *ret);
 */
LEAF_ENTRY(ucas_32)
	ldil	L%VM_MAXUSER_ADDRESS, %t1
	comb,>>= %arg0, %t1, 1f

	/* setup fault handler */
	mfctl	CR_CURLWP, %r31
	ldw	L_ADDR(%r31), %r31
	ldil	L%_copy_on_fault, %t2
	ldo	R%_copy_on_fault(%t2), %t2
	stw	%t2, PCB_ONFAULT+U_PCB(%r31)

	ldw	U_PCB+PCB_SPACE(%r31), %t1
	mtsp	%t1, %sr1

ucas_ras_start:
	ldw	0(%sr1, %arg0), %t1
	comb,<>	%arg1, %t1, 2f
	 copy	%t1,%ret0
ucas_ras_end:
	stw	%arg2, 0(%sr1, %arg0)
	copy	%arg1, %ret0
2:
	stw	%ret0,	0(%sr1, %arg3)
	bv	0(%rp)
	 copy	%r0, %ret0

1:
	bv	0(%rp)
	 ldi	-1, %ret0
EXIT(ucas_32)
