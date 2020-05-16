/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2008-2010 Edward Tomasz Napierała <trasz@FreeBSD.org>
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
 * ACL support routines specific to NFSv4 access control lists.  These are
 * utility routines for code common across file systems implementing NFSv4
 * ACLs.
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
#if 0
__FBSDID("$FreeBSD: head/sys/kern/subr_acl_nfs4.c 341827 2018-12-11 19:32:16Z mjg $");
#endif
__KERNEL_RCSID(0, "$NetBSD: subr_acl_nfs4.c,v 1.1 2020/05/16 18:31:50 christos Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/acl.h>
#include <sys/kauth.h>

static void	acl_nfs4_trivial_from_mode(struct acl *aclp, mode_t mode);


#else

#include <errno.h>
#include <assert.h>
#include <sys/acl.h>
#include <sys/stat.h>
#define KASSERT(a) assert(a)

#endif /* !_KERNEL */

#if 0
static int
_acl_entry_matches(struct acl_entry *ae, acl_tag_t tag, acl_perm_t perm,
    acl_entry_type_t entry_type)
{
	if (ae->ae_tag != tag)
		return (0);

	if (ae->ae_id != ACL_UNDEFINED_ID)
		return (0);

	if (ae->ae_perm != perm)
		return (0);

	if (ae->ae_entry_type != entry_type)
		return (0);

	if (ae->ae_flags != 0)
		return (0);

	return (1);
}
#endif

static struct acl_entry *
_acl_append(struct acl *aclp, acl_tag_t tag, acl_perm_t perm,
    acl_entry_type_t entry_type)
{
	struct acl_entry *ae;

	KASSERT(aclp->acl_cnt + 1 <= ACL_MAX_ENTRIES);

	ae = &(aclp->acl_entry[aclp->acl_cnt]);
	aclp->acl_cnt++;

	ae->ae_tag = tag;
	ae->ae_id = ACL_UNDEFINED_ID;
	ae->ae_perm = perm;
	ae->ae_entry_type = entry_type;
	ae->ae_flags = 0;

	return (ae);
}

#if 0
static struct acl_entry *
_acl_duplicate_entry(struct acl *aclp, unsigned int entry_index)
{
	size_t i;

	KASSERT(aclp->acl_cnt + 1 <= ACL_MAX_ENTRIES);

	for (i = aclp->acl_cnt; i > entry_index; i--)
		aclp->acl_entry[i] = aclp->acl_entry[i - 1];

	aclp->acl_cnt++;

	return (&(aclp->acl_entry[entry_index + 1]));
}
#endif


#ifdef _KERNEL
void
acl_nfs4_sync_acl_from_mode(struct acl *aclp, mode_t mode,
    int file_owner_id)
{

	acl_nfs4_trivial_from_mode(aclp, mode);
}
#endif /* _KERNEL */

void
__acl_nfs4_sync_mode_from_acl(mode_t *_mode, const struct acl *aclp)
{
	size_t i;
	mode_t old_mode = *_mode, mode = 0, seen = 0;
	const struct acl_entry *ae;

	KASSERT(aclp->acl_cnt <= ACL_MAX_ENTRIES);

	/*
	 * NFSv4 Minor Version 1, draft-ietf-nfsv4-minorversion1-03.txt
	 *
	 * 3.16.6.1. Recomputing mode upon SETATTR of ACL
	 */

	for (i = 0; i < aclp->acl_cnt; i++) {
		ae = &(aclp->acl_entry[i]);

		if (ae->ae_entry_type != ACL_ENTRY_TYPE_ALLOW &&
		    ae->ae_entry_type != ACL_ENTRY_TYPE_DENY)
			continue;

		if (ae->ae_flags & ACL_ENTRY_INHERIT_ONLY)
			continue;

		if (ae->ae_tag == ACL_USER_OBJ) {
			if ((ae->ae_perm & ACL_READ_DATA) &&
			    ((seen & S_IRUSR) == 0)) {
				seen |= S_IRUSR;
				if (ae->ae_entry_type == ACL_ENTRY_TYPE_ALLOW)
					mode |= S_IRUSR;
			}
			if ((ae->ae_perm & ACL_WRITE_DATA) &&
			     ((seen & S_IWUSR) == 0)) {
				seen |= S_IWUSR;
				if (ae->ae_entry_type == ACL_ENTRY_TYPE_ALLOW)
					mode |= S_IWUSR;
			}
			if ((ae->ae_perm & ACL_EXECUTE) &&
			    ((seen & S_IXUSR) == 0)) {
				seen |= S_IXUSR;
				if (ae->ae_entry_type == ACL_ENTRY_TYPE_ALLOW)
					mode |= S_IXUSR;
			}
		} else if (ae->ae_tag == ACL_GROUP_OBJ) {
			if ((ae->ae_perm & ACL_READ_DATA) &&
			    ((seen & S_IRGRP) == 0)) {
				seen |= S_IRGRP;
				if (ae->ae_entry_type == ACL_ENTRY_TYPE_ALLOW)
					mode |= S_IRGRP;
			}
			if ((ae->ae_perm & ACL_WRITE_DATA) &&
			    ((seen & S_IWGRP) == 0)) {
				seen |= S_IWGRP;
				if (ae->ae_entry_type == ACL_ENTRY_TYPE_ALLOW)
					mode |= S_IWGRP;
			}
			if ((ae->ae_perm & ACL_EXECUTE) &&
			    ((seen & S_IXGRP) == 0)) {
				seen |= S_IXGRP;
				if (ae->ae_entry_type == ACL_ENTRY_TYPE_ALLOW)
					mode |= S_IXGRP;
			}
		} else if (ae->ae_tag == ACL_EVERYONE) {
			if (ae->ae_perm & ACL_READ_DATA) {
				if ((seen & S_IRUSR) == 0) {
					seen |= S_IRUSR;
					if (ae->ae_entry_type == ACL_ENTRY_TYPE_ALLOW)
						mode |= S_IRUSR;
				}
				if ((seen & S_IRGRP) == 0) {
					seen |= S_IRGRP;
					if (ae->ae_entry_type == ACL_ENTRY_TYPE_ALLOW)
						mode |= S_IRGRP;
				}
				if ((seen & S_IROTH) == 0) {
					seen |= S_IROTH;
					if (ae->ae_entry_type == ACL_ENTRY_TYPE_ALLOW)
						mode |= S_IROTH;
				}
			}
			if (ae->ae_perm & ACL_WRITE_DATA) {
				if ((seen & S_IWUSR) == 0) {
					seen |= S_IWUSR;
					if (ae->ae_entry_type == ACL_ENTRY_TYPE_ALLOW)
						mode |= S_IWUSR;
				}
				if ((seen & S_IWGRP) == 0) {
					seen |= S_IWGRP;
					if (ae->ae_entry_type == ACL_ENTRY_TYPE_ALLOW)
						mode |= S_IWGRP;
				}
				if ((seen & S_IWOTH) == 0) {
					seen |= S_IWOTH;
					if (ae->ae_entry_type == ACL_ENTRY_TYPE_ALLOW)
						mode |= S_IWOTH;
				}
			}
			if (ae->ae_perm & ACL_EXECUTE) {
				if ((seen & S_IXUSR) == 0) {
					seen |= S_IXUSR;
					if (ae->ae_entry_type == ACL_ENTRY_TYPE_ALLOW)
						mode |= S_IXUSR;
				}
				if ((seen & S_IXGRP) == 0) {
					seen |= S_IXGRP;
					if (ae->ae_entry_type == ACL_ENTRY_TYPE_ALLOW)
						mode |= S_IXGRP;
				}
				if ((seen & S_IXOTH) == 0) {
					seen |= S_IXOTH;
					if (ae->ae_entry_type == ACL_ENTRY_TYPE_ALLOW)
						mode |= S_IXOTH;
				}
			}
		}
	}

	*_mode = mode | (old_mode & ACL_PRESERVE_MASK);
}

/*
 * Populate the ACL with entries inherited from parent_aclp.
 */
static void		
acl_nfs4_inherit_entries(const struct acl *parent_aclp,
    struct acl *child_aclp, mode_t mode, int file_owner_id,
    int is_directory)
{
	int flags, tag;
	size_t i;
	const struct acl_entry *parent_entry;
	struct acl_entry *ae;

	KASSERT(parent_aclp->acl_cnt <= ACL_MAX_ENTRIES);

	for (i = 0; i < parent_aclp->acl_cnt; i++) {
		parent_entry = &(parent_aclp->acl_entry[i]);
		flags = parent_entry->ae_flags;
		tag = parent_entry->ae_tag;

		/*
		 * Don't inherit owner@, group@, or everyone@ entries.
		 */
		if (tag == ACL_USER_OBJ || tag == ACL_GROUP_OBJ ||
		    tag == ACL_EVERYONE)
			continue;

		/*
		 * Entry is not inheritable at all.
		 */
		if ((flags & (ACL_ENTRY_DIRECTORY_INHERIT |
		    ACL_ENTRY_FILE_INHERIT)) == 0)
			continue;

		/*
		 * We're creating a file, but ae is not inheritable
		 * by files.
		 */
		if (!is_directory && (flags & ACL_ENTRY_FILE_INHERIT) == 0)
			continue;

		/*
		 * Entry is inheritable only by files, but has NO_PROPAGATE
		 * flag set, and we're creating a directory, so it wouldn't
		 * propagate to any file in that directory anyway.
		 */
		if (is_directory &&
		    (flags & ACL_ENTRY_DIRECTORY_INHERIT) == 0 &&
		    (flags & ACL_ENTRY_NO_PROPAGATE_INHERIT))
			continue;

		/*
		 * Entry qualifies for being inherited.
		 */
		KASSERT(child_aclp->acl_cnt + 1 <= ACL_MAX_ENTRIES);
		ae = &(child_aclp->acl_entry[child_aclp->acl_cnt]);
		*ae = *parent_entry;
		child_aclp->acl_cnt++;

		ae->ae_flags &= ~ACL_ENTRY_INHERIT_ONLY;
		ae->ae_flags |= ACL_ENTRY_INHERITED;

		/*
		 * If the type of the ACE is neither ALLOW nor DENY,
		 * then leave it as it is and proceed to the next one.
		 */
		if (ae->ae_entry_type != ACL_ENTRY_TYPE_ALLOW &&
		    ae->ae_entry_type != ACL_ENTRY_TYPE_DENY)
			continue;

		/*
		 * If the ACL_ENTRY_NO_PROPAGATE_INHERIT is set, or if
		 * the object being created is not a directory, then clear
		 * the following flags: ACL_ENTRY_NO_PROPAGATE_INHERIT,
		 * ACL_ENTRY_FILE_INHERIT, ACL_ENTRY_DIRECTORY_INHERIT,
		 * ACL_ENTRY_INHERIT_ONLY.
		 */
		if (ae->ae_flags & ACL_ENTRY_NO_PROPAGATE_INHERIT ||
		    !is_directory) {
			ae->ae_flags &= ~(ACL_ENTRY_NO_PROPAGATE_INHERIT |
			ACL_ENTRY_FILE_INHERIT | ACL_ENTRY_DIRECTORY_INHERIT |
			ACL_ENTRY_INHERIT_ONLY);
		}

		/*
		 * If the object is a directory and ACL_ENTRY_FILE_INHERIT
		 * is set, but ACL_ENTRY_DIRECTORY_INHERIT is not set, ensure
		 * that ACL_ENTRY_INHERIT_ONLY is set.
		 */
		if (is_directory &&
		    (ae->ae_flags & ACL_ENTRY_FILE_INHERIT) &&
		    ((ae->ae_flags & ACL_ENTRY_DIRECTORY_INHERIT) == 0)) {
			ae->ae_flags |= ACL_ENTRY_INHERIT_ONLY;
		}

		if (ae->ae_entry_type == ACL_ENTRY_TYPE_ALLOW &&
		    (ae->ae_flags & ACL_ENTRY_INHERIT_ONLY) == 0) {
			/*
			 * Some permissions must never be inherited.
			 */
			ae->ae_perm &= ~(ACL_WRITE_ACL | ACL_WRITE_OWNER |
			    ACL_WRITE_NAMED_ATTRS | ACL_WRITE_ATTRIBUTES);

			/*
			 * Others must be masked according to the file mode.
			 */
			if ((mode & S_IRGRP) == 0)
				ae->ae_perm &= ~ACL_READ_DATA;
			if ((mode & S_IWGRP) == 0)
				ae->ae_perm &=
				    ~(ACL_WRITE_DATA | ACL_APPEND_DATA);
			if ((mode & S_IXGRP) == 0)
				ae->ae_perm &= ~ACL_EXECUTE;
		}
	}
}

/*
 * Calculate inherited ACL in a manner compatible with PSARC/2010/029.
 * It's also being used to calculate a trivial ACL, by inheriting from
 * a NULL ACL.
 */
static void		
acl_nfs4_compute_inherited_acl_psarc(const struct acl *parent_aclp,
    struct acl *aclp, mode_t mode, int file_owner_id, int is_directory)
{
	acl_perm_t user_allow_first = 0, user_deny = 0, group_deny = 0;
	acl_perm_t user_allow, group_allow, everyone_allow;

	KASSERT(aclp->acl_cnt == 0);

	user_allow = group_allow = everyone_allow = ACL_READ_ACL |
	    ACL_READ_ATTRIBUTES | ACL_READ_NAMED_ATTRS | ACL_SYNCHRONIZE;
	user_allow |= ACL_WRITE_ACL | ACL_WRITE_OWNER | ACL_WRITE_ATTRIBUTES |
	    ACL_WRITE_NAMED_ATTRS;

	if (mode & S_IRUSR)
		user_allow |= ACL_READ_DATA;
	if (mode & S_IWUSR)
		user_allow |= (ACL_WRITE_DATA | ACL_APPEND_DATA);
	if (mode & S_IXUSR)
		user_allow |= ACL_EXECUTE;

	if (mode & S_IRGRP)
		group_allow |= ACL_READ_DATA;
	if (mode & S_IWGRP)
		group_allow |= (ACL_WRITE_DATA | ACL_APPEND_DATA);
	if (mode & S_IXGRP)
		group_allow |= ACL_EXECUTE;

	if (mode & S_IROTH)
		everyone_allow |= ACL_READ_DATA;
	if (mode & S_IWOTH)
		everyone_allow |= (ACL_WRITE_DATA | ACL_APPEND_DATA);
	if (mode & S_IXOTH)
		everyone_allow |= ACL_EXECUTE;

	user_deny = ((group_allow | everyone_allow) & ~user_allow);
	group_deny = everyone_allow & ~group_allow;
	user_allow_first = group_deny & ~user_deny;

	if (user_allow_first != 0)
		_acl_append(aclp, ACL_USER_OBJ, user_allow_first,
		    ACL_ENTRY_TYPE_ALLOW);
	if (user_deny != 0)
		_acl_append(aclp, ACL_USER_OBJ, user_deny,
		    ACL_ENTRY_TYPE_DENY);
	if (group_deny != 0)
		_acl_append(aclp, ACL_GROUP_OBJ, group_deny,
		    ACL_ENTRY_TYPE_DENY);

	if (parent_aclp != NULL)
		acl_nfs4_inherit_entries(parent_aclp, aclp, mode,
		    file_owner_id, is_directory);

	_acl_append(aclp, ACL_USER_OBJ, user_allow, ACL_ENTRY_TYPE_ALLOW);
	_acl_append(aclp, ACL_GROUP_OBJ, group_allow, ACL_ENTRY_TYPE_ALLOW);
	_acl_append(aclp, ACL_EVERYONE, everyone_allow, ACL_ENTRY_TYPE_ALLOW);
}

#ifdef _KERNEL
void		
acl_nfs4_compute_inherited_acl(const struct acl *parent_aclp,
    struct acl *child_aclp, mode_t mode, int file_owner_id,
    int is_directory)
{

	acl_nfs4_compute_inherited_acl_psarc(parent_aclp, child_aclp,
	    mode, file_owner_id, is_directory);
}
#endif /* _KERNEL */

/*
 * Calculate trivial ACL in a manner compatible with PSARC/2010/029.
 * Note that this results in an ACL different from (but semantically
 * equal to) the "canonical six" trivial ACL computed using algorithm
 * described in draft-ietf-nfsv4-minorversion1-03.txt, 3.16.6.2.
 */
static void
acl_nfs4_trivial_from_mode(struct acl *aclp, mode_t mode)
{

	aclp->acl_cnt = 0;
	acl_nfs4_compute_inherited_acl_psarc(NULL, aclp, mode, -1, -1);
}

#ifndef _KERNEL
/*
 * This routine is used by libc to implement acl_strip_np(3)
 * and acl_is_trivial_np(3).
 */
void
__acl_nfs4_trivial_from_mode_libc(struct acl *aclp, int mode, int canonical_six)
{

	aclp->acl_cnt = 0;
	acl_nfs4_trivial_from_mode(aclp, mode);
}
#endif /* !_KERNEL */

#ifdef _KERNEL
static int
_acls_are_equal(const struct acl *a, const struct acl *b)
{
	int i;
	const struct acl_entry *entrya, *entryb;

	if (a->acl_cnt != b->acl_cnt)
		return (0);

	for (i = 0; i < b->acl_cnt; i++) {
		entrya = &(a->acl_entry[i]);
		entryb = &(b->acl_entry[i]);

		if (entrya->ae_tag != entryb->ae_tag ||
		    entrya->ae_id != entryb->ae_id ||
		    entrya->ae_perm != entryb->ae_perm ||
		    entrya->ae_entry_type != entryb->ae_entry_type ||
		    entrya->ae_flags != entryb->ae_flags)
			return (0);
	}

	return (1);
}

/*
 * This routine is used to determine whether to remove extended attribute
 * that stores ACL contents.
 */
int
acl_nfs4_is_trivial(const struct acl *aclp, int file_owner_id)
{
	int trivial;
	mode_t tmpmode = 0;
	struct acl *tmpaclp;

	if (aclp->acl_cnt > 6)
		return (0);

	/*
	 * Compute the mode from the ACL, then compute new ACL from that mode.
	 * If the ACLs are identical, then the ACL is trivial.
	 *
	 * XXX: I guess there is a faster way to do this.  However, even
	 *      this slow implementation significantly speeds things up
	 *      for files that don't have non-trivial ACLs - it's critical
	 *      for performance to not use EA when they are not needed.
	 *
	 * First try the PSARC/2010/029 semantics.
	 */
	tmpaclp = acl_alloc(KM_SLEEP);
	__acl_nfs4_sync_mode_from_acl(&tmpmode, aclp);
	acl_nfs4_trivial_from_mode(tmpaclp, tmpmode);
	trivial = _acls_are_equal(aclp, tmpaclp);
	if (trivial) {
		acl_free(tmpaclp);
		return (trivial);
	}

	/*
	 * Check if it's a draft-ietf-nfsv4-minorversion1-03.txt trivial ACL.
	 */
	acl_free(tmpaclp);

	return (trivial);
}

int
acl_nfs4_check(const struct acl *aclp, int is_directory)
{
	size_t i;
	const struct acl_entry *ae;

	/*
	 * The spec doesn't seem to say anything about ACL validity.
	 * It seems there is not much to do here.  There is even no need
	 * to count "owner@" or "everyone@" (ACL_USER_OBJ and ACL_EVERYONE)
	 * entries, as there can be several of them and that's perfectly
	 * valid.  There can be none of them too.  Really.
	 */

	if (aclp->acl_cnt > ACL_MAX_ENTRIES || aclp->acl_cnt <= 0)
		return (EINVAL);

	for (i = 0; i < aclp->acl_cnt; i++) {
		ae = &(aclp->acl_entry[i]);

		switch (ae->ae_tag) {
		case ACL_USER_OBJ:
		case ACL_GROUP_OBJ:
		case ACL_EVERYONE:
			if (ae->ae_id != ACL_UNDEFINED_ID)
				return (EINVAL);
			break;

		case ACL_USER:
		case ACL_GROUP:
			if (ae->ae_id == ACL_UNDEFINED_ID)
				return (EINVAL);
			break;

		default:
			return (EINVAL);
		}

		if ((ae->ae_perm | ACL_NFS4_PERM_BITS) != ACL_NFS4_PERM_BITS)
			return (EINVAL);

		/*
		 * Disallow ACL_ENTRY_TYPE_AUDIT and ACL_ENTRY_TYPE_ALARM for now.
		 */
		if (ae->ae_entry_type != ACL_ENTRY_TYPE_ALLOW &&
		    ae->ae_entry_type != ACL_ENTRY_TYPE_DENY)
			return (EINVAL);

		if ((ae->ae_flags | ACL_FLAGS_BITS) != ACL_FLAGS_BITS)
			return (EINVAL);

		/* Disallow unimplemented flags. */
		if (ae->ae_flags & (ACL_ENTRY_SUCCESSFUL_ACCESS |
		    ACL_ENTRY_FAILED_ACCESS))
			return (EINVAL);

		/* Disallow flags not allowed for ordinary files. */
		if (!is_directory) {
			if (ae->ae_flags & (ACL_ENTRY_FILE_INHERIT |
			    ACL_ENTRY_DIRECTORY_INHERIT |
			    ACL_ENTRY_NO_PROPAGATE_INHERIT | ACL_ENTRY_INHERIT_ONLY))
				return (EINVAL);
		}
	}

	return (0);
}
#endif
