/*	$NetBSD: ibcs2_stat.c,v 1.48.2.1 2017/08/28 17:51:58 skrll Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: ibcs2_stat.c,v 1.48.2.1 2017/08/28 17:51:58 skrll Exp $");

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
#include <sys/vnode.h>
#include <sys/syscallargs.h>
#include <sys/vfs_syscalls.h>

#include <compat/ibcs2/ibcs2_types.h>
#include <compat/ibcs2/ibcs2_fcntl.h>
#include <compat/ibcs2/ibcs2_signal.h>
#include <compat/ibcs2/ibcs2_stat.h>
#include <compat/ibcs2/ibcs2_statfs.h>
#include <compat/ibcs2/ibcs2_syscallargs.h>
#include <compat/ibcs2/ibcs2_ustat.h>
#include <compat/ibcs2/ibcs2_util.h>
#include <compat/ibcs2/ibcs2_utsname.h>

static void bsd_stat2ibcs_stat(struct stat *, struct ibcs2_stat *);
static int cvt_statfs(struct statvfs *, void *, int);
static int cvt_statvfs(struct statvfs *, void *, int);

static void
bsd_stat2ibcs_stat(struct stat *st, struct ibcs2_stat *st4)
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
cvt_statfs(struct statvfs *sp, void *tbuf, int len)
{
	struct ibcs2_statfs ssfs;

	if (len < 0)
		return (EINVAL);
	if (len > sizeof(ssfs))
		len = sizeof(ssfs);

	memset(&ssfs, 0, sizeof ssfs);
	ssfs.f_fstyp = 0;
	ssfs.f_bsize = sp->f_bsize;
	ssfs.f_frsize = sp->f_frsize;
	ssfs.f_blocks = sp->f_blocks;
	ssfs.f_bfree = sp->f_bfree;
	ssfs.f_files = sp->f_files;
	ssfs.f_ffree = sp->f_ffree;
	ssfs.f_fname[0] = 0;
	ssfs.f_fpack[0] = 0;
	return copyout((void *)&ssfs, tbuf, len);
}

static int
cvt_statvfs(struct statvfs *sp, void *tbuf, int len)
{
	struct ibcs2_statvfs ssvfs;

	if (len < 0)
		return (EINVAL);
	if (len > sizeof(ssvfs))
		len = sizeof(ssvfs);

	memset(&ssvfs, 0, sizeof ssvfs);
	ssvfs.f_bsize = sp->f_bsize;
	ssvfs.f_frsize = sp->f_frsize;
	ssvfs.f_blocks = sp->f_blocks;
	ssvfs.f_bfree = sp->f_bfree;
	ssvfs.f_bavail = sp->f_bavail;
	ssvfs.f_files = sp->f_files;
	ssvfs.f_ffree = sp->f_ffree;
	ssvfs.f_favail = sp->f_favail;
	ssvfs.f_fsid = sp->f_fsidx.__fsid_val[0];
	strncpy(ssvfs.f_basetype, sp->f_fstypename, 15);
	ssvfs.f_flag = 0;
	ssvfs.f_namemax = PATH_MAX;
	ssvfs.f_fstr[0] = 0;
	return copyout((void *)&ssvfs, tbuf, len);
}

int
ibcs2_sys_statfs(struct lwp *l, const struct ibcs2_sys_statfs_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct ibcs2_statfs *) buf;
		syscallarg(int) len;
		syscallarg(int) fstype;
	} */
	struct mount *mp;
	struct statvfs *sp;
	int error;
	struct vnode *vp;

	error = namei_simple_user(SCARG(uap, path),
				NSM_FOLLOW_TRYEMULROOT, &vp);
	if (error != 0)
		return (error);
	mp = vp->v_mount;
	sp = &mp->mnt_stat;
	if ((error = VFS_STATVFS(mp, sp)) != 0)
		goto out;
	sp->f_flag = mp->mnt_flag & MNT_VISFLAGMASK;
	error = cvt_statfs(sp, (void *)SCARG(uap, buf), SCARG(uap, len));
out:
	vrele(vp);
	return (error);
}

int
ibcs2_sys_fstatfs(struct lwp *l, const struct ibcs2_sys_fstatfs_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(struct ibcs2_statfs *) buf;
		syscallarg(int) len;
		syscallarg(int) fstype;
	} */
	file_t *fp;
	struct mount *mp;
	struct statvfs *sp;
	int error;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return (error);
	mp = fp->f_vnode->v_mount;
	sp = &mp->mnt_stat;
	if ((error = VFS_STATVFS(mp, sp)) != 0)
		goto out;
	sp->f_flag = mp->mnt_flag & MNT_VISFLAGMASK;
	error = cvt_statfs(sp, (void *)SCARG(uap, buf), SCARG(uap, len));
 out:
 	fd_putfile(SCARG(uap, fd));
	return (error);
}

