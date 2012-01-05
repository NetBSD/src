/* $NetBSD: syscallemu.h,v 1.1 2012/01/05 13:26:51 jmcneill Exp $ */

/*-
 * Copyright (c) 2012 Jared D. McNeill <jmcneill@invisible.ca>
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

#ifndef _HAVE_SYSCALLEMU_H
#define _HAVE_SYSCALLEMU_H

#include <sys/cdefs.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>

struct proc;

/*
 * Process specific syscallemu configuration
 */
struct syscallemu_data { 
	vaddr_t		sce_user_start;
	vaddr_t		sce_user_end;
	void		*sce_md_syscall;
};

/*
 * Get process specific syscallemu data
 */
struct syscallemu_data *	syscallemu_getsce(struct proc *);

/*
 * Set process specific syscallemu data
 */
void	syscallemu_setsce(struct proc *, struct syscallemu_data *);

/*
 * MD specific syscallemu syscall setup (in syscallemu_${MACHINE_ARCH}.c)
 */
void *	md_syscallemu(struct proc *);

/* syscall: "syscallemu" ret: "int" args: "uintptr_t" "uintptr_t" */
#define	SYS_syscallemu		511

struct sys_syscallemu_args {
	syscallarg(uintptr_t)	user_start;
	syscallarg(uintptr_t)	user_end;
};
check_syscall_args(sys_syscallemu);

int	sys_syscallemu(struct lwp *, const struct sys_syscallemu_args *,
		       register_t *);

__CTASSERT(SYS_syscallemu >= SYS_MAXSYSCALL);
__CTASSERT(SYS_syscallemu < SYS_NSYSENT);

#endif /* !_HAVE_SYSCALLEMU_H */
