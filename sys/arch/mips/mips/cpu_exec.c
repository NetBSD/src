/*	$NetBSD: cpu_exec.c,v 1.50.54.1.4.4 2009/08/26 03:46:39 matt Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by Ralph
 * Campbell.
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
 *	@(#)machdep.c	8.3 (Berkeley) 1/12/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu_exec.c,v 1.50.54.1.4.4 2009/08/26 03:46:39 matt Exp $");

#include "opt_compat_netbsd.h"
#include "opt_compat_ultrix.h"
#include "opt_execfmt.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <sys/resourcevar.h>

#include <uvm/uvm_extern.h>

#ifdef EXEC_ECOFF
#include <sys/exec_ecoff.h>
#endif
#include <sys/exec_elf.h>			/* mandatory */
#ifdef COMPAT_09
#include <machine/bsd-aout.h>
#endif
#include <machine/reg.h>
#include <mips/regnum.h>			/* symbolic register indices */

int	mips_elf_makecmds(struct lwp *, struct exec_package *);


/*
 * cpu_exec_aout_makecmds():
 *	cpu-dependent a.out format hook for execve().
 *
 * Determine of the given exec package refers to something which we
 * understand and, if so, set up the vmcmds for it.
 *
 */
int
cpu_exec_aout_makecmds(l, epp)
	struct lwp *l;
	struct exec_package *epp;
{
	int error;

	/* If COMPAT_09 is defined, allow loading of old-style 4.4bsd a.out
	   executables. */
#ifdef COMPAT_09
	struct bsd_aouthdr *hdr = (struct bsd_aouthdr *)epp->ep_hdr;

	/* Only handle paged files (laziness). */
	if (hdr->a_magic != BSD_ZMAGIC)
#endif
	{
		/* If that failed, try old NetBSD-1.1 elf format */
		error = mips_elf_makecmds (l, epp);
		return error;
	}

#ifdef COMPAT_09
	error = vn_marktext(epp->ep_vp);
	if (error)
		return (error);

	epp->ep_taddr = 0x1000;
	epp->ep_entry = hdr->a_entry;
	epp->ep_tsize = hdr->a_text;
	epp->ep_daddr = epp->ep_taddr + hdr->a_text;
	epp->ep_dsize = hdr->a_data + hdr->a_bss;

	/* set up command for text segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, hdr->a_text,
	    epp->ep_taddr, epp->ep_vp, 0, VM_PROT_READ|VM_PROT_EXECUTE);

	/* set up command for data segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, hdr->a_data,
	    epp->ep_daddr, epp->ep_vp, hdr->a_text,
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, hdr->a_bss,
	    epp->ep_daddr + hdr->a_data, NULLVP, 0,
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	return (*epp->ep_esch->ep_setup_stack)(p, epp);
#endif
}

#ifdef EXEC_ECOFF
void
cpu_exec_ecoff_setregs(l, epp, stack)
	struct lwp *l;
	struct exec_package *epp;
	vaddr_t stack;
{
	struct ecoff_exechdr *execp = (struct ecoff_exechdr *)epp->ep_hdr;
	struct frame *f = l->l_md.md_regs;

	f->f_regs[_R_GP] = (register_t)execp->a.gp_value;
}

/*
 * cpu_exec_ecoff_probe()
 *	cpu-dependent ECOFF format hook for execve().
 *
 * Do any machine-dependent diddling of the exec package when doing ECOFF.
 */
int
cpu_exec_ecoff_probe(l, epp)
	struct lwp *l;
	struct exec_package *epp;
{

	/* NetBSD/mips does not have native ECOFF binaries. */
	return ENOEXEC;
}
#endif /* EXEC_ECOFF */

/*
 * mips_elf_makecmds (l, epp)
 *
 * Test if an executable is a MIPS ELF executable.   If it is,
 * try to load it.
 */

int
mips_elf_makecmds (l, epp)
        struct lwp *l;
        struct exec_package *epp;
{
	Elf32_Ehdr *ex = (Elf32_Ehdr *)epp->ep_hdr;
	Elf32_Phdr ph;
	int i, error;
	size_t resid;

