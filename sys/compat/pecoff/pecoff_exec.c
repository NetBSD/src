/*	$NetBSD: pecoff_exec.c,v 1.29.6.1 2006/06/01 22:35:59 kardel Exp $	*/

/*
 * Copyright (c) 2000 Masaru OKI
 * Copyright (c) 1994, 1995, 1998 Scott Bartram
 * Copyright (c) 1994 Adam Glass
 * Copyright (c) 1993, 1994 Christopher G. Demetriou
 * All rights reserved.
 *
 * from compat/ibcs2/ibcs2_exec.c
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
__KERNEL_RCSID(0, "$NetBSD: pecoff_exec.c,v 1.29.6.1 2006/06/01 22:35:59 kardel Exp $");

/*#define DEBUG_PECOFF*/

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/syscall.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/stat.h>

#include <sys/exec_coff.h>
#include <machine/coff_machdep.h>

#include <compat/pecoff/pecoff_exec.h>
#include <compat/pecoff/pecoff_util.h>
#include <compat/pecoff/pecoff_syscall.h>

int pecoff_signature (struct lwp *l, struct vnode *vp,
		      struct pecoff_dos_filehdr *dp);
int pecoff_load_file (struct lwp *l, struct exec_package *epp,
		      const char *path, struct exec_vmcmd_set *vcset,
		      u_long *entry, struct pecoff_args *argp);
void pecoff_load_section (struct exec_vmcmd_set *vcset, struct vnode *vp,
			  struct coff_scnhdr *sh, long *addr,
			  u_long *size, int *prot);
int exec_pecoff_makecmds (struct lwp *l, struct exec_package *epp);
int exec_pecoff_coff_makecmds (struct lwp *l, struct exec_package *epp,
			       struct coff_filehdr *fp, int peofs);
int exec_pecoff_prep_omagic (struct proc *p, struct exec_package *epp,
			     struct coff_filehdr *fp,
			     struct coff_aouthdr *ap, int peofs);
int exec_pecoff_prep_nmagic (struct proc *p, struct exec_package *epp,
			     struct coff_filehdr *fp,
			     struct coff_aouthdr *ap, int peofs);
int exec_pecoff_prep_zmagic (struct lwp *l, struct exec_package *epp,
			     struct coff_filehdr *fp,
			     struct coff_aouthdr *ap, int peofs);


int
pecoff_copyargs(l, pack, arginfo, stackp, argp)
	struct lwp *l;
	struct exec_package *pack;
	struct ps_strings *arginfo;
	char **stackp;
	void *argp;
{
	int len = sizeof(struct pecoff_args);
	struct pecoff_args *ap;
	int error;

	if ((error = copyargs(l, pack, arginfo, stackp, argp)) != 0)
		return error;

	ap = (struct pecoff_args *)pack->ep_emul_arg;
	if ((error = copyout(ap, *stackp, len)) != 0)
		return error;

#if 0 /*  kern_exec.c? */
	free(ap, M_TEMP);
	pack->ep_emul_arg = 0;
#endif

	*stackp += len;
	return error;
}

#define PECOFF_SIGNATURE "PE\0\0"
static const char signature[] = PECOFF_SIGNATURE;

/*
 * Check PE signature.
 */
int
pecoff_signature(l, vp, dp)
	struct lwp *l;
	struct vnode *vp;
	struct pecoff_dos_filehdr *dp;
{
	int error;
	char tbuf[sizeof(signature) - 1];

	if (DOS_BADMAG(dp)) {
		return ENOEXEC;
	}
	error = exec_read_from(l, vp, dp->d_peofs, tbuf, sizeof(tbuf));
	if (error) {
		return error;
	}
	if (memcmp(tbuf, signature, sizeof(signature) - 1) == 0) {
		return 0;
	}
	return EFTYPE;
}

/*
 * load(mmap) file.  for dynamic linker (ld.so.dll)
 */
int
pecoff_load_file(l, epp, path, vcset, entry, argp)
	struct lwp *l;
	struct exec_package *epp;
	const char *path;
	struct exec_vmcmd_set *vcset;
	u_long *entry;
	struct pecoff_args *argp;
{
	int error, peofs, scnsiz, i;
	struct nameidata nd;
	struct vnode *vp;
	struct vattr attr;
	struct pecoff_dos_filehdr dh;
	struct coff_filehdr *fp = 0;
	struct coff_aouthdr *ap;
	struct pecoff_opthdr *wp;
	struct coff_scnhdr *sh = 0;
	const char *bp;

