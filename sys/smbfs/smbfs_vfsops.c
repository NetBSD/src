/*	$NetBSD: smbfs_vfsops.c,v 1.1.2.2 2001/01/08 14:58:21 bouyer Exp $	*/

/*
 * Copyright (c) 2000, Boris Popov
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
 *    This product includes software developed by Boris Popov.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 */

#include "netsmb.h"
#ifndef NNETSMB
#error "SMBFS requires NSMB device"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/malloc.h>

#include <sys/tree.h>

#include <netsmb/smb.h>
#include <netsmb/smb_conn.h>
#include <netsmb/smb_subr.h>
#include <netsmb/smb_dev.h>
#include <netsmb/smb_lock.h>

#include <smbfs/smbfs.h>
#include <smbfs/smbfs_node.h>
#include <smbfs/smbfs_subr.h>

#ifdef FB_CURRENT
#include <sys/bio.h>
#endif
#include <sys/buf.h>

int smbfs_debuglevel = 0;

#ifndef NetBSD
static int smbfs_version = SMBFS_VERSION;
#endif

#ifdef SMBFS_USEZONE
#include <vm/vm_extern.h>
#include <vm/vm_zone.h>

vm_zone_t smbfsmount_zone;
#endif

#ifndef NetBSD
#ifdef SYSCTL_DECL
SYSCTL_DECL(_vfs_smbfs);
#endif
SYSCTL_NODE(_vfs, OID_AUTO, smbfs, CTLFLAG_RW, 0, "SMB/CIFS file system");
SYSCTL_INT(_vfs_smbfs, OID_AUTO, version, CTLFLAG_RD, &smbfs_version, 0, "");
SYSCTL_INT(_vfs_smbfs, OID_AUTO, debuglevel, CTLFLAG_RW, &smbfs_debuglevel, 0, "");
#endif

int smbfs_mount __P((struct mount *, const char *, void *,
			struct nameidata *, struct proc *));
int smbfs_quotactl __P((struct mount *, int, uid_t, caddr_t,
			struct proc *));
int smbfs_root __P((struct mount *, struct vnode **));
int smbfs_start __P((struct mount *, int, struct proc *));
int smbfs_statfs __P((struct mount *, struct statfs *,
			struct proc *));
int smbfs_sync __P((struct mount *, int, struct ucred *,
			struct proc *));
int smbfs_unmount __P((struct mount *, int, struct proc *));
void smbfs_init __P((void));
#ifndef __NetBSD__
static int smbfs_uninit __P((struct vfsconf *vfsp));
#else
static void smbfs_uninit __P((void));
#endif

int smbfs_vget __P((struct mount *mp, ino_t ino,
			struct vnode **vpp));
int smbfs_fhtovp __P((struct mount *, struct fid *,
			struct vnode **));
int smbfs_vptofh __P((struct vnode *, struct fid *));

#ifndef NetBSD
static struct vfsops smbfs_vfsops = {
	smbfs_mount,
	smbfs_start,
	smbfs_unmount,
	smbfs_root,
	smbfs_quotactl,
	smbfs_statfs,
	smbfs_sync,
#if __FreeBSD_version > 400008
	vfs_stdvget,
	vfs_stdfhtovp,		/* shouldn't happen */
	vfs_stdcheckexp,
	vfs_stdvptofh,		/* shouldn't happen */
#else
	smbfs_vget,
	smbfs_fhtovp,
	smbfs_vptofh,
#endif
	smbfs_init,
	smbfs_uninit,
#ifndef FB_RELENG3
	vfs_stdextattrctl,
#else
#define	M_USE_RESERVE	M_KERNEL
	&sysctl___vfs_smbfs
#endif
};


VFS_SET(smbfs_vfsops, smbfs, VFCF_NETWORK);
#else
extern struct vnodeopv_desc smbfs_vnodeop_opv_desc;

struct vnodeopv_desc *smbfs_vnodeopv_descs[] = {
	&smbfs_vnodeop_opv_desc,
	NULL,
};

struct vfsops smbfs_vfsops = {
	MOUNT_SMBFS,
	smbfs_mount,
	smbfs_start,
	smbfs_unmount,
	smbfs_root,
	smbfs_quotactl,
	smbfs_statfs,
	smbfs_sync,
	smbfs_vget,
	smbfs_fhtovp,
	smbfs_vptofh,
	smbfs_init,
#ifdef __NetBSD__
	smbfs_uninit,
#endif
    (int (*) (int *, u_int, void *, size_t *, void *, size_t, struct proc *))
	eopnotsupp,		/* sysctl */
    (int (*)(void)) eopnotsupp,	/* mountroot */
    (int (*)(struct mount *, struct mbuf *, int *, struct ucred **))
	eopnotsupp,		/* checkexp */
	smbfs_vnodeopv_descs,
};

