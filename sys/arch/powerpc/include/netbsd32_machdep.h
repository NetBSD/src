/*	$NetBSD: netbsd32_machdep.h,v 1.1 2003/02/05 07:16:49 matt Exp $	*/

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

typedef	u_int32_t netbsd32_pointer_t;

/*
 * Convert a pointer in the 32-bit world to a valid 64-bit pointer.
 */
#define	NETBSD32PTR64(p32)	((void *)(u_long)(u_int)(p32))

#include <compat/netbsd32/netbsd32.h>
#include <powerpc/frame.h>

/* from <arch/sparc/include/signal.h> */
typedef u_int32_t netbsd32_sigcontextp_t;

struct netbsd32_sigcontext {
	int		sc_onstack;	/* sigstack state to restore */
	int		__sc_mask13;	/* signal mask to restore (old style) */
	/* begin machine dependent portion */
	struct trapframe32 sc_frame;	/* saved registers */
	sigset_t	sc_mask;	/* signal mask to restore (new style) */
};

struct netbsd32_sigcontext13 {
	int	sc_onstack;		/* sigstack state to restore */
	int	sc_mask;		/* signal mask to restore (old style) */
	/* begin machine dependent portion */
	struct trapframe32 sc_frame;	/* saved registers */
};

struct exec_package;
void netbsd32_setregs (struct lwp *, struct exec_package *, u_long);
int netbsd32_sigreturn (struct lwp *, void *, register_t *);
void netbsd32_sendsig (int sig, sigset_t *, u_long);

extern char netbsd32_esigcode[], netbsd32_sigcode[];

/*
 * Need to plug into get sparc specific ioctl's.
 */
#define	NETBSD32_MD_IOCTL	/* enable netbsd32_md_ioctl() */
int netbsd32_md_ioctl (struct file *, netbsd32_u_long, caddr_t, struct proc *);

#endif /* _MACHINE_NETBSD32_H_ */
