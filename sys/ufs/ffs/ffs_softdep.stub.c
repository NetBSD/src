/*	$NetBSD: ffs_softdep.stub.c,v 1.19 2006/10/13 10:21:21 hannken Exp $	*/

/*
 * Copyright 1997 Marshall Kirk McKusick. All Rights Reserved.
 *
 * This code is derived from work done by Greg Ganger at the
 * University of Michigan.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. None of the names of McKusick, Ganger, or the University of Michigan
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MARSHALL KIRK MCKUSICK ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL MARSHALL KIRK MCKUSICK BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ffs_softdep.stub.c	9.1 (McKusick) 7/9/97
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ffs_softdep.stub.c,v 1.19 2006/10/13 10:21:21 hannken Exp $");

#include <sys/param.h>
#include <sys/vnode.h>
#include <sys/systm.h>
#include <ufs/ufs/inode.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>
#include <ufs/ufs/ufs_extern.h>

int
softdep_flushworklist(struct mount *oldmnt __unused, int *countp __unused,
    struct lwp *l __unused)
{

	panic("softdep_flushworklist called");
}

int
softdep_flushfiles(struct mount *oldmnt __unused, int flags __unused,
    struct lwp *l __unused)
{

	panic("softdep_flushfiles called");
}

int
softdep_mount(struct vnode *devvp __unused, struct mount *mp __unused,
    struct fs *fs __unused, kauth_cred_t cred __unused)
{

	return (0);
}

void
softdep_initialize(void)
{

	return;
}

void
softdep_reinitialize(void)
{

	return;
}

void
softdep_setup_inomapdep(struct buf *bp __unused, struct inode *ip __unused,
    ino_t newinum __unused)
{

	panic("softdep_setup_inomapdep called");
}

void
softdep_setup_blkmapdep(struct buf *bp __unused, struct fs *fs __unused,
    daddr_t newblkno __unused)
{

	panic("softdep_setup_blkmapdep called");
}

void
softdep_setup_allocdirect(struct inode *ip __unused, daddr_t lbn __unused,
    daddr_t newblkno __unused, daddr_t oldblkno __unused,
    long newsize __unused, long oldsize __unused, struct buf *bp __unused)
{

	panic("softdep_setup_allocdirect called");
}

void
softdep_setup_allocindir_page(struct inode *ip __unused, daddr_t lbn __unused,
    struct buf *bp __unused, int ptrno __unused, daddr_t newblkno __unused,
    daddr_t oldblkno __unused, struct buf *nbp __unused)
{

	panic("softdep_setup_allocindir_page called");
}

void
softdep_setup_allocindir_meta(struct buf *nbp __unused,
    struct inode *ip __unused, struct buf *bp __unused, int ptrno __unused,
    daddr_t newblkno __unused)
{

	panic("softdep_setup_allocindir_meta called");
}

void
softdep_setup_freeblocks(struct inode *ip __unused, off_t length __unused,
    int flags __unused)
{

	panic("softdep_setup_freeblocks called");
}

void
softdep_freefile(struct vnode *v __unused, ino_t ino __unused,
    int mode __unused)
{
	panic("softdep_freefile called");
}

int
softdep_setup_directory_add(struct buf *bp __unused, struct inode *dp __unused,
    off_t diroffset __unused, ino_t newinum __unused,
    struct buf *newdirbp __unused, int isnewblk __unused)
{

	panic("softdep_setup_directory_add called");
}

void
softdep_change_directoryentry_offset(struct inode *dp __unused,
    caddr_t base __unused __unused, caddr_t oldloc __unused,
    caddr_t newloc __unused, int entrysize __unused)
{

	panic("softdep_change_directoryentry_offset called");
}

void
softdep_setup_remove(struct buf *bp __unused, struct inode *dp __unused,
    struct inode *ip __unused, int isrmdir __unused)
{

	panic("softdep_setup_remove called");
}

void
softdep_setup_directory_change(struct buf *bp __unused,
    struct inode *dp __unused, struct inode *ip __unused,
    ino_t newinum __unused, int isrmdir __unused)
{

	panic("softdep_setup_directory_change called");
}

void
softdep_change_linkcnt(struct inode *ip __unused)
{

	panic("softdep_change_linkcnt called");
}

void
softdep_load_inodeblock(struct inode *ip __unused)
{

	panic("softdep_load_inodeblock called");
}

void
softdep_update_inodeblock(struct inode *ip __unused, struct buf *bp __unused,
    int waitfor __unused)
{

	panic("softdep_update_inodeblock called");
}

void
softdep_fsync_mountdev(struct vnode *vp __unused)
{
	panic("softdep_fsync_mountdev called");
}

int
softdep_sync_metadata(void *v __unused)
{
	return (0);
}

void
softdep_releasefile(struct inode *ip __unused)
{
	panic("softdep_releasefile called");
}
