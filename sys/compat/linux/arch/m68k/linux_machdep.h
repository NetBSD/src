/*	$NetBSD: linux_machdep.h,v 1.1 1998/12/15 19:25:40 itohy Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by ITOH Yasufumi.
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

#ifndef _M68K_LINUX_MACHDEP_H
#define _M68K_LINUX_MACHDEP_H

#include <m68k/frame.h>
#include <compat/linux/common/linux_signal.h>

/*
 * Signal stack definitions for old signal interface.
 */

struct linux_sigcontext {
	linux_old_sigset_t	sc_mask;	/* signal mask to restore */
	u_int			sc_sp;		/* usp to restore */
	u_int			sc_d0;
	u_int			sc_d1;
	u_int			sc_a0;
	u_int			sc_a1;
	u_short			sc_ps;		/* processor status (sr reg) */
	u_int			sc_pc;

	/*
	 * The Linux sigstate (state of hardware).
	 */
	struct linux_sigstate {
		u_short		ss_format:4,	/* frame # */
				ss_vector:12;	/* vector offset */
		struct linux_fpframe {
			u_int		fpf_regs[2][3];	/* only fp0 and fp1 */
			u_int		fpf_fpcr;
			u_int		fpf_fpsr;
			u_int		fpf_fpiar;
			union FPF_u1	FPF_u1;
			union FPF_u2	FPF_u2;
		} ss_fpstate;			/* 68881/68882 state info */
		union F_u	ss_frame;	/* original exception frame */
	} sc_ss;
};

/*
 * The Linux compatible signal frame
 * On Linux, the sigtramp code is on the frame structure.
 */
struct linux_sigframe {
	void		*sf_psigtramp;
	int		sf_signum;	/* signo for handler */
	int		sf_code;	/* Linux stores vector offset here */
	struct linux_sigcontext	*sf_scp; /* context ptr for handler */
	int		sf_sigtramp[2];
	struct linux_sigc2 {
#if LINUX__NSIG_WORDS > 1
		/* This breaks backward compatibility, but Linux 2.1 has.... */
		u_long	c_extrasigmask[LINUX__NSIG_WORDS - 1];
#endif
		struct	linux_sigcontext c_sc;	/* actual context */
	} sf_c;
};

#define LINUX_SF_SIGTRAMP0 0xDEFC0014	/* addaw #20,sp */
#define LINUX_SF_SIGTRAMP1 (0x70004E40 | (LINUX_SYS_sigreturn << 16))
				/* moveq #LINUX_SYS_sigreturn,d0; trap #0 */

/*
 * Signal stack definitions for new RT signal interface.
 */

typedef struct linux_gregset {
	u_int	gr_regs[16];		/* d0-d7/a0-a6/usp */
	u_int	gr_pc;
	u_int	gr_sr;
} linux_gregset_t;

typedef struct linux_fpregset {
	u_int	fpr_fpcr;
	u_int	fpr_fpsr;
	u_int	fpr_fpiar;
	u_int	fpr_regs[8][3];		/* fp0-fp7 */
} linux_fpregset_t;

struct linux_mcontext {
	int			mc_version;
	linux_gregset_t		mc_gregs;
	linux_fpregset_t	mc_fpregs;
};

#define LINUX_MCONTEXT_VERSION	2

struct linux_ucontext {
	u_long			uc_flags;	/* 0 */
	struct linux_ucontext	*uc_link;	/* 0 */
	linux_stack_t		uc_stack;
	struct linux_mcontext	uc_mc;
	struct linux_rt_sigstate {
		struct linux_rt_fpframe {
			union FPF_u1	FPF_u1;
			union FPF_u2	FPF_u2;
		} ss_fpstate;
		unsigned int	ss_unused1:16,
				ss_format:4,
				ss_vector:12;
		union F_u	ss_frame;	/* original exception frame */
		int		ss_unused2[4];
	} uc_ss;
	linux_sigset_t		uc_sigmask;
};

/*
 * The Linux compatible RT signal frame
 * On Linux, the sigtramp code is on the frame structure.
 */
struct linux_rt_sigframe {
	void			*sf_psigtramp;
	int			sf_signum;
	struct linux_siginfo	*sf_pinfo;
	void			*sf_puc;
	struct linux_siginfo	sf_info;
	struct linux_ucontext	sf_uc;
	int			sf_sigtramp[2];
};

#define LINUX_RT_SF_SIGTRAMP0 (0x203C0000 | (LINUX_SYS_rt_sigreturn >> 16))
#define LINUX_RT_SF_SIGTRAMP1 (0x00004E40 | (LINUX_SYS_rt_sigreturn << 16))
			/* movel #LINUX_SYS_rt_sigreturn,#d0; trap #0 */

#ifdef _KERNEL

/* linux_machdep.c */
void linux_sendsig __P((sig_t, int, sigset_t *, u_long));
dev_t linux_fakedev __P((dev_t));

/* linux_sig_machdep.S */
__dead void linux_reenter_syscall __P((struct frame *fp, int stkadj))
					__attribute__((__noreturn__));

#endif /* _KERNEL */

#endif /* _M68K_LINUX_MACHDEP_H */
