/*	$NetBSD: ucontext.h,v 1.3 2003/09/13 15:28:36 kleink Exp $	*/

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

/* uc_flags */
#define _UC_SIGMASK	0x01		/* valid uc_sigmask */
#define _UC_STACK	0x02		/* valid uc_stack */
#define _UC_CPU		0x04		/* valid GPR context in uc_mcontext */
#define _UC_FPU		0x08		/* valid FPU context in uc_mcontext */

/*      
 * The following macros are used to convert from a ucontext to sigcontext,
 * and vice-versa.  This is for building a sigcontext to deliver to old-style
 * signal handlers, and converting back (in the event the handler modifies
 * the context).
 *
 * The only valid use of these macros is to convert a ucontext to a
 * sigcontext for old-style signal delivery and then back to the same
 * ucontext after the handler has returned.
 *
 * The _MCONTEXT_TO_SIGCONTEXT() and _SIGCONTEXT_TO_MCONTEXT() macros
 * should be provided by <machine/signal.h>.
 */     
#define	_UCONTEXT_TO_SIGCONTEXT(uc, sc)					\
do {									\
	(sc)->sc_mask = (uc)->uc_sigmask;				\
	if (((uc)->uc_flags & _UC_STACK) != 0 &&			\
	    ((uc)->uc_stack.ss_flags & SS_ONSTACK) != 0)		\
		(sc)->sc_onstack = 1;					\
	else								\
		(sc)->sc_onstack = 0;					\
									\
	_MCONTEXT_TO_SIGCONTEXT(uc, sc);				\
} while (/*CONSTCOND*/0)

#define	_SIGCONTEXT_TO_UCONTEXT(sc, uc)					\
do {									\
	(uc)->uc_sigmask = (sc)->sc_mask;				\
									\
	/*								\
	 * XXX What do do about uc_stack?  See				\
	 * XXX kern_sig.c:setucontext().				\
	 */								\
									\
	_SIGCONTEXT_TO_MCONTEXT(sc, uc);				\
} while (/*CONSTCOND*/0)

#ifdef _KERNEL
struct lwp;

void	getucontext(struct lwp *, ucontext_t *);
int	setucontext(struct lwp *, const ucontext_t *);
void	cpu_getmcontext(struct lwp *, mcontext_t *, unsigned int *);
int	cpu_setmcontext(struct lwp *, const mcontext_t *, unsigned int);
#endif /* _KERNEL */

#endif /* !_SYS_UCONTEXT_H_ */
