/*	$NetBSD: hfs_vfsops.c,v 1.1 2007/03/06 00:22:04 dillo Exp $	*/

/*-
 * Copyright (c) 2005, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Yevgeny Binder and Dieter Baron.
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
 * Copyright (c) 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 
 /*
 * Copyright (c) 1989, 1991, 1993, 1994
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



/*
 * Apple HFS+ filesystem
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hfs_vfsops.c,v 1.1 2007/03/06 00:22:04 dillo Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/socket.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/mbuf.h>
#include <sys/file.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/lock.h>
#include <sys/sysctl.h>
#include <sys/conf.h>
#include <sys/kauth.h>
#include <sys/stat.h>

#include <miscfs/specfs/specdev.h>

#include <fs/hfsp/hfsp.h>
#include <fs/hfsp/libhfsp.h>

MALLOC_DEFINE(M_HFSPMNT, "hfsp mount", "hfsp mount structures");

extern struct lock hfsp_hashlock;

const struct vnodeopv_desc * const hfsp_vnodeopv_descs[] = {
	&hfsp_vnodeop_opv_desc,
	&hfsp_specop_opv_desc,
	&hfsp_fifoop_opv_desc,
	NULL,
};

struct vfsops hfsp_vfsops = {
	MOUNT_HFSP,
	hfsp_mount,
	hfsp_start,
	hfsp_unmount,
	hfsp_root,
	hfsp_quotactl,
	hfsp_statvfs,
	hfsp_sync,
	hfsp_vget,
	hfsp_fhtovp,
	hfsp_vptofh,
	hfsp_init,
	hfsp_reinit,
	hfsp_done,
	NULL,				/* vfs_mountroot */
	NULL,				/* vfs_snapshot */
	hfsp_extattrctl,
	NULL,				/* vfs_suspendctl */
	hfsp_vnodeopv_descs,
	0,
	{ NULL, NULL },
};
VFS_ATTACH(hfsp_vfsops); /* XXX Is this needed? */

static const struct genfs_ops hfsp_genfsops = {
        .gop_size = genfs_size,
};

int
hfsp_mount(struct mount *mp, const char *path, void *data,
    struct nameidata *ndp, struct lwp *l)
{
	struct hfsp_args args;
	struct vnode *devvp;
	struct hfspmount *hmp;
	int error;
	int update;
	mode_t accessmode;

#ifdef HFSP_DEBUG	
	printf("vfsop = hfsp_mount()\n");
#endif /* HFSP_DEBUG */
	
	if (mp->mnt_flag & MNT_GETARGS) {
		hmp = VFSTOHFSP(mp);
		if (hmp == NULL)
			return EIO;
		args.fspec = NULL;
		return copyout(&args, data, sizeof(args));
	}

	if (data == NULL)
		return EINVAL;

	if ((error = copyin(data, &args, sizeof (struct hfsp_args))) != 0)
		return error;

/* FIXME: For development ONLY - disallow remounting for now */
#if 0
	update = mp->mnt_flag & MNT_UPDATE;
#else
	update = 0;
#endif

