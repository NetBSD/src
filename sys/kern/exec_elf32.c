/*	$NetBSD: exec_elf32.c,v 1.120.2.1 2007/07/09 10:30:57 liamjfoy Exp $	*/

/*-
 * Copyright (c) 1994, 2000, 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 * Copyright (c) 1996 Christopher G. Demetriou
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

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: exec_elf32.c,v 1.120.2.1 2007/07/09 10:30:57 liamjfoy Exp $");

/* If not included by exec_elf64.c, ELFSIZE won't be defined. */
#ifndef ELFSIZE
#define	ELFSIZE		32
#endif

#ifdef _KERNEL_OPT
#include "opt_pax.h"
#endif /* _KERNEL_OPT */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/syscall.h>
#include <sys/signalvar.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/kauth.h>

#include <machine/cpu.h>
#include <machine/reg.h>

#if defined(PAX_MPROTECT) || defined(PAX_SEGVGUARD)
#include <sys/pax.h>
#endif /* PAX_MPROTECT || PAX_SEGVGUARD */

extern const struct emul emul_netbsd;

#define elf_check_header	ELFNAME(check_header)
#define elf_copyargs		ELFNAME(copyargs)
#define elf_load_file		ELFNAME(load_file)
#define elf_load_psection	ELFNAME(load_psection)
#define exec_elf_makecmds	ELFNAME2(exec,makecmds)
#define netbsd_elf_signature	ELFNAME2(netbsd,signature)
#define netbsd_elf_probe	ELFNAME2(netbsd,probe)

int	elf_load_file(struct lwp *, struct exec_package *, char *,
	    struct exec_vmcmd_set *, u_long *, struct elf_args *, Elf_Addr *);
void	elf_load_psection(struct exec_vmcmd_set *, struct vnode *,
	    const Elf_Phdr *, Elf_Addr *, u_long *, int *, int);

int	netbsd_elf_signature(struct lwp *, struct exec_package *, Elf_Ehdr *);
int	netbsd_elf_probe(struct lwp *, struct exec_package *, void *, char *,
	    vaddr_t *);

/* round up and down to page boundaries. */
#define	ELF_ROUND(a, b)		(((a) + (b) - 1) & ~((b) - 1))
#define	ELF_TRUNC(a, b)		((a) & ~((b) - 1))

#define MAXPHNUM	50

/*
 * Copy arguments onto the stack in the normal way, but add some
 * extra information in case of dynamic binding.
 */
int
elf_copyargs(struct lwp *l, struct exec_package *pack,
    struct ps_strings *arginfo, char **stackp, void *argp)
{
	size_t len;
	AuxInfo ai[ELF_AUX_ENTRIES], *a;
	struct elf_args *ap;
	int error;

	if ((error = copyargs(l, pack, arginfo, stackp, argp)) != 0)
		return error;

	a = ai;

	/*
	 * Push extra arguments on the stack needed by dynamically
	 * linked binaries
	 */
	if ((ap = (struct elf_args *)pack->ep_emul_arg)) {
		struct vattr *vap = pack->ep_vap;

		a->a_type = AT_PHDR;
		a->a_v = ap->arg_phaddr;
		a++;

		a->a_type = AT_PHENT;
		a->a_v = ap->arg_phentsize;
		a++;

		a->a_type = AT_PHNUM;
		a->a_v = ap->arg_phnum;
		a++;

		a->a_type = AT_PAGESZ;
		a->a_v = PAGE_SIZE;
		a++;

		a->a_type = AT_BASE;
		a->a_v = ap->arg_interp;
		a++;

		a->a_type = AT_FLAGS;
		a->a_v = 0;
		a++;

		a->a_type = AT_ENTRY;
		a->a_v = ap->arg_entry;
		a++;

		a->a_type = AT_EUID;
		if (vap->va_mode & S_ISUID)
			a->a_v = vap->va_uid;
		else
			a->a_v = kauth_cred_geteuid(l->l_cred);
		a++;

		a->a_type = AT_RUID;
		a->a_v = kauth_cred_getuid(l->l_cred);
		a++;

		a->a_type = AT_EGID;
		if (vap->va_mode & S_ISGID)
			a->a_v = vap->va_gid;
		else
			a->a_v = kauth_cred_getegid(l->l_cred);
		a++;

		a->a_type = AT_RGID;
		a->a_v = kauth_cred_getgid(l->l_cred);
		a++;

		free(ap, M_TEMP);
		pack->ep_emul_arg = NULL;
	}

