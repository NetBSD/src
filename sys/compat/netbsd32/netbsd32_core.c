/*	$NetBSD: netbsd32_core.c,v 1.2 2001/12/09 23:08:34 thorpej Exp $	*/

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
 * netbsd32_core.c: Support for the historic NetBSD core file format,
 * 32-bit version.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_core.c,v 1.2 2001/12/09 23:08:34 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/core.h>

#include <uvm/uvm.h>

#include "netbsd32.h"

int
coredump_netbsd32(struct proc *p, struct vnode *vp, struct ucred *cred)
{
	struct core32 core;
	struct coreseg32 cseg;
	struct vmspace *vm = p->p_vmspace;
	struct vm_map *map = &vm->vm_map;
	struct vm_map_entry *entry;
	vaddr_t start, end, maxstack;
	off_t offset;
	int flag, error;

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
		return (error);

#if 0
	/*
	 * XXX
	 * It would be nice if we at least dumped the signal state (and made it
	 * available at run time to the debugger, as well), but this code
	 * hasn't actually had any effect for a long time, since we don't dump
	 * the user area.  For now, it's dead.
	 */
#endif

	offset = core.c_hdrsize + core.c_seghdrsize + core.c_cpusize;
	maxstack = trunc_page(USRSTACK - ctob(vm->vm_ssize));

	for (entry = map->header.next; entry != &map->header;
	     entry = entry->next) {
		/* Should never happen for a user process. */
		if (UVM_ET_ISSUBMAP(entry))
			panic("coredump_netbsd: user process with submap?");

		if ((entry->protection & VM_PROT_WRITE) == 0)
			continue;

		start = entry->start;
		end = entry->end;

		if (start >= VM_MAXUSER_ADDRESS)
			continue;

		if (end > VM_MAXUSER_ADDRESS)
			end = VM_MAXUSER_ADDRESS;

		if (start >= (vaddr_t)vm->vm_maxsaddr) {
			if (end <= maxstack)
				continue;
			if (start < maxstack)
				start = maxstack;
			flag = CORE_STACK;
		} else
			flag = CORE_DATA;

		/*
		 * Set up a new core file segment.
		 */
		CORE_SETMAGIC(cseg, CORESEGMAGIC, CORE_GETMID(core), flag);
		cseg.c_addr = start;
		cseg.c_size = end - start;

		error = vn_rdwr(UIO_WRITE, vp,
		    (caddr_t)&cseg, core.c_seghdrsize,
		    offset, UIO_SYSSPACE,
		    IO_NODELOCKED|IO_UNIT, cred, NULL, p);
		if (error)
			return (error);

		offset += core.c_seghdrsize;
		error = vn_rdwr(UIO_WRITE, vp,
		    (caddr_t)(long)cseg.c_addr, (int)cseg.c_size,
		    offset, UIO_USERSPACE,
		    IO_NODELOCKED|IO_UNIT, cred, NULL, p);
		if (error)
			return (error);

		offset += cseg.c_size;
		core.c_nseg++;
	}

	/* Now write out the core header. */
	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&core,
	    (int)core.c_hdrsize, (off_t)0,
	    UIO_SYSSPACE, IO_NODELOCKED|IO_UNIT, cred, NULL, p);

	return (error);
}