	/* Check arguments */
	if (args.fspec != NULL) {
		/*
		 * Look up the name and verify that it's sane.
		 */
		NDINIT(ndp, LOOKUP, FOLLOW, UIO_USERSPACE, args.fspec, l);
		if ((error = namei(ndp)) != 0)
			return error;
		devvp = ndp->ni_vp;
	
		if (!update) {
			/*
			 * Be sure this is a valid block device
			 */
			if (devvp->v_type != VBLK)
				error = ENOTBLK;
			else if (bdevsw_lookup(devvp->v_rdev) == NULL)
				error = ENXIO;
		} else {
			/*
			 * Be sure we're still naming the same device
			 * used for our initial mount
			 */
			hmp = VFSTOHFSP(mp);
			if (devvp != hmp->hm_devvp)
				error = EINVAL;
		}
	} else {
		if (update) {
			/* Use the extant mount */
			hmp = VFSTOHFSP(mp);
			devvp = hmp->hm_devvp;
			vref(devvp);
		} else {
			/* New mounts must have a filename for the device */
			return EINVAL;
		}
	}

	
	/*
	 * If mount by non-root, then verify that user has necessary
	 * permissions on the device.
	 */
	if (error == 0 && kauth_authorize_generic(l->l_cred,
            KAUTH_GENERIC_ISSUSER, NULL) != 0) {
		accessmode = VREAD;
		if (update ?
			(mp->mnt_iflag & IMNT_WANTRDWR) != 0 :
			(mp->mnt_flag & MNT_RDONLY) == 0)
			accessmode |= VWRITE;
		vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
		error = VOP_ACCESS(devvp, accessmode, l->l_cred, l);
		VOP_UNLOCK(devvp, 0);
	}

	if (error != 0)
		goto error;

	if (update) {
		printf("HFSP: live remounting not yet supported!\n");
		error = EINVAL;
		goto error;
	}

	/*
	 * Disallow multiple mounts of the same device.
	 * Disallow mounting of a device that is currently in use
	 * (except for root, which might share swap device for miniroot).
	 * Flush out any old buffers remaining from a previous use.
	 */
	if ((error = vfs_mountedon(devvp)) != 0)
		goto error;
	if (vcount(devvp) > 1 && devvp != rootvp) {
		error = EBUSY;
		goto error;
	}

	if ((error = hfsp_mountfs(devvp, mp, l, args.fspec, args.offset)) != 0)
		goto error;
	
	error = set_statvfs_info(path, UIO_USERSPACE, args.fspec, UIO_SYSSPACE,
		mp, l);

#ifdef HFSP_DEBUG
	if(!update) {
		char* volname;
		
		hmp = VFSTOHFSP(mp);
		volname = malloc(hmp->hm_vol.name.length + 1, M_TEMP, M_WAITOK);
		if (volname == NULL)
			printf("could not allocate volname; ignored\n");
		else {
			if (hfsp_unicode_to_ascii(hmp->hm_vol.name.unicode,
				hmp->hm_vol.name.length, volname) == NULL)
				printf("could not convert volume name to ascii; ignored\n");
			else
				printf("mounted volume \"%s\"\n", volname);
			free(volname, M_TEMP);
		}
	}
#endif /* HFSP_DEBUG */
		
	return error;
	
error:
	vrele(devvp);
	return error;
}

int
hfsp_start(struct mount *mp, int flags, struct lwp *l)
{

#ifdef HFSP_DEBUG	
	printf("vfsop = hfsp_start()\n");
#endif /* HFSP_DEBUG */

	return 0;
}

