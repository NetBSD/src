/*	$NetBSD: ucontext.h,v 1.24 2024/05/25 13:44:48 riastradh Exp $	*/

/*-
 * Copyright (c) 1999, 2003, 2024 The NetBSD Foundation, Inc.
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

/* uc_flags */
#define _UC_SIGMASK	0x01		/* valid uc_sigmask */
#define _UC_STACK	0x02		/* valid uc_stack */
#define _UC_CPU		0x04		/* valid GPR context in uc_mcontext */
#define _UC_FPU		0x08		/* valid FPU context in uc_mcontext */
#define	_UC_MD_BIT5	0x00000020	/* MD bits.  see below */
#define	_UC_MD_BIT16	0x00010000
#define	_UC_MD_BIT17	0x00020000
#define	_UC_MD_BIT18	0x00040000
#define	_UC_MD_BIT19	0x00080000
#define	_UC_MD_BIT20	0x00100000
#define	_UC_MD_BIT21	0x00200000
#define	_UC_MD_BIT30	0x40000000

/*
 * if your port needs more MD bits, please choose bits from _UC_MD_BIT*
 * rather than picking random unused bits.
 *
 * For historical reasons, some common flags have machine-dependent
 * definitions.  All platforms must define and handle those flags,
 * which are:
 *
 * 	_UC_TLSBASE	Context contains valid pthread private pointer 
 *
 *	_UC_SETSTACK	Context uses signal stack
 *
 *	_UC_CLRSTACK	Context does not use signal stack
 */

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

#ifndef __UCONTEXT_SIZE
#define	__UCONTEXT_SIZE		sizeof(ucontext_t)
#endif

#ifndef _UC_TLSBASE
#error	_UC_TLSBASE not defined.
#endif

#ifndef _UC_SETSTACK
#error	_UC_SETSTACK not defined.
#endif

#ifndef _UC_CLRSTACK
#error	_UC_CLRSTACK not defined.
#endif

#ifdef _KERNEL
struct lwp;

void	getucontext(struct lwp *, ucontext_t *);
int	setucontext(struct lwp *, const ucontext_t *);
void	cpu_getmcontext(struct lwp *, mcontext_t *, unsigned int *);
int	cpu_setmcontext(struct lwp *, const mcontext_t *, unsigned int);
int	cpu_mcontext_validate(struct lwp *, const mcontext_t *);

#ifdef __UCONTEXT_SIZE
__CTASSERT(sizeof(ucontext_t) == __UCONTEXT_SIZE);
#endif
#endif /* _KERNEL */

#endif /* !_SYS_UCONTEXT_H_ */
