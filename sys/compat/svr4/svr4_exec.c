/*	$NetBSD: svr4_exec.c,v 1.8 1995/03/31 03:06:17 christos Exp $	 */

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

#include <compat/svr4/svr4_util.h>
#include <compat/svr4/svr4_exec.h>

#define SVR4_ALIGN(a, b) ((a) & ~((b) - 1))
#define SVR4_PAGESIZE 4096
#define SVR4_AUX_ARGSIZ (sizeof(AuxInfo) * 8)

/*
 * I know this is horrible, but I cannot load the interpreter after
 * the data segment because brk(2) breaks. I have to load it somewhere
 * before. Programs start at 0x08000000 so I load the interpreter far
 * before.
 */
#define SVR4_INTERP_ADDR	0x01000000

struct svr4_args {
	u_long	arg_entry;	/* progran entry point */
	u_long	arg_interp;	/* Interpreter load address */
	u_long	arg_phaddr;	/* program header address */
	u_long	arg_phentsize;	/* Size of program header */
	u_long	arg_phnum;	/* Number of program headers */
};

static void svr4_setup_args __P((int, struct proc *, struct exec_package *,
			         void *));
static int svr4_check_header __P((Elf32_Ehdr *, int));
static void svr4_load_psection	__P((struct exec_package *, struct vnode *,
		                     Elf32_Phdr *, u_long *, u_long *, int *));
static int svr4_set_segment __P((struct exec_package *, u_long, u_long, int));
static int svr4_read_from __P((struct vnode *, u_long, struct proc *, caddr_t,
			       int));
static int svr4_load_interp __P((struct proc *, char *, struct exec_package *,
				 struct svr4_args *, u_long * last));


#ifdef DEBUG_EXEC_SVR4
static void
print_Ehdr(e)
	Elf32_Ehdr     *e;
{
	printf("e_ident %s, ", e->e_ident);
	printf("e_type %d, ", e->e_type);
	printf("e_machine %d, ", e->e_machine);
	printf("e_version %ld, ", e->e_version);
	printf("e_entry %lx, ", e->e_entry);
	printf("e_phoff %lx, ", e->e_phoff);
	printf("e_shoff %lx, ", e->e_shoff);
	printf("e_flags %lx, ", e->e_flags);
	printf("e_ehsize %d, ", e->e_ehsize);
	printf("e_phentsize %d, ", e->e_phentsize);
	printf("e_phnum %d, ", e->e_phnum);
	printf("e_shentsize %d, ", e->e_shentsize);
	printf("e_shnum %d, ", e->e_shnum);
	printf("e_shstrndx %d\n", e->e_shstrndx);
}


static void
print_Phdr(p)
	Elf32_Phdr     *p;
{
	static char    *types[] =
	{
		"null", "load", "dynamic", "interp",
		"note", "shlib", "phdr", "entry7"
	};

	printf("p_type %ld [%s], ", p->p_type, types[p->p_type & 7]);
	printf("p_offset %lx, ", p->p_offset);
	printf("p_vaddr %lx, ", p->p_vaddr);
	printf("p_paddr %lx, ", p->p_paddr);
	printf("p_filesz %ld, ", p->p_filesz);
	printf("p_memsz %ld, ", p->p_memsz);
	printf("p_flags %lx, ", p->p_flags);
	printf("p_align %ld\n", p->p_align);
}
#endif


/*
 * svr4_setup_args():
 *
 * Push extra arguments on the stack needed by dynamically linked binaries
 */
static void
svr4_setup_args(cmd, pp, ep, sp)
	int			 cmd;
	struct proc		*pp;
	struct exec_package	*ep;
	void			*sp;
{
	AuxInfo *a = sp;
	struct svr4_args *ap = (struct svr4_args *) ep->ep_setup_arg;

	if (cmd == EXEC_SETUP_CLEANUP) {
		if (ap != NULL)
			free((char *) ap, M_TEMP);
		return;
	}
	if (cmd != EXEC_SETUP_ADDARGS)
		return;

	DPRINTF(("phaddr=0x%x, phsize=%d, phnum=%d, interp=0x%x, entry=0x%x\n",
		 ap->arg_phaddr, ap->arg_phentsize, ap->arg_phnum,
		 ap->arg_interp, ap->arg_entry));

	a->au_id = AUX_phdr;
	a->au_v = ap->arg_phaddr;
	a++;

	a->au_id = AUX_phent;
	a->au_v = ap->arg_phentsize;
	a++;

	a->au_id = AUX_phnum;
	a->au_v = ap->arg_phnum;
	a++;

	a->au_id = AUX_pagesz;
	a->au_v = SVR4_PAGESIZE;
	a++;

	a->au_id = AUX_base;
	a->au_v = ap->arg_interp;
	a++;

	a->au_id = AUX_flags;
	a->au_v = 0;
	a++;

	a->au_id = AUX_entry;
	a->au_v = ap->arg_entry;
	a++;

	a->au_id = AUX_null;
	a->au_v = 0;
}


