/*	$NetBSD: exec_elf32.c,v 1.53 2000/07/13 02:35:25 matt Exp $	*/

/*-
 * Copyright (c) 1994 The NetBSD Foundation, Inc.
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

#include "opt_compat_linux.h"
#include "opt_compat_ibcs2.h"
#include "opt_compat_svr4.h"
#include "opt_compat_freebsd.h"
#include "opt_compat_netbsd32.h"
#include "opt_syscall_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/fcntl.h>
#include <sys/syscall.h>
#include <sys/signalvar.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include <sys/mman.h>

#include <machine/cpu.h>
#include <machine/reg.h>

#ifdef COMPAT_NETBSD32
#include <compat/netbsd32/netbsd32_exec.h>
#endif

#ifdef COMPAT_LINUX
#include <compat/linux/common/linux_exec.h>
#endif

#ifdef COMPAT_SVR4
#include <compat/svr4/svr4_exec.h>
#endif

#ifdef COMPAT_IBCS2
#include <compat/ibcs2/ibcs2_exec.h>
#endif

#ifdef COMPAT_FREEBSD
#include <compat/freebsd/freebsd_exec.h>
#endif

int	ELFNAME(check_header) __P((Elf_Ehdr *, int));
int	ELFNAME(load_file) __P((struct proc *, struct exec_package *, char *,
	    struct exec_vmcmd_set *, u_long *, struct elf_args *, Elf_Addr *));
void	ELFNAME(load_psection) __P((struct exec_vmcmd_set *, struct vnode *,
	    const Elf_Phdr *, Elf_Addr *, u_long *, int *, int));

int ELFNAME2(netbsd,signature) __P((struct proc *, struct exec_package *,
    Elf_Ehdr *));
static int ELFNAME2(netbsd,probe) __P((struct proc *, struct exec_package *,
    Elf_Ehdr *, char *, Elf_Addr *));

extern char sigcode[], esigcode[];
#ifdef SYSCALL_DEBUG
extern char *syscallnames[];
#endif

struct emul ELFNAMEEND(emul_netbsd) = {
	"netbsd",
	NULL,
	sendsig,
	SYS_syscall,
	SYS_MAXSYSCALL,
	sysent,
#ifdef SYSCALL_DEBUG
	syscallnames,
#else
	NULL,
#endif
	howmany(ELF_AUX_ENTRIES * sizeof(AuxInfo), sizeof (Elf_Addr)),
	ELFNAME(copyargs),
	setregs,
	sigcode,
	esigcode,
};

int (*ELFNAME(probe_funcs)[]) __P((struct proc *, struct exec_package *,
    Elf_Ehdr *, char *, Elf_Addr *)) = {
#if defined(COMPAT_NETBSD32) && (ELFSIZE == 32)
	    /* This one should go first so it matches instead of netbsd */
	ELFNAME2(netbsd32,probe),
#endif
	ELFNAME2(netbsd,probe),
#if defined(COMPAT_FREEBSD) && (ELFSIZE == 32)
	ELFNAME2(freebsd,probe),		/* XXX not 64-bit safe */
#endif
#if defined(COMPAT_LINUX)
	ELFNAME2(linux,probe),
#endif
#if defined(COMPAT_SVR4) && (ELFSIZE == 32)
	ELFNAME2(svr4,probe),			/* XXX not 64-bit safe */
#endif
#if defined(COMPAT_IBCS2) && (ELFSIZE == 32)
	ELFNAME2(ibcs2,probe),			/* XXX not 64-bit safe */
#endif
};

/* round up and down to page boundaries. */
#define	ELF_ROUND(a, b)		(((a) + (b) - 1) & ~((b) - 1))
#define	ELF_TRUNC(a, b)		((a) & ~((b) - 1))

/*
 * Copy arguments onto the stack in the normal way, but add some
 * extra information in case of dynamic binding.
 */
