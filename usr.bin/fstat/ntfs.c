/*	$NetBSD: ntfs.c,v 1.3 1999/11/01 19:56:20 jdolecek Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jaromir Dolecek.
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
__RCSID("$NetBSD: ntfs.c,v 1.3 1999/11/01 19:56:20 jdolecek Exp $");

#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#define _KERNEL
#include <sys/mount.h>
#undef _KERNEL

#include <ntfs/ntfs.h>
#undef dprintf
#include <ntfs/ntfs_inode.h>

#include <kvm.h>
#include "fstat.h"

int
ntfs_filestat(vp, fsp)
	struct vnode *vp;
	struct filestat *fsp;
{
	struct ntnode ntnode;
	struct fnode fn;
	struct ntfsmount ntm;

	/* to get the ntnode, we have to go in two steps - firstly
	 * to read appropriate struct fnode and then getting the address
	 * of ntnode and reading it's contents */
	if (!KVM_READ(VTOF(vp), &fn, sizeof (fn))) {
		printf("can't read fnode at %p for pid %d", VTOF(vp), Pid);
		return 0;
	}
	if (!KVM_READ(FTONT(&fn), &ntnode, sizeof (ntnode))) {
		printf("can't read ntnode at %p for pid %d", FTONT(&fn), Pid);
		return 0;
	}
	if (!KVM_READ(ntnode.i_mp, &ntm, sizeof (ntm))) {
		printf("can't read ntfsmount at %p for pid %d",
			FTONT(&fn), Pid);
		return 0;
	}

	fsp->fsid = ntnode.i_dev & 0xffff;
	fsp->fileid = (long)ntnode.i_number;
	fsp->mode = (mode_t)ntm.ntm_mode;
	fsp->size = fn.f_size;
	fsp->rdev = 0;  /* XXX */
	return 1;
}
