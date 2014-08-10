/*	$NetBSD: ntfs_vfsops.c,v 1.93.2.1 2014/08/10 06:55:54 tls Exp $	*/

/*-
 * Copyright (c) 1998, 1999 Semen Ustimenko
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	Id: ntfs_vfsops.c,v 1.7 1999/05/31 11:28:30 phk Exp
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ntfs_vfsops.c,v 1.93.2.1 2014/08/10 06:55:54 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/fcntl.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/kauth.h>
#include <sys/module.h>

#include <uvm/uvm_extern.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/specfs/specdev.h>

#include <fs/ntfs/ntfs.h>
#include <fs/ntfs/ntfs_inode.h>
#include <fs/ntfs/ntfs_subr.h>
#include <fs/ntfs/ntfs_vfsops.h>
#include <fs/ntfs/ntfs_ihash.h>
#include <fs/ntfs/ntfsmount.h>

MODULE(MODULE_CLASS_VFS, ntfs, NULL);

MALLOC_JUSTDEFINE(M_NTFSMNT, "NTFS mount", "NTFS mount structure");
MALLOC_JUSTDEFINE(M_NTFSNTNODE,"NTFS ntnode",  "NTFS ntnode information");
MALLOC_JUSTDEFINE(M_NTFSFNODE,"NTFS fnode",  "NTFS fnode information");
MALLOC_JUSTDEFINE(M_NTFSDIR,"NTFS dir",  "NTFS dir buffer");

static int	ntfs_mount(struct mount *, const char *, void *, size_t *);
static int	ntfs_root(struct mount *, struct vnode **);
static int	ntfs_start(struct mount *, int);
static int	ntfs_statvfs(struct mount *, struct statvfs *);
static int	ntfs_sync(struct mount *, int, kauth_cred_t);
static int	ntfs_unmount(struct mount *, int);
static int	ntfs_vget(struct mount *mp, ino_t ino,
			       struct vnode **vpp);
static int	ntfs_mountfs(struct vnode *, struct mount *,
				  struct ntfs_args *, struct lwp *);
static int	ntfs_vptofh(struct vnode *, struct fid *, size_t *);

static void     ntfs_init(void);
static void     ntfs_reinit(void);
static void     ntfs_done(void);
static int      ntfs_fhtovp(struct mount *, struct fid *,
				struct vnode **);
static int      ntfs_mountroot(void);

static const struct genfs_ops ntfs_genfsops = {
	.gop_write = genfs_compat_gop_write,
};

static struct sysctllog *ntfs_sysctl_log;

static int
ntfs_mountroot(void)
{
	struct mount *mp;
	struct lwp *l = curlwp;	/* XXX */
	int error;
	struct ntfs_args args;

	if (device_class(root_device) != DV_DISK)
		return (ENODEV);

	if ((error = vfs_rootmountalloc(MOUNT_NTFS, "root_device", &mp))) {
		vrele(rootvp);
		return (error);
	}

	args.flag = 0;
	args.uid = 0;
	args.gid = 0;
	args.mode = 0777;

	if ((error = ntfs_mountfs(rootvp, mp, &args, l)) != 0) {
		vfs_unbusy(mp, false, NULL);
		vfs_destroy(mp);
		return (error);
	}

	mountlist_append(mp);
	(void)ntfs_statvfs(mp, &mp->mnt_stat);
	vfs_unbusy(mp, false, NULL);
	return (0);
}

static void
ntfs_init(void)
{

	malloc_type_attach(M_NTFSMNT);
	malloc_type_attach(M_NTFSNTNODE);
	malloc_type_attach(M_NTFSFNODE);
	malloc_type_attach(M_NTFSDIR);
	malloc_type_attach(M_NTFSNTVATTR);
	malloc_type_attach(M_NTFSRDATA);
	malloc_type_attach(M_NTFSDECOMP);
	malloc_type_attach(M_NTFSRUN);
	ntfs_nthashinit();
	ntfs_toupper_init();
}

static void
ntfs_reinit(void)
{
	ntfs_nthashreinit();
}

static void
ntfs_done(void)
{
	ntfs_nthashdone();
	malloc_type_detach(M_NTFSMNT);
	malloc_type_detach(M_NTFSNTNODE);
	malloc_type_detach(M_NTFSFNODE);
	malloc_type_detach(M_NTFSDIR);
	malloc_type_detach(M_NTFSNTVATTR);
	malloc_type_detach(M_NTFSRDATA);
	malloc_type_detach(M_NTFSDECOMP);
	malloc_type_detach(M_NTFSRUN);
}

