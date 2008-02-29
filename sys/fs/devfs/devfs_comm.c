/* 	$NetBSD: devfs_comm.c,v 1.1.6.2 2008/02/29 19:39:47 mjf Exp $ */

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Fleming.
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

/*
 * This file contains all code that is used to communicate between a
 * devfs mount and dctl(4) (which in turn communicates with devfsd(8)).
 *
 * The communication between all components can be illustrated by,
 *
 *	------------		---------	    -------------
 *     | devfsd(8)  |<-------->| dctl(4) |<------->| devfs mount |
 *     |	    |	       |         |         |             |
 *      ------------            ---------           -------------
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: devfs_comm.c,v 1.1.6.2 2008/02/29 19:39:47 mjf Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/lwp.h>
#include <sys/filedesc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/kauth.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <machine/cpu.h>

#include <fs/devfs/devfs.h>
#include <fs/devfs/devfs_comm.h>
#include <dev/dctl/dctl.h>

/* 
 * INFO WE NEED:
 *
 *	- mount cookie
 *	- path (in file system)
 *	- device cookie
 *	- block/char node
 *	- user
 *	- group
 *	- mode
 *	- leave room for added crap later (ACLs)
 *
 */
int
devfs_create_node(int32_t mcookie, const char *path, intptr_t dcookie, 
	uid_t uid, gid_t gid, mode_t mode, int flags, dev_t dev)
{
	int error;
	struct proc *p = curlwp->l_proc;
	struct device *dp;
	struct mount *mp;
	struct vnode *vp;
	struct cwdinfo ci;
	struct nameidata nd;
	enum uio_seg seg = UIO_USERSPACE;
	struct vattr vattr;

	memset(&ci, 0, sizeof(ci));

	/* TODO: Probably want our own scope here? KAUTH_DEVFS_MKNOD */
	if ((error = kauth_authorize_system(curlwp->l_cred, KAUTH_SYSTEM_MKNOD,
	    0, NULL, NULL, NULL)) != 0)
		return error;

	/*
	 * We need to make sure that the device identifed by 'dcookie'
	 * is really on the system. If it is, then we have to lookup
	 * the major and minor numbers.
	 *
	 * TODO: There's no reliable way to do this for the moment, at
	 *	least until I implement dynamic dev_ts.
	 */
	TAILQ_FOREACH(dp, &alldevs, dv_list) {
		if ((intptr_t)dp == (intptr_t)dcookie)
			break;
	}

	if (dp == NULL) {
		printf("device not on device list!\n");
		return EINVAL;
	}

	/*
	 * Lookup correct mount for this mcookie
	 */
	mutex_enter(&mountlist_lock);
	CIRCLEQ_FOREACH(mp, &mountlist, mnt_list) {
		if (mp->mnt_stat.f_fsidx.__fsid_val[0] == mcookie)
			break;
		
	}
	mutex_exit(&mountlist_lock);

	if (mp == NULL)
		return EINVAL;

	NDINIT(&nd, CREATE, LOCKPARENT | TRYEMULROOT, seg, path);

	ci = *p->p_cwdi;

	if ((error = VFS_ROOT(mp, &vp)) != 0)
		return error;

	/* chroot */
	rw_enter(&p->p_cwdi->cwdi_lock, RW_WRITER);	
	p->p_cwdi->cwdi_cdir = vp;
	p->p_cwdi->cwdi_rdir = vp;
	rw_exit(&p->p_cwdi->cwdi_lock);

	/* XXX: VFS_ROOT returns the vnode locked; namei needs it unlocked */
	VOP_UNLOCK(vp, 0);

	if ((error = namei(&nd)) != 0) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		vrele(vp);
		return error;
	}

	if (nd.ni_vp != NULL) {
		error = EEXIST;
		goto abort;
	}

	/* 
	 * Ensure that the vnode we got from namei() is on the correct
	 * mount point.
	 */
	if (nd.ni_dvp->v_mount != mp) {
		error = EINVAL;
		goto abort;
	}

	VATTR_NULL(&vattr);
	vattr.va_mode = S_IFCHR | mode;
	vattr.va_type = VCHR;

	/* We're making a device special node for this dev_t */
	vattr.va_rdev = dev;

	/* 
	 * TODO: We currently don't support creating nodes in anything
	 * but the root directory. This will change eventually.
	 */
	error = devfs_internal_mknod(nd.ni_dvp, &nd.ni_vp, &nd.ni_cnd, &vattr);

	if (error == 0)
		vput(nd.ni_vp); /* special device vnode */

abort:
	if (error != 0) {
		VOP_ABORTOP(nd.ni_dvp, &nd.ni_cnd);
		if (nd.ni_dvp == vp) 
			vrele(nd.ni_dvp);
		else
			vput(nd.ni_dvp);
		vrele(nd.ni_vp);
		VOP_UNLOCK(vp, 0);
	}
		
	rw_enter(&p->p_cwdi->cwdi_lock, RW_WRITER);	
	p->p_cwdi->cwdi_rdir = ci.cwdi_rdir;
	p->p_cwdi->cwdi_cdir = ci.cwdi_cdir;
	rw_exit(&p->p_cwdi->cwdi_lock);

	vrele(vp);

	return error;
}

/*
 * This is the function to make a device node in the devfs file system.
 * It is NOT accessible through the vnode operations table because we
 * do not want to allow arbitrary userland processes to create device 
 * nodes. The only userland process that can create device nodes in a devfs
 * is devfsd(8).
 */
int
devfs_internal_mknod(struct vnode *dvp, struct vnode **vpp, 
	struct componentname *cnp, struct vattr *vap)
{
	if (vap->va_type != VBLK && vap->va_type != VCHR &&
	    vap->va_type != VFIFO)
		return EINVAL;

	return devfs_alloc_file(dvp, vpp, vap, cnp, NULL);
}