	/*
	 * Following has ~same effect as emul_find_interp(), but the code
	 * needs to do some more checks while having the vnode open.
	 * emul_find_interp() wouldn't really simplify handling here.
	 */
	if ((error = emul_find(l, NULL, epp->ep_esch->es_emul->e_path,
			       path, &bp, 0))) {
		char *ptr;
		int len;

		len = strlen(path) + 1;
		if (len > MAXPATHLEN)
			return error;
		ptr = malloc(len, M_TEMP, M_WAITOK);
		copystr(path, ptr, len, 0);
		bp = ptr;
	}

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_SYSSPACE, bp, l);
	if ((error = namei(&nd)) != 0) {
		/*XXXUNCONST*/
		free(__UNCONST(bp), M_TEMP);
		return error;
	}
	vp = nd.ni_vp;

	/*
	 * Similarly, if it's not marked as executable, or it's not a regular
	 * file, we don't allow it to be used.
	 */
	if (vp->v_type != VREG) {
		error = EACCES;
		goto badunlock;
	}
	if ((error = VOP_ACCESS(vp, VEXEC, l->l_proc->p_cred, l)) != 0)
		goto badunlock;

	/* get attributes */
	if ((error = VOP_GETATTR(vp, &attr, l->l_proc->p_cred, l)) != 0)
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

	if ((error = vn_marktext(vp)))
		goto badunlock;

	VOP_UNLOCK(vp, 0);
	/*
	 * Read header.
	 */
	error = exec_read_from(l, vp, 0, &dh, sizeof(dh));
	if (error != 0)
		goto bad;
	if ((error = pecoff_signature(l, vp, &dh)) != 0)
		goto bad;
	fp = malloc(PECOFF_HDR_SIZE, M_TEMP, M_WAITOK);
	peofs = dh.d_peofs + sizeof(signature) - 1;
	error = exec_read_from(l, vp, peofs, fp, PECOFF_HDR_SIZE);
	if (error != 0)
		goto bad;
	if (COFF_BADMAG(fp)) {
		error = ENOEXEC;
		goto bad;
	}
	ap = (void *)((char *)fp + sizeof(struct coff_filehdr));
	wp = (void *)((char *)ap + sizeof(struct coff_aouthdr));
	/* read section header */
	scnsiz = sizeof(struct coff_scnhdr) * fp->f_nscns;
	sh = malloc(scnsiz, M_TEMP, M_WAITOK);
	if ((error = exec_read_from(l, vp, peofs + PECOFF_HDR_SIZE, sh,
	    scnsiz)) != 0)
		goto bad;

	/*
	 * Read section header, and mmap.
	 */
	for (i = 0; i < fp->f_nscns; i++) {
		int prot = 0;
		long addr;
		u_long size;

		if (sh[i].s_flags & COFF_STYP_DISCARD)
			continue;
		/* XXX ? */
		if ((sh[i].s_flags & COFF_STYP_TEXT) &&
		    (sh[i].s_flags & COFF_STYP_EXEC) == 0)
			continue;
		if ((sh[i].s_flags & (COFF_STYP_TEXT|COFF_STYP_DATA|
				      COFF_STYP_BSS|COFF_STYP_READ)) == 0)
			continue;
		sh[i].s_vaddr += wp->w_base; /* RVA --> VA */
		pecoff_load_section(vcset, vp, &sh[i], &addr, &size, &prot);
	}
	*entry = wp->w_base + ap->a_entry;
	argp->a_ldbase = wp->w_base;
	argp->a_ldexport = wp->w_imghdr[0].i_vaddr + wp->w_base;

	free(fp, M_TEMP);
	free(sh, M_TEMP);
	/*XXXUNCONST*/
	free(__UNCONST(bp), M_TEMP);
	vrele(vp);
	return 0;

badunlock:
	VOP_UNLOCK(vp, 0);

bad:
	if (fp != 0)
		free(fp, M_TEMP);
	if (sh != 0)
		free(sh, M_TEMP);
	/*XXXUNCONST*/
	free(__UNCONST(bp), M_TEMP);
	vrele(vp);
	return error;
}

/*
 * mmap one section.
 */
