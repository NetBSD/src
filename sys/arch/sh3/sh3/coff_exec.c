/*	$NetBSD: coff_exec.c,v 1.20 2003/07/15 03:35:56 lukem Exp $	*/

/*
 * Copyright (c) 1994, 1995 Scott Bartram
 * Copyright (c) 1994 Adam Glass
 * Copyright (c) 1993, 1994 Christopher G. Demetriou
 * All rights reserved.
 *
 * originally from kern/exec_ecoff.c
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
 *      This product includes software developed by Scott Bartram.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: coff_exec.c,v 1.20 2003/07/15 03:35:56 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/resourcevar.h>
#include <sys/namei.h>
#include <uvm/uvm_extern.h>

#include <sys/exec_coff.h>

static int coff_find_section(struct proc *, struct vnode *,
    struct coff_filehdr *, struct coff_scnhdr *, int);

/*
 * exec_coff_makecmds(): Check if it's an coff-format executable.
 *
 * Given a proc pointer and an exec package pointer, see if the referent
 * of the epp is in coff format.  Check 'standard' magic numbers for
 * this architecture.  If that fails, return failure.
 *
 * This function is  responsible for creating a set of vmcmds which can be
 * used to build the process's vm space and inserting them into the exec
 * package.
 */

int
exec_coff_makecmds(struct proc *p, struct exec_package *epp)
{
	int error;
	struct coff_filehdr *fp = epp->ep_hdr;
	struct coff_aouthdr *ap;

	if (epp->ep_hdrvalid < COFF_HDR_SIZE)
		return ENOEXEC;

	if (COFF_BADMAG(fp))
		return ENOEXEC;

	ap = (void *)((char *)epp->ep_hdr + sizeof(struct coff_filehdr));
	switch (ap->a_magic) {
	case COFF_OMAGIC:
		error = exec_coff_prep_omagic(p, epp, fp, ap);
		break;
	case COFF_NMAGIC:
		error = exec_coff_prep_nmagic(p, epp, fp, ap);
		break;
	case COFF_ZMAGIC:
		error = exec_coff_prep_zmagic(p, epp, fp, ap);
		break;
	default:
		return ENOEXEC;
	}

#ifdef	TODO
	if (error == 0)
		error = cpu_exec_coff_hook(p, epp);
#endif

	if (error)
		kill_vmcmds(&epp->ep_vmcmds);

	return error;
}

/*
 * exec_coff_setup_stack(): Set up the stack segment for a coff
 * executable.
 *
 * Note that the ep_ssize parameter must be set to be the current stack
 * limit; this is adjusted in the body of execve() to yield the
 * appropriate stack segment usage once the argument length is
 * calculated.
 *
 * This function returns an int for uniformity with other (future) formats'
 * stack setup functions.  They might have errors to return.
 */

int
exec_coff_setup_stack(struct proc *p, struct exec_package *epp)
{
	DPRINTF(("enter exec_coff_setup_stack\n"));

	epp->ep_maxsaddr = USRSTACK - MAXSSIZ;
	epp->ep_minsaddr = USRSTACK;
	epp->ep_ssize = p->p_rlimit[RLIMIT_STACK].rlim_cur;

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
	DPRINTF(("VMCMD: addr %lx size %lx\n", epp->ep_maxsaddr,
	    (epp->ep_minsaddr - epp->ep_ssize) - epp->ep_maxsaddr));
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero,
	    ((epp->ep_minsaddr - epp->ep_ssize) - epp->ep_maxsaddr),
	    epp->ep_maxsaddr, NULLVP, 0, VM_PROT_NONE);
	DPRINTF(("VMCMD: addr %lx size %lx\n",
	    epp->ep_minsaddr - epp->ep_ssize,
	    epp->ep_ssize));
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, epp->ep_ssize,
	    (epp->ep_minsaddr - epp->ep_ssize), NULLVP, 0,
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	return 0;
}


/*
 * exec_coff_prep_omagic(): Prepare a COFF OMAGIC binary's exec package
 */

int
exec_coff_prep_omagic(struct proc *p, struct exec_package *epp,
    struct coff_filehdr *fp, struct coff_aouthdr *ap)
{
	epp->ep_taddr = COFF_SEGMENT_ALIGN(fp, ap, ap->a_tstart);
	epp->ep_tsize = ap->a_tsize;
	epp->ep_daddr = COFF_SEGMENT_ALIGN(fp, ap, ap->a_dstart);
	epp->ep_dsize = ap->a_dsize;
	epp->ep_entry = ap->a_entry;

