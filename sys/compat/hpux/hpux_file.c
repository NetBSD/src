/*	$NetBSD: hpux_file.c,v 1.36 2007/04/30 09:20:18 dsl Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: hpux_compat.c 1.64 93/08/05$
 *
 *	@(#)hpux_compat.c	8.4 (Berkeley) 2/13/94
 */

/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: hpux_compat.c 1.64 93/08/05$
 *
 *	@(#)hpux_compat.c	8.4 (Berkeley) 2/13/94
 */

/*
 * File-related routines for HP-UX binary compatibility.  Partially
 * modeled after sys/compat/linux/linux_file.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hpux_file.c,v 1.36 2007/04/30 09:20:18 dsl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/filedesc.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/ioctl.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/ipc.h>
#include <sys/user.h>
#include <sys/vfs_syscalls.h>
#include <sys/mman.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/vmparam.h>

#include <sys/syscallargs.h>

#include <compat/hpux/hpux.h>
#include <compat/hpux/hpux_util.h>
#include <compat/hpux/hpux_termio.h>
#include <compat/hpux/hpux_syscall.h>
#include <compat/hpux/hpux_syscallargs.h>

static int	hpux_stat1 __P((struct lwp *, void *, register_t *, int));
static void	bsd_to_hpux_stat __P((struct stat *, struct hpux_stat *));
static void	bsd_to_hpux_ostat __P((struct stat *, struct hpux_ostat *));

/*
 * HP-UX creat(2) system call.
 *
 * Just call open(2) with the TRUNC, CREAT and WRONLY flags.
 */
int
hpux_sys_creat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_creat_args /* {
		syscallarg(const char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct sys_open_args oa;

	SCARG(&oa, path) = SCARG(uap, path);
	SCARG(&oa, flags) = O_CREAT | O_TRUNC | O_WRONLY;
	SCARG(&oa, mode) = SCARG(uap, mode);

	return sys_open(l, &oa, retval);
}

/*
 * HP-UX open(2) system call.
 *
 * We need to remap some of the bits in the mode mask:
 *
 *	- O_CREAT, O_TRUNC, and O_EXCL must me remapped.
 *	- O_NONBLOCK is remapped and remembered.
 *	- O_FNDELAY is remembered.
 *	- O_SYNCIO is removed entirely.
 */
int
hpux_sys_open(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_open_args /* {
		syscallarg(const char *) path;
		syscallarg(int) flags;
		syscallarg(int) mode;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct sys_open_args oa;
	int flags, nflags, error;

	/*
	 * Deal with the mode flags first, since they will affect
	 * how we check for the alternate path.
	 */
	flags = SCARG(uap, flags);
	nflags =
	   flags & ~(HPUXNONBLOCK|HPUXFSYNCIO|HPUXFEXCL|HPUXFTRUNC|HPUXFCREAT);
	if (flags & HPUXFCREAT) {
		/*
		 * Simulate the pre-NFS behavior that opening a
		 * file for READ+CREATE ignores the CREATE (unless
		 * EXCL is set in which case we will return the
		 * proper error).
		 */
		if ((flags & HPUXFEXCL) || (FFLAGS(flags) & FWRITE))
			nflags |= O_CREAT;
	}
	if (flags & HPUXFTRUNC)
		nflags |= O_TRUNC;
	if (flags & HPUXFEXCL)
		nflags |= O_EXCL;
	if (flags & HPUXNONBLOCK)
		nflags |= O_NDELAY;

	/*
	 * Fill in the new arguments and call the NetBSD open(2).
	 */
	SCARG(&oa, path) = SCARG(uap, path);
	SCARG(&oa, flags) = nflags;
	SCARG(&oa, mode) =  SCARG(uap, mode);

	error = sys_open(l, &oa, retval);

	/*
	 * Record non-blocking mode for fcntl, read, write, etc.
	 */
	if ((error == 0) && (nflags & O_NDELAY))
		p->p_fd->fd_ofileflags[*retval] |=
		    (flags & HPUXNONBLOCK) ?
		        HPUX_UF_NONBLOCK_ON : HPUX_UF_FNDELAY_ON;

	return (error);
}

/*
 * HP-UX fcntl(2) system call.
 */
int
hpux_sys_fcntl(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_fcntl_args /* {
		syscallarg(int) fd;
		syscallarg(int) cmd;
		syscallarg(int) arg;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int arg, mode, error, flg = F_POSIX;
	struct file *fp;
	char *pop;
	struct hpux_flock hfl;
	struct flock fl;
	struct vnode *vp;
	struct sys_fcntl_args fa;

	if ((fp = fd_getfile(p->p_fd, SCARG(uap, fd))) == NULL)
		return (EBADF);

	/* This array dereference is validated by fd_getfile */
	pop = &p->p_fd->fd_ofileflags[SCARG(uap, fd)];
	arg = SCARG(uap, arg);

	switch (SCARG(uap, cmd)) {
	case F_SETFL:
		if (arg & HPUXNONBLOCK)
			*pop |= HPUX_UF_NONBLOCK_ON;
		else
			*pop &= ~HPUX_UF_NONBLOCK_ON;

		if (arg & HPUXNDELAY)
			*pop |= HPUX_UF_FNDELAY_ON;
		else
			*pop &= ~HPUX_UF_FNDELAY_ON;

		if (*pop & (HPUX_UF_NONBLOCK_ON|HPUX_UF_FNDELAY_ON|HPUX_UF_FIONBIO_ON))
			arg |= FNONBLOCK;
		else
			arg &= ~FNONBLOCK;

		arg &= ~(HPUXNONBLOCK|HPUXFSYNCIO|HPUXFREMOTE);
		break;

	case F_GETFL:
	case F_DUPFD:
	case F_GETFD:
	case F_SETFD:
		break;

	case HPUXF_SETLKW:
		flg |= F_WAIT;
		/* Fall into F_SETLK */

	case HPUXF_SETLK:
		if (fp->f_type != DTYPE_VNODE)
			return (EBADF);

		vp = (struct vnode *)fp->f_data;

		/* Copy in the lock structure */
		error = copyin((void *)SCARG(uap, arg), &hfl, sizeof (hfl));
		if (error)
			return (error);

		fl.l_start = hfl.hl_start;
		fl.l_len = hfl.hl_len;
		fl.l_pid = hfl.hl_pid;
		fl.l_type = hfl.hl_type;
		fl.l_whence = hfl.hl_whence;
		if (fl.l_whence == SEEK_CUR)
			fl.l_start += fp->f_offset;

		switch (fl.l_type) {
		case F_RDLCK:
			if ((fp->f_flag & FREAD) == 0)
				return (EBADF);

			p->p_flag |= PK_ADVLOCK;
			return (VOP_ADVLOCK(vp, p, F_SETLK, &fl, flg));

		case F_WRLCK:
			if ((fp->f_flag & FWRITE) == 0)
				return (EBADF);
			p->p_flag |= PK_ADVLOCK;
			return (VOP_ADVLOCK(vp, p, F_SETLK, &fl, flg));

		case F_UNLCK:
			return (VOP_ADVLOCK(vp, p, F_UNLCK, &fl,
			    F_POSIX));

		default:
			return (EINVAL);
		}
		/* NOTREACHED */

	case F_GETLK:
		if (fp->f_type != DTYPE_VNODE)
			return (EBADF);

		vp = (struct vnode *)fp->f_data;

		/* Copy in the lock structure */
		error = copyin((void *)SCARG(uap, arg), &hfl, sizeof (hfl));
		if (error)
			return (error);

		fl.l_start = hfl.hl_start;
		fl.l_len = hfl.hl_len;
		fl.l_pid = hfl.hl_pid;
		fl.l_type = hfl.hl_type;
		fl.l_whence = hfl.hl_whence;
		if (fl.l_whence == SEEK_CUR)
			fl.l_start += fp->f_offset;

		if ((error = VOP_ADVLOCK(vp, p, F_GETLK, &fl, F_POSIX)))
			return (error);

		hfl.hl_start = fl.l_start;
		hfl.hl_len = fl.l_len;
		hfl.hl_pid = fl.l_pid;
		hfl.hl_type = fl.l_type;
		hfl.hl_whence = fl.l_whence;
		return (copyout(&hfl, (void *)SCARG(uap, arg),
		    sizeof (hfl)));

	default:
		return (EINVAL);
	}

	/*
	 * Pass whatever's left on to the NetBSD fcntl(2).
	 */
	SCARG(&fa, fd) = SCARG(uap, fd);
	SCARG(&fa, cmd) = SCARG(uap, cmd);
	SCARG(&fa, arg) = (void *)arg;

	error = sys_fcntl(l, &fa, retval);

	if ((error == 0) && (SCARG(&fa, cmd) == F_GETFL)) {
		mode = *retval;
		*retval &= ~(O_CREAT|O_TRUNC|O_EXCL);
		if (mode & FNONBLOCK) {
			if (*pop & HPUX_UF_NONBLOCK_ON)
				*retval |= HPUXNONBLOCK;

			if ((*pop & HPUX_UF_FNDELAY_ON) == 0)
				*retval &= ~HPUXNDELAY;
		}
		if (mode & O_CREAT)
			*retval |= HPUXFCREAT;

		if (mode & O_TRUNC)
			*retval |= HPUXFTRUNC;

		if (mode & O_EXCL)
			*retval |= HPUXFEXCL;
	}
	return (error);
}

/*
 * HP-UX fstat(2) system call.
 */
int
hpux_sys_fstat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_fstat_args /* {
		syscallarg(int) fd;
		syscallarg(struct hpux_stat *) sb;
	} */ *uap = v;
	struct hpux_stat tmphst;
	struct stat sb;
	int error;

	error = do_sys_fstat(l, SCARG(uap, fd), &sb);
	if (error)
		return error;

	bsd_to_hpux_stat(&sb, &tmphst);

	return copyout(&tmphst, SCARG(uap, sb), sizeof(struct hpux_stat));
}

