/*	$NetBSD: profile.h,v 1.5 2005/12/24 23:24:02 perry Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH5_PROFILE_H
#define _SH5_PROFILE_H

#define	_MCOUNT_DECL	static void _mcount

__weak_alias(mcount, __mcount)
extern void mcount(void) __asm("__mcount");

/*
 * See _PROF_PROLOGUE in <sh5/asm.h>
 * The "selfpc" value is passed on r0, while the "frompc" is passed in r18
 */

#define	MCOUNT					\
__asm("	.globl	mcount				\n" \
"	.type	mcount,@function		\n" \
"mcount:					\n" \
"	addi	r15, -0x88, r15			\n" \
"	st.q	r15, 0x78, r14			\n" \
"	st.q	r15, 0x80, r18			\n" \
"	or	r15, r63, r14			\n" \
"	pta/l	_mcount, tr0			\n" \
"	st.q	r15, 0x00, r2			\n" \
"	st.q	r15, 0x08, r3			\n" \
"	st.q	r15, 0x10, r4			\n" \
"	st.q	r15, 0x18, r5			\n" \
"	st.q	r15, 0x20, r6			\n" \
"	st.q	r15, 0x28, r7			\n" \
"	st.q	r15, 0x30, r8			\n" \
"	st.q	r15, 0x38, r9			\n" \
"	fst.d	r15, 0x40, dr0			\n" \
"	fst.d	r15, 0x48, dr2			\n" \
"	fst.d	r15, 0x50, dr4			\n" \
"	fst.d	r15, 0x58, dr6			\n" \
"	fst.d	r15, 0x60, dr8			\n" \
"	fst.d	r15, 0x68, dr10			\n" \
"	st.q	r15, 0x70, r0			\n" \
"	or	r18, r63, r2			\n" \
"	or	r0, r63, r3			\n" \
"	blink	tr0, r18			\n" \
"	ld.q	r15, 0x00, r2			\n" \
"	ld.q	r15, 0x08, r3			\n" \
"	ld.q	r15, 0x10, r4			\n" \
"	ld.q	r15, 0x18, r5			\n" \
"	ld.q	r15, 0x20, r6			\n" \
"	ld.q	r15, 0x28, r7			\n" \
"	ld.q	r15, 0x30, r8			\n" \
"	ld.q	r15, 0x38, r9			\n" \
"	fld.d	r15, 0x40, dr0			\n" \
"	fld.d	r15, 0x48, dr2			\n" \
"	fld.d	r15, 0x50, dr4			\n" \
"	fld.d	r15, 0x58, dr6			\n" \
"	fld.d	r15, 0x60, dr8			\n" \
"	fld.d	r15, 0x68, dr10			\n" \
"	ld.q	r15, 0x70, r0			\n" \
"	ptabs/l	r0, tr0				\n" \
"	ld.q	r15, 0x78, r14			\n" \
"	ld.q	r15, 0x80, r18			\n" \
"	addi	r15, 0x88, r15			\n" \
"	blink	tr0, r63			\n");

#ifdef _KERNEL
#define	MCOUNT_ENTER					\
	__asm volatile("getcon sr, %0" : "=r"(s));	\
	__asm volatile("putcon %0, sr" :: "r"(s | SH5_CONREG_SR_IMASK_ALL))

#define	MCOUNT_EXIT					\
	__asm volatile("putcon %0, sr" :: "r"(s))
#endif

#endif /* _SH5_PROFILE_H */