	a->a_type = AT_NULL;
	a->a_v = 0;
	a++;

	len = (a - ai) * sizeof(AuxInfo);
	if ((error = copyout(ai, *stackp, len)) != 0)
		return error;
	*stackp += len;

	return 0;
}

/*
 * elf_check_header():
 *
 * Check header for validity; return 0 of ok ENOEXEC if error
 */
int
elf_check_header(Elf_Ehdr *eh, int type)
{

	if (memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0 ||
	    eh->e_ident[EI_CLASS] != ELFCLASS)
		return ENOEXEC;

	switch (eh->e_machine) {

	ELFDEFNNAME(MACHDEP_ID_CASES)

	default:
		return ENOEXEC;
	}

	if (ELF_EHDR_FLAGS_OK(eh) == 0)
		return ENOEXEC;

	if (eh->e_type != type)
		return ENOEXEC;

	if (eh->e_shnum > 32768 || eh->e_phnum > 128)
		return ENOEXEC;

	return 0;
}

/*
 * elf_load_psection():
 *
 * Load a psection at the appropriate address
 */
void
elf_load_psection(struct exec_vmcmd_set *vcset, struct vnode *vp,
    const Elf_Phdr *ph, Elf_Addr *addr, u_long *size, int *prot, int flags)
{
	u_long msize, psize, rm, rf;
	long diff, offset;

	/*
	 * If the user specified an address, then we load there.
	 */
	if (*addr == ELFDEFNNAME(NO_ADDR))
		*addr = ph->p_vaddr;

	if (ph->p_align > 1) {
		/*
		 * Make sure we are virtually aligned as we are supposed to be.
		 */
		diff = ph->p_vaddr - ELF_TRUNC(ph->p_vaddr, ph->p_align);
		KASSERT(*addr - diff == ELF_TRUNC(*addr, ph->p_align));
		/*
		 * But make sure to not map any pages before the start of the
		 * psection by limiting the difference to within a page.
		 */
		diff &= PAGE_MASK;
	} else
		diff = 0;

	*prot |= (ph->p_flags & PF_R) ? VM_PROT_READ : 0;
	*prot |= (ph->p_flags & PF_W) ? VM_PROT_WRITE : 0;
	*prot |= (ph->p_flags & PF_X) ? VM_PROT_EXECUTE : 0;

	/*
	 * Adjust everything so it all starts on a page boundary.
	 */
	*addr -= diff;
	offset = ph->p_offset - diff;
	*size = ph->p_filesz + diff;
	msize = ph->p_memsz + diff;

	if (ph->p_align >= PAGE_SIZE) {
		if ((ph->p_flags & PF_W) != 0) {
			/*
			 * Because the pagedvn pager can't handle zero fill
			 * of the last data page if it's not page aligned we
			 * map the last page readvn.
			 */
			psize = trunc_page(*size);
		} else {
			psize = round_page(*size);
		}
	} else {
		psize = *size;
	}

	if (psize > 0) {
		NEW_VMCMD2(vcset, ph->p_align < PAGE_SIZE ?
		    vmcmd_map_readvn : vmcmd_map_pagedvn, psize, *addr, vp,
		    offset, *prot, flags);
		flags &= VMCMD_RELATIVE;
	}
	if (psize < *size) {
		NEW_VMCMD2(vcset, vmcmd_map_readvn, *size - psize,
		    *addr + psize, vp, offset + psize, *prot, flags);
	}

	/*
	 * Check if we need to extend the size of the segment (does
	 * bss extend page the next page boundary)?
	 */
	rm = round_page(*addr + msize);
	rf = round_page(*addr + *size);

	if (rm != rf) {
		NEW_VMCMD2(vcset, vmcmd_map_zero, rm - rf, rf, NULLVP,
		    0, *prot, flags & VMCMD_RELATIVE);
		*size = msize;
	}
}