	/* Make sure we got enough data to check magic numbers... */
	if (epp->ep_hdrvalid < sizeof (Elf32_Ehdr)) {
#ifdef DIAGNOSTIC
		if (epp->ep_hdrlen < sizeof (Elf32_Ehdr))
			printf ("mips_elf_makecmds: execsw hdrsize too short!\n");
#endif
	    return ENOEXEC;
	}

	/* See if it's got the basic elf magic number leadin... */
	if (memcmp(ex->e_ident, ELFMAG, SELFMAG) != 0) {
		return ENOEXEC;
	}

	/* XXX: Check other magic numbers here. */
	if (ex->e_ident[EI_CLASS] != ELFCLASS32) {
		return ENOEXEC;
	}

	/* See if we got any program header information... */
	if (!ex->e_phoff || !ex->e_phnum) {
		return ENOEXEC;
	}

	error = vn_marktext(epp->ep_vp);
	if (error)
		return (error);

	/* Set the entry point... */
	epp->ep_entry = ex->e_entry;
	epp->ep_taddr = 0;
	epp->ep_tsize = 0;
	epp->ep_daddr = 0;
	epp->ep_dsize = 0;

	for (i = 0; i < ex->e_phnum; i++) {
#ifdef DEBUG
		/*printf("obsolete elf: mapping %x %x %x\n", resid);*/
#endif
		if ((error = vn_rdwr(UIO_READ, epp->ep_vp, (void *)&ph,
				    sizeof ph, ex->e_phoff + i * sizeof ph,
				    UIO_SYSSPACE, IO_NODELOCKED,
				    l->l_cred, &resid, NULL))
		    != 0)
			return error;

		if (resid != 0) {
			return ENOEXEC;
		}

		/* We only care about loadable sections... */
		if (ph.p_type == PT_LOAD) {
			int prot = VM_PROT_READ | VM_PROT_EXECUTE;
			int residue;
			unsigned vaddr, offset, length;

			vaddr = ph.p_vaddr;
			offset = ph.p_offset;
			length = ph.p_filesz;
			residue = ph.p_memsz - ph.p_filesz;

			if (ph.p_flags & PF_W) {
				prot |= VM_PROT_WRITE;
				if (!epp->ep_daddr || vaddr < epp->ep_daddr)
					epp->ep_daddr = vaddr;
				epp->ep_dsize += ph.p_memsz;
				/* Read the data from the file... */
				NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn,
					  length, vaddr,
					  epp->ep_vp, offset, prot);
#ifdef OLD_ELF_DEBUG
/*XXX*/		printf(
	"obsolete elf: NEW_VMCMD len %x va %x off %x prot %x residue %x\n",
			length, vaddr, offset, prot, residue);
#endif /*ELF_DEBUG*/

				if (residue) {
					vaddr &= ~(PAGE_SIZE - 1);
					offset &= ~(PAGE_SIZE - 1);
					length = roundup (length + ph.p_vaddr
							  - vaddr, PAGE_SIZE);
					residue = (ph.p_vaddr + ph.p_memsz)
						  - (vaddr + length);
				}
			} else {
				vaddr &= ~(PAGE_SIZE - 1);
				offset &= ~(PAGE_SIZE - 1);
				length = roundup (length + ph.p_vaddr - vaddr,
						  PAGE_SIZE);
				residue = (ph.p_vaddr + ph.p_memsz)
					  - (vaddr + length);
				if (!epp->ep_taddr || vaddr < epp->ep_taddr)
					epp->ep_taddr = vaddr;
				epp->ep_tsize += ph.p_memsz;
				/* Map the data from the file... */
				NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn,
					  length, vaddr,
					  epp->ep_vp, offset, prot);
			}
			/* If part of the segment is just zeros (e.g., bss),
			   map that. */
			if (residue > 0) {
#ifdef OLD_ELF_DEBUG
/*XXX*/			printf(
	"old elf:resid NEW_VMCMD len %x va %x off %x prot %x residue %x\n",
				length, vaddr + length, offset, prot, residue);
#endif /*ELF_DEBUG*/

				NEW_VMCMD (&epp->ep_vmcmds, vmcmd_map_zero,
					   residue, vaddr + length,
					   NULLVP, 0, prot);
			}
		}
	}

	epp->ep_maxsaddr = USRSTACK - MAXSSIZ;
	epp->ep_minsaddr = USRSTACK;
	epp->ep_ssize = l->l_proc->p_rlimit[RLIMIT_STACK].rlim_cur;

	/*
	 * set up commands for stack.  note that this takes *two*, one to
	 * map the part of the stack which we can access, and one to map
	 * the part which we can't.
	 *
	 * arguably, it could be made into one, but that would require the
	 * addition of another mapping proc, which is unnecessary
	 *
	 * note that in memory, things assumed to be: 0 ....... ep_maxsaddr
	 * <stack> ep_minsaddr
	 */
	NEW_VMCMD2(&epp->ep_vmcmds, vmcmd_map_zero,
	    ((epp->ep_minsaddr - epp->ep_ssize) - epp->ep_maxsaddr),
	    epp->ep_maxsaddr, NULLVP, 0, VM_PROT_NONE, VMCMD_STACK);
	NEW_VMCMD2(&epp->ep_vmcmds, vmcmd_map_zero, epp->ep_ssize,
	    (epp->ep_minsaddr - epp->ep_ssize), NULLVP, 0,
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE, VMCMD_STACK);

	return 0;
}

