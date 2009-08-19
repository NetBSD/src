/* $NetBSD: syscall.c,v 1.2.78.2 2009/08/19 18:46:22 yamt Exp $ */

/*
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 *
 * Author: 
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


#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: syscall.c,v 1.2.78.2 2009/08/19 18:46:22 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <machine/frame.h>

void	syscall_intern(struct proc *);
void	syscall_plain(struct lwp *, u_int64_t, struct trapframe *);
void	syscall_fancy(struct lwp *, u_int64_t, struct trapframe *);

void
syscall_intern(struct proc *p)
{
printf("%s: not yet\n", __func__);
	return;
}

/*
 * Process a system call.
 */
void
syscall_plain(struct lwp *l, u_int64_t code, struct trapframe *framep)
{
printf("%s: not yet\n", __func__);
	return;
}

void
syscall_fancy(struct lwp *l, u_int64_t code, struct trapframe *framep)
{
printf("%s: not yet\n", __func__);
	return;
}

/*
 * Process the tail end of a fork() for the child.
 */
void
child_return(void *arg)
{
printf("%s: not yet\n", __func__);
	return;
}
