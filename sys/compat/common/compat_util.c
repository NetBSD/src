/* 	$NetBSD: compat_util.c,v 1.15 2000/06/26 14:38:54 mrg Exp $	*/

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
 * If cflag is set, we check if an attempt can be made to create
 * the named file, i.e. we check if the directory it should
 * be in exists.
 *
 * In case of success, emul_find returns 0:
 * 	If sgp is provided, the path is in user space, and pbuf gets
 *	allocated in user space (in the stackgap). Otherwise the path
 *	is already in kernel space and a kernel buffer gets allocated
 *	and returned in pbuf, that must be freed by the user.
 * In case of error, the error number is returned and *pbuf = path.
 */
int
emul_find(p, sgp, prefix, path, pbuf, cflag)
	struct proc	 *p;
	caddr_t		 *sgp;		/* Pointer to stackgap memory */
	const char	 *prefix;
	const char	 *path;
	const char	**pbuf;
	int		  cflag;
{
	struct nameidata	 nd;
	struct nameidata	 ndroot;
	struct vattr		 vat;
	struct vattr		 vatroot;
	int			 error;
	char			*ptr, *buf, *cp;
	const char		*pr;
	size_t			 sz, len;

	buf = (char *)malloc(MAXPATHLEN, M_TEMP, M_WAITOK);
	*pbuf = path;

	for (ptr = buf, pr = prefix; (*ptr = *pr) != '\0'; ptr++, pr++)
		continue;

	sz = MAXPATHLEN - (ptr - buf);

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
		(void)memcpy(buf, &ptr[3], len);
		ptr = buf;
		goto good;
	}

	/*
	 * We know that there is a / somewhere in this pathname.
	 * Search backwards for it, to find the file's parent dir
	 * to see if it exists in the alternate tree. If it does,
	 * and we want to create a file (cflag is set). We don't
	 * need to worry about the root comparison in this case.
	 */

	if (cflag) {
		for (cp = &ptr[len] - 1; *cp != '/'; cp--)
			;
		*cp = '\0';

		NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, buf, p);

		if ((error = namei(&nd)) != 0)
			goto bad;

		*cp = '/';
	}
	else {
		NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, buf, p);

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
		NDINIT(&ndroot, LOOKUP, FOLLOW, UIO_SYSSPACE, prefix, p);

		if ((error = namei(&ndroot)) != 0)
			goto bad2;

		if ((error = VOP_GETATTR(nd.ni_vp, &vat, p->p_ucred, p)) != 0)
			goto bad3;

		if ((error = VOP_GETATTR(ndroot.ni_vp, &vatroot, p->p_ucred, p))
		    != 0)
			goto bad3;

		if (vat.va_fsid == vatroot.va_fsid &&
		    vat.va_fileid == vatroot.va_fileid) {
			error = ENOENT;
			goto bad3;
		}
	}

	vrele(nd.ni_vp);
	if (!cflag)
		vrele(ndroot.ni_vp);

good:
	if (sgp == NULL)
		*pbuf = buf;
	else {
		sz = &ptr[len] - buf;
		*pbuf = stackgap_alloc(sgp, sz + 1);
		if ((error = copyout(buf, (void *)*pbuf, sz)) != 0) {
			*pbuf = path;
			return error;
		}
		free(buf, M_TEMP);
	}
	return 0;

bad3:
	vrele(ndroot.ni_vp);
bad2:
	vrele(nd.ni_vp);
bad:
	free(buf, M_TEMP);
	return error;
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
stackgap_init(e)
	struct emul *e;
{

#define szsigcode ((caddr_t)(e->e_esigcode - e->e_sigcode))
	return STACKGAPBASE;
#undef szsigcode
}


void *
stackgap_alloc(sgp, sz)
	caddr_t *sgp;
	size_t sz;
{
	void *p = (void *) *sgp;

	*sgp += ALIGN(sz);
	return p;
}

void
compat_offseterr(vp, msg)
	struct vnode *vp;
	char *msg;
{
	struct mount *mp;

	mp = vp->v_mount;

	log(LOG_ERR, "%s: dir offset too large on fs %s (mounted from %s)\n",
	    msg, mp->mnt_stat.f_mntonname, mp->mnt_stat.f_mntfromname);
	uprintf("%s: dir offset too large for emulated program\n", msg);
}
