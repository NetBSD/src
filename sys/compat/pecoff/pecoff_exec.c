/*	$NetBSD: pecoff_exec.c,v 1.8.2.2 2001/08/24 00:08:56 nathanw Exp $	*/

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

int pecoff_signature __P((struct proc *p, struct vnode *vp,
			    struct pecoff_dos_filehdr *dp));
int pecoff_load_file __P((struct proc *p, struct exec_package *epp,
			 char *path, struct exec_vmcmd_set *vcset,
			 u_long *entry, struct pecoff_args *argp));
void pecoff_load_section __P((struct exec_vmcmd_set *vcset, struct vnode *vp,
			     struct coff_scnhdr *sh, long *addr,
			     u_long *size, int *prot));
int exec_pecoff_makecmds __P((struct proc *p, struct exec_package *epp));
int exec_pecoff_coff_makecmds __P((struct proc *p, struct exec_package *epp,
				  struct coff_filehdr *fp, int peofs));
int exec_pecoff_setup_stack __P((struct proc *p, struct exec_package *epp));
int exec_pecoff_prep_omagic __P((struct proc *p, struct exec_package *epp,
				     struct coff_filehdr *fp,
				     struct coff_aouthdr *ap, int peofs));
int exec_pecoff_prep_nmagic __P((struct proc *p, struct exec_package *epp,
				     struct coff_filehdr *fp,
				     struct coff_aouthdr *ap, int peofs));
int exec_pecoff_prep_zmagic __P((struct proc *p, struct exec_package *epp,
				     struct coff_filehdr *fp,
				     struct coff_aouthdr *ap, int peofs));


extern char sigcode[], esigcode[];
#ifdef __HAVE_SYSCALL_INTERN
void syscall_intern __P((void));
#else
void syscall __P((void));
#endif

#if notyet
const struct emul emul_pecoff = {
	"pecoff",
	"/emul/pecoff",
#ifndef __HAVE_MINIMAL_EMUL
	0,
	0,
	SYS_syscall,
	SYS_MAXSYSCALL,
#endif
	sysent,
#ifdef SYSCALL_DEBUG
	syscallnames,
#else
	0,
#endif
	sendsig,
	trapsignal,
	sigcode,
	esigcode,
	NULL,
	NULL,
	NULL,
#ifdef __HAVE_SYSCALL_INTERN
	syscall_intern,
#else
	syscall,
#endif
};
#endif

