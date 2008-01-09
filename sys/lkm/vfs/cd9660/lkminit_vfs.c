/* $NetBSD: lkminit_vfs.c,v 1.4.46.1 2008/01/09 01:56:56 matt Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: lkminit_vfs.c,v 1.4.46.1 2008/01/09 01:56:56 matt Exp $");

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

#include <fs/cd9660/iso.h>
#include <fs/cd9660/cd9660_extern.h>

int cd9660_lkmentry(struct lkm_table *, int, int);

/*
 * This is the vfsops table for the file system in question
 */
extern struct vfsops cd9660_vfsops;

/*
 * declare the filesystem
 */
MOD_VFS("cd9660", -1, &cd9660_vfsops);

/*
 * take care of fs specific sysctl nodes
 */
static int load(struct lkm_table *, int);
static int unload(struct lkm_table *, int);
static struct sysctllog *_cd9660_log;

/*
 * entry point
 */
int
cd9660_lkmentry(lkmtp, cmd, ver)
	struct lkm_table *lkmtp;
	int cmd;
	int ver;
{

	DISPATCH(lkmtp, cmd, ver, load, unload, lkm_nofunc)
}

static int
load(lkmtp, cmd)
	struct lkm_table *lkmtp;
	int cmd;
{

	sysctl_vfs_cd9660_setup(&_cd9660_log);
	return (0);
}

static int
unload(lkmtp, cmd)
	struct lkm_table *lkmtp;
	int cmd;
{

	sysctl_teardown(&_cd9660_log);
	return (0);
}
