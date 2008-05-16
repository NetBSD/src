/*	$NetBSD: svr4_machdep.h,v 1.4.78.1 2008/05/16 02:22:44 yamt Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
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

#ifndef _M68K_SVR4_MACHDEP_H_
#define _M68K_SVR4_MACHDEP_H_

#include <compat/svr4/svr4_types.h>

#ifdef _KERNEL
#include <m68k/frame.h>
#endif

/*
 * Machine-dependent portions [M68K].
 */

#define SVR4_M68K_D0		0
#define SVR4_M68K_D1		1
#define SVR4_M68K_D2		2
#define SVR4_M68K_D3		3
#define SVR4_M68K_D4		4
#define SVR4_M68K_D5		5
#define SVR4_M68K_D6		6
#define SVR4_M68K_D7		7
#define SVR4_M68K_A0		8
#define SVR4_M68K_A1		9
#define SVR4_M68K_A2		10
#define SVR4_M68K_A3		11
#define SVR4_M68K_A4		12
#define SVR4_M68K_A5		13
#define SVR4_M68K_A6		14
#define SVR4_M68K_A7		15
#define SVR4_M68K_PC		16
#define SVR4_M68K_PS		17
#define SVR4_M68K_MAXREG	18

#define SVR4_M68K_FP		SVR4_M68K_A6
#define SVR4_M68K_SP		SVR4_M68K_A7

typedef int		svr4_greg_t;
typedef svr4_greg_t	svr4_gregset_t[SVR4_M68K_MAXREG];

typedef struct svr4_fpregset {
	int	f_pcr;
	int	f_psr;
	int	f_piaddr;
	int	f_fpregs[8][3];
} svr4_fpregset_t;

typedef struct svr4_mcontext {
	svr4_gregset_t	gregs;
	svr4_fpregset_t	fpregs;
	union {
		long		mc_state[202];
#ifdef _KERNEL
		struct {
			/* Rest of exception frame. */
			unsigned int	format;
			unsigned int	vector;
			union F_u	exframe;
			/* Rest of FPU frame. */
			union FPF_u1	fpf_u1;
			union FPF_u2	fpf_u2;
		} frame;
#endif
	} mc_pad;
} svr4_mcontext_t;

#define mc_state	mc_pad.mc_state

/* No padding in ucontext_t; we need all the `padding' in mcontext_t. */
#undef SVR4_UC_MACHINE_PAD

/*
 * sysm68k() commands
 */
#define SVR4_SYSARCH_SETNAME		56
#define SVR4_SYSARCH_BIURW		92
#define SVR4_SYSARCH_EXOP		93
#define SVR4_SYSARCH_AB_STKFRM		94
#define SVR4_SYSARCH_LIMUSER		95
#define SVR4_SYSARCH_KERNADDR		99

void	svr4_syscall_intern(struct proc *);

#endif /* !_M68K_SVR4_MACHDEP_H_ */
