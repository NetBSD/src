/*	$NetBSD: netbsd32_compat_30.c,v 1.30.18.1 2017/12/03 11:36:55 jdolecek Exp $	*/

/*
 * Copyright (c) 1998, 2001 Matthew R. Green
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_compat_30.c,v 1.30.18.1 2017/12/03 11:36:55 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ktrace.h>
#include <sys/resourcevar.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/namei.h>
#include <sys/statvfs.h>
#include <sys/syscallargs.h>
#include <sys/proc.h>
#include <sys/dirent.h>
#include <sys/kauth.h>
#include <sys/vfs_syscalls.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>
#include <compat/sys/mount.h>


int
compat_30_netbsd32_getdents(struct lwp *l, const struct compat_30_netbsd32_getdents_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) buf;
		syscallarg(netbsd32_size_t) count;
	} */
	file_t *fp;
	int error, done;
	char  *buf;
	netbsd32_size_t count;

	/* Limit the size on any kernel buffers used by VOP_READDIR */
	count = min(MAXBSIZE, SCARG(uap, count));

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return (error);
	if ((fp->f_flag & FREAD) == 0) {
		error = EBADF;
		goto out;
	}
	if (count == 0)
		goto out;

	buf = kmem_alloc(count, KM_SLEEP);
	error = vn_readdir(fp, buf, UIO_SYSSPACE, count, &done, l, 0, 0);
	if (error == 0) {
		*retval = netbsd32_to_dirent12(buf, done);
		error = copyout(buf, SCARG_P32(uap, buf), *retval);
	}
	kmem_free(buf, count);
 out:
 	fd_putfile(SCARG(uap, fd));
	return (error);
}

int
compat_30_netbsd32___stat13(struct lwp *l, const struct compat_30_netbsd32___stat13_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_stat13p_t) ub;
	} */
	struct netbsd32_stat13 sb32;
	struct stat sb;
	int error;
	const char *path;

	path = SCARG_P32(uap, path);

	error = do_sys_stat(path, FOLLOW, &sb);
	if (error)
		return (error);
	netbsd32_from___stat13(&sb, &sb32);
	error = copyout(&sb32, SCARG_P32(uap, ub), sizeof(sb32));
	return (error);
}

int
compat_30_netbsd32___fstat13(struct lwp *l, const struct compat_30_netbsd32___fstat13_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_stat13p_t) sb;
	} */
	struct netbsd32_stat13 sb32;
	struct stat ub;
	int error;

	error = do_sys_fstat(SCARG(uap, fd), &ub);
	if (error == 0) {
		netbsd32_from___stat13(&ub, &sb32);
		error = copyout(&sb32, SCARG_P32(uap, sb), sizeof(sb32));
	}
	return (error);
}

int
compat_30_netbsd32___lstat13(struct lwp *l, const struct compat_30_netbsd32___lstat13_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_stat13p_t) ub;
	} */
	struct netbsd32_stat13 sb32;
	struct stat sb;
	int error;
	const char *path;

	path = SCARG_P32(uap, path);

	error = do_sys_stat(path, NOFOLLOW, &sb);
	if (error)
		return (error);
	netbsd32_from___stat13(&sb, &sb32);
	error = copyout(&sb32, SCARG_P32(uap, ub), sizeof(sb32));
	return (error);
}