/*
 * svr4_check_header():
 *
 * Check header for validity; return 0 of ok ENOEXEC if error
 */
static int
svr4_check_header(eh, type)
	Elf32_Ehdr     *eh;
	int             type;
{
#ifdef sparc
  /* #$%@#$%@#$%! */
# define memcmp bcmp
#endif
	if (memcmp(eh->e_ident, Elf32_e_ident, Elf32_e_siz) != 0) {
		DPRINTF(("Not an elf file\n"));
		return ENOEXEC;
	}

	switch (eh->e_machine) {
#ifdef i386
	case Elf32_em_386:
	case Elf32_em_486:
#endif
#ifdef sparc
	case Elf32_em_sparc:
#endif
		break;

	default:
		DPRINTF(("Unsupported elf machine type %d\n", eh->e_machine));
		return ENOEXEC;
	}

	if (eh->e_type != type) {
		DPRINTF(("Not an elf executable\n"));
		return ENOEXEC;
	}

	return 0;
}


/*
 * svr4_load_psection():
 * 
 * Load a psection at the appropriate address
 */
static void
svr4_load_psection(epp, vp, ph, addr, size, prot)
	struct exec_package	*epp;
	struct vnode		*vp;
	Elf32_Phdr		*ph;
	u_long			*addr;
	u_long			*size;
	int			*prot;
{
	u_long	uaddr;
	long	diff;
	long	offset;
	u_long	msize;

	/*
         * If the user specified an address, then we load there.
         */
	if (*addr != ~0) {
		uaddr = *addr + ph->p_align;
		*addr = SVR4_ALIGN(uaddr, ph->p_align);
		uaddr = SVR4_ALIGN(ph->p_vaddr, ph->p_align);
		diff = ph->p_vaddr - uaddr;
	} else {
		uaddr = ph->p_vaddr;
		*addr = SVR4_ALIGN(uaddr, ph->p_align);
		diff = uaddr - *addr;
	}

	*prot |= (ph->p_flags & Elf32_pf_r) ? VM_PROT_READ : 0;
	*prot |= (ph->p_flags & Elf32_pf_w) ? VM_PROT_WRITE : 0;
	*prot |= (ph->p_flags & Elf32_pf_x) ? VM_PROT_EXECUTE : 0;

	offset = ph->p_offset - diff;
	*size = ph->p_filesz + diff;
	msize = ph->p_memsz + diff;

	DPRINTF(("Elf Seg@ 0x%x/0x%x sz %d/%d off 0x%x/0x%x[%d] algn 0x%x\n",
		 ph->p_vaddr, *addr, *size, msize, ph->p_offset, offset,
		 diff, ph->p_align));

	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, *size,
		  *addr, vp, offset, *prot);

	/*
         * Check if we need to extend the size of the segment
         */
	{
		u_long	rm = round_page(*addr + msize);
		u_long	rf = round_page(*addr + *size);
		if (rm != rf) {
			DPRINTF(("zeropad 0x%x-0x%x\n", rf, rm));
			NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, rm - rf,
				  rf, NULLVP, 0, *prot);
			*size = msize;
		}
	}
}


/*
 * svr4_set_segment():
 *
 * Decide if the segment is text or data, depending on the protection
 * and set it appropriately
 */
static int
svr4_set_segment(epp, vaddr, size, prot)
	struct exec_package	*epp;
	u_long			 vaddr;
	u_long			 size;
	int			 prot;
{
	/*
         * Kludge: Unfortunately the current implementation of
         * exec package assumes a single text and data segment.
         * In Elf we can have more, but here we limit ourselves
         * to two and hope :-(
         * We also assume that the text is r-x, and data is rwx.
         */
	switch (prot) {
	case (VM_PROT_READ | VM_PROT_EXECUTE):
		if (epp->ep_tsize != ~0) {
			DPRINTF(("More than one text segment\n"));
			return ENOEXEC;
		}
		epp->ep_taddr = vaddr;
		epp->ep_tsize = size;
		DPRINTF(("Elf Text@ 0x%x, size %d\n", vaddr, size));
		break;

	case (VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE):
		if (epp->ep_dsize != ~0) {
			DPRINTF(("More than one data segment\n"));
			return ENOEXEC;
		}
		epp->ep_daddr = vaddr;
		epp->ep_dsize = size;

		DPRINTF(("Elf Data@ 0x%x, size %d\n", vaddr, size));
		break;

	default:
		DPRINTF(("Bad protection 0%o\n", prot));
		return ENOEXEC;
	}
	return 0;
}


