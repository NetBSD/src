/*	$NetBSD: msdosfs_vfsops.c,v 1.24.2.7 2008/01/21 09:45:47 yamt Exp $	*/

/*-
 * Copyright (C) 1994, 1995, 1997 Wolfgang Solfrank.
 * Copyright (C) 1994, 1995, 1997 TooLs GmbH.
 * All rights reserved.
 * Original code by Paul Popelka (paulp@uts.amdahl.com) (see below).
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Written by Paul Popelka (paulp@uts.amdahl.com)
 *
 * You can do anything you want with this software, just don't say you wrote
 * it, and don't remove this notice.
 *
 * This software is provided "as is".
 *
 * The author supplies this software to be publicly redistributed on the
 * understanding that the author is not responsible for the correct
 * functioning of this software in any circumstances and is not liable for
 * any damages caused by this software.
 *
 * October 1992
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: msdosfs_vfsops.c,v 1.24.2.7 2008/01/21 09:45:47 yamt Exp $");

#if defined(_KERNEL_OPT)
#include "opt_quota.h"
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <miscfs/specfs/specdev.h> /* XXX */	/* defines v_rdev */
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/file.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <sys/conf.h>
#include <sys/kauth.h>

#include <fs/msdosfs/bpb.h>
#include <fs/msdosfs/bootsect.h>
#include <fs/msdosfs/direntry.h>
#include <fs/msdosfs/denode.h>
#include <fs/msdosfs/msdosfsmount.h>
#include <fs/msdosfs/fat.h>

#ifdef MSDOSFS_DEBUG
#define DPRINTF(a) uprintf a
#else
#define DPRINTF(a)
#endif

#define MSDOSFS_NAMEMAX(pmp) \
	(pmp)->pm_flags & MSDOSFSMNT_LONGNAME ? WIN_MAXLEN : 12

VFS_PROTOS(msdosfs);

int msdosfs_mountfs(struct vnode *, struct mount *, struct lwp *,
    struct msdosfs_args *);

static int update_mp(struct mount *, struct msdosfs_args *);

MALLOC_JUSTDEFINE(M_MSDOSFSMNT, "MSDOSFS mount", "MSDOS FS mount structure");
MALLOC_JUSTDEFINE(M_MSDOSFSFAT, "MSDOSFS fat", "MSDOS FS fat table");
MALLOC_JUSTDEFINE(M_MSDOSFSTMP, "MSDOSFS temp", "MSDOS FS temp. structures");

#define ROOTNAME "root_device"

extern const struct vnodeopv_desc msdosfs_vnodeop_opv_desc;

const struct vnodeopv_desc * const msdosfs_vnodeopv_descs[] = {
	&msdosfs_vnodeop_opv_desc,
	NULL,
};

struct vfsops msdosfs_vfsops = {
	MOUNT_MSDOS,
	sizeof (struct msdosfs_args),
	msdosfs_mount,
	msdosfs_start,
	msdosfs_unmount,
	msdosfs_root,
	(void *)eopnotsupp,		/* vfs_quotactl */
	msdosfs_statvfs,
	msdosfs_sync,
	msdosfs_vget,
	msdosfs_fhtovp,
	msdosfs_vptofh,
	msdosfs_init,
	msdosfs_reinit,
	msdosfs_done,
	msdosfs_mountroot,
	(int (*)(struct mount *, struct vnode *, struct timespec *)) eopnotsupp,
	vfs_stdextattrctl,
	(void *)eopnotsupp,		/* vfs_suspendctl */
	msdosfs_vnodeopv_descs,
	0,
	{ NULL, NULL },
};
VFS_ATTACH(msdosfs_vfsops);

static int
update_mp(mp, argp)
	struct mount *mp;
	struct msdosfs_args *argp;
{
	struct msdosfsmount *pmp = VFSTOMSDOSFS(mp);
	int error;

	pmp->pm_gid = argp->gid;
	pmp->pm_uid = argp->uid;
	pmp->pm_mask = argp->mask & ALLPERMS;
	pmp->pm_dirmask = argp->dirmask & ALLPERMS;
	pmp->pm_gmtoff = argp->gmtoff;
	pmp->pm_flags |= argp->flags & MSDOSFSMNT_MNTOPT;

	/*
	 * GEMDOS knows nothing (yet) about win95
	 */
	if (pmp->pm_flags & MSDOSFSMNT_GEMDOSFS)
		pmp->pm_flags |= MSDOSFSMNT_NOWIN95;

	if (pmp->pm_flags & MSDOSFSMNT_NOWIN95)
		pmp->pm_flags |= MSDOSFSMNT_SHORTNAME;
	else if (!(pmp->pm_flags &
	    (MSDOSFSMNT_SHORTNAME | MSDOSFSMNT_LONGNAME))) {
		struct vnode *rtvp;

		/*
		 * Try to divine whether to support Win'95 long filenames
		 */
		if (FAT32(pmp))
			pmp->pm_flags |= MSDOSFSMNT_LONGNAME;
		else {
			if ((error = msdosfs_root(mp, &rtvp)) != 0)
				return error;
			pmp->pm_flags |= findwin95(VTODE(rtvp))
				? MSDOSFSMNT_LONGNAME
					: MSDOSFSMNT_SHORTNAME;
			vput(rtvp);
		}
	}

