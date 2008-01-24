/*	$NetBSD: hfs_vfsops.c,v 1.14 2008/01/24 17:32:53 ad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: hfs_vfsops.c,v 1.14 2008/01/24 17:32:53 ad Exp $");

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

#include <fs/hfs/hfs.h>
#include <fs/hfs/libhfs.h>

MALLOC_JUSTDEFINE(M_HFSMNT, "hfs mount", "hfs mount structures");

extern struct lock hfs_hashlock;

const struct vnodeopv_desc * const hfs_vnodeopv_descs[] = {
	&hfs_vnodeop_opv_desc,
	&hfs_specop_opv_desc,
	&hfs_fifoop_opv_desc,
	NULL,
};

struct vfsops hfs_vfsops = {
	MOUNT_HFS,
	sizeof (struct hfs_args),
	hfs_mount,
	hfs_start,
	hfs_unmount,
	hfs_root,
	(void *)eopnotsupp,		/* vfs_quotactl */
	hfs_statvfs,
	hfs_sync,
	hfs_vget,
	hfs_fhtovp,
	hfs_vptofh,
	hfs_init,
	hfs_reinit,
	hfs_done,
	NULL,				/* vfs_mountroot */
	NULL,				/* vfs_snapshot */
	vfs_stdextattrctl,
	(void *)eopnotsupp,		/* vfs_suspendctl */
	hfs_vnodeopv_descs,
	0,
	{ NULL, NULL },
};
VFS_ATTACH(hfs_vfsops); /* XXX Is this needed? */

static const struct genfs_ops hfs_genfsops = {
        .gop_size = genfs_size,
};

int
hfs_mount(struct mount *mp, const char *path, void *data, size_t *data_len)
{
	struct lwp *l = curlwp;
	struct nameidata nd;
	struct hfs_args *args = data;
	struct vnode *devvp;
	struct hfsmount *hmp;
	int error;
	int update;
	mode_t accessmode;

	if (*data_len < sizeof *args)
		return EINVAL;

#ifdef HFS_DEBUG	
	printf("vfsop = hfs_mount()\n");
#endif /* HFS_DEBUG */
	
	if (mp->mnt_flag & MNT_GETARGS) {
		hmp = VFSTOHFS(mp);
		if (hmp == NULL)
			return EIO;
		args->fspec = NULL;
		*data_len = sizeof *args;
		return 0;
	}

	if (data == NULL)
		return EINVAL;

/* FIXME: For development ONLY - disallow remounting for now */
#if 0
	update = mp->mnt_flag & MNT_UPDATE;
#else
	update = 0;
#endif

	/* Check arguments */
	if (args->fspec != NULL) {
		/*
		 * Look up the name and verify that it's sane.
		 */
		NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, args->fspec);
		if ((error = namei(&nd)) != 0)
			return error;
		devvp = nd.ni_vp;
	
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
			hmp = VFSTOHFS(mp);
			if (devvp != hmp->hm_devvp)
				error = EINVAL;
		}
	} else {
		if (update) {
			/* Use the extant mount */
			hmp = VFSTOHFS(mp);
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
		error = VOP_ACCESS(devvp, accessmode, l->l_cred);
		VOP_UNLOCK(devvp, 0);
	}

	if (error != 0)
		goto error;

	if (update) {
		printf("HFS: live remounting not yet supported!\n");
		error = EINVAL;
		goto error;
	}

	if ((error = hfs_mountfs(devvp, mp, l, args->fspec)) != 0)
		goto error;
	
	error = set_statvfs_info(path, UIO_USERSPACE, args->fspec, UIO_USERSPACE,
		mp->mnt_op->vfs_name, mp, l);

#ifdef HFS_DEBUG
	if(!update) {
		char* volname;
		
		hmp = VFSTOHFS(mp);
		volname = malloc(hmp->hm_vol.name.length + 1, M_TEMP, M_WAITOK);
		if (volname == NULL)
			printf("could not allocate volname; ignored\n");
		else {
			if (hfs_unicode_to_ascii(hmp->hm_vol.name.unicode,
				hmp->hm_vol.name.length, volname) == NULL)
				printf("could not convert volume name to ascii; ignored\n");
			else
				printf("mounted volume \"%s\"\n", volname);
			free(volname, M_TEMP);
		}
	}
#endif /* HFS_DEBUG */
		
	return error;
	
error:
	vrele(devvp);
	return error;
}

int
hfs_start(struct mount *mp, int flags)
{

#ifdef HFS_DEBUG	
	printf("vfsop = hfs_start()\n");
#endif /* HFS_DEBUG */

	return 0;
}

