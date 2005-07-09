/*	$NetBSD: vfs_xattr.c,v 1.1 2005/07/09 01:05:24 thorpej Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 */

/*
 * VFS extended attribute support.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_xattr.c,v 1.1 2005/07/09 01:05:24 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/extattr.h>
#include <sys/sysctl.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

/*
 * Push extended attribute configuration information into the VFS.
 *
 * NOTE: Not all file systems that support extended attributes will
 * require the use of this system call.
 */
int
sys_extattrctl(struct lwp *l, void *v, register_t *retval)
{
	struct sys_extattrctl_args /* {
		syscallarg(const char *) path;
		syscallarg(int) cmd;
		syscallarg(const char *) filename;
		syscallarg(int) attrnamespace;
		syscallarg(const char *) attrname;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct vnode *vp;
	struct nameidata nd;
	struct mount *mp;
	char attrname[EXTATTR_MAXNAMELEN];
	int error;

	if (SCARG(uap, attrname) != NULL) {
		error = copyinstr(SCARG(uap, attrname), attrname,
		    sizeof(attrname), NULL);
		if (error)
			return (error);
	}

	vp = NULL;
	if (SCARG(uap, filename) != NULL) {
		NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE,
		    SCARG(uap, filename), p);
		error = namei(&nd);
		if (error)
			return (error);
		vp = nd.ni_vp;
	}

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	error = namei(&nd);
	if (error) {
		if (vp != NULL)
			vput(vp);
		return (error);
	}

	error = vn_start_write(nd.ni_vp, &mp, V_WAIT | V_PCATCH);
	if (error) {
		if (vp != NULL)
			vput(vp);
		return (error);
	}

	error = VFS_EXTATTRCTL(mp, SCARG(uap, cmd), vp,
	    SCARG(uap, attrnamespace),
	    SCARG(uap, attrname) != NULL ? attrname : NULL, p);

	vn_finished_write(mp, 0);

	if (vp != NULL)
		vrele(vp);

	return (error);
}

/*****************************************************************************
 * Internal routines to manipulate file system extended attributes:
 *	- set
 *	- get
 *	- delete
 *	- list
 *****************************************************************************/

/*
 * extattr_set_vp:
 *
 *	Set a named extended attribute on a file or directory.
 */
static int
extattr_set_vp(struct vnode *vp, int attrnamespace, const char *attrname,
    const void *data, size_t nbytes, struct proc *p, register_t *retval)
{
	struct mount *mp;
	struct uio auio;
	struct iovec aiov;
	ssize_t cnt;
	int error;

	error = vn_start_write(vp, &mp, V_WAIT | V_PCATCH);
	if (error)
		return (error);
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);

	aiov.iov_base = __UNCONST(data);	/* XXXUNCONST kills const */
	aiov.iov_len = nbytes;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = 0;
	if (nbytes > INT_MAX) {
		error = EINVAL;
		goto done;
	}
	auio.uio_resid = nbytes;
	auio.uio_rw = UIO_WRITE;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_procp = p;
	cnt = nbytes;

	error = VOP_SETEXTATTR(vp, attrnamespace, attrname, &auio,
	    p->p_ucred, p);
	cnt -= auio.uio_resid;
	retval[0] = cnt;

 done:
	VOP_UNLOCK(vp, 0);
	vn_finished_write(mp, 0);
	return (error);
}

/*
 * extattr_get_vp:
 *
 *	Get a named extended attribute on a file or directory.
 */
static int
extattr_get_vp(struct vnode *vp, int attrnamespace, const char *attrname,
    void *data, size_t nbytes, struct proc *p, register_t *retval)
{
	struct uio auio, *auiop;
	struct iovec aiov;
	ssize_t cnt;
	size_t size, *sizep;
	int error;

	VOP_LEASE(vp, p, p->p_ucred, LEASE_READ);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);

	/*
	 * Slightly unusual semantics: if the user provides a NULL data
	 * pointer, they don't want to receive the data, just the maximum
	 * read length.
	 */
	auiop = NULL;
	sizep = NULL;
	cnt = 0;
	if (data != NULL) {
		aiov.iov_base = data;
		aiov.iov_len = nbytes;
		auio.uio_iov = &aiov;
		auio.uio_offset = 0;
		if (nbytes > INT_MAX) {
			error = EINVAL;
			goto done;
		}
		auio.uio_resid = nbytes;
		auio.uio_rw = UIO_READ;
		auio.uio_segflg = UIO_USERSPACE;
		auio.uio_procp = p;
		auiop = &auio;
		cnt = nbytes;
	} else
		sizep = &size;

	error = VOP_GETEXTATTR(vp, attrnamespace, attrname, auiop, sizep,
	    p->p_ucred, p);

	if (auiop != NULL) {
		cnt -= auio.uio_resid;
		retval[0] = cnt;
	} else
		retval[0] = size;

 done:
	VOP_UNLOCK(vp, 0);
	return (error);
}

