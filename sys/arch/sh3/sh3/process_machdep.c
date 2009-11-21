/*	$NetBSD: process_machdep.c,v 1.18 2009/11/21 17:40:28 rmind Exp $	*/

/*
 * Copyright (c) 1993 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
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
 * From:
 *	Id: procfs_i386.c,v 4.1 1993/12/17 10:47:45 jsp Rel
 */

/*
 * Copyright (c) 1995, 1996, 1997
 *	Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1993 Jan-Simon Pendry
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
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
 * From:
 *	Id: procfs_i386.c,v 4.1 1993/12/17 10:47:45 jsp Rel
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: process_machdep.c,v 1.18 2009/11/21 17:40:28 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/ptrace.h>

#include <machine/psl.h>
#include <machine/reg.h>

#include "opt_compat_netbsd.h"
#include "opt_coredump.h"
#include "opt_ptrace.h"

#ifdef COMPAT_40
static int process_machdep_doregs40(struct lwp *, struct lwp *, struct uio *);
static int process_machdep_read_regs40(struct lwp *l, struct __reg40 *);
static int process_machdep_write_regs40(struct lwp *l, struct __reg40 *);
#endif /* COMPAT_40 */


#if defined(PTRACE) || defined(COREDUMP)

static inline struct trapframe *
process_frame(struct lwp *l)
{

	return (l->l_md.md_regs);
}

int
process_read_regs(struct lwp *l, struct reg *regs)
{
	struct trapframe *tf = process_frame(l);

	regs->r_spc = tf->tf_spc;
	regs->r_ssr = tf->tf_ssr;
	regs->r_gbr = tf->tf_gbr;
	regs->r_macl = tf->tf_macl;
	regs->r_mach = tf->tf_mach;
	regs->r_pr = tf->tf_pr;
	regs->r_r14 = tf->tf_r14;
	regs->r_r13 = tf->tf_r13;
	regs->r_r12 = tf->tf_r12;
	regs->r_r11 = tf->tf_r11;
	regs->r_r10 = tf->tf_r10;
	regs->r_r9 = tf->tf_r9;
	regs->r_r8 = tf->tf_r8;
	regs->r_r7 = tf->tf_r7;
	regs->r_r6 = tf->tf_r6;
	regs->r_r5 = tf->tf_r5;
	regs->r_r4 = tf->tf_r4;
	regs->r_r3 = tf->tf_r3;
	regs->r_r2 = tf->tf_r2;
	regs->r_r1 = tf->tf_r1;
	regs->r_r0 = tf->tf_r0;
	regs->r_r15 = tf->tf_r15;

	return (0);
}

#endif /* PTRACE || COREDUMP */


#ifdef PTRACE

int
process_write_regs(struct lwp *l, const struct reg *regs)
{
	struct trapframe *tf = process_frame(l);

	/*
	 * Check for security violations.
	 */
	if (((regs->r_ssr ^ tf->tf_ssr) & PSL_USERSTATIC) != 0) {
		return (EINVAL);
	}

	tf->tf_spc = regs->r_spc;
	tf->tf_ssr = regs->r_ssr;
	tf->tf_pr = regs->r_pr;

	tf->tf_gbr = regs->r_gbr;
	tf->tf_mach = regs->r_mach;
	tf->tf_macl = regs->r_macl;
	tf->tf_r14 = regs->r_r14;
	tf->tf_r13 = regs->r_r13;
	tf->tf_r12 = regs->r_r12;
	tf->tf_r11 = regs->r_r11;
	tf->tf_r10 = regs->r_r10;
	tf->tf_r9 = regs->r_r9;
	tf->tf_r8 = regs->r_r8;
	tf->tf_r7 = regs->r_r7;
	tf->tf_r6 = regs->r_r6;
	tf->tf_r5 = regs->r_r5;
	tf->tf_r4 = regs->r_r4;
	tf->tf_r3 = regs->r_r3;
	tf->tf_r2 = regs->r_r2;
	tf->tf_r1 = regs->r_r1;
	tf->tf_r0 = regs->r_r0;
	tf->tf_r15 = regs->r_r15;

	return (0);
}


#ifdef __HAVE_PTRACE_MACHDEP