int
hfsp_mountfs(struct vnode *devvp, struct mount *mp, struct lwp *l,
    const char *devpath, uint64_t offset)
{
	hfsp_callback_args cbargs;
	hfsp_libcb_argsopen argsopen;
	hfsp_libcb_argsread argsread;
	struct hfspmount *hmp;
	kauth_cred_t cred;
	int error;
	
	cred = l ? l->l_cred : NOCRED;
	error = 0;
	hmp = NULL;

	/* Create mounted volume structure. */
	hmp = (struct hfspmount*)malloc(sizeof(struct hfspmount),
            M_HFSPMNT, M_WAITOK);
	if (hmp == NULL) {
		error = ENOMEM;
		goto error;
	}
	memset(hmp, 0, sizeof(struct hfspmount));
	
	mp->mnt_data = hmp;
	mp->mnt_flag |= MNT_LOCAL;
	vfs_getnewfsid(mp);

	hmp->hm_mountp = mp;
	hmp->hm_dev = devvp->v_rdev;
	hmp->hm_devvp = devvp;
	
	/*
	 * Use libhfsp to open the volume and read the volume header and other
	 * useful information.
	 */
	 
	hfsplib_init_cbargs(&cbargs);
	argsopen.cred = argsread.cred = cred;
	argsopen.l = argsread.l = l;
	argsopen.devvp = devvp;
	cbargs.read = (void*)&argsread;
	cbargs.openvol = (void*)&argsopen;

	if ((error = hfsplib_open_volume(devpath, offset, mp->mnt_flag & MNT_RDONLY,
		&hmp->hm_vol, &cbargs)) != 0)
		goto error;
		
	/* Make sure this is not a journaled volume whose journal is dirty. */
	if (!hfsplib_is_journal_clean(&hmp->hm_vol)) {
		printf("volume journal is dirty; not mounting\n");
		error = EIO;
		goto error;
	}

	hmp->offset = offset >> DEV_BSHIFT;
	mp->mnt_fs_bshift = 0;
        while ((1 << mp->mnt_fs_bshift) < hmp->hm_vol.vh.block_size)
		mp->mnt_fs_bshift++;
	mp->mnt_dev_bshift = DEV_BSHIFT;

	return 0;
	
error:
	if (hmp != NULL)
		free(hmp, M_HFSPMNT);
		
	return error;
}

int
hfsp_unmount(struct mount *mp, int mntflags, struct lwp *l)
{
	hfsp_callback_args cbargs;
	hfsp_libcb_argsread argsclose;
	struct hfspmount* hmp;
	int error;
	int flags;
	
#ifdef HFSP_DEBUG	
	printf("vfsop = hfsp_unmount()\n");
#endif /* HFSP_DEBUG */

	hmp = VFSTOHFSP(mp);
	
	flags = 0;
	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;
	
	if ((error = vflush(mp, NULLVP, flags)) != 0)
		return error;

	hfsplib_init_cbargs(&cbargs);
	argsclose.l = l;
	cbargs.closevol = (void*)&argsclose;
	hfsplib_close_volume(&hmp->hm_vol, &cbargs);
	
	vput(hmp->hm_devvp);

	free(hmp, M_HFSPMNT);
	mp->mnt_data = NULL;
	mp->mnt_flag &= ~MNT_LOCAL;
	
	return error;
}

int
hfsp_root(struct mount *mp, struct vnode **vpp)
{
	struct vnode *nvp;
	int error;

#ifdef HFSP_DEBUG	
	printf("vfsop = hfsp_root()\n");
#endif /* HFSP_DEBUG */
	
	if ((error = VFS_VGET(mp, HFSP_CNID_ROOT_FOLDER, &nvp)) != 0)
		return error;
	*vpp = nvp;
	
	return 0;
}

int
hfsp_quotactl(struct mount *mp, int cmds, uid_t uid, void *arg,
    struct lwp *l)
{

#ifdef HFSP_DEBUG	
	printf("vfsop = hfsp_quotactl()\n");
#endif /* HFSP_DEBUG */

	return EOPNOTSUPP;
}

int
hfsp_statvfs(struct mount *mp, struct statvfs *sbp, struct lwp *l)
{
	hfsp_volume_header_t *vh;
	
#ifdef HFSP_DEBUG	
	printf("vfsop = hfsp_statvfs()\n");
#endif /* HFSP_DEBUG */

	vh = &VFSTOHFSP(mp)->hm_vol.vh;
	
	sbp->f_bsize = vh->block_size;
	sbp->f_frsize = sbp->f_bsize;
	sbp->f_iosize = 4096;/* mac os x uses a 4 kb io size, so do the same */
	sbp->f_blocks = vh->total_blocks;
	sbp->f_bfree = vh->free_blocks; /* total free blocks */
	sbp->f_bavail = vh->free_blocks; /* blocks free for non superuser */
	sbp->f_bresvd = 0;
	sbp->f_files =  vh->file_count; /* total files */
	sbp->f_ffree = (1<<31) - vh->file_count; /* free file nodes */
	copy_statvfs_info(sbp, mp);
	
	return 0;
}

