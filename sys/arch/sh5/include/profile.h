/*	$NetBSD: profile.h,v 1.3.2.2 2002/07/16 00:41:18 gehenna Exp $	*/

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

#define	_MCOUNT_DECL	void __mcount

/*
 * See _PROF_PROLOGUE in <sh5/asm.h>
 * The "selfpc" value is passed on r0, while the "frompc" is passed in r18
 */

#ifdef _LP64

#define	MCOUNT					\
__asm("	.globl	_mcount				\n" \
"	.type	_mcount,@function		\n" \
"_mcount:					\n" \
"	addi	r15, -96, r15			\n" \
"	st.q	r15, 88, r0			\n" \
"	st.q	r15, 80, r14			\n" \
"	or	r15, r63, r14			\n" \
"	pta/l	___mcount, tr0			\n" \
"	st.q	r15, 72, r18			\n" \
"	st.q	r15, 64, r2			\n" \
"	st.q	r15, 56, r3			\n" \
"	st.q	r15, 48, r4			\n" \
"	st.q	r15, 40, r5			\n" \
"	st.q	r15, 32, r6			\n" \
"	st.q	r15, 24, r7			\n" \
"	st.q	r15, 16, r8			\n" \
"	st.q	r15, 8, r9			\n" \
"	or	r18, r63, r2			\n" \
"	or	r0, r63, r3			\n" \
"	blink	tr0, r18			\n" \
"	ld.q	r15, 88, r0			\n" \
"	ld.q	r15, 72, r18			\n" \
"	ld.q	r15, 64, r2			\n" \
"	ld.q	r15, 56, r3			\n" \
"	ld.q	r15, 48, r4			\n" \
"	ld.q	r15, 40, r5			\n" \
"	ld.q	r15, 32, r6			\n" \
"	ld.q	r15, 24, r7			\n" \
"	ld.q	r15, 16, r8			\n" \
"	ld.q	r15, 8, r9			\n" \
"	ptabs/l	r0, tr0				\n" \
"	ld.q	r15, 80, r14			\n" \
"	addi	r15, 96, r15			\n" \
"	blink	tr0, r63			\n");

#else	/* !_LP64 */

#define	MCOUNT					\
__asm("	.globl	_mcount				\n" \
"	.type	_mcount,@function		\n" \
"_mcount:					\n" \
"	addi.l	r15, -80, r15			\n" \
"	st.l	r15, 76, r0			\n" \
"	st.l	r15, 72, r14			\n" \
"	or	r15, r63, r14			\n" \
"	pta/l	___mcount, tr0			\n" \
"	st.q	r15, 64, r18			\n" \
"	st.q	r15, 56, r2			\n" \
"	st.q	r15, 48, r3			\n" \
"	st.q	r15, 40, r4			\n" \
"	st.q	r15, 32, r5			\n" \
"	st.q	r15, 24, r6			\n" \
"	st.q	r15, 16, r7			\n" \
"	st.q	r15, 8, r8			\n" \
"	st.q	r15, 0, r9			\n" \
"	or	r18, r63, r2			\n" \
"	or	r0, r63, r3			\n" \
"	blink	tr0, r18			\n" \
"	ld.l	r15, 76, r0			\n" \
"	ld.q	r15, 64, r18			\n" \
"	ld.q	r15, 56, r2			\n" \
"	ld.q	r15, 48, r3			\n" \
"	ld.q	r15, 40, r4			\n" \
"	ld.q	r15, 32, r5			\n" \
"	ld.q	r15, 24, r6			\n" \
"	ld.q	r15, 16, r7			\n" \
"	ld.q	r15, 8, r8			\n" \
"	ld.q	r15, 0, r9			\n" \
"	ptabs/l	r0, tr0				\n" \
"	ld.l	r15, 72, r14			\n" \
"	addi.l	r15, 80, r15			\n" \
"	blink	tr0, r63			\n");
#endif

#ifdef _KERNEL
#define	MCOUNT_ENTER					\
	__asm volatile("getcon sr, %0" : "=r"(s));	\
	__asm volatile("putcon %0, sr" :: "r"(s | SH5_CONREG_SR_IMASK_ALL))

#define	MCOUNT_EXIT					\
	__asm volatile("putcon %0, sr" :: "r"(s))
#endif

#endif /* _SH5_PROFILE_H */