	/* set up command for text and data segments */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn,
	    ap->a_tsize + ap->a_dsize, epp->ep_taddr, epp->ep_vp,
	    COFF_TXTOFF(fp, ap),
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
#ifdef	__sh__
	if (ap->a_bsize > 0)
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, ap->a_bsize,
		    COFF_ROUND(ap->a_dstart + ap->a_dsize, COFF_LDPGSZ),
		    NULLVP, 0,
		    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
#else
	if (ap->a_bsize > 0)
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, ap->a_bsize,
		    COFF_SEGMENT_ALIGN(fp, ap,
			ap->a_dstart + ap->a_dsize),
		    NULLVP, 0,
		    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
#endif

	return exec_coff_setup_stack(p, epp);
}

/*
 * exec_coff_prep_nmagic(): Prepare a 'native' NMAGIC COFF binary's exec
 *                          package.
 */

int
exec_coff_prep_nmagic(p, epp, fp, ap)
	struct proc *p;
	struct exec_package *epp;
	struct coff_filehdr *fp;
	struct coff_aouthdr *ap;
{
	epp->ep_taddr = COFF_SEGMENT_ALIGN(fp, ap, ap->a_tstart);
	epp->ep_tsize = ap->a_tsize;
	epp->ep_daddr = COFF_ROUND(ap->a_dstart, COFF_LDPGSZ);
	epp->ep_dsize = ap->a_dsize;
	epp->ep_entry = ap->a_entry;

	/* set up command for text segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, epp->ep_tsize,
	    epp->ep_taddr, epp->ep_vp, COFF_TXTOFF(fp, ap),
	    VM_PROT_READ|VM_PROT_EXECUTE);

	/* set up command for data segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, epp->ep_dsize,
	    epp->ep_daddr, epp->ep_vp, COFF_DATOFF(fp, ap),
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	if (ap->a_bsize > 0)
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, ap->a_bsize,
		    COFF_SEGMENT_ALIGN(fp, ap,
			ap->a_dstart + ap->a_dsize),
		    NULLVP, 0,
		    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	return exec_coff_setup_stack(p, epp);
}

/*
 * coff_find_section - load specified section header
 *
 * TODO - optimize by reading all section headers in at once
 */

static int
coff_find_section(struct proc *p, struct vnode *vp, struct coff_filehdr *fp,
    struct coff_scnhdr *sh, int s_type)
{
	int i, pos, resid, siz, error;

	pos = COFF_HDR_SIZE;
	for (i = 0; i < fp->f_nscns; i++, pos += sizeof(struct coff_scnhdr)) {
		siz = sizeof(struct coff_scnhdr);
		error = vn_rdwr(UIO_READ, vp, (caddr_t) sh,
		    siz, pos, UIO_SYSSPACE, IO_NODELOCKED, p->p_ucred,
		    &resid, p);
		if (error) {
			DPRINTF(("section hdr %d read error %d\n", i, error));
			return error;
		}
		siz -= resid;
		if (siz != sizeof(struct coff_scnhdr)) {
			DPRINTF(("incomplete read: hdr %d ask=%d, rem=%d got %d\n",
			    s_type, sizeof(struct coff_scnhdr),
			    resid, siz));
			return ENOEXEC;
		}
		DPRINTF(("found section: %lu\n", sh->s_flags));
		if (sh->s_flags == s_type)
			return 0;
	}
	return ENOEXEC;
}

/*
 * exec_coff_prep_zmagic(): Prepare a COFF ZMAGIC binary's exec package
 *
 * First, set the various offsets/lengths in the exec package.
 *
 * Then, mark the text image busy (so it can be demand paged) or error
 * out if this is not possible.  Finally, set up vmcmds for the
 * text, data, bss, and stack segments.
 */

int
exec_coff_prep_zmagic(struct proc *p, struct exec_package *epp,
    struct coff_filehdr *fp, struct coff_aouthdr *ap)
{
	int error;
	u_long offset;
	long dsize;
#ifndef	__sh__
	long  baddr, bsize;
#endif
	struct coff_scnhdr sh;

	DPRINTF(("enter exec_coff_prep_zmagic\n"));

