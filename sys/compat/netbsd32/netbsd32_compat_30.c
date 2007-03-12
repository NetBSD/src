/*	$NetBSD: netbsd32_compat_30.c,v 1.16.2.1 2007/03/12 05:52:31 rmind Exp $	*/

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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_compat_30.c,v 1.16.2.1 2007/03/12 05:52:31 rmind Exp $");

#include "opt_nfsserver.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
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
compat_30_netbsd32_getdents(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_30_netbsd32_getdents_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) buf;
		syscallarg(netbsd32_size_t) count;
	} */ *uap = v;
	struct file *fp;
	int error, done;
	char  *buf;
	netbsd32_size_t count;
	struct proc *p = l->l_proc;

	/* Limit the size on any kernel buffers used by VOP_READDIR */
	count = min(MAXBSIZE, SCARG(uap, count));

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	if ((fp->f_flag & FREAD) == 0) {
		error = EBADF;
		goto out;
	}
	buf = malloc(count, M_TEMP, M_WAITOK);
	error = vn_readdir(fp, buf, UIO_SYSSPACE, count, &done, l, 0, 0);
	if (error == 0) {
		*retval = netbsd32_to_dirent12(buf, done);
		error = copyout(buf, NETBSD32PTR64(SCARG(uap, buf)), *retval);
	}
	free(buf, M_TEMP);
 out:
	FILE_UNUSE(fp, l);
	return (error);
}

int
compat_30_netbsd32___stat13(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_30_netbsd32___stat13_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_stat13p_t) ub;
	} */ *uap = v;
	struct netbsd32_stat13 sb32;
	struct stat sb;
	int error;
	void *sg;
	const char *path;
	struct proc *p = l->l_proc;

	path = (char *)NETBSD32PTR64(SCARG(uap, path));
	sg = stackgap_init(p, 0);
	CHECK_ALT_EXIST(l, &sg, path);

	error = do_sys_stat(l, path, FOLLOW, &sb);
	if (error)
		return (error);
	netbsd32_from___stat13(&sb, &sb32);
	error = copyout(&sb32, (void *)NETBSD32PTR64(SCARG(uap, ub)),
	    sizeof(sb32));
	return (error);
}

int
compat_30_netbsd32___fstat13(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_30_netbsd32___fstat13_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_stat13p_t) sb;
	} */ *uap = v;
	int fd = SCARG(uap, fd);
	struct proc *p = l->l_proc;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct netbsd32_stat13 sb32;
	struct stat ub;
	int error = 0;

	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	FILE_USE(fp);
	error = (*fp->f_ops->fo_stat)(fp, &ub, l);
	FILE_UNUSE(fp, l);

	if (error == 0) {
		netbsd32_from___stat13(&ub, &sb32);
		error = copyout(&sb32, (void *)NETBSD32PTR64(SCARG(uap, sb)),
		    sizeof(sb32));
	}
	return (error);
}

int
compat_30_netbsd32___lstat13(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_30_netbsd32___lstat13_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_stat13p_t) ub;
	} */ *uap = v;
	struct netbsd32_stat13 sb32;
	struct stat sb;
	int error;
	void *sg;
	const char *path;
	struct proc *p = l->l_proc;

	path = (char *)NETBSD32PTR64(SCARG(uap, path));
	sg = stackgap_init(p, 0);
	CHECK_ALT_EXIST(l, &sg, path);

	error = do_sys_stat(l, path, NOFOLLOW, &sb);
	if (error)
		return (error);
	netbsd32_from___stat13(&sb, &sb32);
	error = copyout(&sb32, (void *)NETBSD32PTR64(SCARG(uap, ub)),
	    sizeof(sb32));
	return (error);
}

int
compat_30_netbsd32_fhstat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_30_netbsd32_fhstat_args /* {
		syscallarg(const netbsd32_fhandlep_t) fhp;
		syscallarg(netbsd32_stat13p_t) sb);
	} */ *uap = v;
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

	if ((error = copyin(NETBSD32PTR64(SCARG(uap, fhp)), &fh,
	    sizeof(fh))) != 0)
		return (error);

	if ((mp = vfs_getvfs(&fh.fh_fsid)) == NULL)
		return (ESTALE);
	if (mp->mnt_op->vfs_fhtovp == NULL)
		return EOPNOTSUPP;
	if ((error = VFS_FHTOVP(mp, (struct fid*)&fh.fh_fid, &vp)))
		return (error);
	error = vn_stat(vp, &sb, l);
	vput(vp);
	if (error)
		return (error);
	netbsd32_from___stat13(&sb, &sb32);
	error = copyout(&sb32, NETBSD32PTR64(SCARG(uap, sb)), sizeof(sb));
	return (error);
}