int
hfs_mountfs(struct vnode *devvp, struct mount *mp, struct lwp *l,
    const char *devpath)
{
	hfs_callback_args cbargs;
	hfs_libcb_argsopen argsopen;
	hfs_libcb_argsread argsread;
	struct hfsmount *hmp;
	kauth_cred_t cred;
	int error;
	
	cred = l ? l->l_cred : NOCRED;
	error = 0;
	hmp = NULL;

	/* Create mounted volume structure. */
	hmp = (struct hfsmount*)malloc(sizeof(struct hfsmount),
            M_HFSMNT, M_WAITOK);
	if (hmp == NULL) {
		error = ENOMEM;
		goto error;
	}
	memset(hmp, 0, sizeof(struct hfsmount));
	
	mp->mnt_data = hmp;
	mp->mnt_flag |= MNT_LOCAL;
	vfs_getnewfsid(mp);

	hmp->hm_mountp = mp;
	hmp->hm_dev = devvp->v_rdev;
	hmp->hm_devvp = devvp;
	
	/*
	 * Use libhfs to open the volume and read the volume header and other
	 * useful information.
	 */
	 
	hfslib_init_cbargs(&cbargs);
	argsopen.cred = argsread.cred = cred;
	argsopen.l = argsread.l = l;
	argsopen.devvp = devvp;
	cbargs.read = (void*)&argsread;
	cbargs.openvol = (void*)&argsopen;

	if ((error = hfslib_open_volume(devpath, mp->mnt_flag & MNT_RDONLY,
		&hmp->hm_vol, &cbargs)) != 0)
		goto error;
		
	/* Make sure this is not a journaled volume whose journal is dirty. */
	if (!hfslib_is_journal_clean(&hmp->hm_vol)) {
		printf("volume journal is dirty; not mounting\n");
		error = EIO;
		goto error;
	}

	mp->mnt_fs_bshift = 0;
        while ((1 << mp->mnt_fs_bshift) < hmp->hm_vol.vh.block_size)
		mp->mnt_fs_bshift++;
	mp->mnt_dev_bshift = DEV_BSHIFT;

	return 0;
	
error:
	if (hmp != NULL)
		free(hmp, M_HFSMNT);
		
	return error;
}

int
hfs_unmount(struct mount *mp, int mntflags)
{
	hfs_callback_args cbargs;
	hfs_libcb_argsread argsclose;
	struct hfsmount* hmp;
	int error;
	int flags;
	
#ifdef HFS_DEBUG	
	printf("vfsop = hfs_unmount()\n");
#endif /* HFS_DEBUG */

	hmp = VFSTOHFS(mp);
	
	flags = 0;
	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;
	
	if ((error = vflush(mp, NULLVP, flags)) != 0)
		return error;

	hfslib_init_cbargs(&cbargs);
	argsclose.l = curlwp;
	cbargs.closevol = (void*)&argsclose;
	hfslib_close_volume(&hmp->hm_vol, &cbargs);
	
	vput(hmp->hm_devvp);

	free(hmp, M_HFSMNT);
	mp->mnt_data = NULL;
	mp->mnt_flag &= ~MNT_LOCAL;
	
	return error;
}

int
hfs_root(struct mount *mp, struct vnode **vpp)
{
	struct vnode *nvp;
	int error;

#ifdef HFS_DEBUG	
	printf("vfsop = hfs_root()\n");
#endif /* HFS_DEBUG */
	
	if ((error = VFS_VGET(mp, HFS_CNID_ROOT_FOLDER, &nvp)) != 0)
		return error;
	*vpp = nvp;
	
	return 0;
}

