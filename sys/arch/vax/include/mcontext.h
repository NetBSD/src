/*	$NetBSD: mcontext.h,v 1.9 2018/02/15 15:53:57 kamil Exp $	*/

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
 * Layout of mcontext_t.
 * As on Alpha, this maps directly to `struct reg'.
 */

#define	_NGREG	17		/* R0-31, AP, SP, FP, PC, PSL */

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
#define	_REG_AP		12
#define	_REG_FP		13
#define	_REG_SP		14
#define	_REG_PC		15
#define	_REG_PSL	16

typedef struct {
	__gregset_t	__gregs;	/* General Purpose Register set */
} mcontext_t;

/* Machine-dependent uc_flags */
#define _UC_TLSBASE	0x00080000

#define	_UC_MACHINE_SP(uc)	((uc)->uc_mcontext.__gregs[_REG_SP])
#define	_UC_MACHINE_FP(uc)	((uc)->uc_mcontext.__gregs[_REG_FP])
#define	_UC_MACHINE_PC(uc)	((uc)->uc_mcontext.__gregs[_REG_PC])
#define	_UC_MACHINE_INTRV(uc)	((uc)->uc_mcontext.__gregs[_REG_R0])

#define	_UC_MACHINE_SET_PC(uc, pc)	_UC_MACHINE_PC(uc) = (pc)

#ifndef _KERNEL
#include <sys/syscall.h>

static __inline void *
__lwp_getprivate_fast(void)
{
	register void *tcb __asm("r0");
	__asm("chmk %0" :: "i"(SYS__lwp_getprivate) : "r0");
	return tcb;
}
#endif

#endif	/* !_VAX_MCONTEXT_H_ */