	mp->mnt_stat.f_namemax = MSDOSFS_NAMEMAX(pmp);

	return 0;
}

int
msdosfs_mountroot()
{
	struct mount *mp;
	struct lwp *l = curlwp;	/* XXX */
	int error;
	struct msdosfs_args args;

	if (device_class(root_device) != DV_DISK)
		return (ENODEV);

	if ((error = vfs_rootmountalloc(MOUNT_MSDOS, "root_device", &mp))) {
		vrele(rootvp);
		return (error);
	}

	args.flags = MSDOSFSMNT_VERSIONED;
	args.uid = 0;
	args.gid = 0;
	args.mask = 0777;
	args.version = MSDOSFSMNT_VERSION;
	args.dirmask = 0777;

	if ((error = msdosfs_mountfs(rootvp, mp, l, &args)) != 0) {
		mp->mnt_op->vfs_refcount--;
		vfs_unbusy(mp);
		vfs_destroy(mp);
		return (error);
	}

	if ((error = update_mp(mp, &args)) != 0) {
		(void)msdosfs_unmount(mp, 0);
		vfs_unbusy(mp);
		vfs_destroy(mp);
		vrele(rootvp);
		return (error);
	}

	mutex_enter(&mountlist_lock);
	CIRCLEQ_INSERT_TAIL(&mountlist, mp, mnt_list);
	mutex_exit(&mountlist_lock);
	(void)msdosfs_statvfs(mp, &mp->mnt_stat);
	vfs_unbusy(mp);
	return (0);
}

/*
 * mp - path - addr in user space of mount point (ie /usr or whatever)
 * data - addr in user space of mount params including the name of the block
 * special file to treat as a filesystem.
 */