int
compat_30_netbsd32_fhstatvfs1(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_30_netbsd32_fhstatvfs1_args /* {
		syscallarg(const netbsd32_fhandlep_t) fhp;
		syscallarg(netbsd32_statvfsp_t) buf;
		syscallarg(int) flags;
	} */ *uap = v;
	struct statvfs *sbuf;
	struct netbsd32_statvfs *s32;
	fhandle_t *fh;
	struct vnode *vp;
	int error;

	/*
	 * Must be super user
	 */
	if ((error = kauth_authorize_system(l->l_cred,
	    KAUTH_SYSTEM_FILEHANDLE, 0, NULL, NULL, NULL)) != 0)
		return error;

	if ((error = vfs_copyinfh_alloc(NETBSD32PTR64(SCARG(uap, fhp)), 
	    FHANDLE_SIZE_COMPAT, &fh)) != 0)
		goto bad;
	if ((error = vfs_fhtovp(fh, &vp)) != 0)
		goto bad;

	sbuf = (struct statvfs *)malloc(sizeof(struct statvfs), M_TEMP,
	    M_WAITOK);
	error = dostatvfs(vp->v_mount, sbuf, l, SCARG(uap, flags), 1);
	vput(vp);
	if (error != 0)
		goto out;

	s32 = (struct netbsd32_statvfs *)
	    malloc(sizeof(struct netbsd32_statvfs), M_TEMP, M_WAITOK);
	netbsd32_from_statvfs(sbuf, s32);
	error = copyout(s32, (void *)NETBSD32PTR64(SCARG(uap, buf)),
	    sizeof(struct netbsd32_statvfs));
	free(s32, M_TEMP);

out:
	free(sbuf, M_TEMP);
bad:
	vfs_copyinfh_free(fh);
	return (error);
}

int
compat_30_netbsd32_socket(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_30_netbsd32_socket_args /* {
		syscallarg(int) domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
	} */ *uap = v;
	struct compat_30_sys_socket_args ua;

	NETBSD32TO64_UAP(domain);
	NETBSD32TO64_UAP(type);
	NETBSD32TO64_UAP(protocol);
	return (compat_30_sys_socket(l, &ua, retval));
}

int
compat_30_netbsd32_getfh(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_30_netbsd32_getfh_args /* {
		syscallarg(const netbsd32_charp) fname;
		syscallarg(netbsd32_compat_30_fhandlep_t) fhp;
	} */ *uap = v;
	struct compat_30_sys_getfh_args ua;

	NETBSD32TOP_UAP(fname, const char);
	NETBSD32TOP_UAP(fhp, struct compat_30_fhandle);
	/* Lucky for us a fhandle_t doesn't change sizes */
	return (compat_30_sys_getfh(l, &ua, retval));
}


int compat_30_netbsd32_sys___fhstat30(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_30_netbsd32_sys___fhstat30_args /* {
		syscallarg(const netbsd32_fhandlep_t) fhp;
		syscallarg(netbsd32_statp_t) sb;
	} */ *uap = v;
	struct stat sb;
	struct netbsd32_stat sb32;
	int error;
	fhandle_t *fh;
	struct vnode *vp;

	/*
	 * Must be super user
	 */
	if ((error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_FILEHANDLE,
	    0, NULL, NULL, NULL)))
		return error;

	if ((error = vfs_copyinfh_alloc(NETBSD32PTR64(SCARG(uap, fhp)),
	    FHANDLE_SIZE_COMPAT, &fh)) != 0)
		goto bad;

	if ((error = vfs_fhtovp(fh, &vp)) != 0)
		goto bad;

	error = vn_stat(vp, &sb, l);
	vput(vp);
	if (error)
		goto bad;
	netbsd32_from___stat30(&sb, &sb32);
	error = copyout(&sb32, NETBSD32PTR64(SCARG(uap, sb)), sizeof(sb));
bad:
	vfs_copyinfh_free(fh);
	return error;
}

/*
 * Open a file given a file handle.
 *
 * Check permissions, allocate an open file structure,
 * and call the device open routine if any.
 */
int
compat_30_netbsd32_fhopen(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_30_netbsd32_fhopen_args /* {
		syscallarg(const fhandle_t *) fhp;
		syscallarg(int) flags;
	} */ *uap = v;
	struct compat_30_sys_fhopen_args ua;

	NETBSD32TOP_UAP(fhp, struct compat_30_fhandle);
	NETBSD32TO64_UAP(flags);
	return (compat_30_sys_fhopen(l, &ua, retval));
}
