/*	$NetBSD: support.S,v 1.1.2.2 2002/06/23 17:37:05 jdolecek Exp $	*/

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
 * NOTICE: This is not a standalone file.  To use it, #include it in
 * your port's locore.S, like so:
 * 
 *      #include <hppa/hppa/support.S>
 */

/*
 * Miscellaneous support routines common to all hppa ports.
 */

/*
 * void fdcache(pa_space_t sp, vaddr_t va, vsize_t size);
 */
	.import	dcache_stride, data
LEAF_ENTRY(fdcache)
	ldil	L%dcache_stride,t1
	ldw	R%dcache_stride(t1), arg3

	comb,=	arg2, r0, fdc_none	/* no bytes, no flush */

	mtsp	arg0, sr1		/* move the space register to sr1 */
	add	arg1, arg2, arg0	/* get the last byte to flush in arg0 */

	zdep	arg3, 27, 28, t1	/* get size of a 16X loop in t1 */
	comb,<	arg2, t1, fdc_short	/* check for count < 16 * stride */
	addi	-1, t1, t1		/* compute size of large loop - 1 */

	andcm	arg2, t1, t1		/* L = count - (count mod lenbigloop) */
	add	arg1, t1, t1		/* ub for big loop is lb + L */

	fdc,m	arg3(sr1, arg1)		/* Start flushing first cache line. */
fdc_long
	fdc,m	arg3(sr1, arg1)
	fdc,m	arg3(sr1, arg1)
	fdc,m	arg3(sr1, arg1)
	fdc,m	arg3(sr1, arg1)
	fdc,m	arg3(sr1, arg1)
	fdc,m	arg3(sr1, arg1)
	fdc,m	arg3(sr1, arg1)
	fdc,m	arg3(sr1, arg1)
	fdc,m	arg3(sr1, arg1)
	fdc,m	arg3(sr1, arg1)
	fdc,m	arg3(sr1, arg1)
	fdc,m	arg3(sr1, arg1)
	fdc,m	arg3(sr1, arg1)
	fdc,m	arg3(sr1, arg1)
	fdc,m	arg3(sr1, arg1)
	comb,<<,n arg1, t1, fdc_long
	fdc,m	arg3(sr1, arg1)
fdc_short				/* flush one line at a time */
	comb,<<,n arg1, arg0, fdc_short
	fdc,m	arg3(sr1, arg1)

	addi	-1, arg0, arg1
	fdc	r0(sr1, arg1)

fdc_none
	sync
	syncdma
	bv	r0(r2)
	sync
EXIT(fdcache)

/*
 * void pdcache(pa_space_t sp, vaddr_t va, vsize_t size);
 */
	.import	dcache_stride, data
LEAF_ENTRY(pdcache)
	ldil	L%dcache_stride,t1
	ldw	R%dcache_stride(t1), arg3

	comb,=	arg2, r0, pdc_none	/* no bytes, no purge */

	mtsp	arg0, sr1		/* move the space register to sr1 */
	add	arg1, arg2, arg0	/* get the last byte to flush in arg0 */

	zdep	arg3, 27, 28, t1	/* get size of a 16X loop in t1 */
	comb,<	arg2, t1, pdc_short	/* check for count < 16 * stride */
	addi	-1, t1, t1		/* compute size of large loop - 1 */

	andcm	arg2, t1, t1		/* L = count - (count mod lenbigloop) */
	add	arg1, t1, t1		/* ub for big loop is lb + L */

	pdc,m	arg3(sr1, arg1)		/* Start flushing first cache line. */
pdc_long
	pdc,m	arg3(sr1, arg1)
	pdc,m	arg3(sr1, arg1)
	pdc,m	arg3(sr1, arg1)
	pdc,m	arg3(sr1, arg1)
	pdc,m	arg3(sr1, arg1)
	pdc,m	arg3(sr1, arg1)
	pdc,m	arg3(sr1, arg1)
	pdc,m	arg3(sr1, arg1)
	pdc,m	arg3(sr1, arg1)
	pdc,m	arg3(sr1, arg1)
	pdc,m	arg3(sr1, arg1)
	pdc,m	arg3(sr1, arg1)
	pdc,m	arg3(sr1, arg1)
	pdc,m	arg3(sr1, arg1)
	pdc,m	arg3(sr1, arg1)
	comb,<<,n arg1, t1, pdc_long
	pdc,m	arg3(sr1, arg1)
pdc_short				/* flush one line at a time */
	comb,<<,n arg1, arg0, pdc_short
	pdc,m	arg3(sr1, arg1)

	addi	-1, arg0, arg1
	pdc	r0(sr1, arg1)

