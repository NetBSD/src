/*	$NetBSD: netbsd32_acl.c,v 1.1 2020/05/16 18:31:48 christos Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_acl.c,v 1.1 2020/05/16 18:31:48 christos Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/lwp.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/acl.h>
#include <sys/syscallargs.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

/* Syscalls conversion */
int
netbsd32___acl_get_file(struct lwp *l,
     const struct netbsd32___acl_get_file_args *uap, register_t *retval)
{

	return kern___acl_get_path(l, SCARG_P32(uap, path), SCARG(uap, type),
	    SCARG_P32(uap, aclp), NSM_FOLLOW_NOEMULROOT);
}

/*
 * Given a file path, get an ACL for it; don't follow links.
 */
int
netbsd32___acl_get_link(struct lwp *l,
    const struct netbsd32___acl_get_link_args *uap, register_t *retval)
{

	return kern___acl_get_path(l, SCARG_P32(uap, path), SCARG(uap, type),
	    SCARG_P32(uap, aclp), NSM_NOFOLLOW_NOEMULROOT);
}

/*
 * Given a file path, set an ACL for it.
 */
int
netbsd32___acl_set_file(struct lwp *l,
    const struct netbsd32___acl_set_file_args *uap, register_t *retval)
{

	return kern___acl_set_path(l, SCARG_P32(uap, path), SCARG(uap, type),
	    SCARG_P32(uap, aclp), NSM_FOLLOW_NOEMULROOT);
}

/*
 * Given a file path, set an ACL for it; don't follow links.
 */
int
netbsd32___acl_set_link(struct lwp *l,
    const struct netbsd32___acl_set_link_args *uap, register_t *retval)
{

	return kern___acl_set_path(l, SCARG_P32(uap, path), SCARG(uap, type),
	    SCARG_P32(uap, aclp), NSM_NOFOLLOW_NOEMULROOT);
}

/*
 * Given a file descriptor, get an ACL for it.
 */
int
netbsd32___acl_get_fd(struct lwp *l,
    const struct netbsd32___acl_get_fd_args *uap, register_t *retval)
{
	struct file *fp;
	int error;
	error = fd_getvnode(SCARG(uap, filedes), &fp);
	if (error == 0) {
		error = vacl_get_acl(l, fp->f_vnode, SCARG(uap, type),
		    SCARG_P32(uap, aclp));
		fd_putfile(SCARG(uap, filedes));
	}
	return error;
}

/*
 * Given a file descriptor, set an ACL for it.
 */
int
netbsd32___acl_set_fd(struct lwp *l,
    const struct netbsd32___acl_set_fd_args *uap, register_t *retval)
{
	struct file *fp;
	int error;

	error = fd_getvnode(SCARG(uap, filedes), &fp);
	if (error == 0) {
		error = vacl_set_acl(l, fp->f_vnode, SCARG(uap, type),
		    SCARG_P32(uap, aclp));
		fd_putfile(SCARG(uap, filedes));
	}
	return error;
}

/*
 * Given a file path, delete an ACL from it.
 */
int
netbsd32___acl_delete_file(struct lwp *l,
    const struct netbsd32___acl_delete_file_args *uap, register_t *retval)
{

	return kern___acl_delete_path(l, SCARG_P32(uap, path), SCARG(uap, type),
	    NSM_FOLLOW_NOEMULROOT);
}

/*
 * Given a file path, delete an ACL from it; don't follow links.
 */
int
netbsd32___acl_delete_link(struct lwp *l,
    const struct netbsd32___acl_delete_link_args *uap, register_t *retval)
{
	return kern___acl_delete_path(l, SCARG_P32(uap, path), SCARG(uap, type),
	    NSM_NOFOLLOW_NOEMULROOT);
}

/*
 * Given a file path, delete an ACL from it.
 */
int
netbsd32___acl_delete_fd(struct lwp *l,
    const struct netbsd32___acl_delete_fd_args *uap, register_t *retval)
{
	struct file *fp;
	int error;

	error = fd_getvnode(SCARG(uap, filedes), &fp);
	if (error == 0) {
		error = vacl_delete(l, fp->f_vnode, SCARG(uap, type));
		fd_putfile(SCARG(uap, filedes));
	}
	return error;
}

/*
 * Given a file path, check an ACL for it.
 */
int
netbsd32___acl_aclcheck_file(struct lwp *l,
    const struct netbsd32___acl_aclcheck_file_args *uap, register_t *retval)
{

	return kern___acl_aclcheck_path(l, SCARG_P32(uap, path),
	    SCARG(uap, type), SCARG_P32(uap, aclp), NSM_FOLLOW_NOEMULROOT);
}

/*
 * Given a file path, check an ACL for it; don't follow links.
 */
int
netbsd32___acl_aclcheck_link(struct lwp *l,
    const struct netbsd32___acl_aclcheck_link_args *uap, register_t *retval)
{

	return kern___acl_aclcheck_path(l, SCARG_P32(uap, path),
	    SCARG(uap, type), SCARG_P32(uap, aclp), NSM_NOFOLLOW_NOEMULROOT);
}

/*
 * Given a file descriptor, check an ACL for it.
 */
int
netbsd32___acl_aclcheck_fd(struct lwp *l,
    const struct netbsd32___acl_aclcheck_fd_args *uap, register_t *retval)
{
	struct file *fp;
	int error;

	error = fd_getvnode(SCARG(uap, filedes), &fp);
	if (error == 0) {
		error = vacl_aclcheck(l, fp->f_vnode, SCARG(uap, type),
		    SCARG_P32(uap, aclp));
		fd_putfile(SCARG(uap, filedes));
	}
	return error;
}
