/*	$NetBSD: svr4_exec.c,v 1.2.2.2 1994/08/15 22:48:37 mycroft Exp $	*/

/*
 * Copyright (c) 1994 Christos Zoulas
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/exec.h>
#include <sys/resourcevar.h>
#include <sys/wait.h>

#include <sys/mman.h>
#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#include <vm/vm_pager.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/exec.h>

#include <compat/svr4/svr4_exec.h>


#ifdef DEBUG_SVR4
#define DPRINTF(a) printf a
#else
#define DPRINTF(a)
#endif

/*
 * exec_svr4_elf_makecmds(): Prepare an Elf binary's exec package
 *
 * First, set of the various offsets/lengths in the exec package.
 *
 * Then, mark the text image busy (so it can be demand paged) or error
 * out if this is not possible.  Finally, set up vmcmds for the
 * text, data, bss, and stack segments.
 */
int
exec_svr4_elf_makecmds(p, epp)
    struct proc *p;
    struct exec_package *epp;
{
    Elf32_Ehdr *eh = epp->ep_hdr;
    Elf32_Phdr ph; 
    int error;
    struct exec_vmcmd *ccmdp;
    int i, resid;
    int s;
    long diff;
    u_long pos;

    if (epp->ep_hdrvalid < sizeof(Elf32_Ehdr))
	return ENOEXEC;

    if (memcmp(eh->e_ident, Elf32_e_ident, Elf32_e_siz) != 0) {
	DPRINTF(("Not an elf file\n"));
	return ENOEXEC;
    }

    if (eh->e_type != Elf32_et_exec) {
	DPRINTF(("Not an elf executable\n"));
	return ENOEXEC;
    }

    if (eh->e_machine != Elf32_em_386 && eh->e_machine != Elf32_em_486) {
	DPRINTF(("Not an elf/386 or 486 executable\n"));
	return ENOEXEC;
    }

    /*
     * check if vnode is in open for writing, because we want to
     * demand-page out of it.  if it is, don't do it, for various
     * reasons
     */
    if (epp->ep_vp->v_writecount != 0) {
#ifdef DIAGNOSTIC
	if (epp->ep_vp->v_flag & VTEXT)
	    panic("exec: a VTEXT vnode has writecount != 0\n");
#endif
	return ETXTBSY;
    }

    epp->ep_emul = EMUL_IBCS2_ELF;
    epp->ep_tsize = ~0;
    epp->ep_dsize = ~0;
    pos = eh->e_phoff;
    for (i = 0; i < eh->e_phnum; i++, pos += sizeof(ph)) {
	u_long vaddr, offset;
	int prot = 0;
	s = sizeof(ph);

	if (error = vn_rdwr(UIO_READ, epp->ep_vp, (caddr_t) &ph, s, pos,
			    UIO_SYSSPACE, IO_NODELOCKED, p->p_ucred,
			    &resid, p)) {
	    DPRINTF(("Program header %d read error %d\n", i, error));
	    return error;
	}
	s -= resid;
	if (s != sizeof(ph)) {
	    DPRINTF(("Incomplete read for header %d ask=%d, rem=%d got %d\n",
		 sizeof(ph), resid, s));
	    return ENOEXEC;
	}

	if (ph.p_type != Elf32_pt_load)
	    continue;

	/*
	 * Kludge: Unfortunately the current implementation of
	 * exec package assumes a single text and data segment.
	 * In Elf we can have more, but here we limit ourselves
	 * to two and hope :-(
	 * We also assume that the text is rx, and data is rwx.
	 */
#define SVR4_ALIGN(a, b) ((a) & ~((b) - 1))

	vaddr = SVR4_ALIGN(ph.p_vaddr, ph.p_align);
	diff =  ph.p_vaddr - vaddr;
	offset = ph.p_offset - diff;
	s = ph.p_memsz + diff;
	DPRINTF(("Elf Segment@ %x, size %d, offset %x\n",
		 ph.p_vaddr, ph.p_memsz, ph.p_offset));

	prot |= (ph.p_flags & Elf32_pf_r) ? VM_PROT_READ : 0;
	prot |= (ph.p_flags & Elf32_pf_w) ? VM_PROT_WRITE : 0;
	prot |= (ph.p_flags & Elf32_pf_x) ? VM_PROT_EXECUTE : 0;

	switch (prot) {
	case (VM_PROT_READ|VM_PROT_EXECUTE):
	    if (epp->ep_tsize != ~0) {
		DPRINTF(("More than one text segment\n"));
		return ENOEXEC;
	    }

	    epp->ep_taddr = vaddr;
	    epp->ep_tsize = s;
	    DPRINTF(("Elf Text@ %x, size %d, offset %x\n",
		     vaddr, s, offset));
	    break;

	case (VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE):
	    if (epp->ep_dsize != ~0) {
		DPRINTF(("More than one data segment\n"));
		return ENOEXEC;
	    }

	    epp->ep_daddr = vaddr;
	    epp->ep_dsize = s;

	    DPRINTF(("Elf Data@ %x, size %d, offset %x\n",
		     vaddr, s, offset));
	    break;

	default:
	    return ENOEXEC;
	}
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, s,
		  vaddr, epp->ep_vp, offset, prot);
    }

    epp->ep_entry = eh->e_entry;

    DPRINTF(("Elf entry@ %x\n", epp->ep_entry));
    epp->ep_vp->v_flag |= VTEXT;

    return exec_aout_setup_stack(p, epp);
}