int
hfs_statvfs(struct mount *mp, struct statvfs *sbp)
{
	hfs_volume_header_t *vh;
	
#ifdef HFS_DEBUG	
	printf("vfsop = hfs_statvfs()\n");
#endif /* HFS_DEBUG */

	vh = &VFSTOHFS(mp)->hm_vol.vh;
	
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
hfs_sync(struct mount *mp, int waitfor, kauth_cred_t cred)
{

#ifdef HFS_DEBUG	
	printf("vfsop = hfs_sync()\n");
#endif /* HFS_DEBUG */

	return 0;
}

/*
 * an ino_t corresponds directly to a CNID in our particular case,
 * since both are conveniently 32-bit numbers
 */
int
hfs_vget(struct mount *mp, ino_t ino, struct vnode **vpp)
{
    return hfs_vget_internal(mp, ino, HFS_DATAFORK, vpp);
}

/*
 * internal version with extra arguments to allow accessing resource fork
 */
int
hfs_vget_internal(struct mount *mp, ino_t ino, uint8_t fork,
    struct vnode **vpp)
{
	struct hfsmount *hmp;
	struct hfsnode *hnode;
	struct vnode *vp;
	hfs_callback_args cbargs;
	hfs_cnid_t cnid;
	hfs_catalog_keyed_record_t rec;
	hfs_catalog_key_t key; /* the search key used to find this file on disk */
	dev_t dev;
	int error;

#ifdef HFS_DEBUG	
	printf("vfsop = hfs_vget()\n");
#endif /* HFS_DEBUG */

	hnode = NULL;
	vp = NULL;
	hmp = VFSTOHFS(mp);
	dev = hmp->hm_dev;
	cnid = (hfs_cnid_t)ino;

	if (fork != HFS_RSRCFORK)
	    fork = HFS_DATAFORK;

	/* Check if this vnode has already been allocated. If so, just return it. */
	if ((*vpp = hfs_nhashget(dev, cnid, fork, LK_EXCLUSIVE)) != NULL)
		return 0;

	/* Allocate a new vnode/inode. */
	if ((error = getnewvnode(VT_HFS, mp, hfs_vnodeop_p, &vp)) != 0)
		goto error;

	/*
	 * If someone beat us to it while sleeping in getnewvnode(),
	 * push back the freshly allocated vnode we don't need, and return.
	 */

	do {
		if ((*vpp = hfs_nhashget(dev, cnid, fork, LK_EXCLUSIVE))
		    != NULL) {
			ungetnewvnode(vp);
			return 0;
		}
	} while (lockmgr(&hfs_hashlock, LK_EXCLUSIVE|LK_SLEEPFAIL, 0));

	vp->v_vflag |= VV_LOCKSWORK;
	
	MALLOC(hnode, struct hfsnode *, sizeof(struct hfsnode), M_TEMP,
		M_WAITOK + M_ZERO);
	vp->v_data = hnode;
	genfs_node_init(vp, &hfs_genfsops);
	
	hnode->h_vnode = vp;
	hnode->h_hmp = hmp;
	hnode->dummy = 0x1337BABE;
	
	/*
	 * We need to put this vnode into the hash chain and lock it so that other
	 * requests for this inode will block if they arrive while we are sleeping
	 * waiting for old data structures to be purged or for the contents of the
	 * disk portion of this inode to be read. The hash chain requires the node's
	 * device and cnid to be known. Since this information was passed in the
	 * arguments, fill in the appropriate hfsnode fields without reading having
	 * to read the disk.
	 */
	hnode->h_dev = dev;
	hnode->h_rec.cnid = cnid;
	hnode->h_fork = fork;

	hfs_nhashinsert(hnode);
	lockmgr(&hfs_hashlock, LK_RELEASE, 0);


	/*
	 * Read catalog record from disk.
	 */
	hfslib_init_cbargs(&cbargs);
	
	if (hfslib_find_catalog_record_with_cnid(&hmp->hm_vol, cnid,
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
	 * Initialize the vnode from the hfsnode, check for aliases.
	 * Note that the underlying vnode may change.
	 */
	hfs_vinit(mp, hfs_specop_p, hfs_fifoop_p, &vp);

	hnode->h_devvp = hmp->hm_devvp;	
	VREF(hnode->h_devvp);  /* Increment the ref count to the volume's device. */

	/* Make sure UVM has allocated enough memory. (?) */
	if (hnode->h_rec.rec_type == HFS_REC_FILE) {
		if (hnode->h_fork == HFS_DATAFORK)
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
hfs_fhtovp(struct mount *mp, struct fid *fhp, struct vnode **vpp)
{

#ifdef HFS_DEBUG	
	printf("vfsop = hfs_fhtovp()\n");
#endif /* HFS_DEBUG */

	return EOPNOTSUPP;
}

int
hfs_vptofh(struct vnode *vp, struct fid *fhp, size_t *fh_size)
{

#ifdef HFS_DEBUG	
	printf("vfsop = hfs_vptofh()\n");
#endif /* HFS_DEBUG */

	return EOPNOTSUPP;
}

void
hfs_init(void)
{
	hfs_callbacks	callbacks;

#ifdef HFS_DEBUG	
	printf("vfsop = hfs_init()\n");
#endif /* HFS_DEBUG */

	malloc_type_attach(M_HFSMNT);

	callbacks.error = hfs_libcb_error;
	callbacks.allocmem = hfs_libcb_malloc;
	callbacks.reallocmem = hfs_libcb_realloc;
	callbacks.freemem = hfs_libcb_free;
	callbacks.openvol = hfs_libcb_opendev;
	callbacks.closevol = hfs_libcb_closedev;
	callbacks.read = hfs_libcb_read;

	hfs_nhashinit();
	hfslib_init(&callbacks);
}

void
hfs_reinit(void)
{

#ifdef HFS_DEBUG	
	printf("vfsop = hfs_reinit()\n");
#endif /* HFS_DEBUG */

	return;
}

void
hfs_done(void)
{

#ifdef HFS_DEBUG	
	printf("vfsop = hfs_done()\n");
#endif /* HFS_DEBUG */

	malloc_type_detach(M_HFSMNT);

	hfslib_done();
	hfs_nhashdone();
}

int
hfs_mountroot(void)
{

#ifdef HFS_DEBUG	
	printf("vfsop = hfs_mountroot()\n");
#endif /* HFS_DEBUG */

	return EOPNOTSUPP;
}
