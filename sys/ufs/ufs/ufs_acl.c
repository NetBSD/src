/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 1999-2003 Robert N. M. Watson
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
 */

/*
 * Support for POSIX.1e access control lists: UFS-specific support functions.
 */

#include <sys/cdefs.h>
#if 0
__FBSDID("$FreeBSD: head/sys/ufs/ufs/ufs_acl.c 356669 2020-01-13 02:31:51Z mjg $");
#endif
__KERNEL_RCSID(0, "$NetBSD: ufs_acl.c,v 1.5 2023/02/22 21:49:45 riastradh Exp $");

#if defined(_KERNEL_OPT)
#include "opt_ffs.h"
#include "opt_quota.h"
#include "opt_wapbl.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/wapbl.h>
#include <sys/types.h>
#include <sys/acl.h>
#include <sys/event.h>
#include <sys/extattr.h>
#include <sys/proc.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/acl.h>
#include <ufs/ufs/extattr.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>
#include <ufs/ufs/ufs_wapbl.h>

#include <ufs/ffs/fs.h>

#ifdef UFS_ACL

/*
 * Synchronize an ACL and an inode by copying over appropriate inode fields
 * to the passed ACL.  Assumes an ACL that would satisfy acl_posix1e_check(),
 * and may panic if not.
 */
void
ufs_sync_acl_from_inode(struct inode *ip, struct acl *acl)
{
	struct acl_entry	*acl_mask, *acl_group_obj;
	int	i;

	/*
	 * Update ACL_USER_OBJ, ACL_OTHER, but simply identify ACL_MASK
	 * and ACL_GROUP_OBJ for use after we know whether ACL_MASK is
	 * present.
	 */
	acl_mask = NULL;
	acl_group_obj = NULL;
	for (i = 0; i < acl->acl_cnt; i++) {
		switch (acl->acl_entry[i].ae_tag) {
		case ACL_USER_OBJ:
			acl->acl_entry[i].ae_perm = acl_posix1e_mode_to_perm(
			    ACL_USER_OBJ, ip->i_mode);
			acl->acl_entry[i].ae_id = ACL_UNDEFINED_ID;
			break;

		case ACL_GROUP_OBJ:
			acl_group_obj = &acl->acl_entry[i];
			acl->acl_entry[i].ae_id = ACL_UNDEFINED_ID;
			break;

		case ACL_OTHER:
			acl->acl_entry[i].ae_perm = acl_posix1e_mode_to_perm(
			    ACL_OTHER, ip->i_mode);
			acl->acl_entry[i].ae_id = ACL_UNDEFINED_ID;
			break;

		case ACL_MASK:
			acl_mask = &acl->acl_entry[i];
			acl->acl_entry[i].ae_id = ACL_UNDEFINED_ID;
			break;

		case ACL_USER:
		case ACL_GROUP:
			break;

		default:
			panic("ufs_sync_acl_from_inode(): bad ae_tag");
		}
	}

	if (acl_group_obj == NULL)
		panic("ufs_sync_acl_from_inode(): no ACL_GROUP_OBJ");

	if (acl_mask == NULL) {
		/*
		 * There is no ACL_MASK, so update ACL_GROUP_OBJ.
		 */
		acl_group_obj->ae_perm = acl_posix1e_mode_to_perm(
		    ACL_GROUP_OBJ, ip->i_mode);
	} else {
		/*
		 * Update the ACL_MASK entry instead of ACL_GROUP_OBJ.
		 */
		acl_mask->ae_perm = acl_posix1e_mode_to_perm(ACL_GROUP_OBJ,
		    ip->i_mode);
	}
}

/*
 * Calculate what the inode mode should look like based on an authoritative
 * ACL for the inode.  Replace only the fields in the inode that the ACL
 * can represent.
 */
void
ufs_sync_inode_from_acl(struct acl *acl, struct inode *ip)
{

	ip->i_mode &= ACL_PRESERVE_MASK;
	ip->i_mode |= acl_posix1e_acl_to_mode(acl);
	DIP_ASSIGN(ip, mode, ip->i_mode);
}

/*
 * Retrieve NFSv4 ACL, skipping access checks.  Must be used in UFS code
 * instead of VOP_GETACL() when we don't want to be restricted by the user
 * not having ACL_READ_ACL permission, e.g. when calculating inherited ACL
 * or in ufs_vnops.c:ufs_accessx().
 */
