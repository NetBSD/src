/*	$NetBSD: mcontext.h,v 1.4 2003/01/20 05:26:46 matt Exp $	*/

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

#define	_REG_R0		0
#define	_REG_R1		1
#define	_REG_R2		2
#define	_REG_R3		3
#define	_REG_R4		4
#define	_REG_R5		5
#define	_REG_R6		6
#define	_REG_R7		7
#define	_REG_R8		8
#define	_REG_R9		9
#define	_REG_R10	10
#define	_REG_R11	11
#define	_REG_R12	12
#define	_REG_R13	13
#define	_REG_R14	14
#define	_REG_R15	15
#define	_REG_R16	16
#define	_REG_R17	17
#define	_REG_R18	18
#define	_REG_R19	19
#define	_REG_R20	20
#define	_REG_R21	21
#define	_REG_R22	22
#define	_REG_R23	23
#define	_REG_R24	24
#define	_REG_R25	25
#define	_REG_R26	26
#define	_REG_R27	27
#define	_REG_R28	28
#define	_REG_R29	29
#define	_REG_R30	30
#define	_REG_R31	31
#define	_REG_CR		32		/* Condition Register */
#define	_REG_LR		33		/* Link Register */
#define	_REG_PC		34		/* PC (copy of SRR0) */
#define	_REG_MSR	35		/* MSR (copy of SRR1) */
#define	_REG_CTR	36		/* Count Register */
#define	_REG_XER	37		/* Integet Exception Reigster */
#define	_REG_MQ		38		/* MQ Register (POWER only) */

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