int
msdosfs_mount(mp, path, data, data_len)
	struct mount *mp;
	const char *path;
	void *data;
	size_t *data_len;
{
	struct lwp *l = curlwp;
	struct nameidata nd;
	struct vnode *devvp;	  /* vnode for blk device to mount */
	struct msdosfs_args *args = data; /* holds data from mount request */
	/* msdosfs specific mount control block */
	struct msdosfsmount *pmp = NULL;
	int error, flags;
	mode_t accessmode;

	if (*data_len < sizeof *args)
		return EINVAL;

	if (mp->mnt_flag & MNT_GETARGS) {
		pmp = VFSTOMSDOSFS(mp);
		if (pmp == NULL)
			return EIO;
		args->fspec = NULL;
		args->uid = pmp->pm_uid;
		args->gid = pmp->pm_gid;
		args->mask = pmp->pm_mask;
		args->flags = pmp->pm_flags;
		args->version = MSDOSFSMNT_VERSION;
		args->dirmask = pmp->pm_dirmask;
		args->gmtoff = pmp->pm_gmtoff;
		*data_len = sizeof *args;
		return 0;
	}

	/*
	 * If not versioned (i.e. using old mount_msdos(8)), fill in
	 * the additional structure items with suitable defaults.
	 */
	if ((args->flags & MSDOSFSMNT_VERSIONED) == 0) {
		args->version = 1;
		args->dirmask = args->mask;
	}

	/*
	 * Reset GMT offset for pre-v3 mount structure args.
	 */
	if (args->version < 3)
		args->gmtoff = 0;

	/*
	 * If updating, check whether changing from read-only to
	 * read/write; if there is no device name, that's all we do.
	 */
	if (mp->mnt_flag & MNT_UPDATE) {
		pmp = VFSTOMSDOSFS(mp);
		error = 0;
		if (!(pmp->pm_flags & MSDOSFSMNT_RONLY) && (mp->mnt_flag & MNT_RDONLY)) {
			flags = WRITECLOSE;
			if (mp->mnt_flag & MNT_FORCE)
				flags |= FORCECLOSE;
			error = vflush(mp, NULLVP, flags);
		}
		if (!error && (mp->mnt_flag & MNT_RELOAD))
			/* not yet implemented */
			error = EOPNOTSUPP;
		if (error) {
			DPRINTF(("vflush %d\n", error));
			return (error);
		}
		if ((pmp->pm_flags & MSDOSFSMNT_RONLY) && (mp->mnt_iflag & IMNT_WANTRDWR)) {
			/*
			 * If upgrade to read-write by non-root, then verify
			 * that user has necessary permissions on the device.
			 */
			if (kauth_authorize_generic(l->l_cred,
			    KAUTH_GENERIC_ISSUSER, NULL) != 0) {
				devvp = pmp->pm_devvp;
				vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
				error = VOP_ACCESS(devvp, VREAD | VWRITE,
						   l->l_cred);
				VOP_UNLOCK(devvp, 0);
				DPRINTF(("VOP_ACCESS %d\n", error));
				if (error)
					return (error);
			}
			pmp->pm_flags &= ~MSDOSFSMNT_RONLY;
		}
		if (args->fspec == NULL) {
			DPRINTF(("missing fspec\n"));
			return EINVAL;
		}
	}
	/*
	 * Not an update, or updating the name: look up the name
	 * and verify that it refers to a sensible block device.
	 */
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, args->fspec);
	if ((error = namei(&nd)) != 0) {
		DPRINTF(("namei %d\n", error));
		return (error);
	}
	devvp = nd.ni_vp;

	if (devvp->v_type != VBLK) {
		DPRINTF(("not block\n"));
		vrele(devvp);
		return (ENOTBLK);
	}
	if (bdevsw_lookup(devvp->v_rdev) == NULL) {
		DPRINTF(("no block switch\n"));
		vrele(devvp);
		return (ENXIO);
	}
	/*
	 * If mount by non-root, then verify that user has necessary
	 * permissions on the device.
	 */
	if (kauth_authorize_generic(l->l_cred, KAUTH_GENERIC_ISSUSER, NULL) != 0) {
		accessmode = VREAD;
		if ((mp->mnt_flag & MNT_RDONLY) == 0)
			accessmode |= VWRITE;
		vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
		error = VOP_ACCESS(devvp, accessmode, l->l_cred);
		VOP_UNLOCK(devvp, 0);
		if (error) {
			DPRINTF(("VOP_ACCESS2 %d\n", error));
			vrele(devvp);
			return (error);
		}
	}
	if ((mp->mnt_flag & MNT_UPDATE) == 0) {
		int xflags;

		/*
		 * Disallow multiple mounts of the same device.
		 * Disallow mounting of a device that is currently in use
		 * (except for root, which might share swap device for
		 * miniroot).
		 */
		error = vfs_mountedon(devvp);
		if (error) {
			DPRINTF(("vfs_mountedon %d\n", error));
			goto fail;
		}
		if (vcount(devvp) > 1 && devvp != rootvp) {
			DPRINTF(("vcount %d\n", error));
			error = EBUSY;
			goto fail;
		}
		if (mp->mnt_flag & MNT_RDONLY)
			xflags = FREAD;
		else
			xflags = FREAD|FWRITE;
		error = VOP_OPEN(devvp, xflags, FSCRED);
		if (error) {
			DPRINTF(("VOP_OPEN %d\n", error));
			goto fail;
		}
		error = msdosfs_mountfs(devvp, mp, l, args);
		if (error) {
			DPRINTF(("msdosfs_mountfs %d\n", error));
			vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
			(void) VOP_CLOSE(devvp, xflags, NOCRED);
			VOP_UNLOCK(devvp, 0);
			goto fail;
		}
#ifdef MSDOSFS_DEBUG		/* only needed for the printf below */
		pmp = VFSTOMSDOSFS(mp);
#endif
	} else {
		vrele(devvp);
		if (devvp != pmp->pm_devvp) {
			DPRINTF(("devvp %p pmp %p\n", 
			    devvp, pmp->pm_devvp));
			return (EINVAL);	/* needs translation */
		}
	}
	if ((error = update_mp(mp, args)) != 0) {
		msdosfs_unmount(mp, MNT_FORCE);
		DPRINTF(("update_mp %d\n", error));
		return error;
	}

#ifdef MSDOSFS_DEBUG
	printf("msdosfs_mount(): mp %p, pmp %p, inusemap %p\n", mp, pmp, pmp->pm_inusemap);
#endif
	return set_statvfs_info(path, UIO_USERSPACE, args->fspec, UIO_USERSPACE,
	    mp->mnt_op->vfs_name, mp, l);

fail:
	vrele(devvp);
	return (error);
}

int
msdosfs_mountfs(devvp, mp, l, argp)
	struct vnode *devvp;
	struct mount *mp;
	struct lwp *l;
	struct msdosfs_args *argp;
{
	struct msdosfsmount *pmp;
	struct buf *bp;
	dev_t dev = devvp->v_rdev;
	struct partinfo dpart;
	union bootsector *bsp;
	struct byte_bpb33 *b33;
	struct byte_bpb50 *b50;
	struct byte_bpb710 *b710;
	u_int8_t SecPerClust;
	int	ronly, error, tmp;
	int	bsize, dtype, fstype, secsize;
	u_int64_t psize;

	/* Flush out any old buffers remaining from a previous use. */
	if ((error = vinvalbuf(devvp, V_SAVE, l->l_cred, l, 0, 0)) != 0)
		return (error);

	ronly = (mp->mnt_flag & MNT_RDONLY) != 0;

	bp  = NULL; /* both used in error_exit */
	pmp = NULL;

