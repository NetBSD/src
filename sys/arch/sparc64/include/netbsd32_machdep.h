/*	$NetBSD: netbsd32_machdep.h,v 1.6.8.4 2002/02/28 04:12:13 nathanw Exp $	*/

/*
 * Copyright (c) 1998, 2001 Matthew R. Green
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _MACHINE_NETBSD32_H_
#define _MACHINE_NETBSD32_H_

#include <sys/types.h>
#include <sys/proc.h>
#include <compat/netbsd32/netbsd32.h>

/* from <arch/sparc/include/signal.h> */
typedef u_int32_t netbsd32_sigcontextp_t;

struct netbsd32_sigcontext {
	int		sc_onstack;	/* sigstack state to restore */
	int		__sc_mask13;	/* signal mask to restore (old style) */
	/* begin machine dependent portion */
	int		sc_sp;		/* %sp to restore */
	int		sc_pc;		/* pc to restore */
	int		sc_npc;		/* npc to restore */
	int		sc_psr;		/* pstate to restore */
	int		sc_g1;		/* %g1 to restore */
	int		sc_o0;		/* %o0 to restore */
	sigset_t	sc_mask;	/* signal mask to restore (new style) */
};

struct netbsd32_sigcontext13 {
	int	sc_onstack;		/* sigstack state to restore */
	int	sc_mask;		/* signal mask to restore (old style) */
	/* begin machine dependent portion */
	int	sc_sp;			/* %sp to restore */
	int	sc_pc;			/* pc to restore */
	int	sc_npc;			/* npc to restore */
	int	sc_psr;			/* pstate to restore */
	int	sc_g1;			/* %g1 to restore */
	int	sc_o0;			/* %o0 to restore */
};

struct exec_package;
void netbsd32_setregs (struct lwp *l, struct exec_package *pack, u_long stack);
int netbsd32_sigreturn (struct proc *p, void *v, register_t *retval);
void netbsd32_sendsig (sig_t catcher, int sig, sigset_t *mask, u_long code);

extern char netbsd32_esigcode[], netbsd32_sigcode[];

/*
 * Need to plug into get sparc specific ioctl's.
 */
#define	NETBSD32_MD_IOCTL	/* enable netbsd32_md_ioctl() */
int netbsd32_md_ioctl (struct file *, netbsd32_u_long, caddr_t, struct proc *);

#endif /* _MACHINE_NETBSD32_H_ */