#define	M_USE_RESERVE	M_WAITOK
#ifndef index
#define index strchr
#endif
#endif


#ifdef MODULE_DEPEND
MODULE_DEPEND(smbfs, netsmb, 1, 1, 1);
MODULE_DEPEND(smbfs, libiconv, 1, 1, 1);
#endif

int smbfs_pbuf_freecnt = -1;	/* start out unlimited */

int
smbfs_mount(struct mount *mp, const char *path, void *data, 
	struct nameidata *ndp, struct proc *p)
{
	struct smbfs_args args; 	  /* will hold data from mount request */
	struct smbmount *smp = NULL;
	struct smb_conn *scp;
	struct smb_share *ssp = NULL;
	struct vnode *vp;
	struct smb_cred scred;
	size_t size;
	int error;
	char *pc, *pe;

	SMBVDEBUG("\n");
	if (data == NULL) {
		printf("missing data argument\n");
		return EINVAL;
	}
	if (mp->mnt_flag & MNT_UPDATE) {
		printf("MNT_UPDATE not implemented\n");
		return EOPNOTSUPP;
	}
	error = copyin(data, (caddr_t)&args, sizeof(struct smbfs_args));
	if (error)
		return error;
	if (args.version != SMBFS_VERSION) {
		printf("mount version mismatch: kernel=%d, mount=%d\n",
		    SMBFS_VERSION, args.version);
		return EINVAL;
	}
	smb_makescred(&scred, p, p->p_ucred);
	error = smb_dev2share(args.dev, SMBM_EXEC, &scred, &ssp);
	if (error) {
		printf("invalid device handle %d\n", args.dev);
		return error;
	}
	scp = SSTOCN(ssp);
	smb_share_unlock(ssp, 0, &scred);
	mp->mnt_stat.f_iosize = SSTOVC(ssp)->vc_txmax;

#ifdef SMBFS_USEZONE
	smp = zalloc(smbfsmount_zone);
#else
        MALLOC(smp, struct smbmount*, sizeof(*smp), M_SMBFSDATA, M_USE_RESERVE);
#endif
        if (smp == NULL) {
                printf("could not alloc smbmount\n");
                error = ENOMEM;
		goto bad;
        }
	bzero(smp, sizeof(*smp));
        mp->mnt_data = (qaddr_t)smp;
	smp->sm_share = ssp;
	smp->sm_root = NULL;
        smp->sm_args = args;
	smp->sm_caseopt = args.caseopt;
	smp->sm_args.file_mode = (smp->sm_args.file_mode &
			    (S_IRWXU|S_IRWXG|S_IRWXO)) | S_IFREG;
	smp->sm_args.dir_mode  = (smp->sm_args.dir_mode &
			    (S_IRWXU|S_IRWXG|S_IRWXO)) | S_IFDIR;

	simple_lock_init(&smp->sm_npslock);
	error = copyinstr(path, mp->mnt_stat.f_mntonname, MNAMELEN - 1, &size);
	if (error)
		goto bad;
	bzero(mp->mnt_stat.f_mntonname + size, MNAMELEN - size);
	pc = mp->mnt_stat.f_mntfromname;
	pe = pc + sizeof(mp->mnt_stat.f_mntfromname);
	bzero(pc, MNAMELEN);
	*pc++ = '/';
	*pc++ = '/';
	pc=index(strncpy(pc, SSTOVC(ssp)->vc_user, pe - pc - 2), 0);
	if (pc < pe-1) {
		*(pc++) = '@';
		pc = index(strncpy(pc, scp->sc_srvname, pe - pc - 2), 0);
		if (pc < pe - 1) {
			*(pc++) = '/';
			strncpy(pc, ssp->ss_name, pe - pc - 2);
		}
	}
	/* protect against invalid mount points */
	smp->sm_args.mount_point[sizeof(smp->sm_args.mount_point) - 1] = '\0';
#ifdef __NetBSD__
	vfs_getnewfsid(mp);
#else
vfs_getnewfsid(mp, MOUNT_SMBFS);
#endif
	error = smbfs_root(mp, &vp);
	if (error)
		goto bad;
	VOP_UNLOCK(vp, 0/*  , curproc */);
	SMBVDEBUG("root.v_usecount = %ld\n", vp->v_usecount);
	return error;
bad:
        if (smp)
#ifdef SMBFS_USEZONE
		zfree(smbfsmount_zone, smp);
#else
		free(smp, M_SMBFSDATA);
#endif
	if (ssp)
		smb_share_put(ssp, p);
        return error;
}

