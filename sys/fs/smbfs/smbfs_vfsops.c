/*	$NetBSD: smbfs_vfsops.c,v 1.10 2003/02/23 18:47:10 jdolecek Exp $	*/

/*
 * Copyright (c) 2000-2001, Boris Popov
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
 *
 * FreeBSD: src/sys/fs/smbfs/smbfs_vfsops.c,v 1.5 2001/12/13 13:08:34 sheldonh Exp
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/malloc.h>


#include <netsmb/smb.h>
#include <netsmb/smb_conn.h>
#include <netsmb/smb_subr.h>
#include <netsmb/smb_dev.h>

#include <fs/smbfs/smbfs.h>
#include <fs/smbfs/smbfs_node.h>
#include <fs/smbfs/smbfs_subr.h>

#ifndef __NetBSD__
SYSCTL_NODE(_vfs, OID_AUTO, smbfs, CTLFLAG_RW, 0, "SMB/CIFS file system");
SYSCTL_INT(_vfs_smbfs, OID_AUTO, version, CTLFLAG_RD, &smbfs_version, 0, "");
#endif

static MALLOC_DEFINE(M_SMBFSHASH, "SMBFS hash", "SMBFS hash table");

int smbfs_mount(struct mount *, const char *, void *,
		struct nameidata *, struct proc *);
int smbfs_quotactl(struct mount *, int, uid_t, caddr_t, struct proc *);
int smbfs_root(struct mount *, struct vnode **);
int smbfs_start(struct mount *, int, struct proc *);
int smbfs_statfs(struct mount *, struct statfs *, struct proc *);
int smbfs_sync(struct mount *, int, struct ucred *, struct proc *);
int smbfs_unmount(struct mount *, int, struct proc *);
void smbfs_init(void);
void smbfs_reinit(void);
void smbfs_uninit(void);

int smbfs_vget(struct mount *mp, ino_t ino, struct vnode **vpp);
int smbfs_fhtovp(struct mount *, struct fid *, struct vnode **);
int smbfs_vptofh(struct vnode *, struct fid *);

#ifndef __NetBSD__
int smbfs_pbuf_freecnt = -1;	/* start out unlimited */
#endif

extern struct vnodeopv_desc smbfs_vnodeop_opv_desc;

const struct vnodeopv_desc *smbfs_vnodeopv_descs[] = {
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
	smbfs_reinit,
	smbfs_uninit,
	(int (*) (int *, u_int, void *, size_t *, void *, size_t, 
		  struct proc *)) eopnotsupp, /* sysctl */
	(int (*) (void)) eopnotsupp, /* mountroot */
	(int (*) (struct mount *, struct mbuf *, int *, 
		  struct ucred **)) eopnotsupp, /* checkexp */
	smbfs_vnodeopv_descs,
};

int
smbfs_mount(struct mount *mp, const char *path, void *data,
	struct nameidata *ndp, struct proc *p)
{
	struct smbfs_args args; 	  /* will hold data from mount request */
	struct smbmount *smp = NULL;
	struct smb_vc *vcp;
	struct smb_share *ssp = NULL;
	struct vnode *vp;
	struct smb_cred scred;
	size_t size;
	int error;

	if (mp->mnt_flag & MNT_GETARGS) {
		smp = VFSTOSMBFS(mp);
		if (smp == NULL)
			return EIO;
		return copyout(&smp->sm_args, data, sizeof(smp->sm_args));
	}

	if (mp->mnt_flag & MNT_UPDATE)
		return EOPNOTSUPP;

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
	if (error)
		return error;
	smb_share_unlock(ssp, 0);
	vcp = SSTOVC(ssp);
	mp->mnt_stat.f_iosize = vcp->vc_txmax;

        MALLOC(smp, struct smbmount *, sizeof(*smp), M_SMBFSDATA, M_WAITOK);
	memset(smp, 0, sizeof(*smp));
        mp->mnt_data = smp;

	smp->sm_hash = hashinit(desiredvnodes, HASH_LIST, 
				M_SMBFSHASH, M_WAITOK, &smp->sm_hashlen);

	lockinit(&smp->sm_hashlock, PVFS, "smbfsh", 0, 0);
	smp->sm_share = ssp;
	smp->sm_root = NULL;
        smp->sm_args = args;
	smp->sm_caseopt = args.caseopt;
	smp->sm_args.file_mode = (smp->sm_args.file_mode &
			    (S_IRWXU|S_IRWXG|S_IRWXO)) | S_IFREG;
	smp->sm_args.dir_mode  = (smp->sm_args.dir_mode &
			    (S_IRWXU|S_IRWXG|S_IRWXO)) | S_IFDIR;