void *
ELFNAME(copyargs)(pack, arginfo, stack, argp)
	struct exec_package *pack;
	struct ps_strings *arginfo;
	void *stack;
	void *argp;
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
		a->a_v = NBPG;
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
ELFNAME(check_header)(eh, type)
	Elf_Ehdr *eh;
	int type;
{

	if (memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0 ||
	    eh->e_ident[EI_CLASS] != ELFCLASS)
		return ENOEXEC;

	switch (eh->e_machine) {

	ELFDEFNNAME(MACHDEP_ID_CASES)

	default:
		return ENOEXEC;
	}

	if (eh->e_type != type)
		return ENOEXEC;

	return 0;
}

/*
 * elf_load_psection():
 *
 * Load a psection at the appropriate address
 */
void
ELFNAME(load_psection)(vcset, vp, ph, addr, size, prot, flags)
	struct exec_vmcmd_set *vcset;
	struct vnode *vp;
	const Elf_Phdr *ph;
	Elf_Addr *addr;
	u_long *size;
	int *prot;
	int flags;
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
 * elf_read_from():
 *
 *	Read from vnode into buffer at offset.
 */
int
ELFNAME(read_from)(p, vp, off, buf, size)
	struct vnode *vp;
	u_long off;
	struct proc *p;
	caddr_t buf;
	int size;
{
	int error;
	size_t resid;

	if ((error = vn_rdwr(UIO_READ, vp, buf, size, off, UIO_SYSSPACE,
	    0, p->p_ucred, &resid, p)) != 0)
		return error;
	/*
	 * See if we got all of it
	 */
	if (resid != 0)
		return ENOEXEC;
	return 0;
}

/*
 * elf_load_file():
 *
 * Load a file (interpreter/library) pointed to by path
 * [stolen from coff_load_shlib()]. Made slightly generic
 * so it might be used externally.
 */
