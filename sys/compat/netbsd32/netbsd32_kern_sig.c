/*	$NetBSD: netbsd32_kern_sig.c,v 1.1 2001/06/06 21:25:11 mrg Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)kern_sig.c	8.14 (Berkeley) 5/14/95
 * from: NetBSD: kern_sig.c,v 1.112 2001/02/26 21:58:30 lukem Exp
 */

#include <sys/param.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/core.h>
#include <sys/fcntl.h>

#include <uvm/uvm_extern.h>

#include <compat/netbsd32/netbsd32.h>

/*
 * Same as coredump, but generates a 32-bit image.
 */
int
coredump32(struct proc *p, struct vnode *vp)
{
	struct vmspace	*vm;
	struct ucred	*cred;
	int		error, error1;
	struct core32	core;

	vm = p->p_vmspace;
	cred = p->p_cred->pc_ucred;
#if 0
	/*
	 * XXX
	 * It would be nice if we at least dumped the signal state (and made it
	 * available at run time to the debugger, as well), but this code
	 * hasn't actually had any effect for a long time, since we don't dump
	 * the user area.  For now, it's dead.
	 */
	memcpy(&p->p_addr->u_kproc.kp_proc, p, sizeof(struct proc));
	fill_eproc(p, &p->p_addr->u_kproc.kp_eproc);
#endif

	core.c_midmag = 0;
	strncpy(core.c_name, p->p_comm, MAXCOMLEN);
	core.c_nseg = 0;
	core.c_signo = p->p_sigctx.ps_sig;
	core.c_ucode = p->p_sigctx.ps_code;
	core.c_cpusize = 0;
	core.c_tsize = (u_long)ctob(vm->vm_tsize);
	core.c_dsize = (u_long)ctob(vm->vm_dsize);
	core.c_ssize = (u_long)round_page(ctob(vm->vm_ssize));
	error = cpu_coredump32(p, vp, cred, &core);
	if (error)
		goto out;
	/*
	 * uvm_coredump() spits out all appropriate segments.
	 * All that's left to do is to write the core header.
	 */
	error = uvm_coredump32(p, vp, cred, &core);
	if (error)
		goto out;
	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&core,
	    (int)core.c_hdrsize, (off_t)0,
	    UIO_SYSSPACE, IO_NODELOCKED|IO_UNIT, cred, NULL, p);
 out:
	VOP_UNLOCK(vp, 0);
	error1 = vn_close(vp, FWRITE, cred, p);
	if (error == 0)
		error = error1;
	return (error);
}
