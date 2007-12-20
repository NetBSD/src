/*	$NetBSD: linux_file64.c,v 1.44 2007/12/20 23:02:54 dsl Exp $	*/

/*-
 * Copyright (c) 1995, 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz.
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
 * Linux 64bit filesystem calls. Used on 32bit archs, not used on 64bit ones.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_file64.c,v 1.44 2007/12/20 23:02:54 dsl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/dirent.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vfs_syscalls.h>
#include <sys/vnode.h>
#include <sys/tty.h>
#include <sys/conf.h>

#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_fcntl.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/common/linux_dirent.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>

#include <compat/linux/linux_syscallargs.h>

#ifndef alpha

# ifndef COMPAT_LINUX32

static void bsd_to_linux_stat(struct stat *, struct linux_stat64 *);

/*
 * Convert a NetBSD stat structure to a Linux stat structure.
 * Only the order of the fields and the padding in the structure
 * is different. linux_fakedev is a machine-dependent function
 * which optionally converts device driver major/minor numbers
 * (XXX horrible, but what can you do against code that compares
 * things against constant major device numbers? sigh)
 */
static void
bsd_to_linux_stat(struct stat *bsp, struct linux_stat64 *lsp)
{
	lsp->lst_dev     = linux_fakedev(bsp->st_dev, 0);
	lsp->lst_ino     = bsp->st_ino;
	lsp->lst_mode    = (linux_mode_t)bsp->st_mode;
	if (bsp->st_nlink >= (1 << 15))
		lsp->lst_nlink = (1 << 15) - 1;
	else
		lsp->lst_nlink = (linux_nlink_t)bsp->st_nlink;
	lsp->lst_uid     = bsp->st_uid;
	lsp->lst_gid     = bsp->st_gid;
	lsp->lst_rdev    = linux_fakedev(bsp->st_rdev, 1);
	lsp->lst_size    = bsp->st_size;
	lsp->lst_blksize = bsp->st_blksize;
	lsp->lst_blocks  = bsp->st_blocks;
	lsp->lst_atime   = bsp->st_atime;
	lsp->lst_mtime   = bsp->st_mtime;
	lsp->lst_ctime   = bsp->st_ctime;
#  ifdef LINUX_STAT64_HAS_NSEC
	lsp->lst_atime_nsec   = bsp->st_atimensec;
	lsp->lst_mtime_nsec   = bsp->st_mtimensec;
	lsp->lst_ctime_nsec   = bsp->st_ctimensec;
#  endif
#  if LINUX_STAT64_HAS_BROKEN_ST_INO
	lsp->__lst_ino   = (linux_ino_t) bsp->st_ino;
#  endif
}

/*
 * The stat functions below are plain sailing. stat and lstat are handled
 * by one function to avoid code duplication.
 */
int
linux_sys_fstat64(struct lwp *l, const struct linux_sys_fstat64_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(struct linux_stat64 *) sp;
	} */
	struct linux_stat64 tmplst;
	struct stat tmpst;
	int error;

	error = do_sys_fstat(l,  SCARG(uap, fd), &tmpst);
	if (error != 0)
		return error;

	bsd_to_linux_stat(&tmpst, &tmplst);

	return copyout(&tmplst, SCARG(uap, sp), sizeof tmplst);
}

static int
linux_do_stat64(struct lwp *l, const struct linux_sys_stat64_args *uap, register_t *retval, int flags)
{
	struct linux_stat64 tmplst;
	struct stat tmpst;
	int error;

	error = do_sys_stat(l, SCARG(uap, path), flags, &tmpst);
	if (error != 0)
		return error;

	bsd_to_linux_stat(&tmpst, &tmplst);

	return copyout(&tmplst, SCARG(uap, sp), sizeof tmplst);
}

int
linux_sys_stat64(struct lwp *l, const struct linux_sys_stat64_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct linux_stat64 *) sp;
	} */

	return linux_do_stat64(l, uap, retval, FOLLOW);
}

int
linux_sys_lstat64(struct lwp *l, const struct linux_sys_lstat64_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct linux_stat64 *) sp;
	} */

	return linux_do_stat64(l, (const void *)uap, retval, NOFOLLOW);
}