static int
ntfs_mount (
	struct mount *mp,
	const char *path,
	void *data,
	size_t *data_len)
{
	struct lwp *l = curlwp;
	int		err = 0, flags;
	struct vnode	*devvp;
	struct ntfs_args *args = data;

	if (args == NULL)
		return EINVAL;
	if (*data_len < sizeof *args)
		return EINVAL;

	if (mp->mnt_flag & MNT_GETARGS) {
		struct ntfsmount *ntmp = VFSTONTFS(mp);
		if (ntmp == NULL)
			return EIO;
		args->fspec = NULL;
		args->uid = ntmp->ntm_uid;
		args->gid = ntmp->ntm_gid;
		args->mode = ntmp->ntm_mode;
		args->flag = ntmp->ntm_flag;
		*data_len = sizeof *args;
		return 0;
	}
	/*
	 ***
	 * Mounting non-root file system or updating a file system
	 ***
	 */

	/*
	 * If updating, check whether changing from read-only to
	 * read/write; if there is no device name, that's all we do.
	 */
	if (mp->mnt_flag & MNT_UPDATE) {
		printf("ntfs_mount(): MNT_UPDATE not supported\n");
		return (EINVAL);
	}

	/*
	 * Not an update, or updating the name: look up the name
	 * and verify that it refers to a sensible block device.
	 */
	err = namei_simple_user(args->fspec,
				NSM_FOLLOW_NOEMULROOT, &devvp);
	if (err) {
		/* can't get devvp!*/
		return (err);
	}

	if (devvp->v_type != VBLK) {
		err = ENOTBLK;
		goto fail;
	}
	if (bdevsw_lookup(devvp->v_rdev) == NULL) {
		err = ENXIO;
		goto fail;
	}
	if (mp->mnt_flag & MNT_UPDATE) {
#if 0
		/*
		 ********************
		 * UPDATE
		 ********************
		 */

		if (devvp != ntmp->um_devvp) {
			err = EINVAL;	/* needs translation */
			goto fail;
		}

		/*
		 * Update device name only on success
		 */
		err = set_statvfs_info(NULL, UIO_USERSPACE, args->fspec,
		    UIO_USERSPACE, mp->mnt_op->vfs_name, mp, p);
		if (err)
			goto fail;

		vrele(devvp);
#endif
	} else {
		/*
		 ********************
		 * NEW MOUNT
		 ********************
		 */

		/*
		 * Since this is a new mount, we want the names for
		 * the device and the mount point copied in.  If an
		 * error occurs,  the mountpoint is discarded by the
		 * upper level code.
		 */

		/* Save "last mounted on" info for mount point (NULL pad)*/
		err = set_statvfs_info(path, UIO_USERSPACE, args->fspec,
		    UIO_USERSPACE, mp->mnt_op->vfs_name, mp, l);
		if (err)
			goto fail;

		if (mp->mnt_flag & MNT_RDONLY)
			flags = FREAD;
		else
			flags = FREAD|FWRITE;
		vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
		err = VOP_OPEN(devvp, flags, FSCRED);
		VOP_UNLOCK(devvp);
		if (err)
			goto fail;
		err = ntfs_mountfs(devvp, mp, args, l);
		if (err) {
			vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
			(void)VOP_CLOSE(devvp, flags, NOCRED);
			VOP_UNLOCK(devvp);
			goto fail;
		}
	}

	/*
	 * Initialize FS stat information in mount struct; uses both
	 * mp->mnt_stat.f_mntonname and mp->mnt_stat.f_mntfromname
	 *
	 * This code is common to root and non-root mounts
	 */
	(void)VFS_STATVFS(mp, &mp->mnt_stat);
	return (err);

fail:
	vrele(devvp);
	return (err);
}

/*
 * Common code for mount and mountroot
 */
