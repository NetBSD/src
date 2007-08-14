/*	$NetBSD: ukfs.c,v 1.1 2007/08/14 15:56:17 pooka Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Finnish Cultural Foundation.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * This library enables access to files systems directly without
 * involving system calls.
 */

#define __NAMEIFLAGS_EXPOSE
#define __UIO_EXPOSE
#define __VFSOPS_EXPOSE
#include <sys/types.h>
#include <sys/namei.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/vnode.h>
#include <sys/vnode_if.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rump.h"
#include "ukfs.h"

struct ukfs {
	struct mount *ukfs_mp;
	struct vnode *ukfs_rvp;
};

struct mount *
ukfs_getmp(struct ukfs *ukfs)
{

	return ukfs->ukfs_mp;
}

struct vnode *
ukfs_getrvp(struct ukfs *ukfs)
{

	return ukfs->ukfs_rvp;
}

int
ukfs_init()
{

	rump_init();

	return 0;
}

struct ukfs *
ukfs_mount(const char *vfsname, const char *devpath, const char *mountpath,
	int mntflags, void *arg, size_t alen)
{
	struct ukfs *fs;
	struct vfsops *vfsops;
	struct mount *mp;
	int rv = 0;

	vfsops = rump_vfs_getopsbyname(vfsname);
	if (vfsops == NULL)
		return NULL;

	rump_mountinit(&mp, vfsops);

	fs = malloc(sizeof(struct ukfs));
	if (fs == NULL)
		return NULL;
	memset(fs, 0, sizeof(struct ukfs));

	mp->mnt_flag = mntflags;
	rump_fakeblk_register(devpath);
	rv = VFS_MOUNT(mp, mountpath, arg, &alen, curlwp);
	if (rv) {
		warnx("VFS_MOUNT %d", rv);
		goto out;
	}

	/* XXX: this doesn't belong here, but it'll be gone soon altogether */
	if ((1<<mp->mnt_fs_bshift) < getpagesize()
	    && (mntflags & MNT_RDONLY) == 0) {
		rv = EOPNOTSUPP;
		warnx("Sorry, fs bsize < PAGE_SIZE not yet supported for rw");
		goto out;
	}
	fs->ukfs_mp = mp;
	rump_fakeblk_deregister(devpath);

	rv = VFS_STATVFS(mp, &mp->mnt_stat, NULL);
	if (rv) {
		warnx("VFS_STATVFS %d", rv);
		goto out;
	}

	rv = VFS_ROOT(mp, &fs->ukfs_rvp);
	if (rv) {
		warnx("VFS_ROOT %d", rv);
		goto out;
	}

 out:
	if (rv) {
		if (fs->ukfs_mp)
			rump_mountdestroy(fs->ukfs_mp);
		free(fs);
		errno = rv;
		fs = NULL;
	}

	return fs;
}

void
ukfs_unmount(struct ukfs *fs, int dounmount)
{
	int rv;

	if (dounmount) {
		rv = VFS_SYNC(fs->ukfs_mp, MNT_WAIT, rump_cred, curlwp);
		rv += VFS_UNMOUNT(fs->ukfs_mp, 0, curlwp);
		assert(rv == 0);
	}

	rump_mountdestroy(fs->ukfs_mp);

	free(fs);
}
