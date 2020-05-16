/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 1999-2006 Robert N. M. Watson
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
 * Developed by the TrustedBSD Project.
 *
 * ACL support routines specific to POSIX.1e access control lists.  These are
 * utility routines for code common across file systems implementing POSIX.1e
 * ACLs.
 */

#include <sys/cdefs.h>
#if 0
__FBSDID("$FreeBSD: head/sys/kern/subr_acl_posix1e.c 341827 2018-12-11 19:32:16Z mjg $");
#endif
__KERNEL_RCSID(0, "$NetBSD: subr_acl_posix1e.c,v 1.1 2020/05/16 18:31:50 christos Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/kauth.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/acl.h>

/*
 * For the purposes of filesystems maintaining the _OBJ entries in an inode
 * with a mode_t field, this routine converts a mode_t entry to an
 * acl_perm_t.
 */
acl_perm_t
acl_posix1e_mode_to_perm(acl_tag_t tag, mode_t mode)
{
	acl_perm_t	perm = 0;

	switch(tag) {
	case ACL_USER_OBJ:
		if (mode & S_IXUSR)
			perm |= ACL_EXECUTE;
		if (mode & S_IRUSR)
			perm |= ACL_READ;
		if (mode & S_IWUSR)
			perm |= ACL_WRITE;
		return (perm);

	case ACL_GROUP_OBJ:
		if (mode & S_IXGRP)
			perm |= ACL_EXECUTE;
		if (mode & S_IRGRP)
			perm |= ACL_READ;
		if (mode & S_IWGRP)
			perm |= ACL_WRITE;
		return (perm);

	case ACL_OTHER:
		if (mode & S_IXOTH)
			perm |= ACL_EXECUTE;
		if (mode & S_IROTH)
			perm |= ACL_READ;
		if (mode & S_IWOTH)
			perm |= ACL_WRITE;
		return (perm);

	default:
		printf("%s: invalid tag (%u)\n", __func__, tag);
		return (0);
	}
}

/*
 * Given inode information (uid, gid, mode), return an acl entry of the
 * appropriate type.
 */
struct acl_entry
acl_posix1e_mode_to_entry(acl_tag_t tag, uid_t uid, gid_t gid, mode_t mode)
{
	struct acl_entry	acl_entry;

	acl_entry.ae_tag = tag;
	acl_entry.ae_perm = acl_posix1e_mode_to_perm(tag, mode);
	acl_entry.ae_entry_type = 0;
	acl_entry.ae_flags = 0;
	switch(tag) {
	case ACL_USER_OBJ:
		acl_entry.ae_id = uid;
		break;

	case ACL_GROUP_OBJ:
		acl_entry.ae_id = gid;
		break;

	case ACL_OTHER:
		acl_entry.ae_id = ACL_UNDEFINED_ID;
		break;

	default:
		acl_entry.ae_id = ACL_UNDEFINED_ID;
		printf("acl_posix1e_mode_to_entry: invalid tag (%d)\n", tag);
	}

	return (acl_entry);
}

/*
 * Utility function to generate a file mode given appropriate ACL entries.
 */
mode_t
acl_posix1e_perms_to_mode(struct acl_entry *acl_user_obj_entry,
    struct acl_entry *acl_group_obj_entry, struct acl_entry *acl_other_entry)
{
	mode_t	mode;

	mode = 0;
	if (acl_user_obj_entry->ae_perm & ACL_EXECUTE)
		mode |= S_IXUSR;
	if (acl_user_obj_entry->ae_perm & ACL_READ)
		mode |= S_IRUSR;
	if (acl_user_obj_entry->ae_perm & ACL_WRITE)
		mode |= S_IWUSR;
	if (acl_group_obj_entry->ae_perm & ACL_EXECUTE)
		mode |= S_IXGRP;
	if (acl_group_obj_entry->ae_perm & ACL_READ)
		mode |= S_IRGRP;
	if (acl_group_obj_entry->ae_perm & ACL_WRITE)
		mode |= S_IWGRP;
	if (acl_other_entry->ae_perm & ACL_EXECUTE)
		mode |= S_IXOTH;
	if (acl_other_entry->ae_perm & ACL_READ)
		mode |= S_IROTH;
	if (acl_other_entry->ae_perm & ACL_WRITE)
		mode |= S_IWOTH;

	return (mode);
}

/*
 * Utility function to generate a file mode given a complete POSIX.1e access
 * ACL.  Note that if the ACL is improperly formed, this may result in a
 * panic.
 */
mode_t
acl_posix1e_acl_to_mode(struct acl *acl)
{
	struct acl_entry *acl_mask, *acl_user_obj, *acl_group_obj, *acl_other;
	int i;

	/*
	 * Find the ACL entries relevant to a POSIX permission mode.
	 */
	acl_user_obj = acl_group_obj = acl_other = acl_mask = NULL;
	for (i = 0; i < acl->acl_cnt; i++) {
		switch (acl->acl_entry[i].ae_tag) {
		case ACL_USER_OBJ:
			acl_user_obj = &acl->acl_entry[i];
			break;

		case ACL_GROUP_OBJ:
			acl_group_obj = &acl->acl_entry[i];
			break;

		case ACL_OTHER:
			acl_other = &acl->acl_entry[i];
			break;

		case ACL_MASK:
			acl_mask = &acl->acl_entry[i];
			break;

		case ACL_USER:
		case ACL_GROUP:
			break;

		default:
			panic("acl_posix1e_acl_to_mode: bad ae_tag");
		}
	}

	if (acl_user_obj == NULL || acl_group_obj == NULL || acl_other == NULL)
		panic("acl_posix1e_acl_to_mode: missing base ae_tags");

	/*
	 * POSIX.1e specifies that if there is an ACL_MASK entry, we replace
	 * the mode "group" bits with its permissions.  If there isn't, we
	 * use the ACL_GROUP_OBJ permissions.
	 */
	if (acl_mask != NULL)
		return (acl_posix1e_perms_to_mode(acl_user_obj, acl_mask,
		    acl_other));
	else
		return (acl_posix1e_perms_to_mode(acl_user_obj, acl_group_obj,
		    acl_other));
}

/*
 * Perform a syntactic check of the ACL, sufficient to allow an implementing
 * filesystem to determine if it should accept this and rely on the POSIX.1e
 * ACL properties.
 */
int
acl_posix1e_check(struct acl *acl)
{
	int num_acl_user_obj, num_acl_user, num_acl_group_obj, num_acl_group;
	int num_acl_mask, num_acl_other, i;

	/*
	 * Verify that the number of entries does not exceed the maximum
	 * defined for acl_t.
	 *
	 * Verify that the correct number of various sorts of ae_tags are
	 * present:
	 *   Exactly one ACL_USER_OBJ
	 *   Exactly one ACL_GROUP_OBJ
	 *   Exactly one ACL_OTHER
	 *   If any ACL_USER or ACL_GROUP entries appear, then exactly one
	 *   ACL_MASK entry must also appear.
	 *
	 * Verify that all ae_perm entries are in ACL_PERM_BITS.
	 *
	 * Verify all ae_tag entries are understood by this implementation.
	 *
	 * Note: Does not check for uniqueness of qualifier (ae_id) field.
	 */
	num_acl_user_obj = num_acl_user = num_acl_group_obj = num_acl_group =
	    num_acl_mask = num_acl_other = 0;
	if (acl->acl_cnt > ACL_MAX_ENTRIES)
		return (EINVAL);
	for (i = 0; i < acl->acl_cnt; i++) {
		struct acl_entry *ae = &acl->acl_entry[i];
		/*
		 * Check for a valid tag.
		 */
		switch(ae->ae_tag) {
		case ACL_USER_OBJ:
			ae->ae_id = ACL_UNDEFINED_ID; /* XXX */
			if (ae->ae_id != ACL_UNDEFINED_ID)
				return (EINVAL);
			num_acl_user_obj++;
			break;
		case ACL_GROUP_OBJ:
			ae->ae_id = ACL_UNDEFINED_ID; /* XXX */
			if (ae->ae_id != ACL_UNDEFINED_ID)
				return (EINVAL);
			num_acl_group_obj++;
			break;
		case ACL_USER:
			if (ae->ae_id == ACL_UNDEFINED_ID)
				return (EINVAL);
			num_acl_user++;
			break;
		case ACL_GROUP:
			if (ae->ae_id == ACL_UNDEFINED_ID)
				return (EINVAL);
			num_acl_group++;
			break;
		case ACL_OTHER:
			ae->ae_id = ACL_UNDEFINED_ID; /* XXX */
			if (ae->ae_id != ACL_UNDEFINED_ID)
				return (EINVAL);
			num_acl_other++;
			break;
		case ACL_MASK:
			ae->ae_id = ACL_UNDEFINED_ID; /* XXX */
			if (ae->ae_id != ACL_UNDEFINED_ID)
				return (EINVAL);
			num_acl_mask++;
			break;
		default:
			return (EINVAL);
		}
		/*
		 * Check for valid perm entries.
		 */
		if ((acl->acl_entry[i].ae_perm | ACL_PERM_BITS) !=
		    ACL_PERM_BITS)
			return (EINVAL);
	}
	if ((num_acl_user_obj != 1) || (num_acl_group_obj != 1) ||
	    (num_acl_other != 1) || (num_acl_mask != 0 && num_acl_mask != 1))
		return (EINVAL);
	if (((num_acl_group != 0) || (num_acl_user != 0)) &&
	    (num_acl_mask != 1))
		return (EINVAL);
	return (0);
}

/*
 * Given a requested mode for a new object, and a default ACL, combine the
 * two to produce a new mode.  Be careful not to clear any bits that aren't
 * intended to be affected by the POSIX.1e ACL.  Eventually, this might also
 * take the cmask as an argument, if we push that down into
 * per-filesystem-code.
 */
mode_t
acl_posix1e_newfilemode(mode_t cmode, struct acl *dacl)
{
	mode_t mode;

	mode = cmode;
	/*
	 * The current composition policy is that a permission bit must be
	 * set in *both* the ACL and the requested creation mode for it to
	 * appear in the resulting mode/ACL.  First clear any possibly
	 * effected bits, then reconstruct.
	 */
	mode &= ACL_PRESERVE_MASK;
	mode |= (ACL_OVERRIDE_MASK & cmode & acl_posix1e_acl_to_mode(dacl));

	return (mode);
}