int
pecoff_copyargs(pack, arginfo, stackp, argp)
	struct exec_package *pack;
	struct ps_strings *arginfo;
	char **stackp;
	void *argp;
{
	int len = sizeof(struct pecoff_args);
	struct pecoff_imghdr *ap;
	int error;

	if ((error = copyargs(pack, arginfo, stackp, argp)) != 0)
		return error;

	ap = (struct pecoff_imghdr *)pack->ep_emul_arg;
	if ((error = copyout(ap, *stackp, len)) != 0)
		return error;

#if 0 /*  kern_exec.c? */
	free((char *)ap, M_TEMP);
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
pecoff_signature(p, vp, dp)
	struct proc *p;
	struct vnode *vp;
	struct pecoff_dos_filehdr *dp;
{
	int error;
	char buf[sizeof(signature) - 1];

	if (DOS_BADMAG(dp)) {
		return ENOEXEC;
	}
	error = exec_read_from(p, vp, dp->d_peofs, buf, sizeof(buf));
	if (error) {
		return error;
	}
	if (memcmp(buf, signature, sizeof(signature) - 1) == 0) {
		return 0;
	}
	return EFTYPE;
}

/*
 * load(mmap) file.  for dynamic linker (ld.so.dll)
 */
int
pecoff_load_file(p, epp, path, vcset, entry, argp)
	struct proc *p;
	struct exec_package *epp;
	char *path;
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

	VOP_UNLOCK(vp, 0);
	/*
	 * Read header.
	 */
	error = exec_read_from(p, vp, 0, &dh, sizeof(dh));
	if (error != 0)
		goto bad;
	if ((error = pecoff_signature(p, vp, &dh)) != 0)
		goto bad;
	fp = malloc(PECOFF_HDR_SIZE, M_TEMP, M_WAITOK);
	peofs = dh.d_peofs + sizeof(signature) - 1;
	if ((error = exec_read_from(p, vp, peofs, fp, PECOFF_HDR_SIZE)) != 0)
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
	if ((error = exec_read_from(p, vp, peofs + PECOFF_HDR_SIZE, sh,
	    scnsiz)) != 0)
		goto bad;

	/*
	 * Read section header, and mmap.
	 */
	for (i = 0; i < fp->f_nscns; i++) {
		int prot = 0;
		long addr, size;

		if (sh[i].s_flags & COFF_STYP_DISCARD)
			continue;
		/* XXX ? */
		if ((sh[i].s_flags & COFF_STYP_TEXT) &&
		    (sh[i].s_flags & COFF_STYP_EXEC) == 0)
			continue;
		if ((sh[i].s_flags & (COFF_STYP_TEXT|
				      COFF_STYP_DATA|COFF_STYP_BSS)) == 0)
			continue;
		sh[i].s_vaddr += wp->w_base; /* RVA --> VA */
		pecoff_load_section(vcset, vp, &sh[i], &addr, &size, &prot);
	}
	*entry = wp->w_base + ap->a_entry;
	argp->a_ldbase = wp->w_base;
	argp->a_ldexport = wp->w_imghdr[0].i_vaddr + wp->w_base;

	free((char *)fp, M_TEMP);
	free((char *)sh, M_TEMP);
	vrele(vp);
	return 0;

badunlock:
	VOP_UNLOCK(vp, 0);

bad:
	if (fp != 0)
		free((char *)fp, M_TEMP);
	if (sh != 0)
		free((char *)sh, M_TEMP);
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
exec_pecoff_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	int error, peofs;
	struct pecoff_dos_filehdr *dp = epp->ep_hdr;
	struct coff_filehdr *fp;

	/*
	 * mmap EXE file (PE format)
	 * 1. read header (DOS,PE)
	 * 2. mmap code section (READ|EXEC)
	 * 3. mmap other section, such as data (READ|WRITE|EXEC)
	 */
	if (epp->ep_hdrvalid < PECOFF_DOS_HDR_SIZE) {
		return ENOEXEC;
	}
	if ((error = pecoff_signature(p, epp->ep_vp, dp)) != 0)
		return error;
	peofs = dp->d_peofs + sizeof(signature) - 1;
	fp = malloc(PECOFF_HDR_SIZE, M_TEMP, M_WAITOK);
	error = exec_read_from(p, epp->ep_vp, peofs, fp, PECOFF_HDR_SIZE);
	if (error) {
		free(fp, M_TEMP);
		return error;
	}
	error = exec_pecoff_coff_makecmds(p, epp, fp, peofs);

	if (error != 0)
		kill_vmcmds(&epp->ep_vmcmds);

	free(fp, M_TEMP);
	return error;
}

/*
 */
int
exec_pecoff_coff_makecmds(p, epp, fp, peofs)
	struct proc *p;
	struct exec_package *epp;
	struct coff_filehdr *fp;
	int peofs;
{
	struct coff_aouthdr *ap;
	int error;

	if (COFF_BADMAG(fp)) {
		return ENOEXEC;
	}
	ap = (void *)((char *)fp + sizeof(struct coff_filehdr));
	switch (ap->a_magic) {
	case COFF_OMAGIC:
		error = exec_pecoff_prep_omagic(p, epp, fp, ap, peofs);
		break;
	case COFF_NMAGIC:
		error = exec_pecoff_prep_nmagic(p, epp, fp, ap, peofs);
		break;
	case COFF_ZMAGIC:
		error = exec_pecoff_prep_zmagic(p, epp, fp, ap, peofs);
		break;
	default:
		return ENOEXEC;
	}
	
	return error;
}

