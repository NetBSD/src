/*	$NetBSD: profile.h,v 1.5 2000/04/18 17:06:01 tsubai Exp $	*/

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

#define	_MCOUNT_DECL	void mcount

#ifdef PIC
#define _PLT "@plt"
#else
#define _PLT
#endif

#define MCOUNT __asm("		\
	.globl _mcount;		\
_mcount:			\
	stwu	1,-64(1);	\
	stw	3,16(1);	\
	stw	4,20(1);	\
	stw	5,24(1);	\
	stw	6,28(1);	\
	stw	7,32(1);	\
	stw	8,36(1);	\
	stw	9,40(1);	\
	stw	10,44(1);	\
				\
	mflr	4;		\
	stw	4,48(1);	\
	lwz	3,68(1);	\
	bl	mcount" _PLT ";	\
	lwz	3,68(1);	\
	mtlr	3;		\
	lwz	4,48(1);	\
	mtctr	4;		\
				\
	lwz	3,16(1);	\
	lwz	4,20(1);	\
	lwz	5,24(1);	\
	lwz	6,28(1);	\
	lwz	7,32(1);	\
	lwz	8,36(1);	\
	lwz	9,40(1);	\
	lwz	10,44(1);	\
	addi	1,1,64;		\
	bctr;			");

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
