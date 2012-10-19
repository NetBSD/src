/*	$NetBSD: policy.h,v 1.7 2012/10/19 19:58:33 riastradh Exp $	*/

/*-
 * Copyright (c) 2007 Pawel Jakub Dawidek <pjd@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 $ $FreeBSD: src/sys/compat/opensolaris/sys/policy.h,v 1.1 2007/04/06 01:09:06 pjd Exp $
 */

#ifndef _OPENSOLARIS_SYS_POLICY_H_
#define	_OPENSOLARIS_SYS_POLICY_H_

#include <sys/param.h>

#ifdef _KERNEL

#include <sys/vnode.h>

struct mount;
struct vattr;
struct vnode;

int	secpolicy_zfs(kauth_cred_t cred);
int	secpolicy_sys_config(kauth_cred_t cred, int checkonly);
int	secpolicy_zinject(kauth_cred_t cred);
int 	secpolicy_fs_mount(kauth_cred_t cred, struct vnode *mvp,
	    struct mount *vfsp);
int	secpolicy_fs_unmount(kauth_cred_t cred, struct mount *vfsp);
int	secpolicy_basic_link(kauth_cred_t cred);
int	secpolicy_vnode_stky_modify(kauth_cred_t cred);
int 	secpolicy_vnode_owner(kauth_cred_t cred, uid_t owner);
int	secpolicy_vnode_remove(kauth_cred_t cred);
int	secpolicy_vnode_access(kauth_cred_t cred, struct vnode *vp,
	    uid_t owner, int mode);
int 	secpolicy_vnode_chown(kauth_cred_t cred,
            boolean_t check_self);
int	secpolicy_vnode_setdac(kauth_cred_t cred, uid_t owner);
int	secpolicy_vnode_setattr(kauth_cred_t cred, struct vnode *vp,
	    struct vattr *vap, const struct vattr *ovap, int flags,
	    int unlocked_access(void *, int, kauth_cred_t), void *node);
int	secpolicy_vnode_create_gid(kauth_cred_t cred);
int	secpolicy_vnode_utime_modify(kauth_cred_t cred);
int	secpolicy_vnode_setids_setgids(kauth_cred_t cred, gid_t gid);
int	secpolicy_vnode_setid_retain(kauth_cred_t cred, boolean_t issuidroot);
void	secpolicy_setid_clear(struct vattr *vap, kauth_cred_t cred);
int	secpolicy_setid_setsticky_clear(struct vnode *vp, struct vattr *vap,
	    const struct vattr *ovap, kauth_cred_t cred);
int     secpolicy_xvattr(xvattr_t *xvap, uid_t owner, kauth_cred_t cred,
	    vtype_t vtype);

#endif	/* _KERNEL */

#endif	/* _OPENSOLARIS_SYS_POLICY_H_ */
