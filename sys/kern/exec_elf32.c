/*	$NetBSD: exec_elf32.c,v 1.64 2001/07/14 02:08:29 christos Exp $	*/

/*-
 * Copyright (c) 1994, 2000 The NetBSD Foundation, Inc.
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

/* If not included by exec_elf64.c, ELFSIZE won't be defined. */
#ifndef ELFSIZE
#define	ELFSIZE		32
#endif

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

#include <machine/cpu.h>
#include <machine/reg.h>

extern const struct emul emul_netbsd;

int	ELFNAME(check_header)(Elf_Ehdr *, int);
int	ELFNAME(load_file)(struct proc *, struct exec_package *, char *,
	    struct exec_vmcmd_set *, u_long *, struct elf_args *, Elf_Addr *);
void	ELFNAME(load_psection)(struct exec_vmcmd_set *, struct vnode *,
	    const Elf_Phdr *, Elf_Addr *, u_long *, int *, int);

int ELFNAME2(netbsd,signature)(struct proc *, struct exec_package *,
    Elf_Ehdr *);
int ELFNAME2(netbsd,probe)(struct proc *, struct exec_package *,
    void *, char *, vaddr_t *);

/* round up and down to page boundaries. */
#define	ELF_ROUND(a, b)		(((a) + (b) - 1) & ~((b) - 1))
#define	ELF_TRUNC(a, b)		((a) & ~((b) - 1))

/*
 * Copy arguments onto the stack in the normal way, but add some
 * extra information in case of dynamic binding.
 */
void *
ELFNAME(copyargs)(struct exec_package *pack, struct ps_strings *arginfo,
    void *stack, void *argp)
{
	size_t len;
	AuxInfo ai[ELF_AUX_ENTRIES], *a;
	struct elf_args *ap;

	stack = copyargs(pack, arginfo, stack, argp);
	if (!stack)
		return NULL;

	a = ai;

	/*
	 * Push extra arguments on the stack needed by dynamically
	 * linked binaries
	 */
	if ((ap = (struct elf_args *)pack->ep_emul_arg)) {

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

		free((char *)ap, M_TEMP);
		pack->ep_emul_arg = NULL;
	}

	a->a_type = AT_NULL;
	a->a_v = 0;
	a++;

	len = (a - ai) * sizeof(AuxInfo);
	if (copyout(ai, stack, len))
		return NULL;
	stack = (caddr_t)stack + len;

	return stack;
}

/*
 * elf_check_header():
 *
 * Check header for validity; return 0 of ok ENOEXEC if error
 */
int
ELFNAME(check_header)(Elf_Ehdr *eh, int type)
{

	if (memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0 ||
	    eh->e_ident[EI_CLASS] != ELFCLASS)
		return (ENOEXEC);

	switch (eh->e_machine) {

	ELFDEFNNAME(MACHDEP_ID_CASES)

	default:
		return (ENOEXEC);
	}

	if (eh->e_type != type)
		return (ENOEXEC);

	if (eh->e_shnum > 512 ||
	    eh->e_phnum > 128)
		return (ENOEXEC);

	return (0);
}

/*
 * elf_load_psection():
 *
 * Load a psection at the appropriate address
 */