int
hfsp_sync(struct mount *mp, int waitfor, kauth_cred_t cred, struct lwp *l)
{

#ifdef HFSP_DEBUG	
	printf("vfsop = hfsp_sync()\n");
#endif /* HFSP_DEBUG */

	return 0;
}

/*
 * an ino_t corresponds directly to a CNID in our particular case,
 * since both are conveniently 32-bit numbers
 */
int
hfsp_vget(struct mount *mp, ino_t ino, struct vnode **vpp)
{
    return hfsp_vget_internal(mp, ino, HFSP_DATAFORK, vpp);
}

/*
 * internal version with extra arguments to allow accessing resource fork
 */
int
hfsp_vget_internal(struct mount *mp, ino_t ino, uint8_t fork,
    struct vnode **vpp)
{
	struct hfspmount *hmp;
	struct hfspnode *hnode;
	struct vnode *vp;
	hfsp_callback_args cbargs;
	hfsp_cnid_t cnid;
	hfsp_catalog_keyed_record_t rec;
	hfsp_catalog_key_t key; /* the search key used to find this file on disk */
	dev_t dev;
	int error;

#ifdef HFSP_DEBUG	
	printf("vfsop = hfsp_vget()\n");
#endif /* HFSP_DEBUG */

	hnode = NULL;
	vp = NULL;
	hmp = VFSTOHFSP(mp);
	dev = hmp->hm_dev;
	cnid = (hfsp_cnid_t)ino;

	if (fork != HFSP_RSRCFORK)
	    fork = HFSP_DATAFORK;

	/* Check if this vnode has already been allocated. If so, just return it. */
	if ((*vpp = hfsp_nhashget(dev, cnid, fork, LK_EXCLUSIVE)) != NULL)
		return 0;

	/* Allocate a new vnode/inode. */
	if ((error = getnewvnode(VT_HFSP, mp, hfsp_vnodeop_p, &vp)) != 0)
		goto error;

	/*
	 * If someone beat us to it while sleeping in getnewvnode(),
	 * push back the freshly allocated vnode we don't need, and return.
	 */

	do {
		if ((*vpp = hfsp_nhashget(dev, cnid, fork, LK_EXCLUSIVE))
		    != NULL) {
			ungetnewvnode(vp);
			return 0;
		}
	} while (lockmgr(&hfsp_hashlock, LK_EXCLUSIVE|LK_SLEEPFAIL, 0));

	vp->v_flag |= VLOCKSWORK;
	
	MALLOC(hnode, struct hfspnode *, sizeof(struct hfspnode), M_TEMP,
		M_WAITOK + M_ZERO);
	vp->v_data = hnode;
	
	hnode->h_vnode = vp;
	hnode->h_hmp = hmp;
	hnode->dummy = 0x1337BABE;
	
	/*
	 * We need to put this vnode into the hash chain and lock it so that other
	 * requests for this inode will block if they arrive while we are sleeping
	 * waiting for old data structures to be purged or for the contents of the
	 * disk portion of this inode to be read. The hash chain requires the node's
	 * device and cnid to be known. Since this information was passed in the
	 * arguments, fill in the appropriate hfspnode fields without reading having
	 * to read the disk.
	 */
	hnode->h_dev = dev;
	hnode->h_rec.cnid = cnid;
	hnode->h_fork = fork;

	hfsp_nhashinsert(hnode);
	lockmgr(&hfsp_hashlock, LK_RELEASE, 0);


	/*
	 * Read catalog record from disk.
	 */
	hfsplib_init_cbargs(&cbargs);
	
	if (hfsplib_find_catalog_record_with_cnid(&hmp->hm_vol, cnid,
		&rec, &key, &cbargs) != 0) {
		vput(vp);
		error = EBADF;
		goto error;
	}
		
