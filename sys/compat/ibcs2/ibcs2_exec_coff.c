/*	$NetBSD: ibcs2_exec_coff.c,v 1.23.30.1 2009/07/23 23:31:38 jym Exp $	*/

/*
 * Copyright (c) 1994, 1995, 1998 Scott Bartram
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
__KERNEL_RCSID(0, "$NetBSD: ibcs2_exec_coff.c,v 1.23.30.1 2009/07/23 23:31:38 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/exec_coff.h>
#include <sys/resourcevar.h>

#include <sys/mman.h>

#include <sys/cpu.h>
#include <machine/reg.h>
#include <machine/ibcs2_machdep.h>

#include <compat/ibcs2/ibcs2_types.h>
#include <compat/ibcs2/ibcs2_exec.h>
#include <compat/ibcs2/ibcs2_errno.h>
#include <compat/ibcs2/ibcs2_util.h>


int exec_ibcs2_coff_prep_omagic(struct lwp *, struct exec_package *,
				     struct coff_filehdr *,
				     struct coff_aouthdr *);
int exec_ibcs2_coff_prep_nmagic(struct lwp *, struct exec_package *,
				     struct coff_filehdr *,
				     struct coff_aouthdr *);
int exec_ibcs2_coff_prep_zmagic(struct lwp *, struct exec_package *,
				     struct coff_filehdr *,
				     struct coff_aouthdr *);
void cpu_exec_ibcs2_coff_setup(int, struct proc *, struct exec_package *,
				    void *);

static int coff_load_shlib(struct lwp *, const char *,
		struct exec_package *);
static int coff_find_section(struct lwp *, struct vnode *,
				  struct coff_filehdr *, struct coff_scnhdr *,
				  int);

/*
 * exec_ibcs2_coff_makecmds(): Check if it's an coff-format executable.
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
exec_ibcs2_coff_makecmds(struct lwp *l, struct exec_package *epp)
{
	int error;
	struct coff_filehdr *fp = epp->ep_hdr;
	struct coff_aouthdr *ap;

	if (epp->ep_hdrvalid < COFF_HDR_SIZE) {
		DPRINTF(("ibcs2: bad coff hdr size\n"));
		return ENOEXEC;
	}

	if (COFF_BADMAG(fp)) {
		DPRINTF(("ibcs2: bad coff magic\n"));
		return ENOEXEC;
	}

	ap = (void *)((char *)epp->ep_hdr + sizeof(struct coff_filehdr));
	switch (ap->a_magic) {
	case COFF_OMAGIC:
		error = exec_ibcs2_coff_prep_omagic(l, epp, fp, ap);
		break;
	case COFF_NMAGIC:
		error = exec_ibcs2_coff_prep_nmagic(l, epp, fp, ap);
		break;
	case COFF_ZMAGIC:
		error = exec_ibcs2_coff_prep_zmagic(l, epp, fp, ap);
		break;
	default:
		return ENOEXEC;
	}

	if (error)
		kill_vmcmds(&epp->ep_vmcmds);

	return error;
}

/*
 * exec_ibcs2_coff_prep_omagic(): Prepare a COFF OMAGIC binary's exec package
 */

int
exec_ibcs2_coff_prep_omagic(struct lwp *l, struct exec_package *epp, struct coff_filehdr *fp, struct coff_aouthdr *ap)
{
	epp->ep_taddr = COFF_SEGMENT_ALIGN(fp, ap, ap->a_tstart);
	epp->ep_tsize = ap->a_tsize;
	epp->ep_daddr = COFF_SEGMENT_ALIGN(fp, ap, ap->a_dstart);
	epp->ep_dsize = ap->a_dsize;
	epp->ep_entry = ap->a_entry;

	DPRINTF(("ibcs2_omagic: text=%#lx/%#lx, data=%#lx/%#lx, bss=%#lx/%#lx, entry=%#lx\n",
		epp->ep_taddr, epp->ep_tsize,
		epp->ep_daddr, epp->ep_dsize,
		ap->a_dstart + ap->a_dsize, ap->a_bsize,
		epp->ep_entry));

	/* set up command for text and data segments */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn,
		  ap->a_tsize + ap->a_dsize, epp->ep_taddr, epp->ep_vp,
		  COFF_TXTOFF(fp, ap),
		  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	if (ap->a_bsize > 0) {
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, ap->a_bsize,
			  COFF_SEGMENT_ALIGN(fp, ap, ap->a_dstart + ap->a_dsize),
			  NULLVP, 0,
			  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
		epp->ep_dsize += ap->a_bsize;
	}
	/* The following is to make obreak(2) happy.  It expects daddr
	 * to on a page boundary and will round up dsize to a page
	 * address.
	 */
	if (trunc_page(epp->ep_daddr) != epp->ep_daddr) {
		epp->ep_dsize += epp->ep_daddr - trunc_page(epp->ep_daddr);
		epp->ep_daddr = trunc_page(epp->ep_daddr);
		if (epp->ep_taddr + epp->ep_tsize > epp->ep_daddr)
			epp->ep_tsize = epp->ep_daddr - epp->ep_taddr;
	}

	return (*epp->ep_esch->es_setup_stack)(l, epp);
}