void
ELFNAME(load_psection)(struct exec_vmcmd_set *vcset, struct vnode *vp,
    const Elf_Phdr *ph, Elf_Addr *addr, u_long *size, int *prot, int flags)
{
	u_long uaddr, msize, psize, rm, rf;
	long diff, offset;

	/*
	 * If the user specified an address, then we load there.
	 */
	if (*addr != ELFDEFNNAME(NO_ADDR)) {
		if (ph->p_align > 1) {
			*addr = ELF_TRUNC(*addr, ph->p_align);
			uaddr = ELF_TRUNC(ph->p_vaddr, ph->p_align);
		} else
			uaddr = ph->p_vaddr;
		diff = ph->p_vaddr - uaddr;
	} else {
		*addr = uaddr = ph->p_vaddr;
		if (ph->p_align > 1)
			*addr = ELF_TRUNC(uaddr, ph->p_align);
		diff = uaddr - *addr;
	}

	*prot |= (ph->p_flags & PF_R) ? VM_PROT_READ : 0;
	*prot |= (ph->p_flags & PF_W) ? VM_PROT_WRITE : 0;
	*prot |= (ph->p_flags & PF_X) ? VM_PROT_EXECUTE : 0;

	offset = ph->p_offset - diff;
	*size = ph->p_filesz + diff;
	msize = ph->p_memsz + diff;
	psize = round_page(*size);

	if ((ph->p_flags & PF_W) != 0) {
		/*
		 * Because the pagedvn pager can't handle zero fill of the last
		 * data page if it's not page aligned we map the last page
		 * readvn.
		 */
		psize = trunc_page(*size);
	}
	if (psize > 0) {
		NEW_VMCMD2(vcset, vmcmd_map_pagedvn, psize, *addr, vp,
		    offset, *prot, flags);
	}
	if (psize < *size) {
		NEW_VMCMD2(vcset, vmcmd_map_readvn, *size - psize,
		    *addr + psize, vp, offset + psize, *prot, 
		    psize > 0 ? flags & VMCMD_RELATIVE : flags);
	}

	/*
	 * Check if we need to extend the size of the segment
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
ELFNAME(load_file)(struct proc *p, struct exec_package *epp, char *path,
    struct exec_vmcmd_set *vcset, u_long *entry, struct elf_args *ap,
    Elf_Addr *last)
{
	int error, i;
	struct nameidata nd;
	struct vnode *vp;
	struct vattr attr;
	Elf_Ehdr eh;
	Elf_Phdr *ph = NULL;
	Elf_Phdr *base_ph = NULL;
	u_long phsize;
	char *bp = NULL;
	Elf_Addr addr = *last;

	bp = path;
	/*
	 * 1. open file
	 * 2. read filehdr
	 * 3. map text, data, and bss out of it using VM_*
	 */
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_SYSSPACE, path, p);
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
	if ((error = VOP_ACCESS(vp, VEXEC, p->p_ucred, p)) != 0)
		goto badunlock;

	/* get attributes */
	if ((error = VOP_GETATTR(vp, &attr, p->p_ucred, p)) != 0)
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
	VOP_UNLOCK(vp, 0);

	if ((error = exec_read_from(p, vp, 0, &eh, sizeof(eh))) != 0)
		goto bad;

	if ((error = ELFNAME(check_header)(&eh, ET_DYN)) != 0)
		goto bad;

	phsize = eh.e_phnum * sizeof(Elf_Phdr);
	ph = (Elf_Phdr *)malloc(phsize, M_TEMP, M_WAITOK);

	if ((error = exec_read_from(p, vp, eh.e_phoff, ph, phsize)) != 0)
		goto bad;

	/*
	 * Load all the necessary sections
	 */
	for (i = 0; i < eh.e_phnum; i++) {
		u_long size = 0;
		int prot = 0;
		int flags;

		switch (ph[i].p_type) {
		case PT_LOAD:
			if (base_ph == NULL) {
				addr = *last;
				flags = VMCMD_BASE;
			} else {
				addr = ph[i].p_vaddr - base_ph->p_vaddr;
				flags = VMCMD_RELATIVE;
			}
			ELFNAME(load_psection)(vcset, vp, &ph[i], &addr,
			    &size, &prot, flags);
			/* If entry is within this section it must be text */
			if (eh.e_entry >= ph[i].p_vaddr &&
			    eh.e_entry < (ph[i].p_vaddr + size)) {
				/* XXX */
				*entry = addr + eh.e_entry;
#ifdef mips
				*entry -= ph[i].p_vaddr;
#endif
				ap->arg_interp = addr;
			}
			if (base_ph == NULL)
				base_ph = &ph[i];
			addr += size;
			break;

		case PT_DYNAMIC:
		case PT_PHDR:
		case PT_NOTE:
			break;

		default:
			break;
		}
	}

	free((char *)ph, M_TEMP);
	*last = addr;
	vrele(vp);
	return 0;

badunlock:
	VOP_UNLOCK(vp, 0);

