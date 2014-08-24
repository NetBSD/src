/* $NetBSD: osf1_file.c,v 1.42 2014/08/24 12:48:58 maxv Exp $ */

/*
 * Copyright (c) 1999 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
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

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: osf1_file.c,v 1.42 2014/08/24 12:48:58 maxv Exp $");

#if defined(_KERNEL_OPT)
#include "opt_syscall_debug.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/reboot.h>
#include <sys/syscallargs.h>
#include <sys/exec.h>
#include <sys/vnode.h>
#include <sys/socketvar.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/wait.h>
#include <sys/vfs_syscalls.h>
#include <sys/dirent.h>

#include <compat/osf1/osf1.h>
#include <compat/osf1/osf1_syscallargs.h>
#include <compat/common/compat_util.h>
#include <compat/osf1/osf1_cvt.h>
#include <compat/osf1/osf1_dirent.h>

#ifdef SYSCALL_DEBUG
extern int scdebug;
#endif

int
osf1_sys_access(struct lwp *l, const struct osf1_sys_access_args *uap, register_t *retval)
{
	struct sys_access_args a;
	unsigned long leftovers;

	SCARG(&a, path) = SCARG(uap, path);

	/* translate flags */
	SCARG(&a, flags) = emul_flags_translate(osf1_access_flags_xtab,
	    SCARG(uap, flags), &leftovers);
	if (leftovers != 0)
		return (EINVAL);

	return sys_access(l, &a, retval);
}

int
osf1_sys_execve(struct lwp *l, const struct osf1_sys_execve_args *uap, register_t *retval)
{
	struct sys_execve_args ap;

	SCARG(&ap, path) = SCARG(uap, path);
	SCARG(&ap, argp) = SCARG(uap, argp);
	SCARG(&ap, envp) = SCARG(uap, envp);

	return sys_execve(l, &ap, retval);
}

int
osf1_sys_getdirentries(struct lwp *l, const struct osf1_sys_getdirentries_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(char *) buf;
		syscallarg(int) nbytes;
		syscallarg(long *) basep;
	} */
	struct dirent *bdp;
	struct vnode *vp;
	char *inp, *buf;        /* BSD-format */
	int len, reclen;        /* BSD-format */
	char *outp;             /* OSF1-format */
	int resid, osf1_reclen; /* OSF1-format */
	struct file *fp;
	struct uio auio;
	struct iovec aiov;
	struct osf1_dirent idb;
	off_t off, off1;        /* true file offset */
	int buflen, error, eofflag;
	off_t *cookiebuf = NULL, *cookie;
	int ncookies, fd;

	if (SCARG(uap, nbytes) < 0)
		return EINVAL;
	if (SCARG(uap, nbytes) == 0)
		return 0;

	fd = SCARG(uap, fd);
	if ((error = fd_getvnode(fd, &fp)) != 0)
		return (error);
	if ((fp->f_flag & FREAD) == 0) {
		error = EBADF;
		goto out1;
	}

	vp = (struct vnode *)fp->f_data;
	if (vp->v_type != VDIR) {
		error = EINVAL;
		goto out1;
	}

	buflen = min(MAXBSIZE, SCARG(uap, nbytes));
	buf = kmem_alloc(buflen, KM_SLEEP);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	off = off1 = fp->f_offset;
again:
	aiov.iov_base = buf;
	aiov.iov_len = buflen;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_READ;
	auio.uio_resid = buflen;
	auio.uio_offset = off;
	UIO_SETUP_SYSSPACE(&auio);
	/*
	 * First we read into the allocated buffer, then
	 * we massage it into user space, one record at a time.
	 */
	error = VOP_READDIR(vp, &auio, fp->f_cred, &eofflag, &cookiebuf,
	    &ncookies);
	if (error)
		goto out;

	inp = buf;
	outp = (char *)SCARG(uap, buf);
	resid = SCARG(uap, nbytes);
	if ((len = buflen - auio.uio_resid) == 0)
		goto eof;

	for (cookie = cookiebuf; len > 0; len -= reclen) {
		bdp = (struct dirent *)inp;
		reclen = bdp->d_reclen;
		if (reclen & 3)
			panic("osf1_sys_getdirentries: bad reclen");
		if (cookie)
			off = *cookie++; /* each entry points to the next */
		else
			off += reclen;
		if ((off >> 32) != 0) {
			compat_offseterr(vp, "osf1_sys_getdirentries");
			error = EINVAL;
			goto out;
		}
		if (bdp->d_fileno == 0) {
			inp += reclen;  /* it is a hole; squish it out */
			continue;
		}
		osf1_reclen = OSF1_RECLEN(&idb, bdp->d_namlen);
		if (reclen > len || resid < osf1_reclen) {
			/* entry too big for buffer, so just stop */
			outp++;
			break;
		}
		/*
		 * Massage in place to make a OSF1-shaped dirent (otherwise
		 * we have to worry about touching user memory outside of
		 * the copyout() call).
		 */
		idb.d_ino = (osf1_ino_t)bdp->d_fileno;
		idb.d_reclen = (u_short)osf1_reclen;
		idb.d_namlen = (u_short)bdp->d_namlen;
		strlcpy(idb.d_name, bdp->d_name, sizeof(idb.d_name));
		if ((error = copyout((void *)&idb, outp, osf1_reclen)))
			goto out;
		/* advance past this real entry */
		inp += reclen;
		/* advance output past OSF1-shaped entry */
		outp += osf1_reclen;
		resid -= osf1_reclen;
	}

	/* if we squished out the whole block, try again */
	if (outp == (char *)SCARG(uap, buf)) {
		if (cookiebuf)
			free(cookiebuf, M_TEMP);
		cookiebuf = NULL;
		goto again;
	}
	fp->f_offset = off;     /* update the vnode offset */