/*
 * exec_ibcs2_coff_prep_nmagic(): Prepare a 'native' NMAGIC COFF binary's exec
 *                          package.
 */

int
exec_ibcs2_coff_prep_nmagic(struct lwp *l, struct exec_package *epp, struct coff_filehdr *fp, struct coff_aouthdr *ap)
{
	long toverlap, doverlap;
	u_long tsize, tend;

	epp->ep_taddr = ap->a_tstart;
	epp->ep_tsize = ap->a_tsize;
	epp->ep_daddr = ap->a_dstart;
	epp->ep_dsize = ap->a_dsize;
	epp->ep_entry = ap->a_entry;

	DPRINTF(("ibcs2_nmagic: text=%#lx/%#lx, data=%#lx/%#lx, bss=%#lx/%#lx, entry=%#lx\n",
		epp->ep_taddr, epp->ep_tsize,
		epp->ep_daddr, epp->ep_dsize,
		ap->a_dstart + ap->a_dsize, ap->a_bsize,
		epp->ep_entry));

	/* Do the text and data pages overlap?
	 */
	tend = epp->ep_taddr + epp->ep_tsize - 1;
	if (trunc_page(tend) == trunc_page(epp->ep_daddr)) {
		/* If the first page of text is the first page of data,
		 * then we consider it all data.
		 */
		if (trunc_page(epp->ep_taddr) == trunc_page(epp->ep_daddr)) {
			tsize = 0;
		} else {
			tsize = trunc_page(tend) - epp->ep_taddr;
		}

		/* If the text and data file and VA offsets are the
		 * same, simply bring the data segment to start on
		 * the start of the page.
		 */
		if (epp->ep_daddr - epp->ep_taddr ==
		    COFF_DATOFF(fp, ap) - COFF_TXTOFF(fp, ap)) {
			u_long diff = epp->ep_daddr - trunc_page(epp->ep_daddr);
			toverlap = 0;
			doverlap = -diff;
		} else {
			/* otherwise copy the individual pieces */
			toverlap = epp->ep_tsize - tsize;
			doverlap = round_page(epp->ep_daddr) - epp->ep_daddr;
			if (doverlap > epp->ep_dsize)
				doverlap = epp->ep_dsize;
		}
	} else {
		tsize = epp->ep_tsize;
		toverlap = 0;
		doverlap = 0;
	}

	DPRINTF(("nmagic_vmcmds:"));
	if (tsize > 0) {
		/* set up command for text segment */
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, tsize,
			  epp->ep_taddr, epp->ep_vp, COFF_TXTOFF(fp, ap),
			  VM_PROT_READ|VM_PROT_EXECUTE);
		DPRINTF((" map_readvn(%#lx/%#lx@%#lx)",
			epp->ep_taddr, tsize, (u_long) COFF_TXTOFF(fp, ap)));
	}
	if (toverlap > 0) {
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, toverlap,
			  epp->ep_taddr + tsize, epp->ep_vp,
			  COFF_TXTOFF(fp, ap) + tsize,
			  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
		DPRINTF((" map_readvn(%#lx/%#lx@%#lx)",
			epp->ep_taddr + tsize, toverlap,
			COFF_TXTOFF(fp, ap) + tsize));
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_readvn, doverlap,
			  epp->ep_daddr, epp->ep_vp,
			  COFF_DATOFF(fp, ap),
			  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
		DPRINTF((" readvn(%#lx/%#lx@%#lx)", epp->ep_daddr, doverlap,
			COFF_DATOFF(fp, ap)));
	}

	if (epp->ep_dsize > doverlap || doverlap < 0) {
		/* set up command for data segment */
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn,
			  epp->ep_dsize - doverlap, epp->ep_daddr + doverlap,
			  epp->ep_vp, COFF_DATOFF(fp, ap) + doverlap,
			  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
		DPRINTF((" map_readvn(%#lx/%#lx@%#lx)",
			epp->ep_daddr + doverlap, epp->ep_dsize - doverlap,
			COFF_DATOFF(fp, ap) + doverlap));
	}

	/* Handle page remainders for pagedvn.
	 */

	/* set up command for bss segment */
	if (ap->a_bsize > 0) {
		u_long dend = round_page(epp->ep_daddr + epp->ep_dsize);
		u_long dspace = dend - (epp->ep_daddr + epp->ep_dsize);
		if (ap->a_bsize > dspace) {
			NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero,
				  ap->a_bsize - dspace, dend, NULLVP, 0,
				  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
			DPRINTF((" map_zero(%#lx/%#lx)",
				dend, ap->a_bsize - dspace));
		}
		epp->ep_dsize += ap->a_bsize;
	}
	DPRINTF(("\n"));
	/* The following is to make obreak(2) happy.  It expects daddr
	 * to on a page boundary and will round up dsize to a page
	 * address.
	 */
	if (trunc_page(epp->ep_daddr) != epp->ep_daddr) {
		epp->ep_dsize += epp->ep_daddr - trunc_page(epp->ep_daddr);
		epp->ep_daddr = trunc_page(epp->ep_daddr);
		if (epp->ep_taddr + epp->ep_tsize > epp->ep_daddr)
			epp->ep_tsize = epp->ep_daddr - epp->ep_taddr;
	}

	return (*epp->ep_esch->es_setup_stack)(l, epp);
}