/*
 * extattr_delete_vp:
 *
 *	Delete a named extended attribute on a file or directory.
 */
static int
extattr_delete_vp(struct vnode *vp, int attrnamespace, const char *attrname,
    struct proc *p)
{
	struct mount *mp;
	int error;

	error = vn_start_write(vp, &mp, V_WAIT | V_PCATCH);
	if (error)
		return (error);
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);

	error = VOP_DELETEEXTATTR(vp, attrnamespace, attrname, p->p_ucred, p);
	if (error == EOPNOTSUPP)
		error = VOP_SETEXTATTR(vp, attrnamespace, attrname, NULL,
		    p->p_ucred, p);

	VOP_UNLOCK(vp, 0);
	vn_finished_write(mp, 0);
	return (error);
}

/*
 * extattr_list_vp:
 *
 *	Retrieve a list of extended attributes on a file or directory.
 */
static int
extattr_list_vp(struct vnode *vp, int attrnamespace, void *data, size_t nbytes,
    struct proc *p, register_t *retval)
{
	struct uio auio, *auiop;
	size_t size, *sizep;
	struct iovec aiov;
	ssize_t cnt;
	int error;

	VOP_LEASE(vp, p, p->p_ucred, LEASE_READ);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);

	auiop = NULL;
	sizep = NULL;
	cnt = 0;
	if (data != NULL) {
		aiov.iov_base = data;
		aiov.iov_len = nbytes;
		auio.uio_iov = &aiov;
		auio.uio_offset = 0;
		if (nbytes > INT_MAX) {
			error = EINVAL;
			goto done;
		}
		auio.uio_resid = nbytes;
		auio.uio_rw = UIO_READ;
		auio.uio_segflg = UIO_USERSPACE;
		auio.uio_procp = p;
		auiop = &auio;
		cnt = nbytes;
	} else
		sizep = &size;

	error = VOP_LISTEXTATTR(vp, attrnamespace, auiop, sizep,
	    p->p_ucred, p);

	if (auiop != NULL) {
		cnt -= auio.uio_resid;
		retval[0] = cnt;
	} else
		retval[0] = size;

 done:
	VOP_UNLOCK(vp, 0);
	return (error);
}

/*****************************************************************************
 * BSD <sys/extattr.h> API for file system extended attributes
 *****************************************************************************/