void
pecoff_load_section(vcset, vp, sh, addr, size, prot)
	struct exec_vmcmd_set *vcset;
	struct vnode *vp;
	struct coff_scnhdr *sh;
	long *addr;
	u_long *size;
	int *prot;
{
	u_long diff, offset;

	*addr = COFF_ALIGN(sh->s_vaddr);
	diff = (sh->s_vaddr - *addr);
	offset = sh->s_scnptr - diff;
	*size = COFF_ROUND(sh->s_size + diff, COFF_LDPGSZ);

	*prot |= (sh->s_flags & COFF_STYP_EXEC) ? VM_PROT_EXECUTE : 0;
	*prot |= (sh->s_flags & COFF_STYP_READ) ? VM_PROT_READ : 0;
	*prot |= (sh->s_flags & COFF_STYP_WRITE) ? VM_PROT_WRITE : 0;

	if (diff == 0 && offset == COFF_ALIGN(offset))
		NEW_VMCMD(vcset, vmcmd_map_pagedvn, *size, *addr, vp,
 			  offset, *prot);
	else
		NEW_VMCMD(vcset, vmcmd_map_readvn, sh->s_size, sh->s_vaddr, vp,
			  sh->s_scnptr, *prot);

	if (*size < sh->s_paddr) {
		u_long baddr, bsize;

		baddr = *addr + COFF_ROUND(*size, COFF_LDPGSZ);
		bsize = sh->s_paddr - COFF_ROUND(*size, COFF_LDPGSZ);
		DPRINTF(("additional zero space (addr %lx size %ld)\n",
			 baddr, bsize));
		NEW_VMCMD(vcset, vmcmd_map_zero, bsize, baddr,
			  NULLVP, 0, *prot);
		*size = COFF_ROUND(sh->s_paddr, COFF_LDPGSZ);
	}
	DPRINTF(("section %s loaded. (addr %lx size %ld prot %d)\n",
		 sh->s_name, sh->s_vaddr, sh->s_size, *prot));
}

/*
 */
int
exec_pecoff_makecmds(l, epp)
	struct lwp *l;
	struct exec_package *epp;
{
	int error, peofs;
	struct pecoff_dos_filehdr *dp = epp->ep_hdr;
	struct coff_filehdr *fp;
	struct proc *p;

	p = l->l_proc;
	/*
	 * mmap EXE file (PE format)
	 * 1. read header (DOS,PE)
	 * 2. mmap code section (READ|EXEC)
	 * 3. mmap other section, such as data (READ|WRITE|EXEC)
	 */
	if (epp->ep_hdrvalid < PECOFF_DOS_HDR_SIZE) {
		return ENOEXEC;
	}
	if ((error = pecoff_signature(l, epp->ep_vp, dp)) != 0)
		return error;

	if ((error = vn_marktext(epp->ep_vp)) != 0)
		return error;

	peofs = dp->d_peofs + sizeof(signature) - 1;
	fp = malloc(PECOFF_HDR_SIZE, M_TEMP, M_WAITOK);
	error = exec_read_from(l, epp->ep_vp, peofs, fp, PECOFF_HDR_SIZE);
	if (error) {
		free(fp, M_TEMP);
		return error;
	}
	error = exec_pecoff_coff_makecmds(l, epp, fp, peofs);

	if (error != 0)
		kill_vmcmds(&epp->ep_vmcmds);

	free(fp, M_TEMP);
	return error;
}

/*
 */
int
exec_pecoff_coff_makecmds(l, epp, fp, peofs)
	struct lwp *l;
	struct exec_package *epp;
	struct coff_filehdr *fp;
	int peofs;
{
	struct coff_aouthdr *ap;
	struct proc *p;
	int error;

	if (COFF_BADMAG(fp)) {
		return ENOEXEC;
	}
	p = l->l_proc;
	ap = (void *)((char *)fp + sizeof(struct coff_filehdr));
	switch (ap->a_magic) {
	case COFF_OMAGIC:
		error = exec_pecoff_prep_omagic(p, epp, fp, ap, peofs);
		break;
	case COFF_NMAGIC:
		error = exec_pecoff_prep_nmagic(p, epp, fp, ap, peofs);
		break;
	case COFF_ZMAGIC:
		error = exec_pecoff_prep_zmagic(l, epp, fp, ap, peofs);
		break;
	default:
		return ENOEXEC;
	}

	return error;
}

/*
 */
int
exec_pecoff_prep_omagic(p, epp, fp, ap, peofs)
	struct proc *p;
	struct exec_package *epp;
	struct coff_filehdr *fp;
	struct coff_aouthdr *ap;
	int peofs;
{
	return ENOEXEC;
}

/*
 */