/*
 * coff_find_section - load specified section header
 *
 * TODO - optimize by reading all section headers in at once
 */

static int
coff_find_section(struct lwp *l, struct vnode *vp, struct coff_filehdr *fp, struct coff_scnhdr *sh, int s_type)
{
	int i, pos, siz, error;
	size_t resid;

	pos = COFF_HDR_SIZE;
	for (i = 0; i < fp->f_nscns; i++, pos += sizeof(struct coff_scnhdr)) {
		siz = sizeof(struct coff_scnhdr);
		error = vn_rdwr(UIO_READ, vp, (void *) sh,
		    siz, pos, UIO_SYSSPACE, IO_NODELOCKED, l->l_cred,
		    &resid, NULL);
		if (error) {
			DPRINTF(("section hdr %d read error %d\n", i, error));
			return error;
		}
		siz -= resid;
		if (siz != sizeof(struct coff_scnhdr)) {
			DPRINTF(("incomplete read: hdr %d ask=%d, rem=%lu got %d\n",
				 s_type, sizeof(struct coff_scnhdr),
				 (u_long) resid, siz));
			return ENOEXEC;
		}
		/* DPRINTF(("found section: %x\n", sh->s_flags)); */
		if (sh->s_flags == s_type)
			return 0;
	}
	return ENOEXEC;
}

/*
 * exec_ibcs2_coff_prep_zmagic(): Prepare a COFF ZMAGIC binary's exec package
 *
 * First, set the various offsets/lengths in the exec package.
 *
 * Then, mark the text image busy (so it can be demand paged) or error
 * out if this is not possible.  Finally, set up vmcmds for the
 * text, data, bss, and stack segments.
 */

int
exec_ibcs2_coff_prep_zmagic(struct lwp *l, struct exec_package *epp, struct coff_filehdr *fp, struct coff_aouthdr *ap)
{
	int error;
	u_long offset;
	long dsize, baddr, bsize;
	struct coff_scnhdr sh;

	/* DPRINTF(("enter exec_ibcs2_coff_prep_zmagic\n")); */

	/* set up command for text segment */
	error = coff_find_section(l, epp->ep_vp, fp, &sh, COFF_STYP_TEXT);
	if (error) {
		DPRINTF(("can't find text section: %d\n", error));
		return error;
	}
	/* DPRINTF(("COFF text addr %x size %d offset %d\n", sh.s_vaddr,
		 sh.s_size, sh.s_scnptr)); */
	epp->ep_taddr = COFF_ALIGN(sh.s_vaddr);
	offset = sh.s_scnptr - (sh.s_vaddr - epp->ep_taddr);
	epp->ep_tsize = sh.s_size + (sh.s_vaddr - epp->ep_taddr);

	/* DPRINTF(("VMCMD: addr %x size %d offset %d\n", epp->ep_taddr,
		 epp->ep_tsize, offset)); */
#ifdef notyet
	error = vn_marktext(epp->ep_vp);
	if (error)
		return (error);

	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, epp->ep_tsize,
		  epp->ep_taddr, epp->ep_vp, offset,
		  VM_PROT_READ|VM_PROT_EXECUTE);