/*
 * elf_load_file():
 *
 * Load a file (interpreter/library) pointed to by path
 * [stolen from coff_load_shlib()]. Made slightly generic
 * so it might be used externally.
 */
int
elf_load_file(struct lwp *l, struct exec_package *epp, char *path,
    struct exec_vmcmd_set *vcset, u_long *entryoff, struct elf_args *ap,
    Elf_Addr *last)
{
	int error, i;
	struct nameidata nd;
	struct vnode *vp;
	struct vattr attr;
	Elf_Ehdr eh;
	Elf_Phdr *ph = NULL;
	const Elf_Phdr *ph0;
	const Elf_Phdr *base_ph;
	const Elf_Phdr *last_ph;
	u_long phsize;
	Elf_Addr addr = *last;
	struct proc *p;

	p = l->l_proc;

	/*
	 * 1. open file
	 * 2. read filehdr
	 * 3. map text, data, and bss out of it using VM_*
	 */
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_SYSSPACE, path, l);
	if ((error = namei(&nd)) != 0)
		return error;
	vp = nd.ni_vp;

	/*
	 * Similarly, if it's not marked as executable, or it's not a regular
	 * file, we don't allow it to be used.
	 */
	if (vp->v_type != VREG) {
		error = EACCES;
		goto badunlock;
	}
	if ((error = VOP_ACCESS(vp, VEXEC, l->l_cred, l)) != 0)
		goto badunlock;

	/* get attributes */
	if ((error = VOP_GETATTR(vp, &attr, l->l_cred, l)) != 0)
		goto badunlock;

	/*
	 * Check mount point.  Though we're not trying to exec this binary,
	 * we will be executing code from it, so if the mount point
	 * disallows execution or set-id-ness, we punt or kill the set-id.
	 */
	if (vp->v_mount->mnt_flag & MNT_NOEXEC) {
		error = EACCES;
		goto badunlock;
	}
	if (vp->v_mount->mnt_flag & MNT_NOSUID)
		epp->ep_vap->va_mode &= ~(S_ISUID | S_ISGID);

#ifdef notyet /* XXX cgd 960926 */
	XXX cgd 960926: (maybe) VOP_OPEN it (and VOP_CLOSE in copyargs?)
#endif

	error = vn_marktext(vp);
	if (error)
		goto badunlock;

	VOP_UNLOCK(vp, 0);

	if ((error = exec_read_from(l, vp, 0, &eh, sizeof(eh))) != 0)
		goto bad;

	if ((error = elf_check_header(&eh, ET_DYN)) != 0)
		goto bad;

	if (eh.e_phnum > MAXPHNUM)
		goto bad;

	phsize = eh.e_phnum * sizeof(Elf_Phdr);
	ph = (Elf_Phdr *)malloc(phsize, M_TEMP, M_WAITOK);

	if ((error = exec_read_from(l, vp, eh.e_phoff, ph, phsize)) != 0)
		goto bad;

#ifdef ELF_INTERP_NON_RELOCATABLE
	/*
	 * Evil hack:  Only MIPS should be non-relocatable, and the
	 * psections should have a high address (typically 0x5ffe0000).
	 * If it's now relocatable, it should be linked at 0 and the
	 * psections should have zeros in the upper part of the address.
	 * Otherwise, force the load at the linked address.
	 */
	if (*last == ELF_LINK_ADDR && (ph->p_vaddr & 0xffff0000) == 0)
		*last = ELFDEFNNAME(NO_ADDR);