	/*
 	 * We need the disklabel to calculate the size of a FAT entry
	 * later on. Also make sure the partition contains a filesystem
	 * of type FS_MSDOS. This doesn't work for floppies, so we have
	 * to check for them too.
 	 *
 	 * There might still be parts of the msdos fs driver which assume
	 * that the size of a disk block will always be 512 bytes.
	 * Let's root them out...
	 */
	error = VOP_IOCTL(devvp, DIOCGPART, &dpart, FREAD, NOCRED);
	if (error == 0) {
		secsize = dpart.disklab->d_secsize;
		dtype = dpart.disklab->d_type;
		fstype = dpart.part->p_fstype;
		psize = dpart.part->p_size;
	} else {
		struct dkwedge_info dkw;
		error = VOP_IOCTL(devvp, DIOCGWEDGEINFO, &dkw, FREAD, NOCRED);
		secsize = 512;	/* XXX */
		dtype = DTYPE_FLOPPY; /* XXX */
		fstype = FS_MSDOS;
		psize = -1;
		if (error) {
			if (error != ENOTTY) {
				DPRINTF(("Error getting partition info %d\n",
				    error));
				goto error_exit;
			}
		} else {
			fstype = strcmp(dkw.dkw_ptype, DKW_PTYPE_FAT) == 0 ?
			    FS_MSDOS : -1;
			psize = dkw.dkw_size;
		}
	}
	if (argp->flags & MSDOSFSMNT_GEMDOSFS) {
		bsize = secsize;
		if (bsize != 512 ||
		    (dtype != DTYPE_FLOPPY && fstype != FS_MSDOS)) {
			DPRINTF(("bsize %d dtype %d fstype %d\n", bsize, dtype,
			    fstype));
			error = EINVAL;
			goto error_exit;
		}
	} else
		bsize = 0;

	/*
	 * Read the boot sector of the filesystem, and then check the
	 * boot signature.  If not a dos boot sector then error out.
	 */
	if ((error = bread(devvp, 0, secsize, NOCRED, &bp)) != 0)
		goto error_exit;
	bsp = (union bootsector *)bp->b_data;
	b33 = (struct byte_bpb33 *)bsp->bs33.bsBPB;
	b50 = (struct byte_bpb50 *)bsp->bs50.bsBPB;
	b710 = (struct byte_bpb710 *)bsp->bs710.bsBPB;

	if (!(argp->flags & MSDOSFSMNT_GEMDOSFS)) {
		if (bsp->bs50.bsBootSectSig0 != BOOTSIG0
		    || bsp->bs50.bsBootSectSig1 != BOOTSIG1) {
			DPRINTF(("bootsig0 %d bootsig1 %d\n", 
			    bsp->bs50.bsBootSectSig0,
			    bsp->bs50.bsBootSectSig1));
			error = EINVAL;
			goto error_exit;
		}
	}

	pmp = malloc(sizeof *pmp, M_MSDOSFSMNT, M_WAITOK);
	memset(pmp, 0, sizeof *pmp);
	pmp->pm_mountp = mp;

	/*
	 * Compute several useful quantities from the bpb in the
	 * bootsector.  Copy in the dos 5 variant of the bpb then fix up
	 * the fields that are different between dos 5 and dos 3.3.
	 */
	SecPerClust = b50->bpbSecPerClust;
	pmp->pm_BytesPerSec = getushort(b50->bpbBytesPerSec);
	pmp->pm_ResSectors = getushort(b50->bpbResSectors);
	pmp->pm_FATs = b50->bpbFATs;
	pmp->pm_RootDirEnts = getushort(b50->bpbRootDirEnts);
	pmp->pm_Sectors = getushort(b50->bpbSectors);
	pmp->pm_FATsecs = getushort(b50->bpbFATsecs);
	pmp->pm_SecPerTrack = getushort(b50->bpbSecPerTrack);
	pmp->pm_Heads = getushort(b50->bpbHeads);
	pmp->pm_Media = b50->bpbMedia;

	if (!(argp->flags & MSDOSFSMNT_GEMDOSFS)) {
		/* XXX - We should probably check more values here */
    		if (!pmp->pm_BytesPerSec || !SecPerClust
	    		|| pmp->pm_Heads > 255 || pmp->pm_SecPerTrack > 63) {
			DPRINTF(("bytespersec %d secperclust %d "
			    "heads %d secpertrack %d\n", 
			    pmp->pm_BytesPerSec, SecPerClust,
			    pmp->pm_Heads, pmp->pm_SecPerTrack));
			error = EINVAL;
			goto error_exit;
		}
	}

	if (pmp->pm_Sectors == 0) {
		pmp->pm_HiddenSects = getulong(b50->bpbHiddenSecs);
		pmp->pm_HugeSectors = getulong(b50->bpbHugeSectors);
	} else {
		pmp->pm_HiddenSects = getushort(b33->bpbHiddenSecs);
		pmp->pm_HugeSectors = pmp->pm_Sectors;
	}

