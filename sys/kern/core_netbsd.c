/*	$NetBSD: core_netbsd.c,v 1.7 2003/09/14 06:59:14 christos Exp $	*/

/*
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
 * Copyright (c) 1991, 1993 The Regents of the University of California.
 * Copyright (c) 1988 University of Utah.
 *
 * All rights reserved.
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
 *      This product includes software developed by Charles D. Cranor,
 *	Washington University, the University of California, Berkeley and
 *	its contributors.
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
 * from: Utah $Hdr: vm_unix.c 1.1 89/11/07$
 *      @(#)vm_unix.c   8.1 (Berkeley) 6/11/93
 * from: NetBSD: uvm_unix.c,v 1.25 2001/11/10 07:37:01 lukem Exp
 */

/*
 * core_netbsd.c: Support for the historic NetBSD core file format.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: core_netbsd.c,v 1.7 2003/09/14 06:59:14 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/core.h>

#include <uvm/uvm_extern.h>

struct coredump_state {
	struct core core;
	off_t offset;
};

int	coredump_writesegs_netbsd(struct proc *, struct vnode *,
	    struct ucred *, struct uvm_coredump_state *);

int
coredump_netbsd(struct lwp *l, struct vnode *vp, struct ucred *cred)
{
	struct coredump_state cs;
	struct proc *p;
	struct vmspace *vm;
	int error;
	
	p = l->l_proc;
	vm = p->p_vmspace;

	cs.core.c_midmag = 0;
	strncpy(cs.core.c_name, p->p_comm, MAXCOMLEN);
	cs.core.c_nseg = 0;
	cs.core.c_signo = p->p_sigctx.ps_signo;
	cs.core.c_ucode = p->p_sigctx.ps_code;
	cs.core.c_cpusize = 0;
	cs.core.c_tsize = (u_long)ctob(vm->vm_tsize);
	cs.core.c_dsize = (u_long)ctob(vm->vm_dsize);
	cs.core.c_ssize = (u_long)round_page(ctob(vm->vm_ssize));
	error = cpu_coredump(l, vp, cred, &cs.core);
	if (error)
		return (error);

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

	cs.offset = cs.core.c_hdrsize + cs.core.c_seghdrsize +
	    cs.core.c_cpusize;
	error = uvm_coredump_walkmap(p, vp, cred, coredump_writesegs_netbsd,
	    &cs);
	if (error)
		return (error);

	/* Now write out the core header. */
	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&cs.core,
	    (int)cs.core.c_hdrsize, (off_t)0,
	    UIO_SYSSPACE, IO_NODELOCKED|IO_UNIT, cred, NULL, p);

	return (error);
}

int
coredump_writesegs_netbsd(struct proc *p, struct vnode *vp, struct ucred *cred,
    struct uvm_coredump_state *us)
{
	struct coredump_state *cs = us->cookie;
	struct coreseg cseg;
	int flag, error;

	if (us->flags & UVM_COREDUMP_NODUMP)
		return (0);

	if (us->flags & UVM_COREDUMP_STACK)
		flag = CORE_STACK;
	else
		flag = CORE_DATA;

	/*
	 * Set up a new core file segment.
	 */
	CORE_SETMAGIC(cseg, CORESEGMAGIC, CORE_GETMID(cs->core), flag);
	cseg.c_addr = us->start;
	cseg.c_size = us->end - us->start;

	error = vn_rdwr(UIO_WRITE, vp,
	    (caddr_t)&cseg, cs->core.c_seghdrsize,
	    cs->offset, UIO_SYSSPACE,
	    IO_NODELOCKED|IO_UNIT, cred, NULL, p);
	if (error)
		return (error);

	cs->offset += cs->core.c_seghdrsize;
	error = vn_rdwr(UIO_WRITE, vp,
	    (caddr_t) us->start, (int) cseg.c_size,
	    cs->offset, UIO_USERSPACE,
	    IO_NODELOCKED|IO_UNIT, cred, NULL, p);
	if (error)
		return (error);

	cs->offset += cseg.c_size;
	cs->core.c_nseg++;

	return (0);
}
