/*	$NetBSD: vfs.c,v 1.6.14.1 2018/06/25 07:25:25 pgoyette Exp $	*/

/*-
 * Copyright (c) 2006-2007 Pawel Jakub Dawidek <pjd@FreeBSD.org>
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

#include <sys/cdefs.h>
#define __FBSDID(x)
__FBSDID("$FreeBSD: head/sys/cddl/compat/opensolaris/kern/opensolaris_lookup.c 314194 2017-02-24 07:53:56Z avg $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/cred.h>
#include <sys/vfs.h>
#include <sys/pathname.h>
#include <lib/libkern/libkern.h>

int
lookupname(char *dirname, enum uio_seg seg, enum symfollow follow, vnode_t **dirvpp, vnode_t **compvpp)
{
        return (lookupnameat(dirname, seg, follow, dirvpp, compvpp, NULL));
}

int
lookupnameat(char *dirname, enum uio_seg seg, enum symfollow follow,
    vnode_t **dirvpp, vnode_t **compvpp, vnode_t *startvp)
{

	struct nameidata nd;
	int error;

	error = EOPNOTSUPP;

/*      XXX Disable until I upgrade testing kernel.
        KASSERT(dirvpp == NULL);

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, dirname);

	if ((error = nameiat(&nd, startvp)) != 0)
		return error;

	*compvpp = nd.ni_vp;*/

	return (error);
}


void
vfs_setmntopt(vfs_t *vfsp, const char *name, const char *arg,
    int flags)
{

	if (strcmp("ro", name) == 0)
		vfsp->mnt_flag |= MNT_RDONLY;

	if (strcmp("rw", name) == 0)
		vfsp->mnt_flag &= ~MNT_RDONLY;

	if (strcmp("nodevices", name) == 0)
		vfsp->mnt_flag |= MNT_NODEV;

	if (strcmp("noatime", name) == 0)
		vfsp->mnt_flag |= MNT_NOATIME;

	if (strcmp("atime", name) == 0)
		vfsp->mnt_flag &= ~MNT_NOATIME;

	if (strcmp("nosuid", name) == 0)
		vfsp->mnt_flag |= MNT_NOSUID;

	if (strcmp("suid", name) == 0)
		vfsp->mnt_flag &= ~MNT_NOSUID;

	if (strcmp("noexec", name) == 0)
		vfsp->mnt_flag |= MNT_NOEXEC;
	
	if (strcmp("exec", name) == 0)
		vfsp->mnt_flag &= ~MNT_NOEXEC;

	vfsp->mnt_flag |= MNT_LOCAL;
}

void
vfs_clearmntopt(vfs_t *vfsp, const char *name)
{

	if (strcmp("ro", name) == 0)
		vfsp->mnt_flag |= MNT_RDONLY;

	if (strcmp("rw", name) == 0)
		vfsp->mnt_flag &= ~MNT_RDONLY;

	if (strcmp("nodevices", name) == 0)
		vfsp->mnt_flag &= ~MNT_NODEV;

	if (strcmp("noatime", name) == 0)
		vfsp->mnt_flag &= ~MNT_NOATIME;

	if (strcmp("atime", name) == 0)
		vfsp->mnt_flag |= MNT_NOATIME;

	if (strcmp("nosuid", name) == 0)
		vfsp->mnt_flag &= ~MNT_NOSUID;

	if (strcmp("suid", name) == 0)
		vfsp->mnt_flag |= MNT_NOSUID;

	if (strcmp("noexec", name) == 0)
		vfsp->mnt_flag &= ~MNT_NOEXEC;
	
	if (strcmp("exec", name) == 0)
		vfsp->mnt_flag |= MNT_NOEXEC;
}

int
vfs_optionisset(const vfs_t *vfsp, const char *name, char **argp)
{
	
	if (strcmp("ro", name) == 0)
		return (vfsp->mnt_flag & MNT_RDONLY) != 0;

	if (strcmp("rw", name) == 0)
		return (vfsp->mnt_flag & MNT_RDONLY) == 0;

	if (strcmp("nodevices", name) == 0)
		return (vfsp->mnt_flag & MNT_NODEV) != 0;

	if (strcmp("noatime", name) == 0)
		return (vfsp->mnt_flag & MNT_NOATIME) != 0;

	if (strcmp("atime", name) == 0)
		return (vfsp->mnt_flag & MNT_NOATIME) == 0;

	if (strcmp("nosuid", name) == 0)
		return (vfsp->mnt_flag & MNT_NOSUID) != 0;

	if (strcmp("suid", name) == 0)
		return (vfsp->mnt_flag & MNT_NOSUID) == 0;

	if (strcmp("noexec", name) == 0)
		return (vfsp->mnt_flag & MNT_NOEXEC) != 0;
	
	if (strcmp("exec", name) == 0)
		return (vfsp->mnt_flag & MNT_NOEXEC) == 0;
	
	return 0;
}
