/*	$NetBSD: linux_machdep.h,v 1.5 2000/12/14 18:10:14 mycroft Exp $	*/

/*-
 * Copyright (c) 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Eric Haszlakiewicz.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#ifndef _ALPHA_LINUX_MACHDEP_H
#define _ALPHA_LINUX_MACHDEP_H

/*
 * The Linux sigcontext, pretty much a standard alpha trapframe.
 */
struct linux_sigcontext {
	long		sc_onstack;
	long		sc_mask;
	long		sc_pc;
	long		sc_ps;
	long		sc_regs[32];
	long		sc_ownedfp;
	long		sc_fpregs[32];
	unsigned long	sc_fpcr;
	unsigned long	sc_fp_control;
	unsigned long	sc_reserved1, sc_reserved2;
	unsigned long	sc_ssize;
	char *		sc_sbase;
	unsigned long	sc_traparg_a0;
	unsigned long	sc_traparg_a1;
	unsigned long	sc_traparg_a2;
	unsigned long	sc_fp_trap_pc;
	unsigned long	sc_fp_trigger_sum;
	unsigned long	sc_fp_trigger_inst;
};

struct linux_ucontext {
	u_long			uc_flags;
	struct linux_ucontext	*uc_link;
	linux_old_sigset_t	uc_osf_sigmask;
	linux_stack_t		uc_stack;
	struct linux_sigcontext	uc_mcontext;
	linux_sigset_t		uc_sigmask;
};

/*
 * We make the stack look like Linux expects it when calling a signal
 * handler, but use the BSD way of calling the handler and sigreturn().
 */

#define LINUX_INSN_MOV_R30_R16	0x47fe0410
#define LINUX_INSN_LDI_R0	0x201f0000
#define LINUX_INSN_CALLSYS	0x00000083

struct linux_sigframe {
	struct		linux_sigcontext sf_sc;
	unsigned long	extramask[LINUX__NSIG_WORDS-1];
	unsigned int	retcode[3];
};

struct linux_rt_sigframe {
	struct linux_siginfo	info;
	struct linux_ucontext	uc;
	unsigned int		retcode[3];
};

#ifdef _KERNEL
__BEGIN_DECLS
void setup_linux_rt_sigframe __P((struct trapframe *, int, sigset_t *));
void setup_linux_sigframe __P((struct trapframe *, int, sigset_t *));
int linux_restore_sigcontext __P((struct proc *, struct linux_sigcontext,
				  sigset_t *));
void linux_sendsig __P((sig_t, int, sigset_t *, u_long));
dev_t linux_fakedev __P((dev_t));
void linux_syscall_intern __P((struct proc *));
__END_DECLS
#endif /* !_KERNEL */

#endif /* _ALPHA_LINUX_MACHDEP_H */
