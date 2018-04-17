/*	$NetBSD: compat_sysctl_09_43.c,v 1.1.2.1 2018/04/17 07:24:55 pgoyette Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)vfs_syscalls.c	8.28 (Berkeley) 12/10/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: compat_sysctl_09_43.c,v 1.1.2.1 2018/04/17 07:24:55 pgoyette Exp $");

#if defined(_KERNEL_OPT)
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>
#include <sys/module.h>

#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/syscallargs.h>
#include <sys/vfs_syscalls.h>

#include <compat/sys/mount.h>
#include <compat/common/compat_mod.h>

/*
 * sysctl helper routine for vfs.generic.conf lookups.
 */
#if defined(COMPAT_09) || defined(COMPAT_43) || defined(COMPAT_44)

static int
sysctl_vfs_generic_conf(SYSCTLFN_ARGS)
{
        struct vfsconf vfc;
	struct sysctlnode node;
	struct vfsops *vfsp;
	u_int vfsnum;

	if (namelen != 1)
		return (ENOTDIR);
	vfsnum = name[0];
	if (vfsnum >= nmountcompatnames ||
	    mountcompatnames[vfsnum] == NULL)
		return (EOPNOTSUPP);
	vfsp = vfs_getopsbyname(mountcompatnames[vfsnum]);
	if (vfsp == NULL)
		return (EOPNOTSUPP);

	vfc.vfc_vfsops = vfsp;
	strncpy(vfc.vfc_name, vfsp->vfs_name, sizeof(vfc.vfc_name));
	vfc.vfc_typenum = vfsnum;
	vfc.vfc_refcount = vfsp->vfs_refcount;
	vfc.vfc_flags = 0;
	vfc.vfc_mountroot = vfsp->vfs_mountroot;
	vfc.vfc_next = NULL;
	vfs_delref(vfsp);

	node = *rnode;
	node.sysctl_data = &vfc;
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

/*
 * Top level filesystem related information gathering.
 */
static int
compat_sysctl_vfs(struct sysctllog **clog)
{
	int error;

	error = sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "maxtypenum",
		       SYSCTL_DESCR("Highest valid filesystem type number"),
		       NULL, nmountcompatnames, NULL, 0,
		       CTL_VFS, VFS_GENERIC, VFS_MAXTYPENUM, CTL_EOL);
	if (error == EEXIST)
		error = 0;
	if (error != 0)
		return error;

	error = sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "conf",
		       SYSCTL_DESCR("Filesystem configuration information"),
		       sysctl_vfs_generic_conf, 0, NULL,
		       sizeof(struct vfsconf),
		       CTL_VFS, VFS_GENERIC, VFS_CONF, CTL_EOL);

	return error;
}
#endif

static struct sysctllog *clog = NULL;

int
compat_sysctl_09_43_init(void)
{

	return compat_sysctl_vfs(&clog);
}

int
compat_sysctl_09_43_fini(void)
{

	sysctl_teardown(&clog);
	return 0;
}

#ifdef _MODULE

MODULE (MODULE_CLASS_EXEC, compat_sysctl_09_43, NULL);

static int
compat_sysctl_09_43_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return compat_sysctl_09_43_init();
	case MODULE_CMD_FINI:
		return compat_sysctl_09_43_fini();
	default:
		return ENOTTY;
	}
}
#endif
