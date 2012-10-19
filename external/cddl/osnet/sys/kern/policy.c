/*	$NetBSD: policy.c,v 1.6 2012/10/19 22:19:15 riastradh Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 */

/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright (c) 2003, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright 2012, Joyent, Inc. All rights reserved.
 */

#include <sys/param.h>
#include <sys/priv.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/policy.h>

int
secpolicy_zfs(kauth_cred_t cred)
{

	return kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER, NULL);
}

int
secpolicy_sys_config(kauth_cred_t cred, int checkonly __unused)
{

	return kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER, NULL);
}

int
secpolicy_zinject(kauth_cred_t cred)
{

	return kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER, NULL);
}

int
secpolicy_fs_mount(kauth_cred_t cred, struct vnode *mvp, struct mount *vfsp)
{

	return kauth_authorize_system(cred, KAUTH_SYSTEM_MOUNT,
	    KAUTH_REQ_SYSTEM_MOUNT_NEW, vfsp, NULL, NULL);
}

int
secpolicy_fs_unmount(kauth_cred_t cred, struct mount *vfsp)
{

	return kauth_authorize_system(cred, KAUTH_SYSTEM_MOUNT,
	    KAUTH_REQ_SYSTEM_MOUNT_UNMOUNT, vfsp, NULL, NULL);
}

/*
 * This check is done in kern_link(), so we could just return 0 here.
 */
int
secpolicy_basic_link(kauth_cred_t cred)
{

	return kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER, NULL);
}

int
secpolicy_vnode_stky_modify(kauth_cred_t cred)
{

	return kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER, NULL);
}

int
secpolicy_vnode_remove(kauth_cred_t cred)
{

	return kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER, NULL);
}


int
secpolicy_vnode_owner(kauth_cred_t cred, uid_t owner)
{

	if (owner == kauth_cred_getuid(cred))
		return (0);

	return kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER, NULL);
}

int
secpolicy_vnode_access(kauth_cred_t cred, struct vnode *vp, uid_t owner,
    int mode)
{

	return kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER, NULL);
}

int
secpolicy_xvattr(xvattr_t *xvap, uid_t owner, kauth_cred_t cred, vtype_t vtype)
{

	return kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER, NULL);
}

int
secpolicy_vnode_setid_retain(kauth_cred_t cred, boolean_t issuidroot __unused)
{

	return kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER, NULL);
}

int
secpolicy_vnode_setids_setgids(kauth_cred_t cred, gid_t gid)
{

	if (groupmember(gid, cred))
		return (0);

	return kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER, NULL);
}

int
secpolicy_vnode_chown(kauth_cred_t cred, boolean_t check_self)
{

	return kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER, NULL);
}

int
secpolicy_vnode_create_gid(kauth_cred_t cred)
{

	return kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER, NULL);
}

int
secpolicy_vnode_utime_modify(kauth_cred_t cred)
{

	return kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER, NULL);
}

int
secpolicy_vnode_setdac(kauth_cred_t cred, uid_t owner)
{

	if (owner == kauth_cred_getuid(cred))
		return (0);

	return kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER, NULL);
}

int
secpolicy_setid_setsticky_clear(struct vnode *vp, struct vattr *vap,
    const struct vattr *ovap, kauth_cred_t cred)
{
	/*
	 * Privileged processes may set the sticky bit on non-directories,
	 * as well as set the setgid bit on a file with a group that the process
	 * is not a member of. Both of these are allowed in jail(8).
	 */
	if (vp->v_type != VDIR && (vap->va_mode & S_ISTXT)) {
		if (kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER, NULL))
			return (EFTYPE);
	}
	/*
	 * Check for privilege if attempting to set the
	 * group-id bit.
	 */
	if ((vap->va_mode & S_ISGID) != 0)
		return (secpolicy_vnode_setids_setgids(cred, ovap->va_gid));

	return (0);
}

/*
 * XXX Copied from illumos.  Should not be here; should be under
 * external/cddl/osnet/dist.  Not sure why it is even in illumos's
 * policy.c rather than somewhere in vnode.c or something.
 */