int
linux_sys_truncate64(struct lwp *l, const struct linux_sys_truncate64_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(off_t) length;
	} */
	struct sys_truncate_args ta;

	/* Linux doesn't have the 'pad' pseudo-parameter */
	SCARG(&ta, path) = SCARG(uap, path);
	SCARG(&ta, pad) = 0;
	SCARG(&ta, length) = SCARG(uap, length);

	return sys_truncate(l, &ta, retval);
}

int
linux_sys_ftruncate64(struct lwp *l, const struct linux_sys_ftruncate64_args *uap, register_t *retval)
{
	/* {
		syscallarg(unsigned int) fd;
		syscallarg(off_t) length;
	} */
	struct sys_ftruncate_args ta;

	/* Linux doesn't have the 'pad' pseudo-parameter */
	SCARG(&ta, fd) = SCARG(uap, fd);
	SCARG(&ta, pad) = 0;
	SCARG(&ta, length) = SCARG(uap, length);

	return sys_ftruncate(l, &ta, retval);
}
# endif /* !COMPAT_LINUX32 */

# if !defined(__m68k__) && (!defined(__amd64__) || defined(COMPAT_LINUX32))
static void bsd_to_linux_flock64(struct linux_flock64 *,
    const struct flock *);
static void linux_to_bsd_flock64(struct flock *,
    const struct linux_flock64 *);

static void
bsd_to_linux_flock64(struct linux_flock64 *lfp, const struct flock *bfp)
{

	lfp->l_start = bfp->l_start;
	lfp->l_len = bfp->l_len;
	lfp->l_pid = bfp->l_pid;
	lfp->l_whence = bfp->l_whence;
	switch (bfp->l_type) {
	case F_RDLCK:
		lfp->l_type = LINUX_F_RDLCK;
		break;
	case F_UNLCK:
		lfp->l_type = LINUX_F_UNLCK;
		break;
	case F_WRLCK:
		lfp->l_type = LINUX_F_WRLCK;
		break;
	}
}

static void
linux_to_bsd_flock64(struct flock *bfp, const struct linux_flock64 *lfp)
{

	bfp->l_start = lfp->l_start;
	bfp->l_len = lfp->l_len;
	bfp->l_pid = lfp->l_pid;
	bfp->l_whence = lfp->l_whence;
	switch (lfp->l_type) {
	case LINUX_F_RDLCK:
		bfp->l_type = F_RDLCK;
		break;
	case LINUX_F_UNLCK:
		bfp->l_type = F_UNLCK;
		break;
	case LINUX_F_WRLCK:
		bfp->l_type = F_WRLCK;
		break;
	}
}

int
linux_sys_fcntl64(struct lwp *l, const struct linux_sys_fcntl64_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) cmd;
		syscallarg(void *) arg;
	} */
	struct linux_flock64 lfl;
	struct flock bfl;
	int error;
	void *arg = SCARG(uap, arg);
	int cmd = SCARG(uap, cmd);
	int fd = SCARG(uap, fd);

	switch (cmd) {
	case LINUX_F_GETLK64:
		if ((error = copyin(arg, &lfl, sizeof lfl)) != 0)
			return error;
		linux_to_bsd_flock64(&bfl, &lfl);
		error = do_fcntl_lock(l, fd, F_GETLK, &bfl);
		if (error != 0)
			return error;
		bsd_to_linux_flock64(&lfl, &bfl);
		return copyout(&lfl, arg, sizeof lfl);
	case LINUX_F_SETLK64:
	case LINUX_F_SETLKW64:
		cmd = (cmd == LINUX_F_SETLK64 ? F_SETLK : F_SETLKW);
		if ((error = copyin(arg, &lfl, sizeof lfl)) != 0)
			return error;
		linux_to_bsd_flock64(&bfl, &lfl);
		return do_fcntl_lock(l, fd, cmd, &bfl);
	default:
		return linux_sys_fcntl(l, (const void *)uap, retval);
	}
}
# endif /* !m69k && !amd64  && !COMPAT_LINUX32 */

#endif /* !alpha */

