/* 	$NetBSD: compat_util.c,v 1.30.12.1 2006/05/24 15:48:26 tron Exp $	*/

/*-
 * Copyright (c) 1994 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas and Frank van der Linden.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: compat_util.c,v 1.30.12.1 2006/05/24 15:48:26 tron Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/exec.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/syslog.h>
#include <sys/mount.h>


#include <compat/common/compat_util.h>

/*
 * Search an alternate path before passing pathname arguments on
 * to system calls. Useful for keeping a separate 'emulation tree'.
 *
 * According to sflag, we either check for existance of the file or if
 * it can be created or if the file is symlink.
 *
 * In case of success, emul_find returns 0:
 * 	If sgp is provided, the path is in user space, and pbuf gets
 *	allocated in user space (in the stackgap). Otherwise the path
 *	is already in kernel space and a kernel buffer gets allocated
 *	and returned in pbuf, that must be freed by the user.
 * In case of error, the error number is returned and *pbuf = path.
 */
int
emul_find(l, sgp, prefix, path, pbuf, sflag)
	struct lwp *l;
	caddr_t		 *sgp;		/* Pointer to stackgap memory */
	const char	 *prefix;
	const char	 *path;
	const char	**pbuf;
	int		  sflag;
{
	struct proc *p;
	struct nameidata	 nd;
	struct nameidata	 ndroot;
	struct vattr		 vat;
	struct vattr		 vatroot;
	int			 error;
	char			*ptr, *tbuf, *cp;
	const char		*pr;
	size_t			 sz, len;

	p = l->l_proc;
	tbuf = malloc(MAXPATHLEN, M_TEMP, M_WAITOK);
	*pbuf = path;

	for (ptr = tbuf, pr = prefix; (*ptr = *pr) != '\0'; ptr++, pr++)
		continue;

	sz = MAXPATHLEN - (ptr - tbuf);

	/*
	 * If sgp is not given then the path is already in kernel space
	 */
	if (sgp == NULL)
		error = copystr(path, ptr, sz, &len);
	else
		error = copyinstr(path, ptr, sz, &len);

	if (error)
		goto bad;

	if (*ptr != '/') {
		error = EINVAL;
		goto bad;
	}

	/*
	 * We provide an escape method, so that the user can
	 * always specify the real root. If the path is prefixed
	 * by /../ we kill the alternate search
	 */
	if (ptr[1] == '.' && ptr[2] == '.' && ptr[3] == '/') {
		len -= 3;
		(void)memcpy(tbuf, &ptr[3], len);
		ptr = tbuf;
		goto good;
	}

	/*
	 * We know that there is a / somewhere in this pathname.
	 * Search backwards for it, to find the file's parent dir
	 * to see if it exists in the alternate tree. If it does,
	 * and we want to create a file (sflag is set). We don't
	 * need to worry about the root comparison in this case.
	 */

	switch (sflag) {
	case CHECK_ALT_FL_CREAT:
		for (cp = &ptr[len] - 1; *cp != '/'; cp--)
			;
		*cp = '\0';

		NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, tbuf, l);

		if ((error = namei(&nd)) != 0)
			goto bad;

		*cp = '/';
		break;
	case CHECK_ALT_FL_EXISTS:
	case CHECK_ALT_FL_SYMLINK:
		NDINIT(&nd, LOOKUP,
			(sflag == CHECK_ALT_FL_SYMLINK) ? NOFOLLOW : FOLLOW,
			UIO_SYSSPACE, tbuf, l);

		if ((error = namei(&nd)) != 0)
			goto bad;

		/*
		 * We now compare the vnode of the emulation root to the one
		 * vnode asked. If they resolve to be the same, then we
		 * ignore the match so that the real root gets used.
		 * This avoids the problem of traversing "../.." to find the
		 * root directory and never finding it, because "/" resolves
		 * to the emulation root directory. This is expensive :-(
		 */
		NDINIT(&ndroot, LOOKUP, FOLLOW, UIO_SYSSPACE, prefix, l);

		if ((error = namei(&ndroot)) != 0)
			goto bad2;

		if ((error = VOP_GETATTR(nd.ni_vp, &vat, p->p_cred, l)) != 0)
			goto bad3;

		if ((error = VOP_GETATTR(ndroot.ni_vp, &vatroot, p->p_cred, l))
		    != 0)
			goto bad3;

		if (vat.va_fsid == vatroot.va_fsid &&
		    vat.va_fileid == vatroot.va_fileid) {
			error = ENOENT;
			goto bad3;
		}

		break;
	}

	vrele(nd.ni_vp);
	if (sflag == CHECK_ALT_FL_EXISTS)
		vrele(ndroot.ni_vp);