/*
 * svr4_read_from():
 *
 *	Read from vnode into buffer at offset.
 */
static int
svr4_read_from(vp, off, p, buf, size)
	struct vnode	*vp;
	u_long		 off;
	struct proc	*p;
	caddr_t		 buf;
	int		 size;
{
	int	error;
	int	resid;

	DPRINTF(("read from 0x%x to 0x%x size %d\n",
		 off, buf, size));
	if ((error = vn_rdwr(UIO_READ, vp, buf, size,
			     off, UIO_SYSSPACE, IO_NODELOCKED, p->p_ucred,
			     &resid, p)) != 0) {
		DPRINTF(("Bad read error %d\n", error));
		return error;
	}
	/*
         * See if we got all of it
         */
	if (resid != 0) {
		DPRINTF(("Incomplete read for header ask=%d, rem=%d\n",
			 size, resid));
		return error;
	}
	return 0;
}


/*
 * svr4_load_interp():
 *
 * Load an interpreter pointed to by path [stolen from coff_load_shlib()]
 */
static int
svr4_load_interp(p, path, epp, ap, last)
	struct proc		*p;
	char			*path;
	struct exec_package	*epp;
	struct svr4_args	*ap;
	u_long			*last;
{
	int			 error, i;
	struct nameidata	 nd;
	Elf32_Ehdr		 eh;
	Elf32_Phdr		*ph = NULL;
	u_long			 phsize;
	char			*bp = NULL;
	u_long			 addr = *last;

	DPRINTF(("Loading interpreter %s\n", path));

	if ((error = svr4_emul_find(p, NULL, svr4_emul_path, path, &bp)) != 0)
		bp = NULL;
	else
		path = bp;
	/*
         * 1. open interp file
         * 2. read filehdr
         * 3. map text, data, and bss out of it using VM_*
         */
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, path, p);
	/* first get the vnode */
	if ((error = namei(&nd)) != 0) {
		uprintf("cannot find interpreter %s\n", path);
		if (bp != NULL)
			free((char *) bp, M_TEMP);
		return error;
	}
	if ((error = svr4_read_from(nd.ni_vp, 0, p, (caddr_t) &eh,
				    sizeof(eh))) != 0)
		goto bad;

#ifdef DEBUG_EXEC_SVR4
	print_Ehdr(&eh);
#endif

	if ((error = svr4_check_header(&eh, Elf32_et_dyn)) != 0)
		goto bad;

	phsize = eh.e_phnum * sizeof(Elf32_Phdr);
	ph = (Elf32_Phdr *) malloc(phsize, M_TEMP, M_WAITOK);

	if ((error = svr4_read_from(nd.ni_vp, eh.e_phoff, p,
				    (caddr_t) ph, phsize)) != 0)
		goto bad;

	/*
         * Load all the necessary sections
         */
	for (i = 0; i < eh.e_phnum; i++) {
		u_long	size = 0;
		int	prot = 0;
#ifdef DEBUG_EXEC_SVR4
		print_Phdr(&ph[i]);
#endif

		switch (ph[i].p_type) {
		case Elf32_pt_load:
			svr4_load_psection(epp, nd.ni_vp, &ph[i], &addr, &size,
					   &prot);
			/* Assume that the text segment is r-x only */
			if ((prot & PROT_WRITE) == 0) {
				epp->ep_entry = addr + eh.e_entry;
				ap->arg_interp = addr;
				DPRINTF(("Interpreter@ 0x%x\n", addr));
			}
			addr += size;
			break;

		case Elf32_pt_dynamic:
		case Elf32_pt_phdr:
		case Elf32_pt_note:
			break;

		default:
			DPRINTF(("interp: Unexpected program header type %d\n",
				 ph[i].p_type));
			break;
		}
	}