	/* set up command for text segment */
	error = coff_find_section(p, epp->ep_vp, fp, &sh, COFF_STYP_TEXT);
	if (error) {
		DPRINTF(("can't find text section: %d\n", error));
		return error;
	}
	DPRINTF(("COFF text addr %lu size %ld offset %ld\n", sh.s_vaddr,
	    sh.s_size, sh.s_scnptr));
	epp->ep_taddr = COFF_ALIGN(sh.s_vaddr);
	offset = sh.s_scnptr - (sh.s_vaddr - epp->ep_taddr);
	epp->ep_tsize = sh.s_size + (sh.s_vaddr - epp->ep_taddr);

	error = vn_marktext(epp->ep_vp);
	if (error)
		return (error);

	DPRINTF(("VMCMD: addr %lx size %lx offset %lx\n", epp->ep_taddr,
	    epp->ep_tsize, offset));
	if (!(offset & PAGE_MASK) && !(epp->ep_taddr & PAGE_MASK)) {
		epp->ep_tsize =	round_page(epp->ep_tsize);
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, epp->ep_tsize,
		    epp->ep_taddr, epp->ep_vp, offset,
		    VM_PROT_READ|VM_PROT_EXECUTE);
	} else {
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, epp->ep_tsize,
		    epp->ep_taddr, epp->ep_vp, offset,
		    VM_PROT_READ|VM_PROT_EXECUTE);
	}

	/* set up command for data segment */
	error = coff_find_section(p, epp->ep_vp, fp, &sh, COFF_STYP_DATA);
	if (error) {
		DPRINTF(("can't find data section: %d\n", error));
		return error;
	}
	DPRINTF(("COFF data addr %lx size %ld offset %ld\n", sh.s_vaddr,
	    sh.s_size, sh.s_scnptr));
	epp->ep_daddr = COFF_ALIGN(sh.s_vaddr);
	offset = sh.s_scnptr - (sh.s_vaddr - epp->ep_daddr);
	dsize = sh.s_size + (sh.s_vaddr - epp->ep_daddr);
#ifdef __sh__
	epp->ep_dsize = round_page(dsize) + ap->a_bsize;
#else
	epp->ep_dsize = dsize + ap->a_bsize;
#endif

	DPRINTF(("VMCMD: addr %lx size %lx offset %lx\n", epp->ep_daddr,
	    dsize, offset));
	if (!(offset & PAGE_MASK) && !(epp->ep_daddr & PAGE_MASK)) {
		dsize = round_page(dsize);
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, dsize,
		    epp->ep_daddr, epp->ep_vp, offset,
		    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
	} else {
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn,
		    dsize, epp->ep_daddr, epp->ep_vp, offset,
		    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
	}

#ifdef	__sh__
	if (ap->a_bsize > 0) {
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, ap->a_bsize,
		    COFF_ROUND(ap->a_dstart + ap->a_dsize, COFF_LDPGSZ),
		    NULLVP, 0,
		    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
	}
#else
	/* set up command for bss segment */
	baddr = round_page(epp->ep_daddr + dsize);
	bsize = epp->ep_daddr + epp->ep_dsize - baddr;
	if (bsize > 0) {
		DPRINTF(("VMCMD: addr %x size %x offset %x\n",
		    baddr, bsize, 0));
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero,
		    bsize, baddr, NULLVP, 0,
		    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
	}
#endif

#ifdef	TODO
	/* load any shared libraries */
	error = coff_find_section(p, epp->ep_vp, fp, &sh, COFF_STYP_SHLIB);
	if (!error) {
		int resid;
		struct coff_slhdr *slhdr;
		char buf[128], *bufp;	/* FIXME */
		int len = sh.s_size, path_index, entry_len;

		DPRINTF(("COFF shlib size %d offset %d\n",
		    sh.s_size, sh.s_scnptr));

		error = vn_rdwr(UIO_READ, epp->ep_vp, (caddr_t) buf,
		    len, sh.s_scnptr,
		    UIO_SYSSPACE, IO_NODELOCKED, p->p_ucred,
		    &resid, p);
		if (error) {
			DPRINTF(("shlib section read error %d\n", error));
			return ENOEXEC;
		}
		bufp = buf;
		while (len) {
			slhdr = (struct coff_slhdr *)bufp;
			path_index = slhdr->path_index * sizeof(long);
			entry_len = slhdr->entry_len * sizeof(long);

			DPRINTF(("path_index: %d entry_len: %d name: %s\n",
			    path_index, entry_len, slhdr->sl_name));

			error = coff_load_shlib(p, slhdr->sl_name, epp);
			if (error)
				return ENOEXEC;
			bufp += entry_len;
			len -= entry_len;
		}
	}
