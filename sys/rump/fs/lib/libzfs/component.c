/*	$NetBSD: component.c,v 1.1 2011/01/18 22:21:23 haad Exp $ */

/*
 * Copyright (c) 2010 Antti Kantee.  All Rights Reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: component.c,v 1.1 2011/01/18 22:21:23 haad Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/stat.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include <sys/buf.h>
#include <sys/modctl.h>
#include <sys/open.h>
#include <sys/file.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/cmn_err.h>
#include <sys/stat.h>
#include <sys/zfs_ioctl.h>
#include <sys/zfs_vfsops.h>
#include <sys/zfs_znode.h>
#include <sys/zap.h>
#include <sys/spa.h>
#include <sys/spa_impl.h>
#include <sys/vdev.h>
#include <sys/priv_impl.h>
#include <sys/dmu.h>
#include <sys/dsl_dir.h>
#include <sys/dsl_dataset.h>
#include <sys/dsl_prop.h>
#include <sys/dsl_deleg.h>
#include <sys/dmu_objset.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/sunldi.h>
#include <sys/policy.h>
#include <sys/zone.h>
#include <sys/nvpair.h>
#include <sys/pathname.h>
#include <sys/mount.h>
#include <sys/sdt.h>
#include <sys/fs/zfs.h>
#include <sys/zfs_ctldir.h>
#include <sys/zfs_dir.h>
#include <sys/zvol.h>
#include <sharefs/share.h>
#include <sys/dmu_objset.h>
#include <sys/callb.h>
#include <sys/taskq.h>

#include "rump_private.h"
#include "rump_dev_private.h"
#include "rump_vfs_private.h"

RUMP_COMPONENT(RUMP_COMPONENT_KERN_VFS)
{
	int error;

	extern int physmem;

	extern uint_t zfs_fsyncer_key;
	extern uint_t rrw_tsd_key;

	extern const struct bdevsw zfs_bdevsw;
	extern const struct cdevsw zfs_cdevsw;

	static int	zfs_bmajor = -1;
	static int	zfs_cmajor = -1;

	/* Set physmem to fit zfs limits. ZFS_MINMEGS * 2048 / PAGE_SIZE */
	physmem = 262144;
	error = devsw_attach("zfs", &zfs_bdevsw, &zfs_bmajor,
	    &zfs_cdevsw, &zfs_cmajor);
	if (error != 0) {
		zvol_fini();
		zfs_vfsfini();
		spa_fini();
		lwp_specific_key_delete(zfs_fsyncer_key);
		lwp_specific_key_delete(rrw_tsd_key);
	}
	return;
}