bad:
	if (ph != NULL)
		free((char *) ph, M_TEMP);
	if (bp != NULL)
		free((char *) bp, M_TEMP);

	*last = addr;
	vrele(nd.ni_vp);
	return error;
}


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
	struct proc		*p;
	struct exec_package	*epp;
{
	Elf32_Ehdr     *eh = epp->ep_hdr;
	Elf32_Phdr     *ph;
	int             error;
	int             i;
	char            interp[MAXPATHLEN];
	u_long          pos = 0;
	u_long          phsize;

#ifdef DEBUG_EXEC_SVR4
	print_Ehdr(eh);
#endif
	if (epp->ep_hdrvalid < sizeof(Elf32_Ehdr))
		return ENOEXEC;

	if (svr4_check_header(eh, Elf32_et_exec))
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
	phsize = eh->e_phnum * sizeof(Elf32_Phdr);
	ph = (Elf32_Phdr *) malloc(phsize, M_TEMP, M_WAITOK);

	if ((error = svr4_read_from(epp->ep_vp, eh->e_phoff, p,
				    (caddr_t) ph, phsize)) != 0)
		goto bad;

	epp->ep_emul = EMUL_IBCS2_ELF;
	epp->ep_tsize = ~0;
	epp->ep_dsize = ~0;

	interp[0] = '\0';

	/*
         * Load all the necessary sections
         */
	for (i = 0; i < eh->e_phnum; i++) {
		u_long          addr = ~0, size = 0;
		int             prot = 0;
		Elf32_Phdr     *pp = &ph[i];
#ifdef DEBUG_EXEC_SVR4
		print_Phdr(pp);
#endif

		switch (ph[i].p_type) {
		case Elf32_pt_load:
			svr4_load_psection(epp, epp->ep_vp, &ph[i], &addr,
					   &size, &prot);
			if ((error = svr4_set_segment(epp, addr, size,
						      prot)) != 0)
				goto bad;
			break;

		case Elf32_pt_shlib:
			DPRINTF(("No support for COFF libraries (yet)\n"));
			error = ENOEXEC;
			goto bad;

		case Elf32_pt_interp:
			if (pp->p_filesz >= sizeof(interp)) {
				DPRINTF(("Interpreter path too long %d\n",
					 pp->p_filesz));
				goto bad;
			}
			if ((error = svr4_read_from(epp->ep_vp, pp->p_offset, p,
				      (caddr_t) interp, pp->p_filesz)) != 0)
				goto bad;
			break;

		case Elf32_pt_dynamic:
		case Elf32_pt_phdr:
		case Elf32_pt_note:
			break;

		default:
			/*
			 * Not fatal, we don't need to understand everything
			 * :-)
			 */
			DPRINTF(("Unsupported program header type %d\n",
				 pp->p_type));
			break;
		}
	}

	/*
         * Check if we found a dynamically linked binary and arrange to load
         * it's interpreter
         */
	if (interp[0]) {
		struct svr4_args *ap;
		pos = SVR4_INTERP_ADDR;

		ap = (struct svr4_args *) malloc(sizeof(struct svr4_args),
						 M_TEMP, M_WAITOK);
		if ((error = svr4_load_interp(p, interp, epp, ap, &pos)) != 0) {
			free((char *) ap, M_TEMP);
			goto bad;
		}
		/* Arrange to load the program headers. */
		pos = SVR4_ALIGN(pos + SVR4_PAGESIZE, SVR4_PAGESIZE);
		DPRINTF(("Program header @0x%x\n", pos));
		ap->arg_phaddr = pos;
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, phsize,
			  pos, epp->ep_vp, eh->e_phoff,
			  VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
		pos += phsize;

		ap->arg_phentsize = eh->e_phentsize;
		ap->arg_phnum = eh->e_phnum;
		ap->arg_entry = eh->e_entry;

		epp->ep_setup = svr4_setup_args;
		epp->ep_setup_arg = ap;
		epp->ep_setup_arglen = SVR4_AUX_ARGSIZ / sizeof(char *);
	} else
		epp->ep_entry = eh->e_entry;

	DPRINTF(("taddr 0x%x tsize 0x%x daddr 0x%x dsize 0x%x\n",
	       epp->ep_taddr, epp->ep_tsize, epp->ep_daddr, epp->ep_dsize));

	free((char *) ph, M_TEMP);

	DPRINTF(("Elf entry@ 0x%x\n", epp->ep_entry));
	epp->ep_vp->v_flag |= VTEXT;

	return exec_aout_setup_stack(p, epp);

bad:
	free((char *) ph, M_TEMP);
	return ENOEXEC;
}