	memcpy(&hnode->h_rec, &rec, sizeof(hnode->h_rec));
	hnode->h_parent = key.parent_cnid;

	/* XXX Eventually need to add an "ignore permissions" mount option */

	/*
	 * Now convert some of the catalog record's fields into values that make
	 * sense on this system.
	 */
	/* DATE AND TIME */

	/*
	 * Initialize the vnode from the hfspnode, check for aliases.
	 * Note that the underlying vnode may have changed.
	 */

	hfsp_vinit(mp, hfsp_specop_p, hfsp_fifoop_p, &vp);

	genfs_node_init(vp, &hfsp_genfsops);
	hnode->h_devvp = hmp->hm_devvp;	
	VREF(hnode->h_devvp);  /* Increment the ref count to the volume's device. */

	/* Make sure UVM has allocated enough memory. (?) */
	if (hnode->h_rec.rec_type == HFSP_REC_FILE) {
		if (hnode->h_fork == HFSP_DATAFORK)
			uvm_vnp_setsize(vp,
			    hnode->h_rec.file.data_fork.logical_size);
		else
			uvm_vnp_setsize(vp,
			    hnode->h_rec.file.rsrc_fork.logical_size);
	}
	else
		uvm_vnp_setsize(vp, 0); /* no directly reading directories */
		
	*vpp = vp;
	
	return 0;

error:
	*vpp = NULL;
	return error;
}

int
hfsp_fhtovp(struct mount *mp, struct fid *fhp, struct vnode **vpp)
{

#ifdef HFSP_DEBUG	
	printf("vfsop = hfsp_fhtovp()\n");
#endif /* HFSP_DEBUG */

	return EOPNOTSUPP;
}

int
hfsp_vptofh(struct vnode *vp, struct fid *fhp, size_t *fh_size)
{

#ifdef HFSP_DEBUG	
	printf("vfsop = hfsp_vptofh()\n");
#endif /* HFSP_DEBUG */

	return EOPNOTSUPP;
}

void
hfsp_init(void)
{
	hfsp_callbacks	callbacks;

#ifdef HFSP_DEBUG	
	printf("vfsop = hfsp_init()\n");
#endif /* HFSP_DEBUG */

#ifdef _LKM
	malloc_type_attach(M_HFSPMNT);
#endif

	callbacks.error = hfsp_libcb_error;
	callbacks.allocmem = hfsp_libcb_malloc;
	callbacks.reallocmem = hfsp_libcb_realloc;
	callbacks.freemem = hfsp_libcb_free;
	callbacks.openvol = hfsp_libcb_opendev;
	callbacks.closevol = hfsp_libcb_closedev;
	callbacks.read = hfsp_libcb_read;

	hfsp_nhashinit();
	hfsplib_init(&callbacks);
}

void
hfsp_reinit(void)
{

#ifdef HFSP_DEBUG	
	printf("vfsop = hfsp_reinit()\n");
#endif /* HFSP_DEBUG */

	return;
}

void
hfsp_done(void)
{

#ifdef HFSP_DEBUG	
	printf("vfsop = hfsp_done()\n");
#endif /* HFSP_DEBUG */

#ifdef _LKM
	malloc_type_detach(M_HFSPMNT);
#endif

	hfsplib_done();
	hfsp_nhashdone();
}

int
hfsp_mountroot(void)
{

#ifdef HFSP_DEBUG	
	printf("vfsop = hfsp_mountroot()\n");
#endif /* HFSP_DEBUG */

	return EOPNOTSUPP;
}

int hfsp_extattrctl(struct mount *mp, int cmd, struct vnode *vp,
    int attrnamespace, const char *attrname, struct lwp *l)
{

#ifdef HFSP_DEBUG	
	printf("vfsop = hfsp_checkexp()\n");
#endif /* HFSP_DEBUG */

	return EOPNOTSUPP;
}