pdc_none
	sync
	syncdma
	bv	r0(r2)
	sync
EXIT(pdcache)

/*
 * void ficache(pa_space_t sp, vaddr_t va, vsize_t size);
 */
	.import	icache_stride, data
LEAF_ENTRY(ficache)
	ldil	L%icache_stride,t1
	ldw	R%icache_stride(t1), arg3

	comb,=	arg2, r0, fic_none	/* no bytes, no flush */

	mtsp	arg0, sr1		/* move the space register to sr1 */
	add	arg1, arg2, arg0	/* get the last byte to flush in arg0 */

	zdep	arg3, 27, 28, t1	/* get size of a 16X loop in t1 */
	comb,<	arg2, t1, fic_short	/* check for count < 16 * stride */
	addi	-1, t1, t1		/* compute size of large loop - 1 */

	andcm	arg2, t1, t1		/* L = count - (count mod lenbigloop) */
	add	arg1, t1, t1		/* ub for big loop is lb + L */

	fic,m	arg3(sr1, arg1)		/* Start flushing first cache line. */
fic_long
	fic,m	arg3(sr1, arg1)
	fic,m	arg3(sr1, arg1)
	fic,m	arg3(sr1, arg1)
	fic,m	arg3(sr1, arg1)
	fic,m	arg3(sr1, arg1)
	fic,m	arg3(sr1, arg1)
	fic,m	arg3(sr1, arg1)
	fic,m	arg3(sr1, arg1)
	fic,m	arg3(sr1, arg1)
	fic,m	arg3(sr1, arg1)
	fic,m	arg3(sr1, arg1)
	fic,m	arg3(sr1, arg1)
	fic,m	arg3(sr1, arg1)
	fic,m	arg3(sr1, arg1)
	fic,m	arg3(sr1, arg1)
	comb,<<,n arg1, t1, fic_long
	fic,m	arg3(sr1, arg1)
fic_short				/* flush one line at a time */
	comb,<<,n arg1, arg0, fic_short
	fic,m	arg3(sr1, arg1)

	addi	-1, arg0, arg1
	fic	r0(sr1, arg1)

fic_none
	sync
	syncdma
	bv	r0(r2)
	sync
EXIT(ficache)


LEAF_ENTRY(setjmp)
/*
 * Save the other general registers whose contents are expected to remain
 * across function calls.  According to the "HP9000 Series 800 Assembly
 * Language Reference Manual", procedures can use general registers 19-26,
 * 28, 29, 1, and 31 without restoring them.  Hence, we do not save these.
 */
	stwm	r3,4(arg0)
	stwm	r4,4(arg0)
	stwm	r5,4(arg0)
	stwm	r6,4(arg0)
	stwm	r7,4(arg0)
	stwm	r8,4(arg0)
	stwm	r9,4(arg0)
	stwm	r10,4(arg0)
	stwm	r11,4(arg0)
	stwm	r12,4(arg0)
	stwm	r13,4(arg0)
	stwm	r14,4(arg0)
	stwm	r15,4(arg0)
	stwm	r16,4(arg0)
	stwm	r17,4(arg0)
	stwm	r18,4(arg0)
	stwm	r27,4(arg0)	/* Good idea to save the data pointer (dp) */
	stwm	rp,4(arg0)	/* Save the return pointer */
	stwm	sp,4(arg0)	/* Save the original stack pointer */

	bv	0(rp)
	copy	r0, ret0
EXIT(setjmp)

LEAF_ENTRY(longjmp)
/*
 * Restore general registers.
 */
	ldwm	4(arg0),r3
	ldwm	4(arg0),r4
	ldwm	4(arg0),r5
	ldwm	4(arg0),r6
	ldwm	4(arg0),r7
	ldwm	4(arg0),r8
	ldwm	4(arg0),r9
	ldwm	4(arg0),r10
	ldwm	4(arg0),r11
	ldwm	4(arg0),r12
	ldwm	4(arg0),r13
	ldwm	4(arg0),r14
	ldwm	4(arg0),r15
	ldwm	4(arg0),r16
	ldwm	4(arg0),r17
	ldwm	4(arg0),r18
	ldwm	4(arg0),r27
	ldwm	4(arg0),rp	/* Restore return address pointer, */
	ldwm	4(arg0),sp	/* stack pointer, */

	bv	0(rp)
	copy	arg1,ret0	/* Move return value to where it belongs. */
EXIT(longjmp)

