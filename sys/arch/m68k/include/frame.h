/*	$NetBSD: frame.h,v 1.21 2003/09/22 14:18:37 cl Exp $	*/

/*
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: frame.h 1.8 92/12/20$
 *
 *	@(#)frame.h	8.1 (Berkeley) 6/10/93
 */
/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 * from: Utah $Hdr: frame.h 1.8 92/12/20$
 *
 *	@(#)frame.h	8.1 (Berkeley) 6/10/93
 */

#ifndef	_M68K_FRAME_H_
#define	_M68K_FRAME_H_

#include <m68k/cpuframe.h>
#include <m68k/signal.h>

#if defined(COMPAT_16) || !defined(_KERNEL)
/*
 * Stack frame layout when delivering a signal.
 */
struct sigframe_sigcontext {
	int	sf_ra;			/* handler return address */
	int	sf_signum;		/* signal number for handler */
	int	sf_code;		/* additional info for handler */
	struct sigcontext *sf_scp;	/* context pointer for handler */
	struct sigcontext sf_sc;	/* actual context */
	struct sigstate sf_state;	/* state of the hardware */
};
#endif

struct sigframe_siginfo {
	int		sf_ra;		/* return address for handler */
	int		sf_signum;	/* "signum" argument for handler */
	siginfo_t	*sf_sip;	/* "sip" argument for handler */
	ucontext_t	*sf_ucp;	/* "ucp" argument for handler */
	siginfo_t	sf_si;		/* actual saved siginfo */
	ucontext_t	sf_uc;		/* actual saved ucontext */
};

#if defined(_KERNEL)
/*
 * Utility function to relocate the initial frame, make room to restore an
 * exception frame and reenter the syscall.
 */
void reenter_syscall __P((struct frame *, int)) __attribute__((__noreturn__));

/*
 * Create an FPU "idle" frame for use by cpu_setmcontext()
 */
extern void m68k_make_fpu_idle_frame(void);
extern struct fpframe m68k_cached_fpu_idle_frame;

void *getframe(struct lwp *, int, int *);
void buildcontext(struct lwp *, void *, void *);
#ifdef COMPAT_16
void sendsig_sigcontext(ksiginfo_t *, sigset_t *);
#endif

#endif	/* _KERNEL */

#endif	/* _M68K_FRAME_H_ */