/* Unmount the filesystem described by mp. */
int
smbfs_unmount(struct mount *mp, int mntflags, struct proc *p)
{
	struct smbmount *smp = VFSTOSMBFS(mp);
	struct vnode *vp;
	int error, flags;

	SMBVDEBUG("smbfs_unmount: flags=%04x\n", mntflags);
	flags = 0;
	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;
	error = VFS_ROOT(mp, &vp);
	if (error)
		return (error);
	if ((mntflags & MNT_FORCE) == 0 && vp->v_usecount > 2) {
		printf("smbfs_unmount: usecnt=%d\n", vp->v_usecount);
		vput(vp);
		return EBUSY;
	}
	error = vflush(mp, vp, flags);
	if (error) {
		vput(vp);
		return error;
	}
	vput(vp);
	vrele(vp);
	vgone(vp);
	smb_share_put(smp->sm_share, p);
	mp->mnt_data = (qaddr_t)0;

#ifdef SMBFS_USEZONE
	zfree(smbfsmount_zone, smp);
#else
	free(smp, M_SMBFSDATA);
#endif
	mp->mnt_flag &= ~MNT_LOCAL;
	return error;
}

/* 
 * Return locked root vnode of a filesystem
 */
int
smbfs_root(struct mount *mp, struct vnode **vpp)
{
	struct smbmount *smp = VFSTOSMBFS(mp);
	struct vnode *vp;
	struct smbnode *np;
	struct smbfattr fattr;
	struct proc *p = curproc;
	struct ucred *cred = p->p_ucred;
	struct smb_cred scred;
	int error;

	SMBVDEBUG("\n");
	if (smp->sm_root) {
		*vpp = SMBTOV(smp->sm_root);
		vget(*vpp, LK_EXCLUSIVE | LK_RETRY/*  , p */);
		return 0;
	}
	smb_makescred(&scred, p, cred);
	error = smbfs_smb_lookup(NULL, NULL, 0, &fattr, &scred);
	error = smbfs_nget(mp, NULL, "TheRooT", 7, &fattr, &vp);
	if (error)
		return error;
	vp->v_flag |= VROOT;
	np = VTOSMB(vp);
	smp->sm_root = np;
	*vpp = vp;
	return 0;
}

/*
 * Vfs start routine, a no-op.
 */
/* ARGSUSED */
int
smbfs_start(mp, flags, p)
	struct mount *mp;
	int flags;
	struct proc *p;
{
	SMBVDEBUG("flags=%04x\n", flags);
	return 0;
}

/*
 * Do operations associated with quotas, not supported
 */
/* ARGSUSED */
int
smbfs_quotactl(mp, cmd, uid, arg, p)
	struct mount *mp;
	int cmd;
	uid_t uid;
	caddr_t arg;
	struct proc *p;
{
	SMBVDEBUG("return EOPNOTSUPP\n");
	return EOPNOTSUPP;
}

/*ARGSUSED*/
void
smbfs_init(/*  struct vfsconf *vfsp */void)
{
#ifndef NetBSD
#ifndef SMP
	int name[2];
	int olen, ncpu, plen, error;

	name[0] = CTL_HW;
	name[1] = HW_NCPU;
	error = kernel_sysctl(curproc, name, 2, &ncpu, &olen, NULL, 0, &plen);
	if (error == 0 && ncpu > 1)
		printf("warning: smbfs module compiled without SMP support.");
#endif
#endif

	SMBVDEBUG("\n");
#ifdef SMBFS_USEZONE
	smbfsmount_zone = zinit("SMBFSMOUNT", sizeof(struct smbmount), 0, 0, 1);
#endif
	smbfs_hash_init();
	smbfs_pbuf_freecnt = nswbuf / 2 + 1;
	SMBVDEBUG("done.\n");
/*  	return 0; */
}

/*ARGSUSED*/
#ifndef NetBSD
int
smbfs_uninit(struct vfsconf *vfsp)
{

	smbfs_hash_free();
	SMBVDEBUG("done.\n");
	return 0;
}
#else
void
smbfs_uninit(void)
{
	smbfs_hash_free();
	SMBVDEBUG("done.\n");
}
#endif


