/* $NetBSD: mcontext.h,v 1.3 2024/11/03 22:24:22 christos Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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
#ifndef _OR1K_MCONTEXT_H_
#define _OR1K_MCONTEXT_H_

/*
 */

#define	_NGREG	33		/* GR1-31, FPCSR */

typedef	__int64_t	__greg_t;
typedef	__greg_t	__gregset_t[_NGREG];

#define	_REG_R1		0
#define	_REG_R2		1
#define	_REG_R3		2
#define	_REG_R4		3
#define	_REG_R5		4
#define	_REG_R6		5
#define	_REG_R7		6
#define	_REG_R8		7
#define	_REG_R9		8
#define	_REG_R10	9
#define	_REG_R11	10
#define	_REG_R12	11
#define	_REG_R13	12
#define	_REG_R14	13
#define	_REG_R15	14
#define	_REG_R16	15
#define	_REG_R17	16
#define	_REG_R18	17
#define	_REG_R19	18
#define	_REG_R20	19
#define	_REG_R21	20
#define	_REG_R22	21
#define	_REG_R23	22
#define	_REG_R24	23
#define	_REG_R25	24
#define	_REG_R26	25
#define	_REG_R27	26
#define	_REG_R28	27
#define	_REG_R29	28
#define	_REG_R30	29
#define	_REG_R31	30
#define	_REG_PC		31
#define	_REG_FPCSR	32

#define	_REG_SP		_REG_R1
#define	_REG_LR		_REG_R9
#define	_REG_TP		_REG_R10
#define	_REG_RV		_REG_R11
#define	_REG_GP		_REG_R16

typedef struct {
	__gregset_t	__gregs;	/* General Purpose Register set */
	__greg_t	__spare[8];	/* future proof */
} mcontext_t;

/* Machine-dependent uc_flags */
#define	_UC_TLSBASE	0x00080000	/* see <sys/ucontext.h> */

#define _UC_MACHINE_SP(uc)	((uc)->uc_mcontext.__gregs[_REG_SP])
#define _UC_MACHINE_FP(uc)	((uc)->uc_mcontext.__gregs[_REG_R2])
#define _UC_MACHINE_PC(uc)	((uc)->uc_mcontext.__gregs[_REG_PC])
#define _UC_MACHINE_INTRV(uc)	((uc)->uc_mcontext.__gregs[_REG_RV])

#define	_UC_MACHINE_SET_PC(uc, pc)	_UC_MACHINE_PC(uc) = (pc)

#endif /* !_OR1K_MCONTEXT_H_ */