bad:
	if (ph != NULL)
		free((char *)ph, M_TEMP);
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
ELFNAME2(exec,makecmds)(struct proc *p, struct exec_package *epp)
{
	Elf_Ehdr *eh = epp->ep_hdr;
	Elf_Phdr *ph, *pp;
	Elf_Addr phdr = 0, pos = 0;
	int error, i, nload;
	char *interp = NULL;
	u_long phsize;

	if (epp->ep_hdrvalid < sizeof(Elf_Ehdr))
		return ENOEXEC;

	/*
	 * XXX allow for executing shared objects. It seems silly
	 * but other ELF-based systems allow it as well.
	 */
	if (ELFNAME(check_header)(eh, ET_EXEC) != 0 &&
	    ELFNAME(check_header)(eh, ET_DYN) != 0)
		return ENOEXEC;

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
	/*
	 * Allocate space to hold all the program headers, and read them
	 * from the file
	 */
	phsize = eh->e_phnum * sizeof(Elf_Phdr);
	ph = (Elf_Phdr *)malloc(phsize, M_TEMP, M_WAITOK);

	if ((error = exec_read_from(p, epp->ep_vp, eh->e_phoff, ph, phsize)) !=
	    0)
		goto bad;

	epp->ep_taddr = epp->ep_tsize = ELFDEFNNAME(NO_ADDR);
	epp->ep_daddr = epp->ep_dsize = ELFDEFNNAME(NO_ADDR);

	MALLOC(interp, char *, MAXPATHLEN, M_TEMP, M_WAITOK);
	interp[0] = '\0';

	for (i = 0; i < eh->e_phnum; i++) {
		pp = &ph[i];
		if (pp->p_type == PT_INTERP) {
			if (pp->p_filesz >= MAXPATHLEN)
				goto bad;
			if ((error = exec_read_from(p, epp->ep_vp,
			    pp->p_offset, interp, pp->p_filesz)) != 0)
				goto bad;
			break;
		}
	}

	/*
	 * On the same architecture, we may be emulating different systems.
	 * See which one will accept this executable. This currently only
	 * applies to SVR4, and IBCS2 on the i386 and Linux on the i386
	 * and the Alpha.
	 *
	 * Probe functions would normally see if the interpreter (if any)
	 * exists. Emulation packages may possibly replace the interpreter in
	 * interp[] with a changed path (/emul/xxx/<path>).
	 */
	if (!epp->ep_esch->u.elf_probe_func) {
		pos = ELFDEFNNAME(NO_ADDR);
	} else {
		vaddr_t startp = 0;

		error = (*epp->ep_esch->u.elf_probe_func)(p, epp, eh, interp,
							  &startp);
		pos = (Elf_Addr)startp;
		if (error)
			goto bad;
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
			ELFNAME(load_psection)(&epp->ep_vmcmds, epp->ep_vp,
			    &ph[i], &addr, &size, &prot, 0);

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

	/* this breaks on, e.g., OpenBSD-compatible mips shared binaries. */
#ifndef ELF_INTERP_NON_RELOCATABLE
	/*
	 * If no position to load the interpreter was set by a probe
	 * function, pick the same address that a non-fixed mmap(0, ..)
	 * would (i.e. something safely out of the way).
	 */
	if (pos == ELFDEFNNAME(NO_ADDR))
		pos = round_page(epp->ep_daddr + MAXDSIZ);
#endif	/* !ELF_INTERP_NON_RELOCATABLE */

	/*
	 * Check if we found a dynamically linked binary and arrange to load
	 * it's interpreter
	 */
	if (interp[0]) {
		struct elf_args *ap;

		MALLOC(ap, struct elf_args *, sizeof(struct elf_args),
		    M_TEMP, M_WAITOK);
		if ((error = ELFNAME(load_file)(p, epp, interp,
		    &epp->ep_vmcmds, &epp->ep_entry, ap, &pos)) != 0) {
			FREE((char *)ap, M_TEMP);
			goto bad;
		}
		pos += phsize;
		ap->arg_phaddr = phdr;

		ap->arg_phentsize = eh->e_phentsize;
		ap->arg_phnum = eh->e_phnum;
		ap->arg_entry = eh->e_entry;

		epp->ep_emul_arg = ap;
	} else
		epp->ep_entry = eh->e_entry;

#ifdef ELF_MAP_PAGE_ZERO
	/* Dell SVR4 maps page zero, yeuch! */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, PAGE_SIZE, 0,
	    epp->ep_vp, 0, VM_PROT_READ);
#endif
	FREE(interp, M_TEMP);
	free((char *)ph, M_TEMP);
	vn_marktext(epp->ep_vp);
	return exec_elf_setup_stack(p, epp);

bad:
	if (interp)
		FREE(interp, M_TEMP);
	free((char *)ph, M_TEMP);
	kill_vmcmds(&epp->ep_vmcmds);
	return ENOEXEC;
}

int
ELFNAME2(netbsd,signature)(struct proc *p, struct exec_package *epp,
    Elf_Ehdr *eh)
{
	size_t i;
	Elf_Phdr *ph;
	size_t phsize;
	int error;

	phsize = eh->e_phnum * sizeof(Elf_Phdr);
	ph = (Elf_Phdr *)malloc(phsize, M_TEMP, M_WAITOK);
	error = exec_read_from(p, epp->ep_vp, eh->e_phoff, ph, phsize);
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
		error = exec_read_from(p, epp->ep_vp, ephp->p_offset, np,
		    ephp->p_filesz);
		if (error)
			goto next;

		if (np->n_type != ELF_NOTE_TYPE_NETBSD_TAG ||
		    np->n_namesz != ELF_NOTE_NETBSD_NAMESZ ||
		    np->n_descsz != ELF_NOTE_NETBSD_DESCSZ ||
		    memcmp((caddr_t)(np + 1), ELF_NOTE_NETBSD_NAME,
		    ELF_NOTE_NETBSD_NAMESZ))
			goto next;

		error = 0;
		free(np, M_TEMP);
		goto out;

	next:
		free(np, M_TEMP);
		continue;
	}

	error = ENOEXEC;
out:
	free(ph, M_TEMP);
	return (error);
}

int
ELFNAME2(netbsd,probe)(struct proc *p, struct exec_package *epp,
    void *eh, char *itp, vaddr_t *pos)
{
	int error;

	if ((error = ELFNAME2(netbsd,signature)(p, epp, eh)) != 0)
		return error;
	*pos = ELFDEFNNAME(NO_ADDR);
	return 0;
}