int
ntfs_mountfs(struct vnode *devvp, struct mount *mp, struct ntfs_args *argsp, struct lwp *l)
{
	struct buf *bp;
	struct ntfsmount *ntmp;
	dev_t dev = devvp->v_rdev;
	int error, i;
	struct vnode *vp;

	ntmp = NULL;

	/*
	 * Flush out any old buffers remaining from a previous use.
	 */
	vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
	error = vinvalbuf(devvp, V_SAVE, l->l_cred, l, 0, 0);
	VOP_UNLOCK(devvp);
	if (error)
		return (error);

	bp = NULL;

	error = bread(devvp, BBLOCK, BBSIZE, NOCRED, 0, &bp);
	if (error)
		goto out;
	ntmp = malloc( sizeof *ntmp, M_NTFSMNT, M_WAITOK|M_ZERO);
	memcpy( &ntmp->ntm_bootfile,  bp->b_data, sizeof(struct bootfile) );
	brelse( bp , 0 );
	bp = NULL;

	if (strncmp(ntmp->ntm_bootfile.bf_sysid, NTFS_BBID, NTFS_BBIDLEN)) {
		error = EINVAL;
		dprintf(("ntfs_mountfs: invalid boot block\n"));
		goto out;
	}

	{
		int8_t cpr = ntmp->ntm_mftrecsz;
		if( cpr > 0 )
			ntmp->ntm_bpmftrec = ntmp->ntm_spc * cpr;
		else
			ntmp->ntm_bpmftrec = (1 << (-cpr)) / ntmp->ntm_bps;
	}
	dprintf(("ntfs_mountfs(): bps: %d, spc: %d, media: %x, mftrecsz: %d (%d sects)\n",
		ntmp->ntm_bps,ntmp->ntm_spc,ntmp->ntm_bootfile.bf_media,
		ntmp->ntm_mftrecsz,ntmp->ntm_bpmftrec));
	dprintf(("ntfs_mountfs(): mftcn: 0x%x|0x%x\n",
		(u_int32_t)ntmp->ntm_mftcn,(u_int32_t)ntmp->ntm_mftmirrcn));

	ntmp->ntm_mountp = mp;
	ntmp->ntm_dev = dev;
	ntmp->ntm_devvp = devvp;
	ntmp->ntm_uid = argsp->uid;
	ntmp->ntm_gid = argsp->gid;
	ntmp->ntm_mode = argsp->mode;
	ntmp->ntm_flag = argsp->flag;
	mp->mnt_data = ntmp;

	/* set file name encode/decode hooks XXX utf-8 only for now */
	ntmp->ntm_wget = ntfs_utf8_wget;
	ntmp->ntm_wput = ntfs_utf8_wput;
	ntmp->ntm_wcmp = ntfs_utf8_wcmp;

	dprintf(("ntfs_mountfs(): case-%s,%s uid: %d, gid: %d, mode: %o\n",
		(ntmp->ntm_flag & NTFS_MFLAG_CASEINS)?"insens.":"sens.",
		(ntmp->ntm_flag & NTFS_MFLAG_ALLNAMES)?" allnames,":"",
		ntmp->ntm_uid, ntmp->ntm_gid, ntmp->ntm_mode));

	/*
	 * We read in some system nodes to do not allow
	 * reclaim them and to have everytime access to them.
	 */
	{
		int pi[3] = { NTFS_MFTINO, NTFS_ROOTINO, NTFS_BITMAPINO };
		for (i=0; i<3; i++) {
			error = VFS_VGET(mp, pi[i], &(ntmp->ntm_sysvn[pi[i]]));
			if(error)
				goto out1;
			ntmp->ntm_sysvn[pi[i]]->v_vflag |= VV_SYSTEM;
			vref(ntmp->ntm_sysvn[pi[i]]);
			vput(ntmp->ntm_sysvn[pi[i]]);
		}
	}

	/* read the Unicode lowercase --> uppercase translation table,
	 * if necessary */
	if ((error = ntfs_toupper_use(mp, ntmp)))
		goto out1;

	/*
	 * Scan $BitMap and count free clusters
	 */
	error = ntfs_calccfree(ntmp, &ntmp->ntm_cfree);
	if(error)
		goto out1;

	/*
	 * Read and translate to internal format attribute
	 * definition file.
	 */
	{
		int num,j;
		struct attrdef ad;

		/* Open $AttrDef */
		error = VFS_VGET(mp, NTFS_ATTRDEFINO, &vp );
		if(error)
			goto out1;

		/* Count valid entries */
		for(num=0;;num++) {
			error = ntfs_readattr(ntmp, VTONT(vp),
					NTFS_A_DATA, NULL,
					num * sizeof(ad), sizeof(ad),
					&ad, NULL);
			if (error)
				goto out1;
			if (ad.ad_name[0] == 0)
				break;
		}

		/* Alloc memory for attribute definitions */
		ntmp->ntm_ad = (struct ntvattrdef *) malloc(
			num * sizeof(struct ntvattrdef),
			M_NTFSMNT, M_WAITOK);

		ntmp->ntm_adnum = num;

		/* Read them and translate */
		for(i=0;i<num;i++){
			error = ntfs_readattr(ntmp, VTONT(vp),
					NTFS_A_DATA, NULL,
					i * sizeof(ad), sizeof(ad),
					&ad, NULL);
			if (error)
				goto out1;
			j = 0;
			do {
				ntmp->ntm_ad[i].ad_name[j] = ad.ad_name[j];
			} while(ad.ad_name[j++]);
			ntmp->ntm_ad[i].ad_namelen = j - 1;
			ntmp->ntm_ad[i].ad_type = ad.ad_type;
		}

		vput(vp);
	}

	mp->mnt_stat.f_fsidx.__fsid_val[0] = dev;
	mp->mnt_stat.f_fsidx.__fsid_val[1] = makefstype(MOUNT_NTFS);
	mp->mnt_stat.f_fsid = mp->mnt_stat.f_fsidx.__fsid_val[0];
	mp->mnt_stat.f_namemax = NTFS_MAXFILENAME;
	mp->mnt_flag |= MNT_LOCAL;
	spec_node_setmountedfs(devvp, mp);
	return (0);

out1:
	for(i=0;i<NTFS_SYSNODESNUM;i++)
		if(ntmp->ntm_sysvn[i]) vrele(ntmp->ntm_sysvn[i]);

	if (vflush(mp,NULLVP,0)) {
		dprintf(("ntfs_mountfs: vflush failed\n"));
	}
out:
	spec_node_setmountedfs(devvp, NULL);
	if (bp)
		brelse(bp, 0);

	if (error) {
		if (ntmp) {
			if (ntmp->ntm_ad)
				free(ntmp->ntm_ad, M_NTFSMNT);
			free(ntmp, M_NTFSMNT);
		}
	}

	return (error);
}