#endif

	/*
	 * If no position to load the interpreter was set by a probe
	 * function, pick the same address that a non-fixed mmap(0, ..)
	 * would (i.e. something safely out of the way).
	 */
	if (*last == ELFDEFNNAME(NO_ADDR)) {
		u_long limit = 0;
		/*
		 * Find the start and ending addresses of the psections to
		 * be loaded.  This will give us the size.
		 */
		for (i = 0, ph0 = ph, base_ph = NULL; i < eh.e_phnum;
		     i++, ph0++) {
			if (ph0->p_type == PT_LOAD) {
				u_long psize = ph0->p_vaddr + ph0->p_memsz;
				if (base_ph == NULL)
					base_ph = ph0;
				if (psize > limit)
					limit = psize;
			}
		}

		if (base_ph == NULL) {
			error = ENOEXEC;
			goto bad;
		}

		/*
		 * Now compute the size and load address.
		 */
		addr = (*epp->ep_esch->es_emul->e_vm_default_addr)(p,
		    epp->ep_daddr,
		    round_page(limit) - trunc_page(base_ph->p_vaddr));
	} else
		addr = *last; /* may be ELF_LINK_ADDR */

	/*
	 * Load all the necessary sections
	 */
	for (i = 0, ph0 = ph, base_ph = NULL, last_ph = NULL;
	     i < eh.e_phnum; i++, ph0++) {
		switch (ph0->p_type) {
		case PT_LOAD: {
			u_long size;
			int prot = 0;
			int flags;

			if (base_ph == NULL) {
				/*
				 * First encountered psection is always the
				 * base psection.  Make sure it's aligned
				 * properly (align down for topdown and align
				 * upwards for not topdown).
				 */
				base_ph = ph0;
				flags = VMCMD_BASE;
				if (addr == ELF_LINK_ADDR)
					addr = ph0->p_vaddr;
				if (p->p_vmspace->vm_map.flags & VM_MAP_TOPDOWN)
					addr = ELF_TRUNC(addr, ph0->p_align);
				else
					addr = ELF_ROUND(addr, ph0->p_align);
			} else {
				u_long limit = round_page(last_ph->p_vaddr
				    + last_ph->p_memsz);
				u_long base = trunc_page(ph0->p_vaddr);

				/*
				 * If there is a gap in between the psections,
				 * map it as inaccessible so nothing else
				 * mmap'ed will be placed there.
				 */
				if (limit != base) {
					NEW_VMCMD2(vcset, vmcmd_map_zero,
					    base - limit,
					    limit - base_ph->p_vaddr, NULLVP,
					    0, VM_PROT_NONE, VMCMD_RELATIVE);
				}

				addr = ph0->p_vaddr - base_ph->p_vaddr;
				flags = VMCMD_RELATIVE;
			}
			last_ph = ph0;
			elf_load_psection(vcset, vp, &ph[i], &addr,
			    &size, &prot, flags);
			/*
			 * If entry is within this psection then this
			 * must contain the .text section.  *entryoff is
			 * relative to the base psection.
			 */
			if (eh.e_entry >= ph0->p_vaddr &&
			    eh.e_entry < (ph0->p_vaddr + size)) {
				*entryoff = eh.e_entry - base_ph->p_vaddr;
			}
			addr += size;
			break;
		}

		case PT_DYNAMIC:
		case PT_PHDR:
			break;

		case PT_NOTE:
			break;

		default:
			break;
		}
	}

	free(ph, M_TEMP);
	/*
	 * This value is ignored if TOPDOWN.
	 */
	*last = addr;
	vrele(vp);
	return 0;

badunlock:
	VOP_UNLOCK(vp, 0);

bad:
	if (ph != NULL)
		free(ph, M_TEMP);
#ifdef notyet /* XXX cgd 960926 */
	(maybe) VOP_CLOSE it
#endif
	vrele(vp);
	return error;
}

/*
 * exec_elf_makecmds(): Prepare an Elf binary's exec package
 *
 * First, set of the various offsets/lengths in the exec package.
 *
 * Then, mark the text image busy (so it can be demand paged) or error
 * out if this is not possible.  Finally, set up vmcmds for the
 * text, data, bss, and stack segments.
 */
