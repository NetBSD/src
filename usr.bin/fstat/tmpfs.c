/*	$NetBSD: tmpfs.c,v 1.3 2006/05/11 11:56:38 yamt Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: tmpfs.c,v 1.3 2006/05/11 11:56:38 yamt Exp $");

#define __POOL_EXPOSE
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#include <sys/mount.h>

#include <fs/tmpfs/tmpfs.h>

#include <err.h>
#include <kvm.h>
#include "fstat.h"

int
tmpfs_filestat(struct vnode *vp, struct filestat *fsp)
{
	struct tmpfs_node tn;
	struct mount mt;

	if (!KVM_READ(VP_TO_TMPFS_NODE(vp), &tn, sizeof(tn))) {
		dprintf("can't read tmpfs_node at %p for pid %d",
		    VP_TO_TMPFS_NODE(vp), Pid);
		return 0;
	}
	if (!KVM_READ(vp->v_mount, &mt, sizeof(mt))) {
		dprintf("can't read mount at %p for pid %d",
		    vp->v_mount, Pid);
		return 0;
	}

	fsp->fsid = mt.mnt_stat.f_fsidx.__fsid_val[0];
	fsp->fileid = (long)tn.tn_id;
	fsp->mode = tn.tn_mode | getftype(vp->v_type);
	fsp->size = tn.tn_size;
	switch (tn.tn_type) {
	case VBLK:
	case VCHR:
		fsp->rdev = tn.tn_spec.tn_dev.tn_rdev;
		break;
	default:
		fsp->rdev = 0;
		break;
	}

	return 1;
}
