/*	$NetBSD: profile.h,v 1.5.12.1 2002/02/28 04:11:26 nathanw Exp $	*/

/*-
 * Copyright (c) 2000 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define	_MCOUNT_DECL	void __mcount

#ifdef PIC
#define _PLT "@plt"
#else
#define _PLT
#endif

#define MCOUNT				\
__asm("	.globl	_mcount			\n" \
"	.type	_mcount,@function	\n" \
"_mcount:				\n" \
"	stwu	1,-64(1)		\n" \
"	stw	3,16(1)			\n" \
"	stw	4,20(1)			\n" \
"	stw	5,24(1)			\n" \
"	stw	6,28(1)			\n" \
"	stw	7,32(1)			\n" \
"	stw	8,36(1)			\n" \
"	stw	9,40(1)			\n" \
"	stw	10,44(1)		\n" \
"					\n" \
"	mflr	4			\n" \
"	stw	4,48(1)			\n" \
"	lwz	3,68(1)			\n" \
"	bl	__mcount" _PLT "	\n" \
"	lwz	3,68(1)			\n" \
"	mtlr	3			\n" \
"	lwz	4,48(1)			\n" \
"	mtctr	4			\n" \
"					\n" \
"	lwz	3,16(1)			\n" \
"	lwz	4,20(1)			\n" \
"	lwz	5,24(1)			\n" \
"	lwz	6,28(1)			\n" \
"	lwz	7,32(1)			\n" \
"	lwz	8,36(1)			\n" \
"	lwz	9,40(1)			\n" \
"	lwz	10,44(1)		\n" \
"	addi	1,1,64			\n" \
"	bctr				\n" \
"_mcount_end:				\n" \
"	.size	_mcount,_mcount_end-_mcount");

#ifdef _KERNEL
#define MCOUNT_ENTER						\
	__asm volatile("mfmsr %0" : "=r"(s));			\
	if ((s & (PSL_IR | PSL_DR)) != (PSL_IR | PSL_DR))	\
		return;		/* XXX */			\
	s &= ~PSL_POW;						\
	__asm volatile("mtmsr %0" :: "r"(s & ~PSL_EE))

#define MCOUNT_EXIT						\
	__asm volatile("mtmsr %0" :: "r"(s))
#endif
