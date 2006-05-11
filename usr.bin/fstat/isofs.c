/*	$NetBSD: isofs.c,v 1.6 2006/05/11 11:56:38 yamt Exp $	*/
/*-
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: isofs.c,v 1.6 2006/05/11 11:56:38 yamt Exp $");

#include <sys/param.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/mount.h>

#include <isofs/cd9660/iso.h>
#include <isofs/cd9660/cd9660_extern.h>
#include <isofs/cd9660/cd9660_node.h>

#include <kvm.h>
#include <err.h>
#include "fstat.h"

int
isofs_filestat(struct vnode *vp, struct filestat *fsp)
{
	struct iso_node inode;

	if (!KVM_READ(VTOI(vp), &inode, sizeof (inode))) {
		dprintf("can't read inode at %p for pid %d", VTOI(vp), Pid);
		return 0;
	}
	fsp->fsid = inode.i_dev & 0xffff;
	fsp->fileid = (long)inode.i_number;
	fsp->mode = inode.inode.iso_mode;
	fsp->size = inode.i_size;
	fsp->rdev = inode.i_dev;
	return 1;
}
