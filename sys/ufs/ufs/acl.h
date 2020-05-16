/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 1999-2001 Robert N. M. Watson
 * All rights reserved.
 *
 * This software was developed by Robert Watson for the TrustedBSD Project.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: head/sys/ufs/ufs/acl.h 326272 2017-11-27 15:23:17Z pfg $
 */
/*
 * Developed by the TrustedBSD Project.
 * Support for POSIX.1e access control lists.
 */

#ifndef _UFS_UFS_ACL_H_
#define	_UFS_UFS_ACL_H_

#ifdef _KERNEL

struct inode;
int	ufs_getacl_nfs4_internal(struct vnode *vp, struct acl *aclp, struct lwp *l);
int	ufs_setacl_nfs4_internal(struct vnode *vp, struct acl *aclp, struct lwp *l, bool lock);
int	ufs_setacl_posix1e(struct vnode *vp, int type, struct acl *aclp,
    kauth_cred_t cred, struct lwp *l);
void	ufs_sync_acl_from_inode(struct inode *ip, struct acl *acl);
void	ufs_sync_inode_from_acl(struct acl *acl, struct inode *ip);

#ifdef UFS_ACL
int	ufs_getacl(void *);
int	ufs_setacl(void *);
int	ufs_aclcheck(void *);
#else
#define	ufs_getacl	genfs_eopnotsupp
#define	ufs_setacl	genfs_eopnotsupp
#define	ufs_aclcheck	genfs_eopnotsupp
#endif

#endif /* !_KERNEL */

#endif /* !_UFS_UFS_ACL_H_ */
