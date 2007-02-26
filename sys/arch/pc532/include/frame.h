/*	$NetBSD: frame.h,v 1.12.16.1 2007/02/26 09:07:43 yamt Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)frame.h	5.2 (Berkeley) 1/18/91
 */

#ifndef _NS532_FRAME_H_
#define	_NS532_FRAME_H_

#include <sys/signal.h>
#include <machine/reg.h>

/*
 * System stack frames.
 */

/*
 * Exception/Trap Stack Frame
 */

struct trapframe {
	long	tf_msr;		/* For abt.  0 for others. */
	long	tf_tear;	/* For abt.  0 for others. */
	long	tf_trapno;
	struct reg tf_regs;
};

/* Interrupt stack frame */

struct intrframe {
	long	if_vec;
	long	if_pl;		/* the "processor level" for clock. */
	struct reg if_regs;
};

/*
 * System Call Stack Frame
 */

struct syscframe {
	struct reg sf_regs;
};

/*
 * Stack frame inside cpu_switch()
 */
struct switchframe {
	long	sf_pl;
	long	sf_r7;
	long	sf_r6;
	long	sf_r5;
	long	sf_r4;
	long	sf_r3;
	long	sf_fp;
	int	sf_pc;
	struct	lwp *sf_lwp;
};

/*
 * Signal frame
 */
struct sigframe {
	int	sf_ra;			/* return address for handler */
	int	sf_signum;		/* "signum" argument for handler */
	int	sf_code;		/* "code" argument for handler */
	struct	sigcontext *sf_scp;	/* "scp" argument for handler */
	struct	sigcontext sf_sc;	/* actual saved context */
};

#ifdef _KERNEL
void *getframe(struct lwp *, int, int *);
#ifdef COMPAT_16
void sendsig_sigcontext(const ksiginfo_t *, const sigset_t *);
#endif
#endif

#endif /* _NS532_FRAME_H_ */