/*
 * HP-UX stat(2) system call.
 */
int
hpux_sys_stat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{

	return (hpux_stat1(l, v, retval, FOLLOW));
}

/*
 * HP-UX lstat(2) system call.
 */
int
hpux_sys_lstat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{

	return (hpux_stat1(l, v, retval, NOFOLLOW));
}

/*
 * Do the meat of stat(2) and lstat(2).
 */
static int
hpux_stat1(l, v, retval, flags)
	struct lwp *l;
	void *v;
	register_t *retval;
	int flags;
{
	struct hpux_sys_stat_args /* {
		syscallarg(const char *) path;
		syscallarg(struct hpux_stat *) sb;
	} */ *uap = v;
	struct hpux_stat tmphst;
	struct stat sb;
	int error;

	error = do_sys_stat(l, SCARG(uap, path), flags, &sb);
	if (error)
		return (error);

	bsd_to_hpux_stat(&sb, &tmphst);

	return copyout(&tmphst, SCARG(uap, sb), sizeof(struct hpux_stat));
}

/*
 * The old HP-UX fstat(2) system call.
 */
int
hpux_sys_fstat_6x(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_fstat_6x_args /* {
		syscallarg(int) fd;
		syscallarg(struct hpux_ostat *) sb;
	} */ *uap = v;
	struct hpux_ostat tmphst;
	struct stat sb;
	int error;

	error = do_sys_fstat(l, SCARG(uap, fd), &sb);
	if (error)
		return error;

	bsd_to_hpux_ostat(&sb, &tmphst);

	return copyout(&tmphst, SCARG(uap, sb), sizeof(struct hpux_ostat));
}