int
secpolicy_vnode_setattr(kauth_cred_t cred, struct vnode *vp, struct vattr *vap,
    const struct vattr *ovap, int flags,
    int unlocked_access(void *, int, kauth_cred_t), void *node)
{
	int mask = vap->va_mask;
	int error = 0;
	boolean_t skipaclchk = (flags & ATTR_NOACLCHECK) ? B_TRUE : B_FALSE;

	if (mask & AT_SIZE) {
		if (vp->v_type == VDIR) {
			error = EISDIR;
			goto out;
		}

		/*
		 * If ATTR_NOACLCHECK is set in the flags, then we don't
		 * perform the secondary unlocked_access() call since the
		 * ACL (if any) is being checked there.
		 */
		if (skipaclchk == B_FALSE) {
			error = unlocked_access(node, VWRITE, cred);
			if (error)
				goto out;
		}
	}
	if (mask & AT_MODE) {
		/*
		 * If not the owner of the file then check privilege
		 * for two things: the privilege to set the mode at all
		 * and, if we're setting setuid, we also need permissions
		 * to add the set-uid bit, if we're not the owner.
		 * In the specific case of creating a set-uid root
		 * file, we need even more permissions.
		 */
		if ((error = secpolicy_vnode_setdac(cred, ovap->va_uid)) != 0)
			goto out;

		if ((error = secpolicy_setid_setsticky_clear(vp, vap,
		    ovap, cred)) != 0)
			goto out;
	} else
		vap->va_mode = ovap->va_mode;

	if (mask & (AT_UID|AT_GID)) {
		boolean_t checkpriv = B_FALSE;

		/*
		 * Chowning files.
		 *
		 * If you are the file owner:
		 *	chown to other uid		FILE_CHOWN_SELF
		 *	chown to gid (non-member) 	FILE_CHOWN_SELF
		 *	chown to gid (member) 		<none>
		 *
		 * Instead of PRIV_FILE_CHOWN_SELF, FILE_CHOWN is also
		 * acceptable but the first one is reported when debugging.
		 *
		 * If you are not the file owner:
		 *	chown from root			PRIV_FILE_CHOWN + zone
		 *	chown from other to any		PRIV_FILE_CHOWN
		 *
		 */
		if (kauth_cred_getuid(cred) != ovap->va_uid) {
			checkpriv = B_TRUE;
		} else {
			if (((mask & AT_UID) && vap->va_uid != ovap->va_uid) ||
			    ((mask & AT_GID) && vap->va_gid != ovap->va_gid &&
			    !groupmember(vap->va_gid, cred))) {
				checkpriv = B_TRUE;
			}
		}
		/*
		 * If necessary, check privilege to see if update can be done.
		 */
		if (checkpriv &&
		    (error = secpolicy_vnode_chown(cred, ovap->va_uid)) != 0) {
			goto out;
		}

		/*
		 * If the file has either the set UID or set GID bits
		 * set and the caller can set the bits, then leave them.
		 */
		secpolicy_setid_clear(vap, cred);
	}
	if (mask & (AT_ATIME|AT_MTIME)) {
		/*
		 * If not the file owner and not otherwise privileged,
		 * always return an error when setting the
		 * time other than the current (ATTR_UTIME flag set).
		 * If setting the current time (ATTR_UTIME not set) then
		 * unlocked_access will check permissions according to policy.
		 */
		if (kauth_cred_getuid(cred) != ovap->va_uid) {
			if (flags & ATTR_UTIME)
				error = secpolicy_vnode_utime_modify(cred);
			else if (skipaclchk == B_FALSE) {
				error = unlocked_access(node, VWRITE, cred);
				if (error == EACCES &&
				    secpolicy_vnode_utime_modify(cred) == 0)
					error = 0;
			}
			if (error)
				goto out;
		}
	}

	/*
	 * Check for optional attributes here by checking the following:
	 */
	if (mask & AT_XVATTR)
		error = secpolicy_xvattr((xvattr_t *)vap, ovap->va_uid, cred,
		    vp->v_type);
out:
	return (error);
}

