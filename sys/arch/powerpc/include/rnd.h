/*	$NetBSD: rnd.h,v 1.2 2002/03/26 21:50:39 kleink Exp $	*/

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

#ifndef _POWERPC_RND_H_
#define _POWERPC_RND_H_

/*
 * Machine-specific support for rnd(4)
 */

#ifdef _KERNEL

#include <powerpc/spr.h>

#define cpu_hascounter() 1

static __inline u_int32_t
cpu_counter(void)
{
	u_int32_t rv, rtcu, scratch;

	__asm __volatile (
	    "mfpvr	%0		\n"
	    "srwi	%0,%0,16	\n"
	    "cmpwi	%0,%3		\n"
	    "bne	1f		\n"	/* branch if 601 */
	    "lis	%0,0x77		\n"
	    "ori	%0,%0,0x3594	\n"	/* 7.8125e6 */
	    "mfspr	%2,%4		\n"
	    "mullw	%2,%2,%0	\n"
	    "mfspr	%0,%5		\n"
	    "srwi	%0,%0,7		\n"
	    "add	%1,%2,%0	\n"
	    "b		2f		\n"
	    "1:				\n"
	    "mftb	%1		\n"
	    "2:				\n"
	    : "=r"(scratch), "=r"(rv), "=r"(rtcu)
	    : "n"(MPC601), "n"(SPR_RTCU_R), "n"(SPR_RTCL_R));

	return rv;
}

#endif /* _KERNEL */

#endif /* _POWERPC_RND_H_ */