/*
 * The old HP-UX stat(2) system call.
 */
int
hpux_sys_stat_6x(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_stat_6x_args /* {
		syscallarg(const char *) path;
		syscallarg(struct hpux_ostat *) sb;
	} */ *uap = v;
	struct hpux_ostat tmphst;
	struct stat sb;
	int error;

	error = do_sys_stat(l, SCARG(uap, path), FOLLOW, &sb);
	if (error)
		return (error);

	bsd_to_hpux_ostat(&sb, &tmphst);

	return (copyout(&tmphst, SCARG(uap, sb), sizeof(struct hpux_ostat)));
}

/*
 * Convert a NetBSD stat structure to an HP-UX stat structure.
 */
static void
bsd_to_hpux_stat(sb, hsb)
	struct stat *sb;
	struct hpux_stat *hsb;
{

	memset(hsb, 0, sizeof(struct hpux_stat));
	hsb->hst_dev = (long)sb->st_dev;
	hsb->hst_ino = (u_long)sb->st_ino;
	hsb->hst_mode = (u_short)sb->st_mode;
	if (sb->st_nlink >= (1 << 15))
		hsb->hst_nlink = (1 << 15) - 1;
	else
		hsb->hst_nlink = (u_short)sb->st_nlink;
	hsb->hst_uid = (u_long)sb->st_uid;
	hsb->hst_gid = (u_long)sb->st_gid;
	hsb->hst_rdev = (long)bsdtohpuxdev(sb->st_rdev);
	/*
	 * XXX Let's just hope that the old binary doesn't lose.
	 */
	hsb->hst_old_uid = (u_short)sb->st_uid;
	hsb->hst_old_gid = (u_short)sb->st_gid;

	if (sb->st_size < (off_t)(((off_t)1) << 32))
		hsb->hst_size = (long)sb->st_size;
	else
		hsb->hst_size = -2;
	hsb->hst_atime = (long)sb->st_atime;
	hsb->hst_mtime = (long)sb->st_mtime;
	hsb->hst_ctime = (long)sb->st_ctime;
	hsb->hst_blksize = (long)sb->st_blksize;
	hsb->hst_blocks = (long)sb->st_blocks;
}

/*
 * Convert a NetBSD stat structure to an old-style HP-UX stat structure.
 */
static void
bsd_to_hpux_ostat(sb, hsb)
	struct stat *sb;
	struct hpux_ostat *hsb;
{

