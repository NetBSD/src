/*	$NetBSD: ptyfs.c,v 1.1 2004/12/12 22:41:04 christos Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__RCSID("$NetBSD: ptyfs.c,v 1.1 2004/12/12 22:41:04 christos Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#include <sys/mount.h>

#define _KERNEL
#include <fs/ptyfs/ptyfs.h>
#undef _KERNEL

#include <paths.h>
#include <err.h>
#include <kvm.h>
#include <dirent.h>
#include "fstat.h"

static mode_t getptsmajor(void);

/* XXX: Dup code from gen/devname.c */
static mode_t
getptsmajor(void)
{
	DIR *dirp;
	struct dirent *dp;
	struct stat st;
	char buf[MAXPATHLEN];

	if ((dirp = opendir(_PATH_DEV_PTS)) == NULL)
		return (mode_t)~0;

	while ((dp = readdir(dirp)) != NULL) {
		if (dp->d_name[0] == '.')
			continue;
		(void)snprintf(buf, sizeof(buf), "%s%s", _PATH_DEV_PTS,
		    dp->d_name);
		if (stat(buf, &st) == -1)
			continue;
		(void)closedir(dirp);
		return major(st.st_rdev);
	}
	(void)closedir(dirp);
	return (mode_t)~0;
}

int
ptyfs_filestat(struct vnode *vp, struct filestat *fsp)
{
	struct ptyfsnode pn;
	static mode_t maj = 0;

	if (!KVM_READ(VTOPTYFS(vp), &pn, sizeof(pn))) {
		dprintf("can't read ptyfs_node at %p for pid %d",
		    VTOPTYFS(vp), Pid);
		return 0;
	}
	fsp->fsid = 0; /* XXX */
	fsp->fileid = (long)pn.ptyfs_fileno;
	fsp->mode = pn.ptyfs_mode;
	fsp->size = 0;
	if (maj == 0)
		maj = getptsmajor();
	if (maj == ~0)
		fsp->rdev = makedev(maj, pn.ptyfs_pty);
	return 1;
}