#endif
	/* set up entry point */
	epp->ep_entry = ap->a_entry;

#if 1
	DPRINTF(("text addr: %lx size: %ld data addr: %lx size: %ld entry: %lx\n",
	    epp->ep_taddr, epp->ep_tsize,
	    epp->ep_daddr, epp->ep_dsize,
	    epp->ep_entry));
#endif
	return exec_coff_setup_stack(p, epp);
}

#if 0
int
coff_load_shlib(struct proc *p, char *path, struct exec_package *epp)
{
	int error, siz, resid;
	int taddr, tsize, daddr, dsize, offset;
	struct nameidata nd;
	struct coff_filehdr fh, *fhp = &fh;
	struct coff_scnhdr sh, *shp = &sh;
	caddr_t sg = stackgap_init(p, 0);

	/*
	 * 1. open shlib file
	 * 2. read filehdr
	 * 3. map text, data, and bss out of it using VM_*
	 */
#ifdef TODO
	IBCS2_CHECK_ALT_EXIST(p, &sg, path);
#endif
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, path, p);
	/* first get the vnode */
	if ((error = namei(&nd)) != 0) {
		DPRINTF(("coff_load_shlib: can't find library %s\n", path));
		return error;
	}

	siz = sizeof(struct coff_filehdr);
	error = vn_rdwr(UIO_READ, nd.ni_vp, (caddr_t) fhp, siz, 0,
	    UIO_SYSSPACE, IO_NODELOCKED, p->p_ucred, &resid, p);
	if (error) {
		DPRINTF(("filehdr read error %d\n", error));
		vrele(nd.ni_vp);
		return error;
	}
	siz -= resid;
	if (siz != sizeof(struct coff_filehdr)) {
		DPRINTF(("coff_load_shlib: incomplete read: ask=%d, rem=%d got %d\n",
		    sizeof(struct coff_filehdr), resid, siz));
		vrele(nd.ni_vp);
		return ENOEXEC;
	}

	/* load text */
	error = coff_find_section(p, nd.ni_vp, fhp, shp, COFF_STYP_TEXT);
	if (error) {
		DPRINTF(("can't find shlib text section\n"));
		vrele(nd.ni_vp);
		return error;
	}
	DPRINTF(("COFF text addr %x size %d offset %d\n", sh.s_vaddr,
	    sh.s_size, sh.s_scnptr));
	taddr = COFF_ALIGN(shp->s_vaddr);
	offset = shp->s_scnptr - (shp->s_vaddr - taddr);
	tsize = shp->s_size + (shp->s_vaddr - taddr);
	DPRINTF(("VMCMD: addr %x size %x offset %x\n", taddr, tsize, offset));
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, tsize, taddr,
	    nd.ni_vp, offset,
	    VM_PROT_READ|VM_PROT_EXECUTE);

	/* load data */
	error = coff_find_section(p, nd.ni_vp, fhp, shp, COFF_STYP_DATA);
	if (error) {
		DPRINTF(("can't find shlib data section\n"));
		vrele(nd.ni_vp);
		return error;
	}
	DPRINTF(("COFF data addr %x size %d offset %d\n", shp->s_vaddr,
	    shp->s_size, shp->s_scnptr));
	daddr = COFF_ALIGN(shp->s_vaddr);
	offset = shp->s_scnptr - (shp->s_vaddr - daddr);
	dsize = shp->s_size + (shp->s_vaddr - daddr);
	/* epp->ep_dsize = dsize + ap->a_bsize; */

	DPRINTF(("VMCMD: addr %x size %x offset %x\n", daddr, dsize, offset));
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn,
	    dsize, daddr, nd.ni_vp, offset,
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* load bss */
	error = coff_find_section(p, nd.ni_vp, fhp, shp, COFF_STYP_BSS);
	if (!error) {
		int baddr = round_page(daddr + dsize);
		int bsize = daddr + dsize + shp->s_size - baddr;
		if (bsize > 0) {
			DPRINTF(("VMCMD: addr %x size %x offset %x\n",
			    baddr, bsize, 0));
			NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero,
			    bsize, baddr, NULLVP, 0,
			    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
		}
	}
	vrele(nd.ni_vp);

	return 0;
}
#endif

