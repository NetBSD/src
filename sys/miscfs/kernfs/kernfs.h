/*
 * Copyright (c) 1992 The Regents of the University of California
 * Copyright (c) 1990, 1992 Jan-Simon Pendry
 * All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
 * Jan-Simon Pendry.
 *
 * %sccs.redist.c%
 *
 *	%W% (Berkeley) %G%
 *
 * $Id: kernfs.h,v 1.1 1993/03/23 23:56:53 cgd Exp $
 */

#ifdef KERNEL
struct kernfs_mount {
	struct vnode	*kf_root;	/* Root node */
};

struct kernfs_node {
	struct kern_target *kf_kt;
};

#define VFSTOKERNFS(mp)	((struct kernfs_mount *)((mp)->mnt_data))
#define	VTOKERN(vp) ((struct kernfs_node *)(vp)->v_data)

extern struct vnodeops kernfs_vnodeops;
extern struct vfsops kernfs_vfsops;
#endif /* KERNEL */