int
compat_30_netbsd32_fhstat(struct lwp *l, const struct compat_30_netbsd32_fhstat_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_fhandlep_t) fhp;
		syscallarg(netbsd32_stat13p_t) sb;
	} */
	struct stat sb;
	struct netbsd32_stat13 sb32;
	int error;
	struct compat_30_fhandle fh;
	struct mount *mp;
	struct vnode *vp;

	/*
	 * Must be super user
	 */
	if ((error = kauth_authorize_system(l->l_cred,
	    KAUTH_SYSTEM_FILEHANDLE, 0, NULL, NULL, NULL)))
		return (error);

	if ((error = copyin(SCARG_P32(uap, fhp), &fh, sizeof(fh))) != 0)
		return (error);

	if ((mp = vfs_getvfs(&fh.fh_fsid)) == NULL)
		return (ESTALE);
	if (mp->mnt_op->vfs_fhtovp == NULL)
		return EOPNOTSUPP;
	if ((error = VFS_FHTOVP(mp, (struct fid*)&fh.fh_fid, &vp)))
		return (error);
	error = vn_stat(vp, &sb);
	vput(vp);
	if (error)
		return (error);
	netbsd32_from___stat13(&sb, &sb32);
	error = copyout(&sb32, SCARG_P32(uap, sb), sizeof(sb));
	return (error);
}

int
compat_30_netbsd32_fhstatvfs1(struct lwp *l, const struct compat_30_netbsd32_fhstatvfs1_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_fhandlep_t) fhp;
		syscallarg(netbsd32_statvfsp_t) buf;
		syscallarg(int) flags;
	} */
	struct statvfs *sbuf;
	struct netbsd32_statvfs *s32;
	int error;

	sbuf = STATVFSBUF_GET();
	error = do_fhstatvfs(l, SCARG_P32(uap, fhp), FHANDLE_SIZE_COMPAT, sbuf,
	    SCARG(uap, flags));

	if (error != 0) {
		s32 = kmem_alloc(sizeof(*s32), KM_SLEEP);
		netbsd32_from_statvfs(sbuf, s32);
		error = copyout(s32, SCARG_P32(uap, buf), sizeof *s32);
		kmem_free(s32, sizeof(*s32));
	}
	STATVFSBUF_PUT(sbuf);

	return (error);
}

int
compat_30_netbsd32_socket(struct lwp *l, const struct compat_30_netbsd32_socket_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
	} */
	struct compat_30_sys_socket_args ua;

	NETBSD32TO64_UAP(domain);
	NETBSD32TO64_UAP(type);
	NETBSD32TO64_UAP(protocol);
	return (compat_30_sys_socket(l, &ua, retval));
}

int
compat_30_netbsd32_getfh(struct lwp *l, const struct compat_30_netbsd32_getfh_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) fname;
		syscallarg(netbsd32_compat_30_fhandlep_t) fhp;
	} */
	struct compat_30_sys_getfh_args ua;

	NETBSD32TOP_UAP(fname, const char);
	NETBSD32TOP_UAP(fhp, struct compat_30_fhandle);
	/* Lucky for us a fhandle_t doesn't change sizes */
	return (compat_30_sys_getfh(l, &ua, retval));
}


int
compat_30_netbsd32___fhstat30(struct lwp *l, const struct compat_30_netbsd32___fhstat30_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_fhandlep_t) fhp;
		syscallarg(netbsd32_statp_t) sb;
	} */
	struct stat sb;
	struct netbsd32_stat50 sb32;
	int error;

	error = do_fhstat(l, SCARG_P32(uap, fhp), FHANDLE_SIZE_COMPAT, &sb);
	if (error)
		return error;

	netbsd32_from___stat50(&sb, &sb32);
	error = copyout(&sb32, SCARG_P32(uap, sb), sizeof(sb32));
	return error;
}

/*
 * Open a file given a file handle.
 *
 * Check permissions, allocate an open file structure,
 * and call the device open routine if any.
 */
int
compat_30_netbsd32_fhopen(struct lwp *l, const struct compat_30_netbsd32_fhopen_args *uap, register_t *retval)
{
	/* {
		syscallarg(const fhandle_t *) fhp;
		syscallarg(int) flags;
	} */
	struct compat_30_sys_fhopen_args ua;

	NETBSD32TOP_UAP(fhp, struct compat_30_fhandle);
	NETBSD32TO64_UAP(flags);
	return (compat_30_sys_fhopen(l, &ua, retval));
}