good:
	if (sgp == NULL)
		*pbuf = tbuf;
	else {
		sz = &ptr[len] - tbuf;
		*pbuf = stackgap_alloc(p, sgp, sz + 1);
		if (*pbuf == NULL) {
			error = ENAMETOOLONG;
			goto bad;
		}
		/*XXXUNCONST*/
		if ((error = copyout(tbuf, __UNCONST(*pbuf), sz)) != 0) {
			*pbuf = path;
			goto bad;
		}
		free(tbuf, M_TEMP);
	}
	return 0;

bad3:
	vrele(ndroot.ni_vp);
bad2:
	vrele(nd.ni_vp);
bad:
	free(tbuf, M_TEMP);
	return error;
}

/*
 * Search the alternate path for dynamic binary interpreter. If not found
 * there, check if the interpreter exists in within 'proper' tree.
 */
int
emul_find_interp(struct lwp *l, const char *prefix, char *itp)
{
	const char *bp;
	int error;

	if (emul_find(l, NULL, prefix, itp, &bp, CHECK_ALT_FL_EXISTS) == 0) {
		size_t len;

		if ((error = copystr(bp, itp, MAXPATHLEN, &len)))
			return error;
		/*XXXUNCONST*/
		free(__UNCONST(bp), M_TEMP);
	} else {
		/* check filename without the emul prefix */
		struct nameidata nd;

		NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, itp, l);

		if ((error = namei(&nd)))
			return error;

		vrele(nd.ni_vp);
	}

	return (0);
}

/*
 * Translate one set of flags to another, based on the entries in
 * the given table.  If 'leftover' is specified, it is filled in
 * with any flags which could not be translated.
 */
unsigned long
emul_flags_translate(const struct emul_flags_xtab *tab,
		     unsigned long in, unsigned long *leftover)
{
	unsigned long out;

	for (out = 0; tab->omask != 0; tab++) {
		if ((in & tab->omask) == tab->oval) {
			in &= ~tab->omask;
			out |= tab->nval;
		}
	}
	if (leftover != NULL)
		*leftover = in;
	return (out);
}

caddr_t
stackgap_init(p, sz)
	const struct proc *p;
	size_t sz;
{
	if (sz == 0)
		sz = STACKGAPLEN;
	if (sz > STACKGAPLEN)
		panic("size %lu > STACKGAPLEN", (unsigned long)sz);
#define szsigcode ((caddr_t)(p->p_emul->e_esigcode - p->p_emul->e_sigcode))
	return (caddr_t)(((unsigned long)p->p_psstr - (unsigned long)szsigcode
		- sz) & ~ALIGNBYTES);
#undef szsigcode
}


void *
stackgap_alloc(p, sgp, sz)
	const struct proc *p;
	caddr_t *sgp;
	size_t sz;
{
	void *n = (void *) *sgp;
	caddr_t nsgp;
	const struct emul *e = p->p_emul;
	int sigsize = e->e_esigcode - e->e_sigcode;

	sz = ALIGN(sz);
	nsgp = *sgp + sz;
	if (nsgp > (((caddr_t)p->p_psstr) - sigsize))
		return NULL;
	*sgp = nsgp;
	return n;
}

void
compat_offseterr(vp, msg)
	struct vnode *vp;
	const char *msg;
{
	struct mount *mp;

	mp = vp->v_mount;

	log(LOG_ERR, "%s: dir offset too large on fs %s (mounted from %s)\n",
	    msg, mp->mnt_stat.f_mntonname, mp->mnt_stat.f_mntfromname);
	uprintf("%s: dir offset too large for emulated program\n", msg);
}