/*
 * smbfs_statfs call
 */
int
smbfs_statfs(struct mount *mp, struct statfs *sbp, struct proc *p)
{
	struct smbmount *smp = VFSTOSMBFS(mp);
	struct smbnode *np = smp->sm_root;
	struct smb_share *ssp = smp->sm_share;
	struct smb_cred scred;
	int error = 0;

	SMBVDEBUG("\n");
	if (np == NULL)
		return EINVAL;
	
	sbp->f_iosize = SSTOVC(ssp)->vc_txmax;		/* optimal transfer block size */
	/*  sbp->f_spare2 = 0; */			/* placeholder */
	smb_makescred(&scred, p, p->p_ucred);

	if (SMB_DIALECT(SSTOVC(ssp)) >= SMB_DIALECT_LANMAN2_0)
		error = smbfs_smb_statfs2(ssp, sbp, &scred);
	else
		error = smbfs_smb_statfs(ssp, sbp, &scred);
	if (error)
		return error;
	sbp->f_flags = 0;		/* copy of mount exported flags */
	if (sbp != &mp->mnt_stat) {
		sbp->f_fsid = mp->mnt_stat.f_fsid;	/* file system id */
		sbp->f_owner = mp->mnt_stat.f_owner;	/* user that mounted the filesystem */
		sbp->f_type = 0/*  mp->mnt_vfc->vfc_typenum */;	/* type of filesystem */
		bcopy(mp->mnt_stat.f_mntonname, sbp->f_mntonname, MNAMELEN);
		bcopy(mp->mnt_stat.f_mntfromname, sbp->f_mntfromname, MNAMELEN);
	}
#ifndef NetBSD
	strncpy(sbp->f_fstypename, mp->mnt_vfc->vfc_name, MFSNAMELEN);
#else
	strncpy(sbp->f_fstypename, MOUNT_SMBFS, MFSNAMELEN-1);
#endif
	return 0;
}

/*
 * Flush out the buffer cache
 */
/* ARGSUSED */
int
smbfs_sync(mp, waitfor, cred, p)
	struct mount *mp;
	int waitfor;
	struct ucred *cred;
	struct proc *p;
{
	register struct vnode *vp;
	int error, allerror = 0;
	/*
	 * Force stale buffer cache information to be flushed.
	 */
	SMBVDEBUG("\n");
loop:
	for (vp = mp->mnt_vnodelist.lh_first;
	     vp != NULL;
	     vp = vp->v_mntvnodes.le_next) {
		/*
		 * If the vnode that we are about to sync is no longer
		 * associated with this mount point, start over.
		 */
		if (vp->v_mount != mp)
			goto loop;
#ifndef NetBSD
#ifndef FB_RELENG3
		if (VOP_ISLOCKED(vp, NULL) || TAILQ_EMPTY(&vp->v_dirtyblkhd) ||
#else
		if (VOP_ISLOCKED(vp) || TAILQ_EMPTY(&vp->v_dirtyblkhd) ||
#endif
		    waitfor == MNT_LAZY)
#else
		if (VOP_ISLOCKED(vp) || vp->v_dirtyblkhd.lh_first == NULL)
#endif
			continue;
		if (vget(vp, LK_EXCLUSIVE/*  , p */))
			goto loop;
		error = VOP_FSYNC(vp, cred,
		    waitfor == MNT_WAIT ? FSYNC_WAIT : 0, 0, 0, p);
		if (error)
			allerror = error;
		vput(vp);
	}
	return (allerror);
}

#if __FreeBSD_version < 400009
/*
 * smbfs flat namespace lookup. Unsupported.
 */
/* ARGSUSED */
int smbfs_vget(mp, ino, vpp)
	struct mount *mp;
	ino_t ino;
	struct vnode **vpp;
{
	return (EOPNOTSUPP);
}

/* ARGSUSED */
int smbfs_fhtovp(mp, fhp, /*  nam, */ vpp/*  , exflagsp, credanonp */)
	register struct mount *mp;
	struct fid *fhp;
/*  	struct sockaddr *nam; */
	struct vnode **vpp;
/*  	int *exflagsp; */
/*  	struct ucred **credanonp; */
{
	return (EINVAL);
}

/*
 * Vnode pointer to File handle, should never happen either
 */
/* ARGSUSED */
int
smbfs_vptofh(vp, fhp)
	struct vnode *vp;
	struct fid *fhp;
{
	return (EINVAL);
}

#endif /* __FreeBSD_version < 400009 */