int
sys_extattr_set_fd(struct lwp *l, void *v, register_t *retval)
{
	struct sys_extattr_set_fd_args /* {
		syscallarg(int) fd;
		syscallarg(int) attrnamespace;
		syscallarg(const char *) attrname;
		syscallarg(const void *) data;
		syscallarg(size_t) nbytes;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct file *fp;
	struct vnode *vp;
	char attrname[EXTATTR_MAXNAMELEN];
	int error;

	error = copyinstr(SCARG(uap, attrname), attrname, sizeof(attrname),
	    NULL);
	if (error)
		return (error);

	error = getvnode(p->p_fd, SCARG(uap, fd), &fp);
	if (error)
		return (error);
	vp = (struct vnode *) fp->f_data;

	error = extattr_set_vp(vp, SCARG(uap, attrnamespace), attrname,
	    SCARG(uap, data), SCARG(uap, nbytes), p, retval);

	FILE_UNUSE(fp, p);
	return (error);
}

int
sys_extattr_set_file(struct lwp *l, void *v, register_t *retval)
{
	struct sys_extattr_set_file_args /* {
		syscallarg(const char *) path;
		syscallarg(int) attrnamespace;
		syscallarg(const char *) attrname;
		syscallarg(const void *) data;
		syscallarg(size_t) nbytes;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct nameidata nd;
	char attrname[EXTATTR_MAXNAMELEN];
	int error;

	error = copyinstr(SCARG(uap, attrname), attrname, sizeof(attrname),
	    NULL);
	if (error)
		return (error);

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	error = namei(&nd);
	if (error)
		return (error);

	error = extattr_set_vp(nd.ni_vp, SCARG(uap, attrnamespace), attrname,
	    SCARG(uap, data), SCARG(uap, nbytes), p, retval);

	vrele(nd.ni_vp);
	return (error);
}

int
sys_extattr_set_link(struct lwp *l, void *v, register_t *retval)
{
	struct sys_extattr_set_link_args /* {
		syscallarg(const char *) path;
		syscallarg(int) attrnamespace;
		syscallarg(const char *) attrname;
		syscallarg(const void *) data;
		syscallarg(size_t) nbytes;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct nameidata nd;
	char attrname[EXTATTR_MAXNAMELEN];
	int error;

	error = copyinstr(SCARG(uap, attrname), attrname, sizeof(attrname),
	    NULL);
	if (error)
		return (error);

	NDINIT(&nd, LOOKUP, NOFOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	error = namei(&nd);
	if (error)
		return (error);

	error = extattr_set_vp(nd.ni_vp, SCARG(uap, attrnamespace), attrname,
	    SCARG(uap, data), SCARG(uap, nbytes), p, retval);

	vrele(nd.ni_vp);
	return (error);
}

int
sys_extattr_get_fd(struct lwp *l, void *v, register_t *retval)
{
	struct sys_extattr_get_fd_args /* {
		syscallarg(int) fd;
		syscallarg(int) attrnamespace;
		syscallarg(const char *) attrname;
		syscallarg(void *) data;
		syscallarg(size_t) nbytes;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct file *fp;
	struct vnode *vp;
	char attrname[EXTATTR_MAXNAMELEN];
	int error;

	error = copyinstr(SCARG(uap, attrname), attrname, sizeof(attrname),
	    NULL);
	if (error)
		return (error);

	error = getvnode(p->p_fd, SCARG(uap, fd), &fp);
	if (error)
		return (error);
	vp = (struct vnode *) fp->f_data;

	error = extattr_get_vp(vp, SCARG(uap, attrnamespace), attrname,
	    SCARG(uap, data), SCARG(uap, nbytes), p, retval);

	FILE_UNUSE(fp, p);
	return (error);
}

int
sys_extattr_get_file(struct lwp *l, void *v, register_t *retval)
{
	struct sys_extattr_get_file_args /* {
		syscallarg(const char *) path;
		syscallarg(int) attrnamespace;
		syscallarg(const char *) attrname;
		syscallarg(void *) data;
		syscallarg(size_t) nbytes;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct nameidata nd;
	char attrname[EXTATTR_MAXNAMELEN];
	int error;

	error = copyinstr(SCARG(uap, attrname), attrname, sizeof(attrname),
	    NULL);
	if (error)
		return (error);

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	error = namei(&nd);
	if (error)
		return (error);

	error = extattr_get_vp(nd.ni_vp, SCARG(uap, attrnamespace), attrname,
	    SCARG(uap, data), SCARG(uap, nbytes), p, retval);

	vrele(nd.ni_vp);
	return (error);
}

int
sys_extattr_get_link(struct lwp *l, void *v, register_t *retval)
{
	struct sys_extattr_get_link_args /* {
		syscallarg(const char *) path;
		syscallarg(int) attrnamespace;
		syscallarg(const char *) attrname;
		syscallarg(void *) data;
		syscallarg(size_t) nbytes;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct nameidata nd;
	char attrname[EXTATTR_MAXNAMELEN];
	int error;

	error = copyinstr(SCARG(uap, attrname), attrname, sizeof(attrname),
	    NULL);
	if (error)
		return (error);

	NDINIT(&nd, LOOKUP, NOFOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	error = namei(&nd);
	if (error)
		return (error);

	error = extattr_get_vp(nd.ni_vp, SCARG(uap, attrnamespace), attrname,
	    SCARG(uap, data), SCARG(uap, nbytes), p, retval);

	vrele(nd.ni_vp);
	return (error);
}

int
sys_extattr_delete_fd(struct lwp *l, void *v, register_t *retval)
{
	struct sys_extattr_delete_fd_args /* {
		syscallarg(int) fd;
		syscallarg(int) attrnamespace;
		syscallarg(const char *) attrname;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct file *fp;
	struct vnode *vp;
	char attrname[EXTATTR_MAXNAMELEN];
	int error;

	error = copyinstr(SCARG(uap, attrname), attrname, sizeof(attrname),
	    NULL);
	if (error)
		return (error);

	error = getvnode(p->p_fd, SCARG(uap, fd), &fp);
	if (error)
		return (error);
	vp = (struct vnode *) fp->f_data;

	error = extattr_delete_vp(vp, SCARG(uap, attrnamespace), attrname, p);

	FILE_UNUSE(fp, p);
	return (error);
}

int
sys_extattr_delete_file(struct lwp *l, void *v, register_t *retval)
{
	struct sys_extattr_delete_file_args /* {
		syscallarg(const char *) path;
		syscallarg(int) attrnamespace;
		syscallarg(const char *) attrname;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct nameidata nd;
	char attrname[EXTATTR_MAXNAMELEN];
	int error;

	error = copyinstr(SCARG(uap, attrname), attrname, sizeof(attrname),
	    NULL);
	if (error)
		return (error);

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	error = namei(&nd);
	if (error)
		return (error);

	error = extattr_delete_vp(nd.ni_vp, SCARG(uap, attrnamespace), attrname,
	    p);

	vrele(nd.ni_vp);
	return (error);
}

int
sys_extattr_delete_link(struct lwp *l, void *v, register_t *retval)
{
	struct sys_extattr_delete_link_args /* {
		syscallarg(const char *) path;
		syscallarg(int) attrnamespace;
		syscallarg(const char *) attrname;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct nameidata nd;
	char attrname[EXTATTR_MAXNAMELEN];
	int error;

	error = copyinstr(SCARG(uap, attrname), attrname, sizeof(attrname),
	    NULL);
	if (error)
		return (error);

	NDINIT(&nd, LOOKUP, NOFOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	error = namei(&nd);
	if (error)
		return (error);

	error = extattr_delete_vp(nd.ni_vp, SCARG(uap, attrnamespace), attrname,
	    p);

	vrele(nd.ni_vp);
	return (error);
}

int
sys_extattr_list_fd(struct lwp *l, void *v, register_t *retval)
{
	struct sys_extattr_list_fd_args /* {
		syscallarg(int) fd;
		syscallarg(int) attrnamespace;
		syscallarg(void *) data;
		syscallarg(size_t) nbytes;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct file *fp;
	struct vnode *vp;
	int error;

	error = getvnode(p->p_fd, SCARG(uap, fd), &fp);
	if (error)
		return (error);
	vp = (struct vnode *) fp->f_data;

	error = extattr_list_vp(vp, SCARG(uap, attrnamespace),
	    SCARG(uap, data), SCARG(uap, nbytes), p, retval);

	FILE_UNUSE(fp, p);
	return (error);
}

int
sys_extattr_list_file(struct lwp *l, void *v, register_t *retval)
{
	struct sys_extattr_list_file_args /* {
		syscallarg(const char *) path;
		syscallarg(int) attrnamespace;
		syscallarg(void *) data;
		syscallarg(size_t) nbytes;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct nameidata nd;
	int error;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	error = namei(&nd);
	if (error)
		return (error);

	error = extattr_list_vp(nd.ni_vp, SCARG(uap, attrnamespace),
	    SCARG(uap, data), SCARG(uap, nbytes), p, retval);

	vrele(nd.ni_vp);
	return (error);
}

int
sys_extattr_list_link(struct lwp *l, void *v, register_t *retval)
{
	struct sys_extattr_list_link_args /* {
		syscallarg(const char *) path;
		syscallarg(int) attrnamespace;
		syscallarg(void *) data;
		syscallarg(size_t) nbytes;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct nameidata nd;
	int error;

	NDINIT(&nd, LOOKUP, NOFOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	error = namei(&nd);
	if (error)
		return (error);

	error = extattr_list_vp(nd.ni_vp, SCARG(uap, attrnamespace),
	    SCARG(uap, data), SCARG(uap, nbytes), p, retval);

	vrele(nd.ni_vp);
	return (error);
}