int
ufs_getacl_nfs4_internal(struct vnode *vp, struct acl *aclp, struct lwp *l)
{
	int error;
	size_t len;
	struct inode *ip = VTOI(vp);

	len = sizeof(*aclp);
	bzero(aclp, len);

	error = vn_extattr_get(vp, IO_NODELOCKED,
	    NFS4_ACL_EXTATTR_NAMESPACE, NFS4_ACL_EXTATTR_NAME, &len, aclp, l);
	aclp->acl_maxcnt = ACL_MAX_ENTRIES;
	if (error == ENOATTR) {
		/*
		 * Legitimately no ACL set on object, purely
		 * emulate it through the inode.
		 */
		acl_nfs4_sync_acl_from_mode(aclp, ip->i_mode, ip->i_uid);

		return (0);
	}

	if (error)
		return (error);

	if (len != sizeof(*aclp)) {
		/*
		 * A short (or long) read, meaning that for
		 * some reason the ACL is corrupted.  Return
		 * EPERM since the object DAC protections
		 * are unsafe.
		 */
		printf("%s: Loaded invalid ACL ("
		    "%zu bytes), inumber %ju on %s\n", __func__, len,
		    (uintmax_t)ip->i_number, ip->i_fs->fs_fsmnt);

		return (EPERM);
	}

	error = acl_nfs4_check(aclp, vp->v_type == VDIR);
	if (error) {
		printf("%s: Loaded invalid ACL "
		    "(failed acl_nfs4_check), inumber %ju on %s\n", __func__,
		    (uintmax_t)ip->i_number, ip->i_fs->fs_fsmnt);

		return (EPERM);
	}

	return (0);
}

static int
ufs_getacl_nfs4(struct vop_getacl_args *ap, struct lwp *l)
{
	int error;

	if ((ap->a_vp->v_mount->mnt_flag & MNT_NFS4ACLS) == 0)
		return (EINVAL);

	error = VOP_ACCESSX(ap->a_vp, VREAD_ACL, ap->a_cred);
	if (error)
		return (error);

	error = ufs_getacl_nfs4_internal(ap->a_vp, ap->a_aclp, l);

	return (error);
}

/*
 * Read POSIX.1e ACL from an EA.  Return error if its not found
 * or if any other error has occurred.
 */
static int
ufs_get_oldacl(acl_type_t type, struct oldacl *old, struct vnode *vp,
    struct lwp *l)
{
	int error;
	size_t len;
	struct inode *ip = VTOI(vp);

	len = sizeof(*old);

	switch (type) {
	case ACL_TYPE_ACCESS:
		error = vn_extattr_get(vp, IO_NODELOCKED,
		    POSIX1E_ACL_ACCESS_EXTATTR_NAMESPACE,
		    POSIX1E_ACL_ACCESS_EXTATTR_NAME, &len, old, l);
		break;
	case ACL_TYPE_DEFAULT:
		if (vp->v_type != VDIR)
			return (EINVAL);
		error = vn_extattr_get(vp, IO_NODELOCKED,
		    POSIX1E_ACL_DEFAULT_EXTATTR_NAMESPACE,
		    POSIX1E_ACL_DEFAULT_EXTATTR_NAME, &len, old, l);
		break;
	default:
		return (EINVAL);
	}

	if (error != 0)
		return (error);

	if (len != sizeof(*old)) {
		/*
		 * A short (or long) read, meaning that for some reason
		 * the ACL is corrupted.  Return EPERM since the object
		 * DAC protections are unsafe.
		 */
		printf("%s: Loaded invalid ACL "
		    "(len = %zu), inumber %ju on %s\n", __func__, len,
		    (uintmax_t)ip->i_number, ip->i_fs->fs_fsmnt);
		return (EPERM);
	}

	return (0);
}

/*
 * Retrieve the ACL on a file.
 *
 * As part of the ACL is stored in the inode, and the rest in an EA,
 * assemble both into a final ACL product.  Right now this is not done
 * very efficiently.
 */