#else
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, epp->ep_tsize,
		  epp->ep_taddr, epp->ep_vp, offset,
		  VM_PROT_READ|VM_PROT_EXECUTE);
#endif

	/* set up command for data segment */
	error = coff_find_section(l, epp->ep_vp, fp, &sh, COFF_STYP_DATA);
	if (error) {
		DPRINTF(("can't find data section: %d\n", error));
		return error;
	}
	/* DPRINTF(("COFF data addr %x size %d offset %d\n", sh.s_vaddr,
		 sh.s_size, sh.s_scnptr)); */
	epp->ep_daddr = COFF_ALIGN(sh.s_vaddr);
	offset = sh.s_scnptr - (sh.s_vaddr - epp->ep_daddr);
	dsize = sh.s_size + (sh.s_vaddr - epp->ep_daddr);
	epp->ep_dsize = dsize + ap->a_bsize;

	/* DPRINTF(("VMCMD: addr %x size %d offset %d\n", epp->ep_daddr,
		 dsize, offset)); */
#ifdef notyet
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, dsize,
		  epp->ep_daddr, epp->ep_vp, offset,
		  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
#else
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn,
		  dsize, epp->ep_daddr, epp->ep_vp, offset,
		  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
#endif

	/* set up command for bss segment */
	baddr = round_page(epp->ep_daddr + dsize);
	bsize = epp->ep_daddr + epp->ep_dsize - baddr;
	if (bsize > 0) {
		/* DPRINTF(("VMCMD: addr %x size %d offset %d\n",
			 baddr, bsize, 0)); */
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero,
			  bsize, baddr, NULLVP, 0,
			  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
	}

	/* load any shared libraries */
	error = coff_find_section(l, epp->ep_vp, fp, &sh, COFF_STYP_SHLIB);
	if (!error) {
		size_t resid;
		struct coff_slhdr *slhdr;
		char *tbuf, *bufp;
		size_t len = sh.s_size, path_index, entry_len;

		if (len > 64 * 1024)
			return ENOEXEC;

		tbuf = malloc(len, M_TEMP, M_WAITOK);
		if (tbuf == NULL)
			return ENOEXEC;

		/* DPRINTF(("COFF shlib size %d offset %d\n",
			 sh.s_size, sh.s_scnptr)); */

		error = vn_rdwr(UIO_READ, epp->ep_vp, tbuf,
				len, sh.s_scnptr,
				UIO_SYSSPACE, IO_NODELOCKED, l->l_cred,
				&resid, NULL);
		if (error) {
			DPRINTF(("shlib section read error %d\n", error));
			free(tbuf, M_TEMP);
			return ENOEXEC;
		}
		bufp = tbuf;
		while (len) {
			slhdr = (struct coff_slhdr *)bufp;

			if (slhdr->path_index > LONG_MAX / sizeof(long) ||
			    slhdr->entry_len > LONG_MAX / sizeof(long)) {
				free(tbuf, M_TEMP);
				return ENOEXEC;
			}

			path_index = slhdr->path_index * sizeof(long);
			entry_len = slhdr->entry_len * sizeof(long);

			if (entry_len > len) {
				free(tbuf, M_TEMP);
				return ENOEXEC;
			}

			/* DPRINTF(("path_index: %d entry_len: %d name: %s\n",
				 path_index, entry_len, slhdr->sl_name)); */

			error = coff_load_shlib(l, slhdr->sl_name, epp);
			if (error) {
				free(tbuf, M_TEMP);
				return ENOEXEC;
			}
			bufp += entry_len;
			len -= entry_len;
		}
		free(tbuf, M_TEMP);
	}

	/* set up entry point */
	epp->ep_entry = ap->a_entry;

	DPRINTF(("ibcs2_zmagic: text addr: %#lx size: %#lx data addr: %#lx size: %#lx entry: %#lx\n",
		 epp->ep_taddr, epp->ep_tsize,
		 epp->ep_daddr, epp->ep_dsize,
		 epp->ep_entry));

	/* The following is to make obreak(2) happy.  It expects daddr
	 * to on a page boundary and will round up dsize to a page
	 * address.
	 */
	if (trunc_page(epp->ep_daddr) != epp->ep_daddr) {
		epp->ep_dsize += epp->ep_daddr - trunc_page(epp->ep_daddr);
		epp->ep_daddr = trunc_page(epp->ep_daddr);
		if (epp->ep_taddr + epp->ep_tsize > epp->ep_daddr)
			epp->ep_tsize = epp->ep_daddr - epp->ep_taddr;
	}


	return (*epp->ep_esch->es_setup_stack)(l, epp);
}

