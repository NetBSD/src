/*
 * Copyright (c) 1989 The Regents of the University of California.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	from: @(#)vfs_conf.c	7.3 (Berkeley) 6/28/90
 *	$Id: vfs_conf.c,v 1.13 1993/12/18 04:22:43 mycroft Exp $
 */

#include <sys/param.h>
#include <sys/mount.h>

/*
 * These define the root filesystem and device.
 */
struct mount *rootfs;
struct vnode *rootdir;

/*
 * Set up the filesystem operations for vnodes.
 * The types are defined in mount.h.
 */
extern	struct vfsops ufs_vfsops;

#ifdef NFSCLIENT
extern	struct vfsops nfs_vfsops;
#endif

#ifdef MFS
extern	struct vfsops mfs_vfsops;
#endif

#ifdef MSDOSFS
extern	struct vfsops msdosfs_vfsops;
#endif

#ifdef ISOFS
extern	struct vfsops isofs_vfsops;
#endif

#ifdef FDESC
extern	struct vfsops fdesc_vfsops;
#endif

#ifdef KERNFS
extern	struct vfsops kernfs_vfsops;
#endif

#ifdef DEVFS
extern	struct vfsops devfs_vfsops;
#endif

#ifdef PROCFS
extern	struct vfsops procfs_vfsops;
#endif

struct vfsops *vfssw[] = {
	(struct vfsops *)0,	/* 0 = MOUNT_NONE */
	&ufs_vfsops,		/* 1 = MOUNT_UFS */
#ifdef NFSCLIENT
	&nfs_vfsops,		/* 2 = MOUNT_NFS */
#else
	(struct vfsops *)0,
#endif
#ifdef MFS
	&mfs_vfsops,		/* 3 = MOUNT_MFS */
#else
	(struct vfsops *)0,
#endif
#ifdef MSDOSFS
	&msdosfs_vfsops,	/* 4 = MOUNT_MSDOS */
#else
	(struct vfsops *)0,
#endif
#ifdef ISOFS
	&isofs_vfsops,		/* 5 = MOUNT_ISOFS */
#else
	(struct vfsops *)0,
#endif
#ifdef FDESC
	&fdesc_vfsops,		/* 6 = MOUNT_FDESC */
#else
	(struct vfsops *)0,
#endif
#ifdef KERNFS
	&kernfs_vfsops,		/* 7 = MOUNT_KERNFS */
#else
	(struct vfsops *)0,
#endif
#ifdef DEVFS
	&devfs_vfsops,		/* 8 = MOUNT_DEVFS */
#else
	(struct vfsops *)0,
#endif
	(struct vfsops *)0,	/* 9 = MOUNT_AFS */
#ifdef PROCFS
	&procfs_vfsops,		/* 10 = MOUNT_PROCFS */
#else
	(struct vfsops *)0,
#endif
};