static int
ufs_getacl_posix1e(struct vop_getacl_args *ap, struct lwp *l)
{
	struct inode *ip = VTOI(ap->a_vp);
	int error;
	struct oldacl *old;

	/*
	 * XXX: If ufs_getacl() should work on file systems not supporting
	 * ACLs, remove this check.
	 */
	if ((ap->a_vp->v_mount->mnt_flag & MNT_POSIX1EACLS) == 0)
		return (EINVAL);

	old = kmem_zalloc(sizeof(*old), KM_SLEEP);

	/*
	 * Attempt to retrieve the ACL from the extended attributes.
	 */
	error = ufs_get_oldacl(ap->a_type, old, ap->a_vp, l);
	switch (error) {
	/*
	 * XXX: If ufs_getacl() should work on filesystems
	 * without the EA configured, add case EOPNOTSUPP here.
	 */
	case ENOATTR:
		switch (ap->a_type) {
		case ACL_TYPE_ACCESS:
			/*
			 * Legitimately no ACL set on object, purely
			 * emulate it through the inode.  These fields will
			 * be updated when the ACL is synchronized with
			 * the inode later.
			 */
			old->acl_cnt = 3;
			old->acl_entry[0].ae_tag = ACL_USER_OBJ;
			old->acl_entry[0].ae_id = ACL_UNDEFINED_ID;
			old->acl_entry[0].ae_perm = ACL_PERM_NONE;
			old->acl_entry[1].ae_tag = ACL_GROUP_OBJ;
			old->acl_entry[1].ae_id = ACL_UNDEFINED_ID;
			old->acl_entry[1].ae_perm = ACL_PERM_NONE;
			old->acl_entry[2].ae_tag = ACL_OTHER;
			old->acl_entry[2].ae_id = ACL_UNDEFINED_ID;
			old->acl_entry[2].ae_perm = ACL_PERM_NONE;
			break;

		case ACL_TYPE_DEFAULT:
			/*
			 * Unlike ACL_TYPE_ACCESS, there is no relationship
			 * between the inode contents and the ACL, and it is
			 * therefore possible for the request for the ACL
			 * to fail since the ACL is undefined.  In this
			 * situation, return success and an empty ACL,
			 * as required by POSIX.1e.
			 */
			old->acl_cnt = 0;
			break;
		}
		/* FALLTHROUGH */
	case 0:
		error = acl_copy_oldacl_into_acl(old, ap->a_aclp);
		if (error != 0)
			break;

		if (ap->a_type == ACL_TYPE_ACCESS)
			ufs_sync_acl_from_inode(ip, ap->a_aclp);
	default:
		break;
	}

	kmem_free(old, sizeof(*old));
	return (error);
}

int
ufs_getacl(void *v)
{
	struct vop_getacl_args *ap = v;

	if ((ap->a_vp->v_mount->mnt_flag & (MNT_POSIX1EACLS | MNT_NFS4ACLS)) == 0)
		return (EOPNOTSUPP);

	if (ap->a_type == ACL_TYPE_NFS4)
		return (ufs_getacl_nfs4(ap, curlwp));

	return (ufs_getacl_posix1e(ap, curlwp));
}

/*
 * Set NFSv4 ACL without doing any access checking.  This is required
 * e.g. by the UFS code that implements ACL inheritance, or from
 * ufs_vnops.c:ufs_chmod(), as some of the checks have to be skipped
 * in that case, and others are redundant.
 */
int
ufs_setacl_nfs4_internal(struct vnode *vp, struct acl *aclp,
    struct lwp *l, bool lock)
{
	int error;
	mode_t mode;
	struct inode *ip = VTOI(vp);

	KASSERT(acl_nfs4_check(aclp, vp->v_type == VDIR) == 0);

	if (acl_nfs4_is_trivial(aclp, ip->i_uid)) {
		error = vn_extattr_rm(vp, IO_NODELOCKED,
		    NFS4_ACL_EXTATTR_NAMESPACE, NFS4_ACL_EXTATTR_NAME, l);
		/*
		 * An attempt to remove ACL from a file that didn't have
		 * any extended entries is not an error.
		 */
		if (error == ENOATTR)
			error = 0;

	} else {
		error = vn_extattr_set(vp, IO_NODELOCKED,
		    NFS4_ACL_EXTATTR_NAMESPACE, NFS4_ACL_EXTATTR_NAME,
		    sizeof(*aclp), aclp, l);
	}

	/*
	 * Map lack of attribute definition in UFS_EXTATTR into lack of
	 * support for ACLs on the filesystem.
	 */
	if (error == ENOATTR)
		return (EOPNOTSUPP);

	if (error)
		return (error);

	mode = ip->i_mode;

	__acl_nfs4_sync_mode_from_acl(&mode, aclp);

	if (lock && (error = UFS_WAPBL_BEGIN(vp->v_mount)) != 0)
		return error;

	ip->i_mode &= ACL_PRESERVE_MASK;
	ip->i_mode |= mode;
	DIP_ASSIGN(ip, mode, ip->i_mode);
	ip->i_flag |= IN_CHANGE;