/*
 * Linux 'readdir' call. This code is mostly taken from the
 * SunOS getdents call (see compat/sunos/sunos_misc.c), though
 * an attempt has been made to keep it a little cleaner.
 *
 * The d_off field contains the offset of the next valid entry,
 * unless the older Linux getdents(2), which used to have it set
 * to the offset of the entry itself. This function also doesn't
 * need to deal with the old count == 1 glibc problem.
 *
 * Read in BSD-style entries, convert them, and copy them out.
 *
 * Note that this doesn't handle union-mounted filesystems.
 */
#ifndef COMPAT_LINUX32
int
linux_sys_getdents64(struct lwp *l, const struct linux_sys_getdents64_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(struct linux_dirent64 *) dent;
		syscallarg(unsigned int) count;
	} */
	struct dirent *bdp;
	struct vnode *vp;
	char *inp, *tbuf;		/* BSD-format */
	int len, reclen;		/* BSD-format */
	char *outp;			/* Linux-format */
	int resid, linux_reclen = 0;	/* Linux-format */
	struct file *fp;
	struct uio auio;
	struct iovec aiov;
	struct linux_dirent64 idb;
	off_t off;		/* true file offset */
	int buflen, error, eofflag, nbytes;
	struct vattr va;
	off_t *cookiebuf = NULL, *cookie;
	int ncookies;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(l->l_proc->p_fd, SCARG(uap, fd), &fp)) != 0)
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

	if ((error = VOP_GETATTR(vp, &va, l->l_cred)))
		goto out1;

	nbytes = SCARG(uap, count);
	buflen = min(MAXBSIZE, nbytes);
	if (buflen < va.va_blocksize)
		buflen = va.va_blocksize;
	tbuf = malloc(buflen, M_TEMP, M_WAITOK);

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	off = fp->f_offset;
again:
	aiov.iov_base = tbuf;
	aiov.iov_len = buflen;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_READ;
	auio.uio_resid = buflen;
	auio.uio_offset = off;
	UIO_SETUP_SYSSPACE(&auio);
	/*
         * First we read into the malloc'ed buffer, then
         * we massage it into user space, one record at a time.
         */
	error = VOP_READDIR(vp, &auio, fp->f_cred, &eofflag, &cookiebuf,
	    &ncookies);
	if (error)
		goto out;

	inp = tbuf;
	outp = (void *)SCARG(uap, dent);
	resid = nbytes;
	if ((len = buflen - auio.uio_resid) == 0)
		goto eof;

	for (cookie = cookiebuf; len > 0; len -= reclen) {
		bdp = (struct dirent *)inp;
		reclen = bdp->d_reclen;
		if (reclen & 3)
			panic("linux_readdir");
		if (bdp->d_fileno == 0) {
			inp += reclen;	/* it is a hole; squish it out */
			if (cookie)
				off = *cookie++;
			else
				off += reclen;
			continue;
		}
		linux_reclen = LINUX_RECLEN(&idb, bdp->d_namlen);
		if (reclen > len || resid < linux_reclen) {
			/* entry too big for buffer, so just stop */
			outp++;
			break;
		}
		if (cookie)
			off = *cookie++;	/* each entry points to next */
		else
			off += reclen;
		/*
		 * Massage in place to make a Linux-shaped dirent (otherwise
		 * we have to worry about touching user memory outside of
		 * the copyout() call).
		 */
		idb.d_ino = bdp->d_fileno;
		idb.d_type = bdp->d_type;
		idb.d_off = off;
		idb.d_reclen = (u_short)linux_reclen;
		strcpy(idb.d_name, bdp->d_name);
		if ((error = copyout((void *)&idb, outp, linux_reclen)))
			goto out;
		/* advance past this real entry */
		inp += reclen;
		/* advance output past Linux-shaped entry */
		outp += linux_reclen;
		resid -= linux_reclen;
	}

	/* if we squished out the whole block, try again */
	if (outp == (void *)SCARG(uap, dent))
		goto again;
	fp->f_offset = off;	/* update the vnode offset */

eof:
	*retval = nbytes - resid;
out:
	VOP_UNLOCK(vp, 0);
	if (cookiebuf)
		free(cookiebuf, M_TEMP);
	free(tbuf, M_TEMP);
out1:
	FILE_UNUSE(fp, l);
	return error;
}
#endif /* !COMPAT_LINUX32 */