eof:
	*retval = SCARG(uap, nbytes) - resid;
out:
	VOP_UNLOCK(vp);
	if (cookiebuf)
		free(cookiebuf, M_TEMP);
	kmem_free(buf, buflen);
	if (SCARG(uap, basep) != NULL)
		error = copyout(&off1, SCARG(uap, basep), sizeof(long));
out1:
	fd_putfile(fd);
	return error;
}

/*
 * Get file status; this version does not follow links.
 */
/* ARGSUSED */
int
osf1_sys_lstat(struct lwp *l, const struct osf1_sys_lstat_args *uap, register_t *retval)
{
	struct stat sb;
	struct osf1_stat osb;
	int error;

	error = do_sys_stat(SCARG(uap, path), NOFOLLOW, &sb);
	if (error)
		return (error);
	osf1_cvt_stat_from_native(&sb, &osb);
	error = copyout(&osb, SCARG(uap, ub), sizeof (osb));
	return (error);
}

/*
 * Get file status; this version does not follow links.
 */
/* ARGSUSED */
int
osf1_sys_lstat2(struct lwp *l, const struct osf1_sys_lstat2_args *uap, register_t *retval)
{
	struct stat sb;
	struct osf1_stat2 osb;
	int error;

	error = do_sys_stat(SCARG(uap, path), NOFOLLOW, &sb);
	if (error)
		return (error);
	osf1_cvt_stat2_from_native(&sb, &osb);
	error = copyout((void *)&osb, (void *)SCARG(uap, ub), sizeof (osb));
	return (error);
}

int
osf1_sys_mknod(struct lwp *l, const struct osf1_sys_mknod_args *uap, register_t *retval)
{

	return do_sys_mknod(l, SCARG(uap, path), SCARG(uap, mode),
	    osf1_cvt_dev_to_native(SCARG(uap, dev)), retval, UIO_USERSPACE);
}

int
osf1_sys_open(struct lwp *l, const struct osf1_sys_open_args *uap, register_t *retval)
{
	struct sys_open_args a;
	const char *path;
	unsigned long leftovers;
#ifdef SYSCALL_DEBUG
	char pnbuf[1024];

	if (scdebug &&
	    copyinstr(SCARG(uap, path), pnbuf, sizeof pnbuf, NULL) == 0)
		printf("osf1_open: open: %s\n", pnbuf);
#endif

	/* translate flags */
	SCARG(&a, flags) = emul_flags_translate(osf1_open_flags_xtab,
	    SCARG(uap, flags), &leftovers);
	if (leftovers != 0)
		return (EINVAL);

	/* copy mode, no translation necessary */
	SCARG(&a, mode) = SCARG(uap, mode);

	/* pick appropriate path */
	path = SCARG(uap, path);
	SCARG(&a, path) = path;

	return sys_open(l, &a, retval);
}

int
osf1_sys_pathconf(struct lwp *l, const struct osf1_sys_pathconf_args *uap, register_t *retval)
{
	struct sys_pathconf_args a;
	int error;

	SCARG(&a, path) = SCARG(uap, path);

	error = osf1_cvt_pathconf_name_to_native(SCARG(uap, name),
	    &SCARG(&a, name));

	if (error == 0)
		error = sys_pathconf(l, &a, retval);

	return (error);
}

/*
 * Get file status; this version follows links.
 */
/* ARGSUSED */
int
osf1_sys_stat(struct lwp *l, const struct osf1_sys_stat_args *uap, register_t *retval)
{
	struct stat sb;
	struct osf1_stat osb;
	int error;

	error = do_sys_stat(SCARG(uap, path), FOLLOW, &sb);
	if (error)
		return (error);
	osf1_cvt_stat_from_native(&sb, &osb);
	error = copyout((void *)&osb, (void *)SCARG(uap, ub), sizeof (osb));
	return (error);
}

/*
 * Get file status; this version follows links.
 */
/* ARGSUSED */
int
osf1_sys_stat2(struct lwp *l, const struct osf1_sys_stat2_args *uap, register_t *retval)
{
	struct stat sb;
	struct osf1_stat2 osb;
	int error;

	error = do_sys_stat(SCARG(uap, path), FOLLOW, &sb);
	if (error)
		return (error);
	osf1_cvt_stat2_from_native(&sb, &osb);
	error = copyout((void *)&osb, (void *)SCARG(uap, ub), sizeof (osb));
	return (error);
}

int
osf1_sys_truncate(struct lwp *l, const struct osf1_sys_truncate_args *uap, register_t *retval)
{
	struct sys_truncate_args a;

	SCARG(&a, path) = SCARG(uap, path);
	SCARG(&a, PAD) = 0;
	SCARG(&a, length) = SCARG(uap, length);

	return sys_truncate(l, &a, retval);
}

int
osf1_sys_utimes(struct lwp *l, const struct osf1_sys_utimes_args *uap, register_t *retval)
{
	struct osf1_timeval otv;
	struct timeval tv[2], *tvp;
	int error;

	if (SCARG(uap, tptr) == NULL)
		tvp = NULL;
	else {
		/* get the OSF/1 timeval argument */
		error = copyin(SCARG(uap, tptr), &otv, sizeof otv);
		if (error != 0)
			return error;

		/* fill in and copy out the NetBSD timeval */
		tv[0].tv_sec = otv.tv_sec;
		tv[0].tv_usec = otv.tv_usec;
		/* Set access and modified to the same time */
		tv[1].tv_sec = otv.tv_sec;
		tv[1].tv_usec = otv.tv_usec;
		tvp = tv;
	}

	return do_sys_utimes(l, NULL, SCARG(uap, path), FOLLOW,
			    tvp, UIO_SYSSPACE);
}