	error = UFS_UPDATE(vp, NULL, NULL, 0);
	if (lock)
		UFS_WAPBL_END(vp->v_mount);

	return (error);
}

static int
ufs_setacl_nfs4(struct vop_setacl_args *ap, struct lwp *l)
{
	int error;
	struct inode *ip = VTOI(ap->a_vp);

	if ((ap->a_vp->v_mount->mnt_flag & MNT_NFS4ACLS) == 0)
		return (EINVAL);

	if (ap->a_vp->v_mount->mnt_flag & MNT_RDONLY)
		return (EROFS);

	if (ap->a_aclp == NULL)
		return (EINVAL);

	error = VOP_ACLCHECK(ap->a_vp, ap->a_type, ap->a_aclp, ap->a_cred);
	if (error)
		return (error);

	/*
	 * Authorize the ACL operation.
	 */
	if (ip->i_flags & (IMMUTABLE | APPEND))
		return (EPERM);

	/*
	 * Must hold VWRITE_ACL or have appropriate privilege.
	 */
	if ((error = VOP_ACCESSX(ap->a_vp, VWRITE_ACL, l->l_cred)))
		return (error);

	/*
	 * With NFSv4 ACLs, chmod(2) may need to add additional entries.
	 * Make sure it has enough room for that - splitting every entry
	 * into two and appending "canonical six" entries at the end.
	 */
	if (ap->a_aclp->acl_cnt > (ACL_MAX_ENTRIES - 6) / 2)
		return (ENOSPC);

	error = ufs_setacl_nfs4_internal(ap->a_vp, ap->a_aclp, l, true);

	return (error);
}

/*
 * Set the ACL on a file.
 *
 * As part of the ACL is stored in the inode, and the rest in an EA,
 * this is necessarily non-atomic, and has complex authorization.
 * As ufs_setacl() includes elements of ufs_chown() and ufs_chmod(),
 * a fair number of different access checks may be required to go ahead
 * with the operation at all.
 */
int
ufs_setacl_posix1e(struct vnode *vp, int type, struct acl *aclp,
    kauth_cred_t cred, struct lwp *l)
{
	struct inode *ip = VTOI(vp);
	int error;
	struct oldacl *old;

	if ((vp->v_mount->mnt_flag & MNT_POSIX1EACLS) == 0)
		return (EINVAL);

	/*
	 * If this is a set operation rather than a delete operation,
	 * invoke VOP_ACLCHECK() on the passed ACL to determine if it is
	 * valid for the target.  This will include a check on ap->a_type.
	 */
	if (aclp != NULL) {
		/*
		 * Set operation.
		 */
		error = VOP_ACLCHECK(vp, type, aclp, cred);
		if (error != 0)
			return (error);
	} else {
		/*
		 * Delete operation.
		 * POSIX.1e allows only deletion of the default ACL on a
		 * directory (ACL_TYPE_DEFAULT).
		 */
		if (type != ACL_TYPE_DEFAULT)
			return (EINVAL);
		if (vp->v_type != VDIR)
			return (ENOTDIR);
	}

	if (vp->v_mount->mnt_flag & MNT_RDONLY)
		return (EROFS);

	/*
	 * Authorize the ACL operation.
	 */
	if (ip->i_flags & (IMMUTABLE | APPEND))
		return (EPERM);

	/*
	 * Must hold VADMIN (be file owner) or have appropriate privilege.
	 */
	if ((error = VOP_ACCESS(vp, VADMIN, cred)))
		return (error);

	switch(type) {
	case ACL_TYPE_ACCESS:
		old = kmem_zalloc(sizeof(*old), KM_SLEEP);
		error = acl_copy_acl_into_oldacl(aclp, old);
		if (error == 0) {
			error = vn_extattr_set(vp, IO_NODELOCKED,
			    POSIX1E_ACL_ACCESS_EXTATTR_NAMESPACE,
			    POSIX1E_ACL_ACCESS_EXTATTR_NAME, sizeof(*old),
			    old, l);
		}
		kmem_free(old, sizeof(*old));
		break;

	case ACL_TYPE_DEFAULT:
		if (aclp == NULL) {
			error = vn_extattr_rm(vp, IO_NODELOCKED,
			    POSIX1E_ACL_DEFAULT_EXTATTR_NAMESPACE,
			    POSIX1E_ACL_DEFAULT_EXTATTR_NAME, l);
			/*
			 * Attempting to delete a non-present default ACL
			 * will return success for portability purposes.
			 * (TRIX)
			 *
			 * XXX: Note that since we can't distinguish
			 * "that EA is not supported" from "that EA is not
			 * defined", the success case here overlaps the
			 * the ENOATTR->EOPNOTSUPP case below.
		 	 */
			if (error == ENOATTR)
				error = 0;
		} else {
			old = kmem_zalloc(sizeof(*old), KM_SLEEP);
			error = acl_copy_acl_into_oldacl(aclp, old);
			if (error == 0) {
				error = vn_extattr_set(vp, IO_NODELOCKED,
				    POSIX1E_ACL_DEFAULT_EXTATTR_NAMESPACE,
				    POSIX1E_ACL_DEFAULT_EXTATTR_NAME,
				    sizeof(*old), (char *) old, l);
			}
			kmem_free(old, sizeof(*old));
		}
		break;

	default:
		error = EINVAL;
	}
	/*
	 * Map lack of attribute definition in UFS_EXTATTR into lack of
	 * support for ACLs on the filesystem.
	 */
	if (error == ENOATTR)
		return (EOPNOTSUPP);
	if (error != 0)
		return (error);

	if (type == ACL_TYPE_ACCESS) {
		/*
		 * Now that the EA is successfully updated, update the
		 * inode and mark it as changed.
		 */
		if ((error = UFS_WAPBL_BEGIN(vp->v_mount)) != 0)
			return error;

		ufs_sync_inode_from_acl(aclp, ip);
		ip->i_flag |= IN_CHANGE;
		error = UFS_UPDATE(vp, NULL, NULL, 0);

		UFS_WAPBL_END(vp->v_mount);
	}

	return (error);
}