	if (pmp->pm_RootDirEnts == 0) {
		unsigned short vers = getushort(b710->bpbFSVers);
		/*
		 * Some say that bsBootSectSig[23] must be zero, but
		 * Windows does not require this and some digital cameras
		 * do not set these to zero.  Therefore, do not insist.
		 */
		if (pmp->pm_Sectors || pmp->pm_FATsecs || vers) {
			DPRINTF(("sectors %d fatsecs %lu vers %d\n",
			    pmp->pm_Sectors, pmp->pm_FATsecs, vers));
			error = EINVAL;
			goto error_exit;
		}
		pmp->pm_fatmask = FAT32_MASK;
		pmp->pm_fatmult = 4;
		pmp->pm_fatdiv = 1;
		pmp->pm_FATsecs = getulong(b710->bpbBigFATsecs);

		/* mirrorring is enabled if the FATMIRROR bit is not set */
		if ((getushort(b710->bpbExtFlags) & FATMIRROR) == 0)
			pmp->pm_flags |= MSDOSFS_FATMIRROR;
		else
			pmp->pm_curfat = getushort(b710->bpbExtFlags) & FATNUM;
	} else
		pmp->pm_flags |= MSDOSFS_FATMIRROR;

	if (argp->flags & MSDOSFSMNT_GEMDOSFS) {
		if (FAT32(pmp)) {
			DPRINTF(("fat32 for gemdos\n"));
			/*
			 * GEMDOS doesn't know fat32.
			 */
			error = EINVAL;
			goto error_exit;
		}

		/*
		 * Check a few values (could do some more):
		 * - logical sector size: power of 2, >= block size
		 * - sectors per cluster: power of 2, >= 1
		 * - number of sectors:   >= 1, <= size of partition
		 */
		if ( (SecPerClust == 0)
		  || (SecPerClust & (SecPerClust - 1))
		  || (pmp->pm_BytesPerSec < bsize)
		  || (pmp->pm_BytesPerSec & (pmp->pm_BytesPerSec - 1))
		  || (pmp->pm_HugeSectors == 0)
		  || (pmp->pm_HugeSectors * (pmp->pm_BytesPerSec / bsize)
		      > psize)) {
			DPRINTF(("consistency checks for gemdos\n"));
			error = EINVAL;
			goto error_exit;
		}
		/*
		 * XXX - Many parts of the msdos fs driver seem to assume that
		 * the number of bytes per logical sector (BytesPerSec) will
		 * always be the same as the number of bytes per disk block
		 * Let's pretend it is.
		 */
		tmp = pmp->pm_BytesPerSec / bsize;
		pmp->pm_BytesPerSec  = bsize;
		pmp->pm_HugeSectors *= tmp;
		pmp->pm_HiddenSects *= tmp;
		pmp->pm_ResSectors  *= tmp;
		pmp->pm_Sectors     *= tmp;
		pmp->pm_FATsecs     *= tmp;
		SecPerClust         *= tmp;
	}

	/* Check that fs has nonzero FAT size */
	if (pmp->pm_FATsecs == 0) {
		DPRINTF(("FATsecs is 0\n"));
		error = EINVAL;
		goto error_exit;
	}

	pmp->pm_fatblk = pmp->pm_ResSectors;
	if (FAT32(pmp)) {
		pmp->pm_rootdirblk = getulong(b710->bpbRootClust);
		pmp->pm_firstcluster = pmp->pm_fatblk
			+ (pmp->pm_FATs * pmp->pm_FATsecs);
		pmp->pm_fsinfo = getushort(b710->bpbFSInfo);
	} else {
		pmp->pm_rootdirblk = pmp->pm_fatblk +
			(pmp->pm_FATs * pmp->pm_FATsecs);
		pmp->pm_rootdirsize = (pmp->pm_RootDirEnts * sizeof(struct direntry)
				       + pmp->pm_BytesPerSec - 1)
			/ pmp->pm_BytesPerSec;/* in sectors */
		pmp->pm_firstcluster = pmp->pm_rootdirblk + pmp->pm_rootdirsize;
	}

	pmp->pm_nmbrofclusters = (pmp->pm_HugeSectors - pmp->pm_firstcluster) /
	    SecPerClust;
	pmp->pm_maxcluster = pmp->pm_nmbrofclusters + 1;
	pmp->pm_fatsize = pmp->pm_FATsecs * pmp->pm_BytesPerSec;

