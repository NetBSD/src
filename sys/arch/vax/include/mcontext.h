/*	$NetBSD: mcontext.h,v 1.1.2.1 2001/11/17 04:28:02 matt Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein.
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

#ifndef _VAX_MCONTEXT_H_
#define _VAX_MCONTEXT_H_

/*
 * Layout of mcontext_t based on the System V Application Binary Interface,
 * Edition 4.1, PowerPC Processor ABI Supplement - September 1995, and
 * extended for the AltiVec Register File.  Note that due to the increased
 * alignment requirements of the latter, the offset of mcontext_t within
 * an ucontext_t is different from System V.
 */

#define	_NGREG	16		/* R0-31, AP, SP, FP, PC */

typedef	int		__greg_t;
typedef	__greg_t	__gregset_t[_NGREG];

struct __gregs {
	__greg_t	__r_r0;		/* GR0-11 */
	__greg_t	__r_r1;
	__greg_t	__r_r2;
	__greg_t	__r_r3;
	__greg_t	__r_r4;
	__greg_t	__r_r5;
	__greg_t	__r_r6;
	__greg_t	__r_r7;
	__greg_t	__r_r8;
	__greg_t	__r_r9;
	__greg_t	__r_r10;
	__greg_t	__r_r11;
	__greg_t	__r_ap;		/* AP */
	__greg_t	__r_sp;		/* SP */
	__greg_t	__r_fp;		/* FP */
	__greg_t	__r_pc;		/* PC */
};

typedef struct {
	__gregset_t	__gregs;	/* General Purpose Register set */
} mcontext_t;

#endif	/* !_VAX_MCONTEXT_H_ */