int
exec_elf_makecmds(struct lwp *l, struct exec_package *epp)
{
	Elf_Ehdr *eh = epp->ep_hdr;
	Elf_Phdr *ph, *pp;
	Elf_Addr phdr = 0, pos = 0;
	int error, i, nload;
	char *interp = NULL;
	u_long phsize;
	struct proc *p;

	if (epp->ep_hdrvalid < sizeof(Elf_Ehdr))
		return ENOEXEC;

	/*
	 * XXX allow for executing shared objects. It seems silly
	 * but other ELF-based systems allow it as well.
	 */
	if (elf_check_header(eh, ET_EXEC) != 0 &&
	    elf_check_header(eh, ET_DYN) != 0)
		return ENOEXEC;

	if (eh->e_phnum > MAXPHNUM)
		return ENOEXEC;

	error = vn_marktext(epp->ep_vp);
	if (error)
		return error;

	/*
	 * Allocate space to hold all the program headers, and read them
	 * from the file
	 */
	p = l->l_proc;
	phsize = eh->e_phnum * sizeof(Elf_Phdr);
	ph = (Elf_Phdr *)malloc(phsize, M_TEMP, M_WAITOK);

	if ((error = exec_read_from(l, epp->ep_vp, eh->e_phoff, ph, phsize)) !=
	    0)
		goto bad;

	epp->ep_taddr = epp->ep_tsize = ELFDEFNNAME(NO_ADDR);
	epp->ep_daddr = epp->ep_dsize = ELFDEFNNAME(NO_ADDR);

	for (i = 0; i < eh->e_phnum; i++) {
		pp = &ph[i];
		if (pp->p_type == PT_INTERP) {
			if (pp->p_filesz >= MAXPATHLEN)
				goto bad;
			interp = PNBUF_GET();
			interp[0] = '\0';
			if ((error = exec_read_from(l, epp->ep_vp,
			    pp->p_offset, interp, pp->p_filesz)) != 0)
				goto bad;
			break;
		}
	}

	/*
	 * On the same architecture, we may be emulating different systems.
	 * See which one will accept this executable.
	 *
	 * Probe functions would normally see if the interpreter (if any)
	 * exists. Emulation packages may possibly replace the interpreter in
	 * interp[] with a changed path (/emul/xxx/<path>).
	 */
	pos = ELFDEFNNAME(NO_ADDR);
	if (epp->ep_esch->u.elf_probe_func) {
		vaddr_t startp = (vaddr_t)pos;

		error = (*epp->ep_esch->u.elf_probe_func)(l, epp, eh, interp,
							  &startp);
		if (error)
			goto bad;
		pos = (Elf_Addr)startp;
	}

	/*
	 * Load all the necessary sections
	 */
	for (i = nload = 0; i < eh->e_phnum; i++) {
		Elf_Addr  addr = ELFDEFNNAME(NO_ADDR);
		u_long size = 0;
		int prot = 0;

		pp = &ph[i];

		switch (ph[i].p_type) {
		case PT_LOAD:
			/*
			 * XXX
			 * Can handle only 2 sections: text and data
			 */
			if (nload++ == 2)
				goto bad;
			elf_load_psection(&epp->ep_vmcmds, epp->ep_vp,
			    &ph[i], &addr, &size, &prot, VMCMD_FIXED);

			/*
			 * Decide whether it's text or data by looking
			 * at the entry point.
			 */
			if (eh->e_entry >= addr &&
			    eh->e_entry < (addr + size)) {
				epp->ep_taddr = addr;
				epp->ep_tsize = size;
				if (epp->ep_daddr == ELFDEFNNAME(NO_ADDR)) {
					epp->ep_daddr = addr;
					epp->ep_dsize = size;
				}
			} else {
				epp->ep_daddr = addr;
				epp->ep_dsize = size;
			}
			break;

		case PT_SHLIB:
			/* SCO has these sections. */
		case PT_INTERP:
			/* Already did this one. */
		case PT_DYNAMIC:
			break;
		case PT_NOTE:
			break;
		case PT_PHDR:
			/* Note address of program headers (in text segment) */
			phdr = pp->p_vaddr;
			break;

		default:
			/*
			 * Not fatal; we don't need to understand everything.
			 */
			break;
		}
	}

#if defined(PAX_MPROTECT) || defined(PAX_SEGVGUARD)
	if (epp->ep_pax_flags)
		pax_adjust(l, epp->ep_pax_flags);
#endif /* PAX_MPROTECT || PAX_SEGVGUARD */

	/*
	 * Check if we found a dynamically linked binary and arrange to load
	 * its interpreter
	 */
	if (interp) {
		struct elf_args *ap;
		int j = epp->ep_vmcmds.evs_used;
		u_long interp_offset;

		MALLOC(ap, struct elf_args *, sizeof(struct elf_args),
		    M_TEMP, M_WAITOK);
		if ((error = elf_load_file(l, epp, interp,
		    &epp->ep_vmcmds, &interp_offset, ap, &pos)) != 0) {
			FREE(ap, M_TEMP);
			goto bad;
		}
		ap->arg_interp = epp->ep_vmcmds.evs_cmds[j].ev_addr;
		epp->ep_entry = ap->arg_interp + interp_offset;
		ap->arg_phaddr = phdr;

		ap->arg_phentsize = eh->e_phentsize;
		ap->arg_phnum = eh->e_phnum;
		ap->arg_entry = eh->e_entry;

		epp->ep_emul_arg = ap;

		PNBUF_PUT(interp);
	} else
		epp->ep_entry = eh->e_entry;

#ifdef ELF_MAP_PAGE_ZERO
	/* Dell SVR4 maps page zero, yeuch! */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, PAGE_SIZE, 0,
	    epp->ep_vp, 0, VM_PROT_READ);
