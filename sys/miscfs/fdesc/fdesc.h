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
 * $Id: fdesc.h,v 1.1 1993/03/23 23:56:31 cgd Exp $
 */

#ifdef KERNEL
struct fdescmount {
	struct vnode	*f_root;	/* Root node */
};

struct fdescnode {
	unsigned	f_fd;			/* Fd to be dup'ed */
	/*int		f_isroot;*/		/* Is this the root */
};

#define VFSTOFDESC(mp)	((struct fdescmount *)((mp)->mnt_data))
#define	VTOFDESC(vp) ((struct fdescnode *)(vp)->v_data)

extern struct vnodeops fdesc_vnodeops;
extern struct vfsops fdesc_vfsops;
#endif /* KERNEL */