	error = copyinstr(path, mp->mnt_stat.f_mntonname, MNAMELEN - 1, &size);
	if (error)
		goto bad;
	memset(mp->mnt_stat.f_mntonname + size, 0, MNAMELEN - size);

	memset(mp->mnt_stat.f_mntfromname, 0, MNAMELEN);
	snprintf(mp->mnt_stat.f_mntfromname, MNAMELEN,
		"//%s@%s/%s",
		vcp->vc_username,
		vcp->vc_srvname,
		ssp->ss_name);

	/* protect against invalid mount points */
	smp->sm_args.mount_point[sizeof(smp->sm_args.mount_point) - 1] = '\0';
	vfs_getnewfsid(mp);
	error = smbfs_root(mp, &vp);
	if (error)
		goto bad;
	VOP_UNLOCK(vp, 0);
	SMBVDEBUG("root.v_usecount = %d\n", vp->v_usecount);

#ifdef DIAGNOSTICS
	SMBERROR("mp=%p\n", mp);
#endif
	return (0);

bad:
        if (smp) {
		if (smp->sm_hash)
			free(smp->sm_hash, M_SMBFSHASH);
#ifdef __NetBSD__
		lockmgr(&smp->sm_hashlock, LK_DRAIN, NULL);
#else
		lockdestroy(&smp->sm_hashlock);
#endif
		free(smp, M_SMBFSDATA);
	}
	if (ssp)
		smb_share_put(ssp, &scred);
        return error;
}

/* Unmount the filesystem described by mp. */
int
smbfs_unmount(struct mount *mp, int mntflags, struct proc *p)
{
	struct smbmount *smp = VFSTOSMBFS(mp);
	struct smb_cred scred;
	int error, flags;

	SMBVDEBUG("smbfs_unmount: flags=%04x\n", mntflags);
	flags = 0;
	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;
#ifdef QUOTA
#endif
	/* Drop the extra reference to root vnode. */
	vrele(SMBTOV(smp->sm_root));

	/* Flush all vnodes. */
	if ((error = vflush(mp, NULLVP, flags)) != 0) {
		vref(SMBTOV(smp->sm_root));
		return error;
	}

	smb_makescred(&scred, p, p->p_ucred);
	smb_share_put(smp->sm_share, &scred);
	mp->mnt_data = NULL;

	if (smp->sm_hash)
		free(smp->sm_hash, M_SMBFSHASH);
#ifdef __NetBSD__
	lockmgr(&smp->sm_hashlock, LK_DRAIN, NULL);
#else
	lockdestroy(&smp->sm_hashlock);
#endif
	free(smp, M_SMBFSDATA);
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

	if (smp->sm_root) {
		*vpp = SMBTOV(smp->sm_root);
		return vget(*vpp, LK_EXCLUSIVE | LK_RETRY);
	}
	smb_makescred(&scred, p, cred);
	error = smbfs_smb_lookup(NULL, NULL, 0, &fattr, &scred);
	if (error)
		return error;
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

void
smbfs_init(void)
{

#ifndef __NetBSD__
	smbfs_pbuf_freecnt = nswbuf / 2 + 1;
#endif
	SMBVDEBUG("done.\n");
}

void
smbfs_reinit(void)
{

	SMBVDEBUG("reinit.\n");
}

void
smbfs_uninit(void)
{

	SMBVDEBUG("uninit.\n");
}

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

	if (np == NULL)
		return EINVAL;
	
	sbp->f_iosize = SSTOVC(ssp)->vc_txmax;		/* optimal transfer block size */
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
		sbp->f_type = 0;
		bcopy(mp->mnt_stat.f_mntonname, sbp->f_mntonname, MNAMELEN);
		bcopy(mp->mnt_stat.f_mntfromname, sbp->f_mntfromname, MNAMELEN);
	}
	strncpy(sbp->f_fstypename, mp->mnt_op->vfs_name, MFSNAMELEN);
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
	struct vnode *vp;
	int error, allerror = 0;
	/*
	 * Force stale buffer cache information to be flushed.
	 */
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
		if (waitfor == MNT_LAZY || VOP_ISLOCKED(vp) || 
		    (LIST_EMPTY(&vp->v_dirtyblkhd) &&
		     vp->v_uobj.uo_npages == 0))
			continue;
		if (vget(vp, LK_EXCLUSIVE))
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
int smbfs_fhtovp(mp, fhp, vpp)
	struct mount *mp;
	struct fid *fhp;
	struct vnode **vpp;
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
