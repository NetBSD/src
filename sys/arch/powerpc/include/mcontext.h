/*	$NetBSD: mcontext.h,v 1.2 2003/01/18 06:23:28 thorpej Exp $	*/

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

#ifndef _POWERPC_MCONTEXT_H_
#define _POWERPC_MCONTEXT_H_

/*
 * Layout of mcontext_t based on the System V Application Binary Interface,
 * Edition 4.1, PowerPC Processor ABI Supplement - September 1995, and
 * extended for the AltiVec Register File.  Note that due to the increased
 * alignment requirements of the latter, the offset of mcontext_t within
 * an ucontext_t is different from System V.
 */

#define	_NGREG	39		/* GR0-31, CR, LR, SRR0, SRR1, CTR, XER, MQ */

typedef	int		__greg_t;
typedef	__greg_t	__gregset_t[_NGREG];

struct __gregs {
	__greg_t	__r_r0;		/* GR0-31 */
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
	__greg_t	__r_r12;
	__greg_t	__r_r13;
	__greg_t	__r_r14;
	__greg_t	__r_r15;
	__greg_t	__r_r16;
	__greg_t	__r_r17;
	__greg_t	__r_r18;
	__greg_t	__r_r19;
	__greg_t	__r_r20;
	__greg_t	__r_r21;
	__greg_t	__r_r22;
	__greg_t	__r_r23;
	__greg_t	__r_r24;
	__greg_t	__r_r25;
	__greg_t	__r_r26;
	__greg_t	__r_r27;
	__greg_t	__r_r28;
	__greg_t	__r_r29;
	__greg_t	__r_r30;
	__greg_t	__r_r31;
	__greg_t	__r_cr;		/* Condition Register */
	__greg_t	__r_lr;		/* Link Register */
	__greg_t	__r_pc;		/* PC (copy of SRR0) */
	__greg_t	__r_msr;	/* MSR (copy of SRR1) */
	__greg_t	__r_ctr;	/* Count Register */
	__greg_t	__r_xer;	/* Integer Exception Register */
	__greg_t	__r_mq;		/* MQ Register (POWER only) */
};

typedef struct {
	double		__fpu_regs[32];	/* FP0-31 */
	unsigned int	__fpu_fpscr;	/* FP Status and Control Register */
	unsigned int	__fpu_valid;	/* Set together with _UC_FPU */
} __fpregset_t;

#define	_NVR	32			/* Number of Vector registers */

typedef struct {
	union __vr {
		unsigned char	__vr8[16];
		unsigned short	__vr16[8];
		unsigned int	__vr32[4];
	} 		__vrs[_NVR] __attribute__((__aligned__(16)));
	unsigned int	__vscr;		/* VSCR */
	unsigned int	__vrsave;	/* VRSAVE */
} __vrf_t;

typedef struct {
	__gregset_t	__gregs;	/* General Purpose Register set */
	__fpregset_t	__fpregs;	/* Floating Point Register set */
	__vrf_t		__vrf;		/* Vector Register File */
} mcontext_t;

/* Machine-dependent uc_flags */
#define	_UC_POWERPC_VEC	0x00010000	/* Vector Register File valid */

#define _UC_MACHINE_SP(uc)	((uc)->uc_mcontext.__gregs[1])

#endif	/* !_POWERPC_MCONTEXT_H_ */