	if (argp->flags & MSDOSFSMNT_GEMDOSFS) {
		if (pmp->pm_nmbrofclusters <= (0xff0 - 2)
		      && (dtype == DTYPE_FLOPPY
			  || (dtype == DTYPE_VND
				&& (pmp->pm_Heads == 1 || pmp->pm_Heads == 2)))
		    ) {
			pmp->pm_fatmask = FAT12_MASK;
			pmp->pm_fatmult = 3;
			pmp->pm_fatdiv = 2;
		} else {
			pmp->pm_fatmask = FAT16_MASK;
			pmp->pm_fatmult = 2;
			pmp->pm_fatdiv = 1;
		}
	} else if (pmp->pm_fatmask == 0) {
		if (pmp->pm_maxcluster
		    <= ((CLUST_RSRVD - CLUST_FIRST) & FAT12_MASK)) {
			/*
			 * This will usually be a floppy disk. This size makes
			 * sure that one fat entry will not be split across
			 * multiple blocks.
			 */
			pmp->pm_fatmask = FAT12_MASK;
			pmp->pm_fatmult = 3;
			pmp->pm_fatdiv = 2;
		} else {
			pmp->pm_fatmask = FAT16_MASK;
			pmp->pm_fatmult = 2;
			pmp->pm_fatdiv = 1;
		}
	}
	if (FAT12(pmp))
		pmp->pm_fatblocksize = 3 * pmp->pm_BytesPerSec;
	else
		pmp->pm_fatblocksize = MAXBSIZE;

	pmp->pm_fatblocksec = pmp->pm_fatblocksize / pmp->pm_BytesPerSec;
	pmp->pm_bnshift = ffs(pmp->pm_BytesPerSec) - 1;

	/*
	 * Compute mask and shift value for isolating cluster relative byte
	 * offsets and cluster numbers from a file offset.
	 */
	pmp->pm_bpcluster = SecPerClust * pmp->pm_BytesPerSec;
	pmp->pm_crbomask = pmp->pm_bpcluster - 1;
	pmp->pm_cnshift = ffs(pmp->pm_bpcluster) - 1;

	/*
	 * Check for valid cluster size
	 * must be a power of 2
	 */
	if (pmp->pm_bpcluster ^ (1 << pmp->pm_cnshift)) {
		DPRINTF(("bpcluster %lu cnshift %lu\n", 
		    pmp->pm_bpcluster, pmp->pm_cnshift));
		error = EINVAL;
		goto error_exit;
	}

	/*
	 * Release the bootsector buffer.
	 */
	brelse(bp, BC_AGE);
	bp = NULL;

	/*
	 * Check FSInfo.
	 */
	if (pmp->pm_fsinfo) {
		struct fsinfo *fp;

		/*
		 * XXX	If the fsinfo block is stored on media with
		 *	2KB or larger sectors, is the fsinfo structure
		 *	padded at the end or in the middle?
		 */
		if ((error = bread(devvp, de_bn2kb(pmp, pmp->pm_fsinfo),
		    pmp->pm_BytesPerSec, NOCRED, &bp)) != 0)
			goto error_exit;
		fp = (struct fsinfo *)bp->b_data;
		if (!memcmp(fp->fsisig1, "RRaA", 4)
		    && !memcmp(fp->fsisig2, "rrAa", 4)
		    && !memcmp(fp->fsisig3, "\0\0\125\252", 4)
		    && !memcmp(fp->fsisig4, "\0\0\125\252", 4))
			pmp->pm_nxtfree = getulong(fp->fsinxtfree);
		else
			pmp->pm_fsinfo = 0;
		brelse(bp, 0);
		bp = NULL;
	}

	/*
	 * Check and validate (or perhaps invalidate?) the fsinfo structure?
	 * XXX
	 */
	if (pmp->pm_fsinfo) {
		if (pmp->pm_nxtfree == (u_long)-1)
			pmp->pm_fsinfo = 0;
	}

	/*
	 * Allocate memory for the bitmap of allocated clusters, and then
	 * fill it in.
	 */
	pmp->pm_inusemap = malloc(((pmp->pm_maxcluster + N_INUSEBITS - 1)
				   / N_INUSEBITS)
				  * sizeof(*pmp->pm_inusemap),
				  M_MSDOSFSFAT, M_WAITOK);

	/*
	 * fillinusemap() needs pm_devvp.
	 */
	pmp->pm_dev = dev;
	pmp->pm_devvp = devvp;

	/*
	 * Have the inuse map filled in.
	 */
	if ((error = fillinusemap(pmp)) != 0) {
		DPRINTF(("fillinusemap %d\n", error));
		goto error_exit;
	}

	/*
	 * If they want fat updates to be synchronous then let them suffer
	 * the performance degradation in exchange for the on disk copy of
	 * the fat being correct just about all the time.  I suppose this
	 * would be a good thing to turn on if the kernel is still flakey.
	 */
	if (mp->mnt_flag & MNT_SYNCHRONOUS)
		pmp->pm_flags |= MSDOSFSMNT_WAITONFAT;