#if EXEC_ELF32
int
mips_netbsd_elf32_probe(struct lwp *l, struct exec_package *epp, void *eh0,
	char *idt, vaddr_t *entry_p)
{
	struct proc * const p = l->l_proc;
	const Elf32_Ehdr * const eh = eh0;
	int old_abi = p->p_md.md_abi;

	/*
	 * Verify we can support the architecture.
	 */
	switch (eh->e_flags & EF_MIPS_ARCH) {
	case EF_MIPS_ARCH_1:
		break;
	case EF_MIPS_ARCH_2:
		if (cpu_arch < CPU_ARCH_MIPS2)
			return ENOEXEC;
		break;
	case EF_MIPS_ARCH_3:
		if (cpu_arch < CPU_ARCH_MIPS4)
			return ENOEXEC;
		break;
	case EF_MIPS_ARCH_4:
		if (cpu_arch < CPU_ARCH_MIPS3)
			return ENOEXEC;
		break;
	case EF_MIPS_ARCH_5:
		if (cpu_arch < CPU_ARCH_MIPS5)
			return ENOEXEC;
		break;
	case EF_MIPS_ARCH_32:
	case EF_MIPS_ARCH_64:
		if (!CPUISMIPSNN)
			return ENOEXEC;
		break;
	}

	switch (eh->e_flags & (EF_MIPS_ABI|EF_MIPS_ABI2)) {
#if !defined(__mips_o32)
	case EF_MIPS_ABI2:
		p->p_md.md_abi = _MIPS_BSD_API_N32;
		if (old_abi != p->p_md.md_abi)
			printf("pid %d(%p): ABI set to N32 (e_flags=%#x)\n", p->p_pid, p, eh->e_flags);
		return 0;
#endif
	case EF_MIPS_ABI_O32:
		p->p_md.md_abi = _MIPS_BSD_API_O32;
		if (old_abi != p->p_md.md_abi)
			printf("pid %d(%p): ABI set to O32 (e_flags=%#x)\n", p->p_pid, p, eh->e_flags);
		return 0;
	default:
		return ENOEXEC;
	}
}

void
coredump_elf32_setup(struct lwp *l, void *eh0)
{
	struct proc * const p = l->l_proc;
	Elf32_Ehdr * const eh = eh0;

	/*
	 * Mark the type of CPU that the dump happened on.
	 */
	if (cpu_arch & CPU_ARCH_MIPS64) {
		eh->e_flags |= EF_MIPS_ARCH_64;
	} else if (cpu_arch & CPU_ARCH_MIPS32) {
		eh->e_flags |= EF_MIPS_ARCH_32;
	} else if (cpu_arch & CPU_ARCH_MIPS5) {
		eh->e_flags |= EF_MIPS_ARCH_5;
	} else if (cpu_arch & CPU_ARCH_MIPS4) {
		eh->e_flags |= EF_MIPS_ARCH_4;
	} else if (cpu_arch & CPU_ARCH_MIPS3) {
		eh->e_flags |= EF_MIPS_ARCH_3;
	} else if (cpu_arch & CPU_ARCH_MIPS2) {
		eh->e_flags |= EF_MIPS_ARCH_2;
	} else {
		eh->e_flags |= EF_MIPS_ARCH_1;
	}

	switch (p->p_md.md_abi) {
	case _MIPS_BSD_API_N32:
		eh->e_flags |= EF_MIPS_ABI2;
		break;
	case _MIPS_BSD_API_O32:
		eh->e_flags |=EF_MIPS_ABI_O32; 
		break;
	}
}
#endif