static int
coff_load_shlib(struct lwp *l, const char *path, struct exec_package *epp)
{
	int error, siz;
	int taddr, tsize, daddr, dsize, offset;
	size_t resid;
	struct vnode *vp;
	struct coff_filehdr fh, *fhp = &fh;
	struct coff_scnhdr sh, *shp = &sh;

	/*
	 * 1. open shlib file
	 * 2. read filehdr
	 * 3. map text, data, and bss out of it using VM_*
	 */
	/* first get the vnode */
	error = namei_simple_kernel(path, NSM_FOLLOW_TRYEMULROOT, &vp);
	if (error != 0) {
		DPRINTF(("coff_load_shlib: can't find library %s\n", path));
		return error;
	}

	siz = sizeof(struct coff_filehdr);
	error = vn_rdwr(UIO_READ, vp, (void *) fhp, siz, 0,
	    UIO_SYSSPACE, IO_NODELOCKED, l->l_cred, &resid, l);
	if (error) {
	    DPRINTF(("filehdr read error %d\n", error));
	    vrele(vp);
	    return error;
	}
	siz -= resid;
	if (siz != sizeof(struct coff_filehdr)) {
	    DPRINTF(("coff_load_shlib: incomplete read: ask=%d, rem=%lu got %d\n",
		     sizeof(struct coff_filehdr), (u_long) resid, siz));
	    vrele(vp);
	    return ENOEXEC;
	}

	/* load text */
	error = coff_find_section(l, vp, fhp, shp, COFF_STYP_TEXT);
	if (error) {
	    DPRINTF(("can't find shlib text section\n"));
	    vrele(vp);
	    return error;
	}
	/* DPRINTF(("COFF text addr %x size %d offset %d\n", sh.s_vaddr,
		 sh.s_size, sh.s_scnptr)); */
	taddr = COFF_ALIGN(shp->s_vaddr);
	offset = shp->s_scnptr - (shp->s_vaddr - taddr);
	tsize = shp->s_size + (shp->s_vaddr - taddr);
	/* DPRINTF(("VMCMD: addr %x size %d offset %d\n", taddr, tsize, offset)); */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, tsize, taddr,
		  vp, offset,
		  VM_PROT_READ|VM_PROT_EXECUTE);

	/* load data */
	error = coff_find_section(l, vp, fhp, shp, COFF_STYP_DATA);
	if (error) {
	    DPRINTF(("can't find shlib data section\n"));
	    vrele(vp);
	    return error;
	}
	/* DPRINTF(("COFF data addr %x size %d offset %d\n", shp->s_vaddr,
		 shp->s_size, shp->s_scnptr)); */
	daddr = COFF_ALIGN(shp->s_vaddr);
	offset = shp->s_scnptr - (shp->s_vaddr - daddr);
	dsize = shp->s_size + (shp->s_vaddr - daddr);
	/* epp->ep_dsize = dsize + ap->a_bsize; */

	/* DPRINTF(("VMCMD: addr %x size %d offset %d\n", daddr, dsize, offset)); */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn,
		  dsize, daddr, vp, offset,
		  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* load bss */
	error = coff_find_section(l, vp, fhp, shp, COFF_STYP_BSS);
	if (!error) {
		int baddr = round_page(daddr + dsize);
		int bsize = daddr + dsize + shp->s_size - baddr;
		if (bsize > 0) {
			/* DPRINTF(("VMCMD: addr %x size %d offset %d\n",
			   baddr, bsize, 0)); */
			NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero,
				  bsize, baddr, NULLVP, 0,
				  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
	    }
	}
	vrele(vp);

	return 0;
}
