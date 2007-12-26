/*	$NetBSD: hpux_exec_aout.c,v 1.15.4.2 2007/12/26 21:38:53 ad Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

/*
 * Copyright (c) 1993, 1994 Christopher G. Demetriou
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
 *	This product includes software developed by Christopher G. Demetriou.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Glue for exec'ing HP-UX executables and the HP-UX execv() system call.
 * Based on sys/kern/exec_aout.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hpux_exec_aout.c,v 1.15.4.2 2007/12/26 21:38:53 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <sys/cpu.h>
#include <machine/reg.h>

#include <sys/syscallargs.h>

#include <compat/hpux/hpux.h>
#include <compat/hpux/hpux_util.h>
#include <compat/hpux/hpux_syscall.h>
#include <compat/hpux/hpux_syscallargs.h>

#include <machine/hpux_machdep.h>

static	int exec_hpux_prep_nmagic(struct lwp *, struct exec_package *);
static	int exec_hpux_prep_zmagic(struct lwp *, struct exec_package *);
static	int exec_hpux_prep_omagic(struct lwp *, struct exec_package *);

int
exec_hpux_makecmds(struct lwp *l, struct exec_package *epp)
{
	struct hpux_exec *hpux_ep = epp->ep_hdr;
	short sysid, magic;
	int error = ENOEXEC;

	magic = HPUX_MAGIC(hpux_ep);
	sysid = HPUX_SYSID(hpux_ep);

	/*
	 * XXX This will lose if there's ever an hp700 port.
	 */
	if (sysid != MID_HPUX)
		return (ENOEXEC);

	/*
	 * HP-UX is a 4k page size system, and executables assume
	 * this.
	 */
	if (PAGE_SIZE != HPUX_LDPGSZ)
		return (ENOEXEC);

	switch (magic) {
	case OMAGIC:
		error = exec_hpux_prep_omagic(l, epp);
		break;

	case NMAGIC:
		error = exec_hpux_prep_nmagic(l, epp);
		break;

	case ZMAGIC:
		error = exec_hpux_prep_zmagic(l, epp);
		break;
	}

	if (error != 0)
		kill_vmcmds(&epp->ep_vmcmds);

	return (error);
}

static int
exec_hpux_prep_nmagic(struct lwp *l, struct exec_package *epp)
{
	struct hpux_exec *execp = epp->ep_hdr;
	long bsize, baddr;

	epp->ep_taddr = 0;
	epp->ep_tsize = execp->ha_text;
	epp->ep_daddr = epp->ep_taddr + roundup(execp->ha_text, HPUX_LDPGSZ);
	epp->ep_dsize = execp->ha_data + execp->ha_bss;
	epp->ep_entry = execp->ha_entry;

	/* set up command for text segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, execp->ha_text,
	    epp->ep_taddr, epp->ep_vp, HPUX_TXTOFF(*execp, NMAGIC),
	    VM_PROT_READ|VM_PROT_EXECUTE);

	/* set up command for data segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, execp->ha_data,
	    epp->ep_daddr, epp->ep_vp, HPUX_DATAOFF(*execp, NMAGIC),
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	baddr = roundup(epp->ep_daddr + execp->ha_data, PAGE_SIZE);
	bsize = epp->ep_daddr + epp->ep_dsize - baddr;
	if (bsize > 0)
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, bsize, baddr,
		    NULLVP, 0, VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	return (*epp->ep_esch->es_setup_stack)(l, epp);
}

static int
exec_hpux_prep_zmagic(struct lwp *l, struct exec_package *epp)
{
	struct hpux_exec *execp = epp->ep_hdr;
	long bsize, baddr;
	long nontext;
	int error;

	error = vn_marktext(epp->ep_vp);
	if (error)
		return (error);

	/*
	 * HP-UX ZMAGIC executables need to have their segment
	 * sizes frobbed.
	 */
	nontext = execp->ha_data + execp->ha_bss;
	execp->ha_text = ctob(btoc(execp->ha_text));
	execp->ha_data = ctob(btoc(execp->ha_data));
	execp->ha_bss = nontext - execp->ha_data;
	if (execp->ha_bss < 0)
		execp->ha_bss = 0;

	epp->ep_taddr = 0;
	epp->ep_tsize = execp->ha_text;
	epp->ep_daddr = epp->ep_taddr + roundup(execp->ha_text, HPUX_LDPGSZ);
	epp->ep_dsize = execp->ha_data + execp->ha_bss;
	epp->ep_entry = execp->ha_entry;

	/* set up command for text segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, execp->ha_text,
	    epp->ep_taddr, epp->ep_vp, HPUX_TXTOFF(*execp, ZMAGIC),
	    VM_PROT_READ|VM_PROT_EXECUTE);

	/* set up command for data segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, execp->ha_data,
	    epp->ep_daddr, epp->ep_vp, HPUX_DATAOFF(*execp, ZMAGIC),
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	baddr = roundup(epp->ep_daddr + execp->ha_data, PAGE_SIZE);
	bsize = epp->ep_daddr + epp->ep_dsize - baddr;
	if (bsize > 0)
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, bsize, baddr,
		    NULLVP, 0, VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	return (*epp->ep_esch->es_setup_stack)(l, epp);
}

/*
 * HP-UX's version of OMAGIC.
 */
static int
exec_hpux_prep_omagic(struct lwp *l, struct exec_package *epp)
{
	struct hpux_exec *execp = epp->ep_hdr;
	long dsize, bsize, baddr;

	epp->ep_taddr = 0;
	epp->ep_tsize = execp->ha_text;
	epp->ep_daddr = epp->ep_taddr + roundup(execp->ha_text, HPUX_LDPGSZ);
	epp->ep_dsize = execp->ha_data + execp->ha_bss;
	epp->ep_entry = execp->ha_entry;

	/* set up command for text and data segments */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn,
	    execp->ha_text + execp->ha_data, epp->ep_taddr, epp->ep_vp,
	    HPUX_TXTOFF(*execp, OMAGIC),
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	baddr = roundup(epp->ep_daddr + execp->ha_data, PAGE_SIZE);
	bsize = epp->ep_daddr + epp->ep_dsize - baddr;
	if (bsize > 0)
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, bsize, baddr,
		    NULLVP, 0, VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/*
	 * Make sure (# of pages) mapped above equals (vm_tsize + vm_dsize);
	 * obreak(2) relies on this fact. Both `vm_tsize' and `vm_dsize' are
	 * computed (in execve(2)) by rounding *up* `ep_tsize' and `ep_dsize'
	 * respectively to page boundaries.
	 * Compensate `ep_dsize' for the amount of data covered by the last
	 * text page.
	 */
	dsize = epp->ep_dsize + execp->ha_text - roundup(execp->ha_text,
							 PAGE_SIZE);
	epp->ep_dsize = (dsize > 0) ? dsize : 0;
	return (*epp->ep_esch->es_setup_stack)(l, epp);
}