int
ptrace_machdep_dorequest(struct lwp *l, struct lwp *lt,
			 int req, void *addr, int data)
{
	struct uio uio;
	struct iovec iov;
	int write = 0;

	switch (req) {
	default:
		return EINVAL;

#ifdef COMPAT_40
	case PT___SETREGS40:
		write = 1;
		/* FALLTHROUGH*/

	case PT___GETREGS40:
		if (!process_validregs(lt))
			return EINVAL;
		iov.iov_base = addr;
		iov.iov_len = sizeof(struct __reg40);
		uio.uio_iov = &iov;
		uio.uio_iovcnt = 1;
		uio.uio_offset = 0;
		uio.uio_resid = sizeof(struct __reg40);
		uio.uio_rw = write ? UIO_WRITE : UIO_READ;
		uio.uio_vmspace = l->l_proc->p_vmspace;
		return process_machdep_doregs40(l, lt, &uio);
#endif	/* COMPAT_40 */
	}
}


#ifdef COMPAT_40

static int
process_machdep_doregs40(struct lwp *curl, struct lwp *l, struct uio *uio)
{
	struct __reg40 r;
	int error;
	char *kv;
	int kl;

	kl = sizeof(r);
	kv = (char *) &r;

	kv += uio->uio_offset;
	kl -= uio->uio_offset;
	if (kl > uio->uio_resid)
		kl = uio->uio_resid;

	if (kl < 0)
		error = EINVAL;
	else
		error = process_machdep_read_regs40(l, &r);
	if (error == 0)
		error = uiomove(kv, kl, uio);
	if (error == 0 && uio->uio_rw == UIO_WRITE) {
		if (l->l_proc->p_stat != SSTOP)
			error = EBUSY;
		else
			error = process_machdep_write_regs40(l, &r);
	}

	uio->uio_offset = 0;
	return error;
}

/*
 * Like process_read_regs() but for old struct reg w/out r_gbr.
 */
static int
process_machdep_read_regs40(struct lwp *l, struct __reg40 *regs)
{
	struct trapframe *tf = process_frame(l);

	regs->r_spc = tf->tf_spc;
	regs->r_ssr = tf->tf_ssr;
	/* no r_gbr in struct __reg40 */
	regs->r_macl = tf->tf_macl;
	regs->r_mach = tf->tf_mach;
	regs->r_pr = tf->tf_pr;
	regs->r_r14 = tf->tf_r14;
	regs->r_r13 = tf->tf_r13;
	regs->r_r12 = tf->tf_r12;
	regs->r_r11 = tf->tf_r11;
	regs->r_r10 = tf->tf_r10;
	regs->r_r9 = tf->tf_r9;
	regs->r_r8 = tf->tf_r8;
	regs->r_r7 = tf->tf_r7;
	regs->r_r6 = tf->tf_r6;
	regs->r_r5 = tf->tf_r5;
	regs->r_r4 = tf->tf_r4;
	regs->r_r3 = tf->tf_r3;
	regs->r_r2 = tf->tf_r2;
	regs->r_r1 = tf->tf_r1;
	regs->r_r0 = tf->tf_r0;
	regs->r_r15 = tf->tf_r15;

	return 0;
}

/*
 * Like process_write_regs() but for old struct reg w/out r_gbr
 */
static int
process_machdep_write_regs40(struct lwp *l, struct __reg40 *regs)
{
	struct trapframe *tf = process_frame(l);

	/*
	 * Check for security violations.
	 */
	if (((regs->r_ssr ^ tf->tf_ssr) & PSL_USERSTATIC) != 0) {
		return EINVAL;
	}

	tf->tf_spc = regs->r_spc;
	tf->tf_ssr = regs->r_ssr;
	tf->tf_pr = regs->r_pr;

	/* no r_gbr in struct __reg40 */
	tf->tf_mach = regs->r_mach;
	tf->tf_macl = regs->r_macl;
	tf->tf_r14 = regs->r_r14;
	tf->tf_r13 = regs->r_r13;
	tf->tf_r12 = regs->r_r12;
	tf->tf_r11 = regs->r_r11;
	tf->tf_r10 = regs->r_r10;
	tf->tf_r9 = regs->r_r9;
	tf->tf_r8 = regs->r_r8;
	tf->tf_r7 = regs->r_r7;
	tf->tf_r6 = regs->r_r6;
	tf->tf_r5 = regs->r_r5;
	tf->tf_r4 = regs->r_r4;
	tf->tf_r3 = regs->r_r3;
	tf->tf_r2 = regs->r_r2;
	tf->tf_r1 = regs->r_r1;
	tf->tf_r0 = regs->r_r0;
	tf->tf_r15 = regs->r_r15;

	return 0;
}

#endif /* COMPAT_40 */

#endif /* __HAVE_PTRACE_MACHDEP  */


int
process_sstep(struct lwp *l, int sstep)
{

	if (sstep)
		return (EINVAL);

	return (0);
}

int
process_set_pc(struct lwp *l, void *addr)
{
	struct trapframe *tf = process_frame(l);

	tf->tf_spc = (int)addr;

	return (0);
}

#endif /* PTRACE */