#endif
	free(ph, M_TEMP);
	return (*epp->ep_esch->es_setup_stack)(l, epp);

bad:
	if (interp)
		PNBUF_PUT(interp);
	free(ph, M_TEMP);
	kill_vmcmds(&epp->ep_vmcmds);
	return ENOEXEC;
}

int
netbsd_elf_signature(struct lwp *l, struct exec_package *epp,
    Elf_Ehdr *eh)
{
	size_t i;
	Elf_Phdr *ph;
	size_t phsize;
	int error;
	int isnetbsd = 0;
	char *ndata;

	epp->ep_pax_flags = 0;
	if (eh->e_phnum > MAXPHNUM)
		return ENOEXEC;

	phsize = eh->e_phnum * sizeof(Elf_Phdr);
	ph = (Elf_Phdr *)malloc(phsize, M_TEMP, M_WAITOK);
	error = exec_read_from(l, epp->ep_vp, eh->e_phoff, ph, phsize);
	if (error)
		goto out;

	for (i = 0; i < eh->e_phnum; i++) {
		Elf_Phdr *ephp = &ph[i];
		Elf_Nhdr *np;

		if (ephp->p_type != PT_NOTE ||
		    ephp->p_filesz > 1024 ||
		    ephp->p_filesz < sizeof(Elf_Nhdr) + ELF_NOTE_NETBSD_NAMESZ)
			continue;

		np = (Elf_Nhdr *)malloc(ephp->p_filesz, M_TEMP, M_WAITOK);
		error = exec_read_from(l, epp->ep_vp, ephp->p_offset, np,
		    ephp->p_filesz);
		if (error)
			goto next;

		ndata = (char *)(np + 1);
		switch (np->n_type) {
		case ELF_NOTE_TYPE_NETBSD_TAG:
			if (np->n_namesz != ELF_NOTE_NETBSD_NAMESZ ||
			    np->n_descsz != ELF_NOTE_NETBSD_DESCSZ ||
			    memcmp(ndata, ELF_NOTE_NETBSD_NAME,
			    ELF_NOTE_NETBSD_NAMESZ))
				goto next;
			isnetbsd = 1;
			break;

		case ELF_NOTE_TYPE_PAX_TAG:
			if (np->n_namesz != ELF_NOTE_PAX_NAMESZ ||
			    np->n_descsz != ELF_NOTE_PAX_DESCSZ ||
			    memcmp(ndata, ELF_NOTE_PAX_NAME,
			    ELF_NOTE_PAX_NAMESZ))
				goto next;
			(void)memcpy(&epp->ep_pax_flags,
			    ndata + ELF_NOTE_PAX_NAMESZ,
			    sizeof(epp->ep_pax_flags));
			break;

		default:
			break;
		}

next:
		free(np, M_TEMP);
		continue;
	}

	error = isnetbsd ? 0 : ENOEXEC;
out:
	free(ph, M_TEMP);
	return error;
}

int
netbsd_elf_probe(struct lwp *l, struct exec_package *epp, void *eh, char *itp,
    vaddr_t *pos)
{
	int error;

	if ((error = netbsd_elf_signature(l, epp, eh)) != 0)
		return error;
#ifdef ELF_INTERP_NON_RELOCATABLE
	*pos = ELF_LINK_ADDR;
#endif
	return 0;
}
