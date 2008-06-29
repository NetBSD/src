/* $NetBSD: lkminit_vfs.c,v 1.9.6.2 2008/06/29 09:33:15 mjf Exp $ */

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael Graff <explorer@flame.org>.
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
__KERNEL_RCSID(0, "$NetBSD: lkminit_vfs.c,v 1.9.6.2 2008/06/29 09:33:15 mjf Exp $");

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/lkm.h>
#include <sys/file.h>
#include <sys/errno.h>

#include <ufs/ffs/ffs_extern.h>

int ffs_lkmentry(struct lkm_table *, int, int);

/*
 * This is the vfsops table for the file system in question
 */
extern struct vfsops ffs_vfsops;

/*
 * take care of fs specific sysctl nodes
 */
static int load(struct lkm_table *, int);
static int unload(struct lkm_table *, int);
static struct sysctllog *_ffs_log;

extern int ffs_log_changeopt;

/*
 * declare the filesystem
 */
MOD_VFS("ffs", -1, &ffs_vfsops);

/*
 * entry point
 */
int
ffs_lkmentry(lkmtp, cmd, ver)
	struct lkm_table *lkmtp;
	int cmd;
	int ver;
{

	DISPATCH(lkmtp, cmd, ver, load, unload, lkm_nofunc)
}

int
load(lkmtp, cmd)
	struct lkm_table *lkmtp;
	int cmd;
{

	sysctl_createv(&_ffs_log, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "vfs", NULL,
		       NULL, 0, NULL, 0,
		       CTL_VFS, CTL_EOL);
	sysctl_createv(&_ffs_log, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "ffs",
		       SYSCTL_DESCR("Berkeley Fast File System"),
		       NULL, 0, NULL, 0,
		       CTL_VFS, 1, CTL_EOL);

	/*
	 * @@@ should we even bother with these first three?
	 */
	sysctl_createv(&_ffs_log, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "doclusterread", NULL,
		       sysctl_notavail, 0, NULL, 0,
		       CTL_VFS, 1, FFS_CLUSTERREAD, CTL_EOL);
	sysctl_createv(&_ffs_log, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "doclusterwrite", NULL,
		       sysctl_notavail, 0, NULL, 0,
		       CTL_VFS, 1, FFS_CLUSTERWRITE, CTL_EOL);
	sysctl_createv(&_ffs_log, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "doreallocblks", NULL,
		       sysctl_notavail, 0, NULL, 0,
		       CTL_VFS, 1, FFS_REALLOCBLKS, CTL_EOL);
#if 0
	sysctl_createv(&_ffs_log, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "doasyncfree",
		       SYSCTL_DESCR("Release dirty blocks asynchronously"),
		       NULL, 0, &doasyncfree, 0,
		       CTL_VFS, 1, FFS_ASYNCFREE, CTL_EOL);
#endif
	sysctl_createv(&_ffs_log, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "log_changeopt",
		       SYSCTL_DESCR("Log changes in optimization strategy"),
		       NULL, 0, &ffs_log_changeopt, 0,
		       CTL_VFS, 1, FFS_LOG_CHANGEOPT, CTL_EOL);
	return (0);
}

int
unload(lkmtp, cmd)
	struct lkm_table *lkmtp;
	int cmd;
{

	sysctl_teardown(&_ffs_log);
	return (0);
}
