/*	$NetBSD: rumpfs.c,v 1.6.4.2 2009/01/19 13:20:27 skrll Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by Google Summer of Code.
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
__KERNEL_RCSID(0, "$NetBSD: rumpfs.c,v 1.6.4.2 2009/01/19 13:20:27 skrll Exp $");

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/kauth.h>
#include <sys/lock.h>
#include <sys/lockf.h>
#include <sys/stat.h>
#include <sys/namei.h>
#include <sys/queue.h>
#include <sys/filedesc.h>
#include <sys/syscallargs.h>

#include <miscfs/fifofs/fifo.h>
#include <miscfs/specfs/specdev.h>
#include <miscfs/syncfs/syncfs.h>
#include <miscfs/genfs/genfs.h>

#include <rump/rumpuser.h>

#include "rump_private.h"
#include "rump_vfs_private.h"

static int rump_vop_lookup(void *);

int (**dead_vnodeop_p)(void *);
const struct vnodeopv_entry_desc dead_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ NULL, NULL }
};
const struct vnodeopv_desc dead_vnodeop_opv_desc =
	{ &dead_vnodeop_p, dead_vnodeop_entries };

int (**sync_vnodeop_p)(void *);
const struct vnodeopv_entry_desc sync_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ NULL, NULL }
};
const struct vnodeopv_desc sync_vnodeop_opv_desc =
	{ &sync_vnodeop_p, sync_vnodeop_entries };

int (**fifo_vnodeop_p)(void *);
const struct vnodeopv_entry_desc fifo_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ NULL, NULL }
};
const struct vnodeopv_desc fifo_vnodeop_opv_desc =
	{ &fifo_vnodeop_p, fifo_vnodeop_entries };

int (**rump_vnodeop_p)(void *);
const struct vnodeopv_entry_desc rump_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, rump_vop_lookup },
	{ &vop_lock_desc, genfs_lock },
	{ &vop_unlock_desc, genfs_unlock },
	{ NULL, NULL }
};
const struct vnodeopv_desc rump_vnodeop_opv_desc =
	{ &rump_vnodeop_p, rump_vnodeop_entries };
const struct vnodeopv_desc * const rump_opv_descs[] = {
	&rump_vnodeop_opv_desc,
	NULL
};

static struct mount mnt_dummy;

static int
rump_makevnode(const char *path, size_t size, enum vtype vt, struct vnode **vpp)
{
	struct vnode *vp;
	int rv;

	vp = kmem_alloc(sizeof(struct vnode), KM_SLEEP);
	vp->v_size = vp->v_writesize = size;
	vp->v_type = vt;
	if (vp->v_type != VBLK)
		if (rump_fakeblk_find(path))
			vp->v_type = VBLK;

	if (vp->v_type != VBLK && vp->v_type != VDIR)
		panic("rump_makevnode: only VBLK/VDIR vnodes supported");

	if (vp->v_type == VBLK) {
		rv = rumpblk_register(path);
		if (rv == -1) {
			rv = EINVAL;
			goto bad;
		}
		spec_node_init(vp, makedev(RUMPBLK, rv));
		vp->v_op = spec_vnodeop_p;
	} else {
		vp->v_op = rump_vnodeop_p;
	}
	vp->v_mount = &mnt_dummy;
	vp->v_vnlock = &vp->v_lock;
	vp->v_usecount = 1;
	mutex_init(&vp->v_interlock, MUTEX_DEFAULT, IPL_NONE);
	memset(&vp->v_lock, 0, sizeof(vp->v_lock));
	rw_init(&vp->v_lock.vl_lock);
	cv_init(&vp->v_cv, "vnode");
	*vpp = vp;

	return 0;

 bad:
	kmem_free(vp, sizeof(*vp));
	return rv;
}

/* from libpuffs, but let's decouple this from that */
static enum vtype
mode2vt(mode_t mode)
{

	switch (mode & S_IFMT) {
	case S_IFIFO:
		return VFIFO;
	case S_IFCHR:
		return VCHR;
	case S_IFDIR:
		return VDIR;
	case S_IFBLK:
		return VBLK;
	case S_IFREG:
		return VREG;
	case S_IFLNK:
		return VLNK;
	case S_IFSOCK:
		return VSOCK;
	default:
		return VBAD; /* XXX: not really true, but ... */
	}
}

/*
 * Simple lookup for faking lookup of device entry for rump file systems 
 */
static int
rump_vop_lookup(void *v)
{
	struct vop_lookup_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
	}; */ *ap = v;
	struct componentname *cnp = ap->a_cnp;
	struct stat sb_node;
	int rv, error;

	/* we handle only some "non-special" cases */
	KASSERT(cnp->cn_nameiop == LOOKUP);
	KASSERT((cnp->cn_flags & ISDOTDOT) == 0);
	KASSERT(cnp->cn_namelen != 0 && cnp->cn_pnbuf[0] != '.');

	if (cnp->cn_flags & FOLLOW)
		rv = rumpuser_stat(cnp->cn_pnbuf, &sb_node, &error);
	else
		rv = rumpuser_lstat(cnp->cn_pnbuf, &sb_node, &error);
	if (rv)
		return error;

	error = rump_makevnode(cnp->cn_pnbuf, sb_node.st_size,
	    mode2vt(sb_node.st_mode), ap->a_vpp);
	if (error)
		return error;

	vn_lock(*ap->a_vpp, LK_RETRY | LK_EXCLUSIVE);
	cnp->cn_consume = strlen(cnp->cn_nameptr + cnp->cn_namelen);
	cnp->cn_flags &= ~REQUIREDIR;

	return 0;
}

void
rumpfs_init()
{
	int rv;

	vfs_opv_init(rump_opv_descs);
	rv = rump_makevnode("/", 0, VDIR, &rootvnode);
	if (rv)
		panic("could not create root vnode: %d", rv);
	rootvnode->v_vflag |= VV_ROOT;
}