	/*
	 * Finish up.
	 */
	if (ronly)
		pmp->pm_flags |= MSDOSFSMNT_RONLY;
	else
		pmp->pm_fmod = 1;
	mp->mnt_data = pmp;
	mp->mnt_stat.f_fsidx.__fsid_val[0] = (long)dev;
	mp->mnt_stat.f_fsidx.__fsid_val[1] = makefstype(MOUNT_MSDOS);
	mp->mnt_stat.f_fsid = mp->mnt_stat.f_fsidx.__fsid_val[0];
	mp->mnt_stat.f_namemax = MSDOSFS_NAMEMAX(pmp);
	mp->mnt_flag |= MNT_LOCAL;
	mp->mnt_dev_bshift = pmp->pm_bnshift;
	mp->mnt_fs_bshift = pmp->pm_cnshift;

#ifdef QUOTA
	/*
	 * If we ever do quotas for DOS filesystems this would be a place
	 * to fill in the info in the msdosfsmount structure. You dolt,
	 * quotas on dos filesystems make no sense because files have no
	 * owners on dos filesystems. of course there is some empty space
	 * in the directory entry where we could put uid's and gid's.
	 */
#endif
	devvp->v_specmountpoint = mp;

	return (0);

error_exit:;
	if (bp)
		brelse(bp, BC_AGE);
	if (pmp) {
		if (pmp->pm_inusemap)
			free(pmp->pm_inusemap, M_MSDOSFSFAT);
		free(pmp, M_MSDOSFSMNT);
		mp->mnt_data = NULL;
	}
	return (error);
}

int
msdosfs_start(struct mount *mp, int flags)
{

	return (0);
}

/*
 * Unmount the filesystem described by mp.
 */