#if EXEC_ELF64
int
mips_netbsd_elf64_probe(struct lwp *l, struct exec_package *epp, void *eh0,
	char *idt, vaddr_t *entry_p)
{
	struct proc * const p = l->l_proc;
	const Elf64_Ehdr * const eh = eh0;
	int old_abi = p->p_md.md_abi;

	switch (eh->e_flags & EF_MIPS_ARCH) {
	case EF_MIPS_ARCH_1:
		return ENOEXEC;
	case EF_MIPS_ARCH_2:
		if (cpu_arch < CPU_ARCH_MIPS2)
			return ENOEXEC;
		break;
	case EF_MIPS_ARCH_3:
		if (cpu_arch < CPU_ARCH_MIPS4)
			return ENOEXEC;
		break;
	case EF_MIPS_ARCH_4:
		if (cpu_arch < CPU_ARCH_MIPS3)
			return ENOEXEC;
		break;
	case EF_MIPS_ARCH_5:
		if (cpu_arch < CPU_ARCH_MIPS5)
			return ENOEXEC;
		break;
	case EF_MIPS_ARCH_32:
		return ENOEXEC;
	case EF_MIPS_ARCH_64:
		if (!CPUISMIPS64)
			return ENOEXEC;
		break;
	}

	switch (eh->e_flags & EF_MIPS_ARCH) {
	case 0:
		p->p_md.md_abi = _MIPS_BSD_API_N64;
		if (old_abi != p->p_md.md_abi)
			printf("pid %d(%p): ABI set to N64 (e_flags=%#x)\n", p->p_pid, p, eh->e_flags);
		return 0;
	case EF_MIPS_ABI_O64:
		p->p_md.md_abi = _MIPS_BSD_API_O64;
		if (old_abi != p->p_md.md_abi)
			printf("pid %d(%p): ABI set to O64 (e_flags=%#x)\n", p->p_pid, p, eh->e_flags);
		return 0;
	default:
		return ENOEXEC;
	}
}

void
coredump_elf64_setup(struct lwp *l, void *eh0)
{
	struct proc * const p = l->l_proc;
	Elf64_Ehdr * const eh = eh0;

	/*
	 * Mark the type of CPU that the dump happened on.
	 */
	if (cpu_arch & CPU_ARCH_MIPS64) {
		eh->e_flags |= EF_MIPS_ARCH_64;
	} else if (cpu_arch & CPU_ARCH_MIPS32) {
		eh->e_flags |= EF_MIPS_ARCH_32;
	} else if (cpu_arch & CPU_ARCH_MIPS5) {
		eh->e_flags |= EF_MIPS_ARCH_5;
	} else if (cpu_arch & CPU_ARCH_MIPS4) {
		eh->e_flags |= EF_MIPS_ARCH_4;
	} else if (cpu_arch & CPU_ARCH_MIPS3) {
		eh->e_flags |= EF_MIPS_ARCH_3;
	} else if (cpu_arch & CPU_ARCH_MIPS2) {
		eh->e_flags |= EF_MIPS_ARCH_2;
	} else {
		eh->e_flags |= EF_MIPS_ARCH_1;
	}
	switch (p->p_md.md_abi) {
	case _MIPS_BSD_API_N64:
		eh->e_flags |= EF_MIPS_ABI2;
		break;
	case _MIPS_BSD_API_O64:
		eh->e_flags |= EF_MIPS_ABI_O64;
		break;
	}
}
#endif
