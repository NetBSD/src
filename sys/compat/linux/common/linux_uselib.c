/*	$NetBSD: linux_uselib.c,v 1.1.2.3 2000/12/13 15:49:50 bouyer Exp $	*/

/*-
 * Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas, Frank van der Linden and Eric Haszlakiewicz.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include <sys/mman.h>
#include <sys/syscallargs.h>

#include <machine/cpu.h>
#include <machine/reg.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_exec.h>
#include <compat/linux/common/linux_machdep.h>

#ifndef EXEC_AOUT
/* define EXEC_AOUT to get prototype from linux_syscall.h */
#define EXEC_AOUT
#endif

#include <compat/linux/linux_syscallargs.h>
#include <compat/linux/linux_syscall.h>

/*
 * The Linux system call to load shared libraries, a.out version. The
 * a.out shared libs are just files that are mapped onto a fixed
 * address in the process' address space. The address is given in
 * a_entry. Read in the header, set up some VM commands and run them.
 *
 * Yes, both text and data are mapped at once, so we're left with
 * writeable text for the shared libs. The Linux crt0 seemed to break
 * sometimes when data was mapped seperately. It munmapped a uselib()
 * of ld.so by hand, which failed with shared text and data for ld.so
 * Yuck.
 *
 * Because of the problem with ZMAGIC executables (text starts
 * at 0x400 in the file, but needs to be mapped at 0), ZMAGIC
 * shared libs are not handled very efficiently :-(
 */

int
linux_sys_uselib(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_uselib_args /* {
		syscallarg(const char *) path;
	} */ *uap = v;
	caddr_t sg;
	long bsize, dsize, tsize, taddr, baddr, daddr;
	struct nameidata ni;
	struct vnode *vp;
	struct exec hdr;
	struct exec_vmcmd_set vcset;
	int i, magic, error;
	size_t rem;

	sg = stackgap_init(p->p_emul);
	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	NDINIT(&ni, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);

	if ((error = namei(&ni)))
		return error;

	vp = ni.ni_vp;

	if ((error = vn_rdwr(UIO_READ, vp, (caddr_t) &hdr, LINUX_AOUT_HDR_SIZE,
			     0, UIO_SYSSPACE, IO_NODELOCKED, p->p_ucred,
			     &rem, p))) {
		vrele(vp);
		return error;
	}

	if (rem != 0) {
		vrele(vp);
		return ENOEXEC;
	}

	if (LINUX_N_MACHTYPE(&hdr) != LINUX_MID_MACHINE)
		return ENOEXEC;

	magic = LINUX_N_MAGIC(&hdr);
	taddr = hdr.a_entry & (~(NBPG - 1));
	tsize = hdr.a_text;
	daddr = taddr + tsize;
	dsize = hdr.a_data + hdr.a_bss;

	if ((hdr.a_text != 0 || hdr.a_data != 0) && vp->v_writecount != 0) {
		vrele(vp);
                return ETXTBSY;
        }
	vn_marktext(vp);

	vcset.evs_cnt = 0;
	vcset.evs_used = 0;

	NEW_VMCMD(&vcset,
		  magic == ZMAGIC ? vmcmd_map_readvn : vmcmd_map_pagedvn,
		  hdr.a_text + hdr.a_data, taddr,
		  vp, LINUX_N_TXTOFF(hdr, magic),
		  VM_PROT_READ|VM_PROT_EXECUTE|VM_PROT_WRITE);

	baddr = roundup(daddr + hdr.a_data, NBPG);
	bsize = daddr + dsize - baddr;
        if (bsize > 0) {
                NEW_VMCMD(&vcset, vmcmd_map_zero, bsize, baddr,
                    NULLVP, 0, VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
	}

	for (i = 0; i < vcset.evs_used && !error; i++) {
		struct exec_vmcmd *vcp;

		vcp = &vcset.evs_cmds[i];
		error = (*vcp->ev_proc)(p, vcp);
	}

	kill_vmcmds(&vcset);

	vrele(vp);

	return error;
}