int
ELFNAME(load_file)(p, epp, path, vcset, entry, ap, last)
	struct proc *p;
	struct exec_package *epp;
	char *path;
	struct exec_vmcmd_set *vcset;
	u_long *entry;
	struct elf_args	*ap;
	Elf_Addr *last;
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

	if ((error = ELFNAME(read_from)(p, vp, 0, (caddr_t) &eh,
	    sizeof(eh))) != 0)
		goto bad;

	if ((error = ELFNAME(check_header)(&eh, ET_DYN)) != 0)
		goto bad;

	phsize = eh.e_phnum * sizeof(Elf_Phdr);
	ph = (Elf_Phdr *)malloc(phsize, M_TEMP, M_WAITOK);

	if ((error = ELFNAME(read_from)(p, vp, eh.e_phoff,
	    (caddr_t) ph, phsize)) != 0)
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
ELFNAME2(exec,makecmds)(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	Elf_Ehdr *eh = epp->ep_hdr;
	Elf_Phdr *ph, *pp;
	Elf_Addr phdr = 0, pos = 0;
	int error, i, n, nload;
	char interp[MAXPATHLEN];
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

	if ((error = ELFNAME(read_from)(p, epp->ep_vp, eh->e_phoff,
	    (caddr_t) ph, phsize)) != 0)
		goto bad;

	epp->ep_taddr = epp->ep_tsize = ELFDEFNNAME(NO_ADDR);
	epp->ep_daddr = epp->ep_dsize = ELFDEFNNAME(NO_ADDR);

	interp[0] = '\0';

	for (i = 0; i < eh->e_phnum; i++) {
		pp = &ph[i];
		if (pp->p_type == PT_INTERP) {
			if (pp->p_filesz >= sizeof(interp))
				goto bad;
			if ((error = ELFNAME(read_from)(p, epp->ep_vp,
			    pp->p_offset, (caddr_t) interp,
			    pp->p_filesz)) != 0)
				goto bad;
			break;
		}
	}

	/*
	 * Setup things for native emulation.
	 */
	epp->ep_emul = &ELFNAMEEND(emul_netbsd);
	pos = ELFDEFNNAME(NO_ADDR);

	/*
	 * On the same architecture, we may be emulating different systems.
	 * See which one will accept this executable. This currently only
	 * applies to SVR4, and IBCS2 on the i386 and Linux on the i386
	 * and the Alpha.
	 *
	 * Probe functions would normally see if the interpreter (if any)
	 * exists. Emulation packages may possibly replace the interpreter in
	 * interp[] with a changed path (/emul/xxx/<path>), and also
	 * set the ep_emul field in the exec package structure.
	 */
	n = sizeof(ELFNAME(probe_funcs)) / sizeof(ELFNAME(probe_funcs)[0]);
	if (n != 0) {
		error = ENOEXEC;
		for (i = 0; i < n && error; i++)
			error = ELFNAME(probe_funcs)[i](p, epp, eh,
			    interp, &pos);
#ifdef notyet
		/*
		 * We should really use a signature in our native binaries
		 * and have our own probe function for matching binaries,
		 * before trying the emulations. For now, if the emulation
		 * probes failed we default to native.
		 */
		if (error)
			goto bad;
#endif
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
#ifndef COMPAT_IBCS2			/* SCO has these sections */
			error = ENOEXEC;
			goto bad;
#endif

		case PT_INTERP:
			/* Already did this one */
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

		ap = (struct elf_args *)malloc(sizeof(struct elf_args),
		    M_TEMP, M_WAITOK);
		if ((error = ELFNAME(load_file)(p, epp, interp,
		    &epp->ep_vmcmds, &epp->ep_entry, ap, &pos)) != 0) {
			free((char *)ap, M_TEMP);
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
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, NBPG, 0, epp->ep_vp, 0,
	    VM_PROT_READ);
#endif
	free((char *)ph, M_TEMP);
	vn_marktext(epp->ep_vp);
	return exec_elf_setup_stack(p, epp);

bad:
	free((char *)ph, M_TEMP);
	kill_vmcmds(&epp->ep_vmcmds);
	return ENOEXEC;
}

int
ELFNAME2(netbsd,signature)(p, epp, eh)
	struct proc *p;
	struct exec_package *epp;
	Elf_Ehdr *eh;
{
	Elf_Phdr *hph, *ph;
	Elf_Nhdr *np = NULL;
	size_t phsize;
	int error;

	phsize = eh->e_phnum * sizeof(Elf_Phdr);
	hph = (Elf_Phdr *)malloc(phsize, M_TEMP, M_WAITOK);
	if ((error = ELFNAME(read_from)(p, epp->ep_vp, eh->e_phoff,
	    (caddr_t)hph, phsize)) != 0)
		goto out1;

	for (ph = hph;  ph < &hph[eh->e_phnum]; ph++) {
		if (ph->p_type != PT_NOTE ||
		    ph->p_filesz < sizeof(Elf_Nhdr) + ELF_NOTE_NETBSD_NAMESZ)
			continue;

		np = (Elf_Nhdr *)malloc(ph->p_filesz, M_TEMP, M_WAITOK);
		if ((error = ELFNAME(read_from)(p, epp->ep_vp, ph->p_offset,
		    (caddr_t)np, ph->p_filesz)) != 0)
			goto out2;

		if (np->n_type != ELF_NOTE_TYPE_OSVERSION) {
			free(np, M_TEMP);
			np = NULL;
			continue;
		}

		/* Check the name and description sizes. */
		if (np->n_namesz != ELF_NOTE_NETBSD_NAMESZ ||
		    np->n_descsz != ELF_NOTE_NETBSD_DESCSZ)
			goto out3;

		/* Is the name "NetBSD\0\0"? */
		if (memcmp((np + 1), ELF_NOTE_NETBSD_NAME,
		    ELF_NOTE_NETBSD_NAMESZ))
			goto out3;

		/* XXX: We could check for the specific emulation here */
		/* All checks succeeded. */
		error = 0;
		goto out2;
	}

out3:
	error = ENOEXEC;
out2:
	if (np)
		free(np, M_TEMP);
out1:
	free(hph, M_TEMP);
	return error;
}

static int
ELFNAME2(netbsd,probe)(p, epp, eh, itp, pos)
	struct proc *p;
	struct exec_package *epp;
	Elf_Ehdr *eh;
	char *itp;
	Elf_Addr *pos;
{
	int error;

	if ((error = ELFNAME2(netbsd,signature)(p, epp, eh)) != 0)
		return error;
	epp->ep_emul = &ELFNAMEEND(emul_netbsd);
	*pos = ELFDEFNNAME(NO_ADDR);
	return 0;
}