int
ibcs2_sys_statvfs(struct lwp *l, const struct ibcs2_sys_statvfs_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct ibcs2_statvfs *) buf;
	} */
	struct mount *mp;
	struct statvfs *sp;
	int error;
	struct vnode *vp;

	error = namei_simple_user(SCARG(uap, path),
				NSM_FOLLOW_TRYEMULROOT, &vp);
	if (error != 0)
		return (error);
	mp = vp->v_mount;
	sp = &mp->mnt_stat;
	if ((error = VFS_STATVFS(mp, sp)) != 0)
		goto out;
	sp->f_flag = mp->mnt_flag & MNT_VISFLAGMASK;
	error = cvt_statvfs(sp, (void *)SCARG(uap, buf),
	    sizeof(struct ibcs2_statvfs));
out:
	vrele(vp);
	return error;
}

int
ibcs2_sys_fstatvfs(struct lwp *l, const struct ibcs2_sys_fstatvfs_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(struct ibcs2_statvfs *) buf;
	} */
	file_t *fp;
	struct mount *mp;
	struct statvfs *sp;
	int error;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return (error);
	mp = fp->f_vnode->v_mount;
	sp = &mp->mnt_stat;
	if ((error = VFS_STATVFS(mp, sp)) != 0)
		goto out;
	sp->f_flag = mp->mnt_flag & MNT_VISFLAGMASK;
	error = cvt_statvfs(sp, SCARG(uap, buf), sizeof(struct ibcs2_statvfs));
 out:
 	fd_putfile(SCARG(uap, fd));
	return (error);
}

int
ibcs2_sys_stat(struct lwp *l, const struct ibcs2_sys_stat_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct ibcs2_stat *) st;
	} */
	struct stat sb;
	struct ibcs2_stat ibcs2_st;
	int error;

	error = do_sys_stat(SCARG(uap, path), FOLLOW, &sb);
	if (error != 0)
		return error;

	bsd_stat2ibcs_stat(&sb, &ibcs2_st);
	return copyout(&ibcs2_st, SCARG(uap, st), sizeof (ibcs2_st));
}

int
ibcs2_sys_lstat(struct lwp *l, const struct ibcs2_sys_lstat_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct ibcs2_stat *) st;
	} */
	struct stat sb;
	struct ibcs2_stat ibcs2_st;
	int error;

	error = do_sys_stat(SCARG(uap, path), NOFOLLOW, &sb);
	if (error != 0)
		return error;

	bsd_stat2ibcs_stat(&sb, &ibcs2_st);
	return copyout((void *)&ibcs2_st, (void *)SCARG(uap, st),
		       sizeof (ibcs2_st));
}

int
ibcs2_sys_fstat(struct lwp *l, const struct ibcs2_sys_fstat_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(struct ibcs2_stat *) st;
	} */
	struct stat sb;
	struct ibcs2_stat ibcs2_st;
	int error;

	error = do_sys_fstat(SCARG(uap, fd), &sb);
	if (error != 0)
		return error;

	bsd_stat2ibcs_stat(&sb, &ibcs2_st);
	return copyout(&ibcs2_st, SCARG(uap, st), sizeof (ibcs2_st));
}

int
ibcs2_sys_utssys(struct lwp *l, const struct ibcs2_sys_utssys_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) a1;
		syscallarg(int) a2;
		syscallarg(int) flag;
	} */

	switch (SCARG(uap, flag)) {
	case 0:			/* uname(struct utsname *) */
	{
		struct ibcs2_utsname sut;

		memset(&sut, 0, ibcs2_utsname_len);
		memcpy(sut.sysname, ostype, sizeof(sut.sysname) - 1);
		memcpy(sut.nodename, hostname, sizeof(sut.nodename));
		sut.nodename[sizeof(sut.nodename)-1] = '\0';
		memcpy(sut.release, osrelease, sizeof(sut.release) - 1);
		strlcpy(sut.version, "1", sizeof(sut.version));
		memcpy(sut.machine, machine, sizeof(sut.machine) - 1);

		return copyout((void *)&sut, (void *)SCARG(uap, a1),
			       ibcs2_utsname_len);
	}

	case 2:			/* ustat(dev_t, struct ustat *) */
	{
		struct ibcs2_ustat xu;

		xu.f_tfree = 20000; /* XXX fixme */
		xu.f_tinode = 32000;
		xu.f_fname[0] = 0;
		xu.f_fpack[0] = 0;
		return copyout((void *)&xu, (void *)SCARG(uap, a2),
                               ibcs2_ustat_len);
	}

	default:
		return ENOSYS;
	}
}