int
exec_pecoff_setup_stack(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	epp->ep_maxsaddr = USRSTACK - MAXSSIZ;
	epp->ep_minsaddr = USRSTACK;
	epp->ep_ssize = p->p_rlimit[RLIMIT_STACK].rlim_cur;
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero,
		  ((epp->ep_minsaddr - epp->ep_ssize) - epp->ep_maxsaddr),
		  epp->ep_maxsaddr, NULLVP, 0, VM_PROT_NONE);
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, epp->ep_ssize,
		  (epp->ep_minsaddr - epp->ep_ssize), NULLVP, 0,
		  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	return 0;
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
exec_pecoff_prep_zmagic(p, epp, fp, ap, peofs)
	struct proc *p;
	struct exec_package *epp;
	struct coff_filehdr *fp;
	struct coff_aouthdr *ap;
	int peofs;
{
	int error, i;
	struct pecoff_opthdr *wp;
	long daddr, dsize, baddr, bsize;
	struct coff_scnhdr *sh;
	struct pecoff_args *argp;
	int scnsiz = sizeof(struct coff_scnhdr) * fp->f_nscns;

	wp = (void *)((char *)ap + sizeof(struct coff_aouthdr));

	epp->ep_daddr = VM_MAXUSER_ADDRESS;
	epp->ep_dsize = 0;
	/* read section header */
	sh = malloc(scnsiz, M_TEMP, M_WAITOK);
	error = exec_read_from(p, epp->ep_vp, peofs + PECOFF_HDR_SIZE, sh,
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

		if (sh[i].s_flags & COFF_STYP_DISCARD)
			continue;
		sh[i].s_vaddr += wp->w_base; /* RVA --> VA */

		if ((sh[i].s_flags & COFF_STYP_TEXT) != 0) {
			/* set up command for text segment */
/*			DPRINTF(("COFF text addr %lx size %ld offset %ld\n",
				 sh[i].s_vaddr, sh[i].s_size, sh[i].s_scnptr));
*/			pecoff_load_section(&epp->ep_vmcmds, epp->ep_vp,
					   &sh[i], &epp->ep_taddr,
					   &epp->ep_tsize, &prot);
		}
		if ((sh[i].s_flags & COFF_STYP_DATA) != 0) {
			/* set up command for data segment */
/*			DPRINTF(("COFF data addr %lx size %ld offset %ld\n",
			sh[i].s_vaddr, sh[i].s_size, sh[i].s_scnptr));*/
			pecoff_load_section(&epp->ep_vmcmds, epp->ep_vp,
					   &sh[i], &daddr, &dsize, &prot);
			epp->ep_daddr = min(epp->ep_daddr, daddr);
			dsize = daddr + dsize - epp->ep_daddr;
			epp->ep_dsize = max(epp->ep_dsize, dsize);
		}
		if ((sh[i].s_flags & COFF_STYP_BSS) != 0) {
			/* set up command for bss segment */
			baddr = sh[i].s_vaddr;
			bsize = sh[i].s_paddr;
			NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero,
				  bsize, baddr, NULLVP, 0,
				  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
			epp->ep_daddr = min(epp->ep_daddr, baddr);
			bsize = baddr + bsize - epp->ep_daddr;
			epp->ep_dsize = max(epp->ep_dsize, bsize);
		}
	}
	/* set up ep_emul_arg */
	argp = malloc(sizeof(struct pecoff_args), M_TEMP, M_WAITOK);
	epp->ep_emul_arg = argp;
	argp->a_base = wp->w_base;
	argp->a_entry = wp->w_base + ap->a_entry;
	argp->a_end = epp->ep_daddr + epp->ep_dsize;
	argp->a_subsystem = wp->w_subvers;
	memcpy(argp->a_imghdr, wp->w_imghdr, sizeof(wp->w_imghdr));
	/* imghdr RVA --> VA */
	for (i = 0; i < 16; i++) {
		argp->a_imghdr[i].i_vaddr += wp->w_base;
	}

	/*
	 * load dynamic linker
	 */
	error = pecoff_load_file(p, epp, "/usr/libexec/ld.so.dll",
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
	return exec_pecoff_setup_stack(p, epp);
}
