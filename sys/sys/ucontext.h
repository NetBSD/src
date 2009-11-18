/*	$NetBSD: ucontext.h,v 1.12 2009/11/18 12:29:22 yamt Exp $	*/

/*-
 * Copyright (c) 1999, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein, and by Jason R. Thorpe.
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

#ifndef _SYS_UCONTEXT_H_
#define _SYS_UCONTEXT_H_

#include <sys/sigtypes.h>
#include <machine/mcontext.h>

typedef struct __ucontext	ucontext_t;

struct __ucontext {
	unsigned int	uc_flags;	/* properties */
	ucontext_t * 	uc_link;	/* context to resume */
	sigset_t	uc_sigmask;	/* signals blocked in this context */
	stack_t		uc_stack;	/* the stack used by this context */
	mcontext_t	uc_mcontext;	/* machine state */
#if defined(_UC_MACHINE_PAD)
	long		__uc_pad[_UC_MACHINE_PAD];
#endif
};

#ifndef _UC_UCONTEXT_ALIGN
#define _UC_UCONTEXT_ALIGN (~0)
#endif

/* uc_flags */
#define _UC_SIGMASK	0x01		/* valid uc_sigmask */
#define _UC_STACK	0x02		/* valid uc_stack */
#define _UC_CPU		0x04		/* valid GPR context in uc_mcontext */
#define _UC_FPU		0x08		/* valid FPU context in uc_mcontext */
#define	_UC_MD		0x40070020	/* MD bits.  see below */

/*
 * _UC_MD includes:
 *	_UC_SETSTACK	0x00010000 (many ports) and 0x00020000 (arm)
 *	_UC_CLRSTACK	0x00020000 (many ports) and 0x00040000 (arm)
 *	_UC_POWERPC_VEC	0x00010000 (powerpc)
 *	_UC_M68K_UC_USER 0x40000000 (m68k)
 *	_UC_UNIQUE	0x00000020 (alpha)
 *	_UC_ARM_VFP	0x00010000 (arm)
 *	_UC_VM		0x00040000 (i386)
 *	_UC_FXSAVE	0x00000020 (i386)
 *
 * if your port needs more MD bits, please try to choose bits from _UC_MD
 * first, rather than picking random unused bits.
 */

#ifdef _KERNEL
struct lwp;

void	getucontext(struct lwp *, ucontext_t *);
void	getucontext_sa(struct lwp *, ucontext_t *);
int	setucontext(struct lwp *, const ucontext_t *);
void	cpu_getmcontext(struct lwp *, mcontext_t *, unsigned int *);
int	cpu_setmcontext(struct lwp *, const mcontext_t *, unsigned int);
#endif /* _KERNEL */

#endif /* !_SYS_UCONTEXT_H_ */