int
ufs_setacl(void *v)
{
	struct vop_setacl_args *ap = v;
	if ((ap->a_vp->v_mount->mnt_flag & (MNT_POSIX1EACLS | MNT_NFS4ACLS)) == 0)
		return (EOPNOTSUPP);

	if (ap->a_type == ACL_TYPE_NFS4)
		return ufs_setacl_nfs4(ap, curlwp);

	return ufs_setacl_posix1e(ap->a_vp, ap->a_type, ap->a_aclp, ap->a_cred,
	    curlwp);
}

static int
ufs_aclcheck_nfs4(struct vop_aclcheck_args *ap, struct lwp *l)
{
	int is_directory = 0;

	if ((ap->a_vp->v_mount->mnt_flag & MNT_NFS4ACLS) == 0)
		return (EINVAL);

	/*
	 * With NFSv4 ACLs, chmod(2) may need to add additional entries.
	 * Make sure it has enough room for that - splitting every entry
	 * into two and appending "canonical six" entries at the end.
	 */
	if (ap->a_aclp->acl_cnt > (ACL_MAX_ENTRIES - 6) / 2)
		return (ENOSPC);

	if (ap->a_vp->v_type == VDIR)
		is_directory = 1;

	return (acl_nfs4_check(ap->a_aclp, is_directory));
}

static int
ufs_aclcheck_posix1e(struct vop_aclcheck_args *ap, struct lwp *l)
{

	if ((ap->a_vp->v_mount->mnt_flag & MNT_POSIX1EACLS) == 0)
		return (EINVAL);

	/*
	 * Verify we understand this type of ACL, and that it applies
	 * to this kind of object.
	 * Rely on the acl_posix1e_check() routine to verify the contents.
	 */
	switch(ap->a_type) {
	case ACL_TYPE_ACCESS:
		break;

	case ACL_TYPE_DEFAULT:
		if (ap->a_vp->v_type != VDIR)
			return (EINVAL);
		break;

	default:
		return (EINVAL);
	}

	if (ap->a_aclp->acl_cnt > OLDACL_MAX_ENTRIES)
		return (EINVAL);

	return (acl_posix1e_check(ap->a_aclp));
}

/*
 * Check the validity of an ACL for a file.
 */
int
ufs_aclcheck(void *v)
{
	struct vop_aclcheck_args *ap = v;

	if ((ap->a_vp->v_mount->mnt_flag & (MNT_POSIX1EACLS | MNT_NFS4ACLS)) == 0)
		return (EOPNOTSUPP);

	if (ap->a_type == ACL_TYPE_NFS4)
		return ufs_aclcheck_nfs4(ap, curlwp);

	return ufs_aclcheck_posix1e(ap, curlwp);
}

#endif /* !UFS_ACL */