void
secpolicy_setid_clear(struct vattr *vap, kauth_cred_t cred)
{
	if (kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER, NULL) != 0)
		return;

	if ((vap->va_mode & (S_ISUID | S_ISGID)) != 0) {
		vap->va_mask |= AT_MODE;
		vap->va_mode &= ~(S_ISUID|S_ISGID);
	}

	return;
}

#ifdef notyet
int
secpolicy_vnode_setdac(kauth_cred_t cred, uid_t owner)
{

	if (owner == cred->cr_uid)
		return (0);
	return (priv_check_cred(cred, PRIV_VFS_ADMIN, 0));
}

int
secpolicy_vnode_setattr(kauth_cred_t cred, struct vnode *vp, struct vattr *vap,
    const struct vattr *ovap, int flags,
    int unlocked_access(void *, int, kauth_cred_t), void *node)
{
	int mask = vap->va_mask;
	int error;

	if (mask & AT_SIZE) {
		if (vp->v_type == VDIR)
			return (EISDIR);
		error = unlocked_access(node, VWRITE, cred);
		if (error)
			return (error);
	}
	if (mask & AT_MODE) {
		/*
		 * If not the owner of the file then check privilege
		 * for two things: the privilege to set the mode at all
		 * and, if we're setting setuid, we also need permissions
		 * to add the set-uid bit, if we're not the owner.
		 * In the specific case of creating a set-uid root
		 * file, we need even more permissions.
		 */
		error = secpolicy_vnode_setdac(cred, ovap->va_uid);
		if (error)
			return (error);
		error = secpolicy_setid_setsticky_clear(vp, vap, ovap, cred);
		if (error)
			return (error);
	} else {
		vap->va_mode = ovap->va_mode;
	}
	if (mask & (AT_UID | AT_GID)) {
		error = secpolicy_vnode_setdac(cred, ovap->va_uid);
		if (error)
			return (error);

		/*
		 * To change the owner of a file, or change the group of a file to a
		 * group of which we are not a member, the caller must have
		 * privilege.
		 */
		if (((mask & AT_UID) && vap->va_uid != ovap->va_uid) ||
		    ((mask & AT_GID) && vap->va_gid != ovap->va_gid &&
		     !groupmember(vap->va_gid, cred))) {
			error = priv_check_cred(cred, PRIV_VFS_CHOWN, 0);
			if (error)
				return (error);
		}

		if (((mask & AT_UID) && vap->va_uid != ovap->va_uid) ||
		    ((mask & AT_GID) && vap->va_gid != ovap->va_gid)) {
			secpolicy_setid_clear(vap, cred);
		}
	}
	if (mask & (AT_ATIME | AT_MTIME)) {
		/*
		 * From utimes(2):
		 * If times is NULL, ... The caller must be the owner of
		 * the file, have permission to write the file, or be the
		 * super-user.
		 * If times is non-NULL, ... The caller must be the owner of
		 * the file or be the super-user.
		 */
		error = secpolicy_vnode_setdac(cred, ovap->va_uid);
		if (error && (vap->va_vaflags & VA_UTIMES_NULL))
			error = unlocked_access(node, VWRITE, cred);
		if (error)
			return (error);
	}
	return (0);
}

int
secpolicy_vnode_create_gid(kauth_cred_t cred)
{

	return (EPERM);
}

int
secpolicy_vnode_setid_retain(kauth_cred_t cred, boolean_t issuidroot __unused)
{

	return (priv_check_cred(cred, PRIV_VFS_RETAINSUGID, 0));
}

void
secpolicy_setid_clear(struct vattr *vap, kauth_cred_t cred)
{

	if (kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER, NULL))
		return;

	if ((vap->va_mode & (S_ISUID | S_ISGID)) != 0) {
		if (priv_check_cred(cred, PRIV_VFS_RETAINSUGID, 0)) {
			vap->va_mask |= AT_MODE;
			vap->va_mode &= ~(S_ISUID|S_ISGID);
		}
	}
}
#endif
