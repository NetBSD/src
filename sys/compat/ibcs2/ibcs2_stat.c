/*	$NetBSD: ibcs2_stat.c,v 1.17.2.3 2002/04/01 07:43:55 nathanw Exp $	*/
/*
 * Copyright (c) 1995, 1998 Scott Bartram
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ibcs2_stat.c,v 1.17.2.3 2002/04/01 07:43:55 nathanw Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/syscallargs.h>

#include <compat/ibcs2/ibcs2_types.h>
#include <compat/ibcs2/ibcs2_fcntl.h>
#include <compat/ibcs2/ibcs2_signal.h>
#include <compat/ibcs2/ibcs2_stat.h>
#include <compat/ibcs2/ibcs2_statfs.h>
#include <compat/ibcs2/ibcs2_syscallargs.h>
#include <compat/ibcs2/ibcs2_ustat.h>
#include <compat/ibcs2/ibcs2_util.h>
#include <compat/ibcs2/ibcs2_utsname.h>

static void bsd_stat2ibcs_stat __P((struct stat *, struct ibcs2_stat *));
static int cvt_statfs __P((struct statfs *, caddr_t, int));
static int cvt_statvfs __P((struct statfs *, caddr_t, int));

static void
bsd_stat2ibcs_stat(st, st4)
	struct stat *st;
	struct ibcs2_stat *st4;
{
	memset(st4, 0, sizeof(*st4));
	st4->st_dev = (ibcs2_dev_t)st->st_dev;
	st4->st_ino = (ibcs2_ino_t)st->st_ino;
	st4->st_mode = (ibcs2_mode_t)st->st_mode;
	if (st->st_nlink >= (1 << 15))
		st4->st_nlink = (1 << 15) - 1;
	else
		st4->st_nlink = (ibcs2_nlink_t)st->st_nlink;
	st4->st_uid = (ibcs2_uid_t)st->st_uid;
	st4->st_gid = (ibcs2_gid_t)st->st_gid;
	st4->st_rdev = (ibcs2_dev_t)st->st_rdev;
	st4->st_size = (ibcs2_off_t)st->st_size;
	st4->st_atim = (ibcs2_time_t)st->st_atime;
	st4->st_mtim = (ibcs2_time_t)st->st_mtime;
	st4->st_ctim = (ibcs2_time_t)st->st_ctime;
}

static int
cvt_statfs(sp, buf, len)
	struct statfs *sp;
	caddr_t buf;
	int len;
{
	struct ibcs2_statfs ssfs;

	memset(&ssfs, 0, sizeof ssfs);
	ssfs.f_fstyp = 0;
	ssfs.f_bsize = sp->f_bsize;
	ssfs.f_frsize = 0;
	ssfs.f_blocks = sp->f_blocks;
	ssfs.f_bfree = sp->f_bfree;
	ssfs.f_files = sp->f_files;
	ssfs.f_ffree = sp->f_ffree;
	ssfs.f_fname[0] = 0;
	ssfs.f_fpack[0] = 0;
	return copyout((caddr_t)&ssfs, buf, len);
}	

static int
cvt_statvfs(sp, buf, len)
	struct statfs *sp;
	caddr_t buf;
	int len;
{
	struct ibcs2_statvfs ssvfs;

	memset(&ssvfs, 0, sizeof ssvfs);
	ssvfs.f_frsize = ssvfs.f_bsize = sp->f_bsize;
	ssvfs.f_blocks = sp->f_blocks;
	ssvfs.f_bfree = sp->f_bfree;
	ssvfs.f_bavail = sp->f_bavail;
	ssvfs.f_files = sp->f_files;
	ssvfs.f_ffree = sp->f_ffree;
	ssvfs.f_favail = sp->f_ffree;
	ssvfs.f_fsid = sp->f_fsid.val[0];
	strncpy(ssvfs.f_basetype, sp->f_fstypename, 15);
	ssvfs.f_flag = 0;
	ssvfs.f_namemax = PATH_MAX;
	ssvfs.f_fstr[0] = 0;
	return copyout((caddr_t)&ssvfs, buf, len);
}	

int
ibcs2_sys_statfs(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ibcs2_sys_statfs_args /* {
		syscallarg(const char *) path;
		syscallarg(struct ibcs2_statfs *) buf;
		syscallarg(int) len;
		syscallarg(int) fstype;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct mount *mp;
	struct statfs *sp;
	int error;
	struct nameidata nd;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	mp = nd.ni_vp->v_mount;
	sp = &mp->mnt_stat;
	vrele(nd.ni_vp);
	if ((error = VFS_STATFS(mp, sp, p)) != 0)
		return (error);
	sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
	return cvt_statfs(sp, (caddr_t)SCARG(uap, buf), SCARG(uap, len));
}

int
ibcs2_sys_fstatfs(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ibcs2_sys_fstatfs_args /* {
		syscallarg(int) fd;
		syscallarg(struct ibcs2_statfs *) buf;
		syscallarg(int) len;
		syscallarg(int) fstype;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct file *fp;
	struct mount *mp;
	struct statfs *sp;
	int error;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	mp = ((struct vnode *)fp->f_data)->v_mount;
	sp = &mp->mnt_stat;
	if ((error = VFS_STATFS(mp, sp, p)) != 0)
		goto out;
	sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
	error = cvt_statfs(sp, (caddr_t)SCARG(uap, buf), SCARG(uap, len));
 out:
	FILE_UNUSE(fp, p);
	return (error);
}

int
ibcs2_sys_statvfs(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ibcs2_sys_statvfs_args /* {
		syscallarg(const char *) path;
		syscallarg(struct ibcs2_statvfs *) buf;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct mount *mp;
	struct statfs *sp;
	int error;
	struct nameidata nd;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	mp = nd.ni_vp->v_mount;
	sp = &mp->mnt_stat;
	vrele(nd.ni_vp);
	if ((error = VFS_STATFS(mp, sp, p)) != 0)
		return (error);
	sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
	return cvt_statvfs(sp, (caddr_t)SCARG(uap, buf),
			   sizeof(struct ibcs2_statvfs));
}

int
ibcs2_sys_fstatvfs(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ibcs2_sys_fstatvfs_args /* {
		syscallarg(int) fd;
		syscallarg(struct ibcs2_statvfs *) buf;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct file *fp;
	struct mount *mp;
	struct statfs *sp;
	int error;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	mp = ((struct vnode *)fp->f_data)->v_mount;
	sp = &mp->mnt_stat;
	if ((error = VFS_STATFS(mp, sp, p)) != 0)
		goto out;
	sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
	error = cvt_statvfs(sp, (caddr_t)SCARG(uap, buf),
			   sizeof(struct ibcs2_statvfs));
 out:
	FILE_UNUSE(fp, p);
	return (error);
}

int
ibcs2_sys_stat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ibcs2_sys_stat_args /* {
		syscallarg(const char *) path;
		syscallarg(struct ibcs2_stat *) st;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct stat st;
	struct ibcs2_stat ibcs2_st;
	struct sys___stat13_args cup;
	int error;
	caddr_t sg = stackgap_init(p, 0);
	SCARG(&cup, ub) = stackgap_alloc(p, &sg, sizeof(st));
	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	SCARG(&cup, path) = SCARG(uap, path);

	if ((error = sys___stat13(l, &cup, retval)) != 0)
		return error;
	if ((error = copyin(SCARG(&cup, ub), &st, sizeof(st))) != 0)
		return error;
	bsd_stat2ibcs_stat(&st, &ibcs2_st);
	return copyout((caddr_t)&ibcs2_st, (caddr_t)SCARG(uap, st),
		       ibcs2_stat_len);
}

int
ibcs2_sys_lstat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ibcs2_sys_lstat_args /* {
		syscallarg(const char *) path;
		syscallarg(struct ibcs2_stat *) st;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct stat st;
	struct ibcs2_stat ibcs2_st;
	struct sys___lstat13_args cup;
	int error;
	caddr_t sg = stackgap_init(p, 0);

	SCARG(&cup, ub) = stackgap_alloc(p, &sg, sizeof(st));
	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	SCARG(&cup, path) = SCARG(uap, path);

	if ((error = sys___lstat13(l, &cup, retval)) != 0)
		return error;
	if ((error = copyin(SCARG(&cup, ub), &st, sizeof(st))) != 0)
		return error;
	bsd_stat2ibcs_stat(&st, &ibcs2_st);
	return copyout((caddr_t)&ibcs2_st, (caddr_t)SCARG(uap, st),
		       ibcs2_stat_len);
}

int
ibcs2_sys_fstat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ibcs2_sys_fstat_args /* {
		syscallarg(int) fd;
		syscallarg(struct ibcs2_stat *) st;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct stat st;
	struct ibcs2_stat ibcs2_st;
	struct sys___fstat13_args cup;
	int error;
	caddr_t sg = stackgap_init(p, 0);

	SCARG(&cup, fd) = SCARG(uap, fd);
	SCARG(&cup, sb) = stackgap_alloc(p, &sg, sizeof(st));
	if ((error = sys___fstat13(l, &cup, retval)) != 0)
		return error;
	if ((error = copyin(SCARG(&cup, sb), &st, sizeof(st))) != 0)
		return error;
	bsd_stat2ibcs_stat(&st, &ibcs2_st);
	return copyout((caddr_t)&ibcs2_st, (caddr_t)SCARG(uap, st),
		       ibcs2_stat_len);
}

int
ibcs2_sys_utssys(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ibcs2_sys_utssys_args /* {
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) flag;
	} */ *uap = v;

	switch (SCARG(uap, flag)) {
	case 0:			/* uname(struct utsname *) */
	{
		struct ibcs2_utsname sut;

		memset(&sut, 0, ibcs2_utsname_len);
		memcpy(sut.sysname, ostype, sizeof(sut.sysname) - 1);
		memcpy(sut.nodename, hostname, sizeof(sut.nodename));
		sut.nodename[sizeof(sut.nodename)-1] = '\0';
		memcpy(sut.release, osrelease, sizeof(sut.release) - 1);
		memcpy(sut.version, "1", sizeof(sut.version) - 1);
		memcpy(sut.machine, machine, sizeof(sut.machine) - 1);

		return copyout((caddr_t)&sut, (caddr_t)SCARG(uap, a1),
			       ibcs2_utsname_len);
	}

	case 2:			/* ustat(dev_t, struct ustat *) */
	{
		struct ibcs2_ustat xu;

		xu.f_tfree = 20000; /* XXX fixme */
		xu.f_tinode = 32000;
		xu.f_fname[0] = 0;
		xu.f_fpack[0] = 0;
		return copyout((caddr_t)&xu, (caddr_t)SCARG(uap, a2),
                               ibcs2_ustat_len);
	}

	default:
		return ENOSYS;
	}
}