	memset(hsb, 0, sizeof(struct hpux_ostat));
	hsb->hst_dev = (u_short)sb->st_dev;
	hsb->hst_ino = (u_short)sb->st_ino;
	hsb->hst_mode = (u_short)sb->st_mode;
	if (sb->st_nlink >= (1 << 15))
		hsb->hst_nlink = (1 << 15) - 1;
	else
		hsb->hst_nlink = (u_short)sb->st_nlink;
	hsb->hst_uid = (u_short)sb->st_uid;
	hsb->hst_gid = (u_short)sb->st_gid;
	hsb->hst_rdev = (u_short)sb->st_rdev;
	if (sb->st_size < (off_t)(((off_t)1) << 32))
		hsb->hst_size = (int)sb->st_size;
	else
		hsb->hst_size = -2;
	hsb->hst_atime = (int)sb->st_atime;
	hsb->hst_mtime = (int)sb->st_mtime;
	hsb->hst_ctime = (int)sb->st_ctime;
}

/*
 * HP-UX access(2) system call.
 */
int
hpux_sys_access(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_access_args /* {
		syscallarg(const char *) path;
		syscallarg(int) flags;
	} */ *uap = v;
	struct proc *p = l->l_proc;

	return (sys_access(l, uap, retval));
}

/*
 * HP-UX unlink(2) system call.
 */
int
hpux_sys_unlink(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_unlink_args /* {
		syscallarg(char *) path;
	} */ *uap = v;
	struct proc *p = l->l_proc;

	return (sys_unlink(l, uap, retval));
}

/*
 * HP-UX chdir(2) system call.
 */
int
hpux_sys_chdir(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_chdir_args /* {
		syscallarg(const char *) path;
	} */ *uap = v;
	struct proc *p = l->l_proc;

	return (sys_chdir(l, uap, retval));
}

/*
 * HP-UX mknod(2) system call.
 */
int
hpux_sys_mknod(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_mknod_args /* {
		syscallarg(const char *) path;
		syscallarg(int) mode;
		syscallarf(int) dev;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct sys_mkfifo_args bma;

	/*
	 * BSD handles FIFOs separately.
	 */
	if (S_ISFIFO(SCARG(uap, mode))) {
		SCARG(&bma, path) = SCARG(uap, path);
		SCARG(&bma, mode) = SCARG(uap, mode);
		return (sys_mkfifo(l, uap, retval));
	} else
		return (sys_mknod(l, uap, retval));
}

/*
 * HP-UX chmod(2) system call.
 */
int
hpux_sys_chmod(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_chmod_args /* {
		syscallarg(const char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	struct proc *p = l->l_proc;

	return (sys_chmod(l, uap, retval));
}

/*
 * HP-UX chown(2) system call.
 */
int
hpux_sys_chown(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_chown_args /* {
		syscallarg(const char *) path;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */ *uap = v;
	struct proc *p = l->l_proc;

	/* XXX What about older HP-UX executables? */

	return (sys___posix_chown(l, uap, retval));
}

/*
 * HP-UX rename(2) system call.
 */
int
hpux_sys_rename(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_rename_args /* {
		syscallarg(const char *) from;
		syscallarg(const char *) to;
	} */ *uap = v;
	struct proc *p = l->l_proc;

	return (sys___posix_rename(l, uap, retval));
}

/*
 * HP-UX mkdir(2) system call.
 */
int
hpux_sys_mkdir(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_mkdir_args /* {
		syscallarg(char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	struct proc *p = l->l_proc;

	return (sys_mkdir(l, uap, retval));
}

/*
 * HP-UX rmdir(2) system call.
 */
int
hpux_sys_rmdir(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_rmdir_args /* {
		syscallarg(const char *) path;
	} */ *uap = v;
	struct proc *p = l->l_proc;

	return (sys_rmdir(l, uap, retval));
}

/*
 * HP-UX symlink(2) system call.
 */
int
hpux_sys_symlink(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_symlink_args /* {
		syscallarg(const char *) path;
		syscallarg(const char *) link;
	} */ *uap = v;
	struct proc *p = l->l_proc;

	return (sys_symlink(l, uap, retval));
}

/*
 * HP-UX readlink(2) system call.
 */
int
hpux_sys_readlink(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_readlink_args /* {
		syscallarg(const char *) path;
		syscallarg(char *) buf;
		syscallarg(int) count;
	} */ *uap = v;
	struct proc *p = l->l_proc;

	return (sys_readlink(l, uap, retval));
}

/*
 * HP-UX truncate(2) system call.
 */
int
hpux_sys_truncate(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_truncate_args /* {
		syscallarg(const char *) path;
		syscallarg(long) length;
	} */ *uap = v;
	struct proc *p = l->l_proc;

	return (compat_43_sys_truncate(l, uap, retval));
}