int
exec_pecoff_prep_nmagic(p, epp, fp, ap, peofs)
	struct proc *p;
	struct exec_package *epp;
	struct coff_filehdr *fp;
	struct coff_aouthdr *ap;
	int peofs;
{
	return ENOEXEC;
}

/*
 */
int
exec_pecoff_prep_zmagic(l, epp, fp, ap, peofs)
	struct lwp *l;
	struct exec_package *epp;
	struct coff_filehdr *fp;
	struct coff_aouthdr *ap;
	int peofs;
{
	int error, i;
	struct pecoff_opthdr *wp;
	long daddr, baddr, bsize;
	u_long tsize, dsize;
	struct coff_scnhdr *sh;
	struct pecoff_args *argp;
	int scnsiz = sizeof(struct coff_scnhdr) * fp->f_nscns;

	wp = (void *)((char *)ap + sizeof(struct coff_aouthdr));

	epp->ep_tsize = ap->a_tsize;
	epp->ep_daddr = VM_MAXUSER_ADDRESS;
	epp->ep_dsize = 0;
	/* read section header */
	sh = malloc(scnsiz, M_TEMP, M_WAITOK);
	error = exec_read_from(l, epp->ep_vp, peofs + PECOFF_HDR_SIZE, sh,
	    scnsiz);
	if (error) {
		free(sh, M_TEMP);
		return error;
	}
	/*
	 * map section
	 */
	for (i = 0; i < fp->f_nscns; i++) {
		int prot = /*0*/VM_PROT_WRITE;
		long s_flags = sh[i].s_flags;

		if ((s_flags & COFF_STYP_DISCARD) != 0)
			continue;
		sh[i].s_vaddr += wp->w_base; /* RVA --> VA */

		if ((s_flags & COFF_STYP_TEXT) != 0) {
			/* set up command for text segment */
/*			DPRINTF(("COFF text addr %lx size %ld offset %ld\n",
				 sh[i].s_vaddr, sh[i].s_size, sh[i].s_scnptr));
*/			pecoff_load_section(&epp->ep_vmcmds, epp->ep_vp,
					   &sh[i], (long *)&epp->ep_taddr,
					   &tsize, &prot);
		} else if ((s_flags & COFF_STYP_BSS) != 0) {
			/* set up command for bss segment */
			baddr = sh[i].s_vaddr;
			bsize = sh[i].s_paddr;
			if (bsize)
				NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero,
				    bsize, baddr, NULLVP, 0,
				    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
			epp->ep_daddr = min(epp->ep_daddr, baddr);
			bsize = baddr + bsize - epp->ep_daddr;
			epp->ep_dsize = max(epp->ep_dsize, bsize);
		} else if ((s_flags & (COFF_STYP_DATA|COFF_STYP_READ)) != 0) {
			/* set up command for data segment */
/*			DPRINTF(("COFF data addr %lx size %ld offset %ld\n",
			sh[i].s_vaddr, sh[i].s_size, sh[i].s_scnptr));*/
			pecoff_load_section(&epp->ep_vmcmds, epp->ep_vp,
					   &sh[i], &daddr, &dsize, &prot);
			epp->ep_daddr = min(epp->ep_daddr, daddr);
			dsize = daddr + dsize - epp->ep_daddr;
			epp->ep_dsize = max(epp->ep_dsize, dsize);
		}
	}
	/* set up ep_emul_arg */
	argp = malloc(sizeof(struct pecoff_args), M_TEMP, M_WAITOK);
	epp->ep_emul_arg = argp;
	argp->a_abiversion = NETBSDPE_ABI_VERSION;
	argp->a_zero = 0;
	argp->a_entry = wp->w_base + ap->a_entry;
	argp->a_end = epp->ep_daddr + epp->ep_dsize;
	argp->a_opthdr = *wp;

	/*
	 * load dynamic linker
	 */
	error = pecoff_load_file(l, epp, "/usr/libexec/ld.so.dll",
				&epp->ep_vmcmds, &epp->ep_entry, argp);
	if (error) {
		free(sh, M_TEMP);
		return error;
	}

#if 0
	DPRINTF(("text addr: %lx size: %ld data addr: %lx size: %ld entry: %lx\n",
		 epp->ep_taddr, epp->ep_tsize,
		 epp->ep_daddr, epp->ep_dsize,
		 epp->ep_entry));
#endif

	free(sh, M_TEMP);
	return (*epp->ep_esch->es_setup_stack)(l, epp);
}