int
msdosfs_unmount(mp, mntflags)
	struct mount *mp;
	int mntflags;
{
	struct msdosfsmount *pmp;
	int error, flags;

	flags = 0;
	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;
#ifdef QUOTA
#endif
	if ((error = vflush(mp, NULLVP, flags)) != 0)
		return (error);
	pmp = VFSTOMSDOSFS(mp);
	if (pmp->pm_devvp->v_type != VBAD)
		pmp->pm_devvp->v_specmountpoint = NULL;
#ifdef MSDOSFS_DEBUG
	{
		struct vnode *vp = pmp->pm_devvp;

		printf("msdosfs_umount(): just before calling VOP_CLOSE()\n");
		printf("flag %08x, usecount %d, writecount %ld, holdcnt %ld\n",
		    vp->v_vflag | vp->v_iflag | vp->v_uflag, vp->v_usecount,
		    vp->v_writecount, vp->v_holdcnt);
		printf("mount %p, op %p\n",
		    vp->v_mount, vp->v_op);
		printf("freef %p, freeb %p, mount %p\n",
		    vp->v_freelist.tqe_next, vp->v_freelist.tqe_prev,
		    vp->v_mount);
		printf("cleanblkhd %p, dirtyblkhd %p, numoutput %d, type %d\n",
		    vp->v_cleanblkhd.lh_first,
		    vp->v_dirtyblkhd.lh_first,
		    vp->v_numoutput, vp->v_type);
		printf("union %p, tag %d, data[0] %08x, data[1] %08x\n",
		    vp->v_socket, vp->v_tag,
		    ((u_int *)vp->v_data)[0],
		    ((u_int *)vp->v_data)[1]);
	}
#endif
	vn_lock(pmp->pm_devvp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_CLOSE(pmp->pm_devvp,
	    pmp->pm_flags & MSDOSFSMNT_RONLY ? FREAD : FREAD|FWRITE, NOCRED);
	vput(pmp->pm_devvp);
	free(pmp->pm_inusemap, M_MSDOSFSFAT);
	free(pmp, M_MSDOSFSMNT);
	mp->mnt_data = NULL;
	mp->mnt_flag &= ~MNT_LOCAL;
	return (error);
}

int
msdosfs_root(mp, vpp)
	struct mount *mp;
	struct vnode **vpp;
{
	struct msdosfsmount *pmp = VFSTOMSDOSFS(mp);
	struct denode *ndep;
	int error;

#ifdef MSDOSFS_DEBUG
	printf("msdosfs_root(); mp %p, pmp %p\n", mp, pmp);
#endif
	if ((error = deget(pmp, MSDOSFSROOT, MSDOSFSROOT_OFS, &ndep)) != 0)
		return (error);
	*vpp = DETOV(ndep);
	return (0);
}

int
msdosfs_statvfs(struct mount *mp, struct statvfs *sbp)
{
	struct msdosfsmount *pmp;

	pmp = VFSTOMSDOSFS(mp);
	sbp->f_bsize = pmp->pm_bpcluster;
	sbp->f_frsize = sbp->f_bsize;
	sbp->f_iosize = pmp->pm_bpcluster;
	sbp->f_blocks = pmp->pm_nmbrofclusters;
	sbp->f_bfree = pmp->pm_freeclustercount;
	sbp->f_bavail = pmp->pm_freeclustercount;
	sbp->f_bresvd = 0;
	sbp->f_files = pmp->pm_RootDirEnts;			/* XXX */
	sbp->f_ffree = 0;	/* what to put in here? */
	sbp->f_favail = 0;	/* what to put in here? */
	sbp->f_fresvd = 0;
	copy_statvfs_info(sbp, mp);
	return (0);
}

int
msdosfs_sync(mp, waitfor, cred)
	struct mount *mp;
	int waitfor;
	kauth_cred_t cred;
{
	struct vnode *vp, *mvp;
	struct denode *dep;
	struct msdosfsmount *pmp = VFSTOMSDOSFS(mp);
	int error, allerror = 0;

	/*
	 * If we ever switch to not updating all of the fats all the time,
	 * this would be the place to update them from the first one.
	 */
	if (pmp->pm_fmod != 0) {
		if (pmp->pm_flags & MSDOSFSMNT_RONLY)
			panic("msdosfs_sync: rofs mod");
		else {
			/* update fats here */
		}
	}
	/* Allocate a marker vnode. */
	if ((mvp = vnalloc(mp)) == NULL)
		return ENOMEM;
	/*
	 * Write back each (modified) denode.
	 */
	mutex_enter(&mntvnode_lock);
loop:
	for (vp = TAILQ_FIRST(&mp->mnt_vnodelist); vp; vp = vunmark(mvp)) {
		vmark(mvp, vp);
		if (vp->v_mount != mp || vismarker(vp))
			continue;
		mutex_enter(&vp->v_interlock);
		dep = VTODE(vp);
		if (waitfor == MNT_LAZY || vp->v_type == VNON ||
		    (((dep->de_flag &
		    (DE_ACCESS | DE_CREATE | DE_UPDATE | DE_MODIFIED)) == 0) &&
		     (LIST_EMPTY(&vp->v_dirtyblkhd) &&
		      UVM_OBJ_IS_CLEAN(&vp->v_uobj)))) {
			mutex_exit(&vp->v_interlock);
			continue;
		}
		mutex_exit(&mntvnode_lock);
		error = vget(vp, LK_EXCLUSIVE | LK_NOWAIT | LK_INTERLOCK);
		if (error) {
			mutex_enter(&mntvnode_lock);
			if (error == ENOENT) {
				(void)vunmark(mvp);
				goto loop;
			}
			continue;
		}
		if ((error = VOP_FSYNC(vp, cred,
		    waitfor == MNT_WAIT ? FSYNC_WAIT : 0, 0, 0)) != 0)
			allerror = error;
		vput(vp);
		mutex_enter(&mntvnode_lock);
	}
	mutex_exit(&mntvnode_lock);
	vnfree(mvp);

	/*
	 * Force stale file system control information to be flushed.
	 */
	if ((error = VOP_FSYNC(pmp->pm_devvp, cred,
	    waitfor == MNT_WAIT ? FSYNC_WAIT : 0, 0, 0)) != 0)
		allerror = error;
#ifdef QUOTA
	/* qsync(mp); */
#endif
	return (allerror);
}

int
msdosfs_fhtovp(mp, fhp, vpp)
	struct mount *mp;
	struct fid *fhp;
	struct vnode **vpp;
{
	struct msdosfsmount *pmp = VFSTOMSDOSFS(mp);
	struct defid defh;
	struct denode *dep;
	int error;

	if (fhp->fid_len != sizeof(struct defid)) {
		DPRINTF(("fid_len %d %zd\n", fhp->fid_len,
		    sizeof(struct defid)));
		return EINVAL;
	}

	memcpy(&defh, fhp, sizeof(defh));
	error = deget(pmp, defh.defid_dirclust, defh.defid_dirofs, &dep);
	if (error) {
		DPRINTF(("deget %d\n", error));
		*vpp = NULLVP;
		return (error);
	}
	*vpp = DETOV(dep);
	return (0);
}

int
msdosfs_vptofh(vp, fhp, fh_size)
	struct vnode *vp;
	struct fid *fhp;
	size_t *fh_size;
{
	struct denode *dep;
	struct defid defh;

	if (*fh_size < sizeof(struct defid)) {
		*fh_size = sizeof(struct defid);
		return E2BIG;
	}
	*fh_size = sizeof(struct defid);
	dep = VTODE(vp);
	memset(&defh, 0, sizeof(defh));
	defh.defid_len = sizeof(struct defid);
	defh.defid_dirclust = dep->de_dirclust;
	defh.defid_dirofs = dep->de_diroffset;
	/* defh.defid_gen = dep->de_gen; */
	memcpy(fhp, &defh, sizeof(defh));
	return (0);
}

int
msdosfs_vget(struct mount *mp, ino_t ino,
    struct vnode **vpp)
{

	return (EOPNOTSUPP);
}

SYSCTL_SETUP(sysctl_vfs_msdosfs_setup, "sysctl vfs.msdosfs subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "vfs", NULL,
		       NULL, 0, NULL, 0,
		       CTL_VFS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "msdosfs",
		       SYSCTL_DESCR("MS-DOS file system"),
		       NULL, 0, NULL, 0,
		       CTL_VFS, 4, CTL_EOL);
	/*
	 * XXX the "4" above could be dynamic, thereby eliminating one
	 * more instance of the "number to vfs" mapping problem, but
	 * "4" is the order as taken from sys/mount.h
	 */
}