static int
ntfs_start (
	struct mount *mp,
	int flags)
{
	return (0);
}

static int
ntfs_unmount(
	struct mount *mp,
	int mntflags)
{
	struct lwp *l = curlwp;
	struct ntfsmount *ntmp;
	int error, ronly = 0, flags, i;

	dprintf(("ntfs_unmount: unmounting...\n"));
	ntmp = VFSTONTFS(mp);

	flags = 0;
	if(mntflags & MNT_FORCE)
		flags |= FORCECLOSE;

	dprintf(("ntfs_unmount: vflushing...\n"));
	error = vflush(mp,NULLVP,flags | SKIPSYSTEM);
	if (error) {
		dprintf(("ntfs_unmount: vflush failed: %d\n",error));
		return (error);
	}

	/* Check if only system vnodes are rest */
	for(i=0;i<NTFS_SYSNODESNUM;i++)
		 if((ntmp->ntm_sysvn[i]) &&
		    (ntmp->ntm_sysvn[i]->v_usecount > 1)) return (EBUSY);

	/* Dereference all system vnodes */
	for(i=0;i<NTFS_SYSNODESNUM;i++)
		 if(ntmp->ntm_sysvn[i]) vrele(ntmp->ntm_sysvn[i]);

	/* vflush system vnodes */
	error = vflush(mp,NULLVP,flags);
	if (error) {
		panic("ntfs_unmount: vflush failed(sysnodes): %d\n",error);
	}

	/* Check if the type of device node isn't VBAD before
	 * touching v_specinfo.  If the device vnode is revoked, the
	 * field is NULL and touching it causes null pointer derefercence.
	 */
	if (ntmp->ntm_devvp->v_type != VBAD)
		spec_node_setmountedfs(ntmp->ntm_devvp, NULL);

	error = vinvalbuf(ntmp->ntm_devvp, V_SAVE, NOCRED, l, 0, 0);
	KASSERT(error == 0);

	/* lock the device vnode before calling VOP_CLOSE() */
	vn_lock(ntmp->ntm_devvp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_CLOSE(ntmp->ntm_devvp, ronly ? FREAD : FREAD|FWRITE,
		NOCRED);
	KASSERT(error == 0);
	VOP_UNLOCK(ntmp->ntm_devvp);

	vrele(ntmp->ntm_devvp);

	/* free the toupper table, if this has been last mounted ntfs volume */
	ntfs_toupper_unuse();

	dprintf(("ntfs_umount: freeing memory...\n"));
	mp->mnt_data = NULL;
	mp->mnt_flag &= ~MNT_LOCAL;
	free(ntmp->ntm_ad, M_NTFSMNT);
	free(ntmp, M_NTFSMNT);
	return (0);
}

static int
ntfs_root(
	struct mount *mp,
	struct vnode **vpp)
{
	struct vnode *nvp;
	int error = 0;

	dprintf(("ntfs_root(): sysvn: %p\n",
		VFSTONTFS(mp)->ntm_sysvn[NTFS_ROOTINO]));
	error = VFS_VGET(mp, (ino_t)NTFS_ROOTINO, &nvp);
	if(error) {
		printf("ntfs_root: VFS_VGET failed: %d\n",error);
		return (error);
	}

	*vpp = nvp;
	return (0);
}

int
ntfs_calccfree(
	struct ntfsmount *ntmp,
	cn_t *cfreep)
{
	struct vnode *vp;
	u_int8_t *tmp;
	int j, error;
	cn_t cfree = 0;
	size_t bmsize, i;

	vp = ntmp->ntm_sysvn[NTFS_BITMAPINO];

	bmsize = VTOF(vp)->f_size;

	tmp = (u_int8_t *) malloc(bmsize, M_TEMP, M_WAITOK);

	error = ntfs_readattr(ntmp, VTONT(vp), NTFS_A_DATA, NULL,
			       0, bmsize, tmp, NULL);
	if (error)
		goto out;

	for(i=0;i<bmsize;i++)
		for(j=0;j<8;j++)
			if(~tmp[i] & (1 << j)) cfree++;
	*cfreep = cfree;

    out:
	free(tmp, M_TEMP);
	return(error);
}

static int
ntfs_statvfs(
	struct mount *mp,
	struct statvfs *sbp)
{
	struct ntfsmount *ntmp = VFSTONTFS(mp);
	u_int64_t mftallocated;

	dprintf(("ntfs_statvfs():\n"));

	mftallocated = VTOF(ntmp->ntm_sysvn[NTFS_MFTINO])->f_allocated;

	sbp->f_bsize = ntmp->ntm_bps;
	sbp->f_frsize = sbp->f_bsize; /* XXX */
	sbp->f_iosize = ntmp->ntm_bps * ntmp->ntm_spc;
	sbp->f_blocks = ntmp->ntm_bootfile.bf_spv;
	sbp->f_bfree = sbp->f_bavail = ntfs_cntobn(ntmp->ntm_cfree);
	sbp->f_ffree = sbp->f_favail = sbp->f_bfree / ntmp->ntm_bpmftrec;
	sbp->f_files = mftallocated / ntfs_bntob(ntmp->ntm_bpmftrec) +
	    sbp->f_ffree;
	sbp->f_fresvd = sbp->f_bresvd = 0; /* XXX */
	sbp->f_flag = mp->mnt_flag;
	copy_statvfs_info(sbp, mp);
	return (0);
}

static int
ntfs_sync (
	struct mount *mp,
	int waitfor,
	kauth_cred_t cred)
{
	/*dprintf(("ntfs_sync():\n"));*/
	return (0);
}

/*ARGSUSED*/
static int
ntfs_fhtovp(
	struct mount *mp,
	struct fid *fhp,
	struct vnode **vpp)
{
	struct ntfid ntfh;
	int error;

	if (fhp->fid_len != sizeof(struct ntfid))
		return EINVAL;
	memcpy(&ntfh, fhp, sizeof(ntfh));
	ddprintf(("ntfs_fhtovp(): %s: %llu\n", mp->mnt_stat.f_mntonname,
	    (unsigned long long)ntfh.ntfid_ino));

	error = ntfs_vgetex(mp, ntfh.ntfid_ino, ntfh.ntfid_attr, NULL,
			LK_EXCLUSIVE, 0, vpp);
	if (error != 0) {
		*vpp = NULLVP;
		return (error);
	}

	/* XXX as unlink/rmdir/mkdir/creat are not currently possible
	 * with NTFS, we don't need to check anything else for now */
	return (0);
}

static int
ntfs_vptofh(
	struct vnode *vp,
	struct fid *fhp,
	size_t *fh_size)
{
	struct ntnode *ntp;
	struct ntfid ntfh;
	struct fnode *fn;

	if (*fh_size < sizeof(struct ntfid)) {
		*fh_size = sizeof(struct ntfid);
		return E2BIG;
	}
	*fh_size = sizeof(struct ntfid);

	ddprintf(("ntfs_fhtovp(): %s: %p\n", vp->v_mount->mnt_stat.f_mntonname,
		vp));

	fn = VTOF(vp);
	ntp = VTONT(vp);
	memset(&ntfh, 0, sizeof(ntfh));
	ntfh.ntfid_len = sizeof(struct ntfid);
	ntfh.ntfid_ino = ntp->i_number;
	ntfh.ntfid_attr = fn->f_attrtype;
#ifdef notyet
	ntfh.ntfid_gen = ntp->i_gen;
#endif
	memcpy(fhp, &ntfh, sizeof(ntfh));
	return (0);
}

int
ntfs_vgetex(
	struct mount *mp,
	ino_t ino,
	u_int32_t attrtype,
	char *attrname,
	u_long lkflags,
	u_long flags,
	struct vnode **vpp)
{
	int error;
	struct ntfsmount *ntmp;
	struct ntnode *ip;
	struct fnode *fp;
	struct vnode *vp;
	enum vtype f_type = VBAD;

	dprintf(("ntfs_vgetex: ino: %llu, attr: 0x%x:%s, lkf: 0x%lx, f:"
	    " 0x%lx\n", (unsigned long long)ino, attrtype,
	    attrname ? attrname : "", (u_long)lkflags, (u_long)flags));

	ntmp = VFSTONTFS(mp);
	*vpp = NULL;

loop:
	/* Get ntnode */
	error = ntfs_ntlookup(ntmp, ino, &ip);
	if (error) {
		printf("ntfs_vget: ntfs_ntget failed\n");
		return (error);
	}

	/* It may be not initialized fully, so force load it */
	if (!(flags & VG_DONTLOADIN) && !(ip->i_flag & IN_LOADED)) {
		error = ntfs_loadntnode(ntmp, ip);
		if(error) {
			printf("ntfs_vget: CAN'T LOAD ATTRIBUTES FOR INO:"
			    " %llu\n", (unsigned long long)ip->i_number);
			ntfs_ntput(ip);
			return (error);
		}
	}

	error = ntfs_fget(ntmp, ip, attrtype, attrname, &fp);
	if (error) {
		printf("ntfs_vget: ntfs_fget failed\n");
		ntfs_ntput(ip);
		return (error);
	}

	if (!(flags & VG_DONTVALIDFN) && !(fp->f_flag & FN_VALID)) {
		if ((ip->i_frflag & NTFS_FRFLAG_DIR) &&
		    (fp->f_attrtype == NTFS_A_DATA && fp->f_attrname == NULL)) {
			f_type = VDIR;
		} else if (flags & VG_EXT) {
			f_type = VNON;
			fp->f_size = fp->f_allocated = 0;
		} else {
			f_type = VREG;

			error = ntfs_filesize(ntmp, fp,
					      &fp->f_size, &fp->f_allocated);
			if (error) {
				ntfs_ntput(ip);
				return (error);
			}
		}

		fp->f_flag |= FN_VALID;
	}

	/*
	 * We may be calling vget() now. To avoid potential deadlock, we need
	 * to release ntnode lock, since due to locking order vnode
	 * lock has to be acquired first.
	 * ntfs_fget() bumped ntnode usecount, so ntnode won't be recycled
	 * prematurely.
	 * Take v_interlock before releasing ntnode lock to avoid races.
	 */
	vp = FTOV(fp);
	if (vp) {
		mutex_enter(vp->v_interlock);
		ntfs_ntput(ip);
		if (vget(vp, lkflags) != 0)
			goto loop;
		*vpp = vp;
		return 0;
	}
	ntfs_ntput(ip);

	error = getnewvnode(VT_NTFS, ntmp->ntm_mountp, ntfs_vnodeop_p,
	    NULL, &vp);
	if(error) {
		ntfs_frele(fp);
		return (error);
	}
	ntfs_ntget(ip);
	error = ntfs_fget(ntmp, ip, attrtype, attrname, &fp);
	if (error) {
		printf("ntfs_vget: ntfs_fget failed\n");
		ntfs_ntput(ip);
		return (error);
	}
	if (FTOV(fp)) {
		/*
		 * Another thread beat us, put back freshly allocated
		 * vnode and retry.
		 */
		ntfs_ntput(ip);
		ungetnewvnode(vp);
		goto loop;
	}
	dprintf(("ntfs_vget: vnode: %p for ntnode: %llu\n", vp,
	    (unsigned long long)ino));

	fp->f_vp = vp;
	vp->v_data = fp;
	if (f_type != VBAD)
		vp->v_type = f_type;
	genfs_node_init(vp, &ntfs_genfsops);

	if (ino == NTFS_ROOTINO)
		vp->v_vflag |= VV_ROOT;

	ntfs_ntput(ip);

	if (lkflags & (LK_EXCLUSIVE | LK_SHARED)) {
		error = vn_lock(vp, lkflags);
		if (error) {
			vput(vp);
			return (error);
		}
	}

	uvm_vnp_setsize(vp, fp->f_size); /* XXX: mess, cf. ntfs_lookupfile() */
	vref(ip->i_devvp);
	*vpp = vp;
	return (0);
}

static int
ntfs_vget(
	struct mount *mp,
	ino_t ino,
	struct vnode **vpp)
{
	return ntfs_vgetex(mp, ino, NTFS_A_DATA, NULL, LK_EXCLUSIVE, 0, vpp);
}

extern const struct vnodeopv_desc ntfs_vnodeop_opv_desc;

const struct vnodeopv_desc * const ntfs_vnodeopv_descs[] = {
	&ntfs_vnodeop_opv_desc,
	NULL,
};

struct vfsops ntfs_vfsops = {
	.vfs_name = MOUNT_NTFS,
	.vfs_min_mount_data = sizeof (struct ntfs_args),
	.vfs_mount = ntfs_mount,
	.vfs_start = ntfs_start,
	.vfs_unmount = ntfs_unmount,
	.vfs_root = ntfs_root,
	.vfs_quotactl = (void *)eopnotsupp,
	.vfs_statvfs = ntfs_statvfs,
	.vfs_sync = ntfs_sync,
	.vfs_vget = ntfs_vget,
	.vfs_fhtovp = ntfs_fhtovp,
	.vfs_vptofh = ntfs_vptofh,
	.vfs_init = ntfs_init,
	.vfs_reinit = ntfs_reinit,
	.vfs_done = ntfs_done,
	.vfs_mountroot = ntfs_mountroot,
	.vfs_snapshot = (void *)eopnotsupp,
	.vfs_extattrctl = vfs_stdextattrctl,
	.vfs_suspendctl = (void *)eopnotsupp,
	.vfs_renamelock_enter = genfs_renamelock_enter,
	.vfs_renamelock_exit = genfs_renamelock_exit,
	.vfs_fsync = (void *)eopnotsupp,
	.vfs_opv_descs = ntfs_vnodeopv_descs
};

static int
ntfs_modcmd(modcmd_t cmd, void *arg)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = vfs_attach(&ntfs_vfsops);
		if (error != 0)
			break;
		sysctl_createv(&ntfs_sysctl_log, 0, NULL, NULL,
			       CTLFLAG_PERMANENT,
			       CTLTYPE_NODE, "ntfs",
			       SYSCTL_DESCR("NTFS file system"),
			       NULL, 0, NULL, 0,
			       CTL_VFS, 20, CTL_EOL);
		/*
		 * XXX the "20" above could be dynamic, thereby eliminating
		 * one more instance of the "number to vfs" mapping problem,
		 * but "20" is the order as taken from sys/mount.h
		 */
		break;
	case MODULE_CMD_FINI:
		error = vfs_detach(&ntfs_vfsops);
		if (error != 0)
			break;
		sysctl_teardown(&ntfs_sysctl_log);
		break;
	default:
		error = ENOTTY;
		break;
	}

	return (error);
}
