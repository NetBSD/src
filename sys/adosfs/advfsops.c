/*
 * Copyright (c) 1994 Christian E. Hopps
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
 *      This product includes software developed by Christian E. Hopps.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: advfsops.c,v 1.1 1994/05/11 18:49:15 chopps Exp $
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/malloc.h>
#include <sys/disklabel.h>
#include <miscfs/specfs/specdev.h> /* XXX */
#include <sys/fcntl.h>
#include <sys/namei.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#include <sys/buf.h>
#include <adosfs/adosfs.h>

int
adosfs_mount(mp, path, data, ndp, p)
	struct mount *mp;
	char *path;
	caddr_t data;
	struct nameidata *ndp;
	struct proc *p;
{
	struct adosfs_args args;
	struct vnode *bdvp;
	int error;

#ifdef ADOSFS_DIAGNOSTIC
	printf("ad_mount(%x, %x, %x, %x, %x)\n", mp, path, data, ndp, p);
#endif
	/*
	 * normally either updatefs or grab a blk vnode from namei to mount
	 */

	if (error = copyin(data, (caddr_t)&args, sizeof(struct adosfs_args)))
		return(error);
	
	if (mp->mnt_flag & MNT_UPDATE)
		return(EOPNOTSUPP);

	/* 
	 * lookup blkdev name and validate.
	 */
	ndp->ni_nameiop = LOOKUP | FOLLOW;
	ndp->ni_segflg = UIO_USERSPACE;
	ndp->ni_dirp = args.fspec;
	if (error = namei(ndp, p)) {
#ifdef ADOSFS_DIAGNOSTIC
		printf("adosfs_mount: namei() failed\n");
#endif
		return(error);
	}
	bdvp = ndp->ni_vp;
	if (bdvp->v_type != VBLK) {
		vrele(bdvp);
		return(ENOTBLK);
	}
	if (major(bdvp->v_rdev) >= nblkdev) {
		vrele(bdvp);
		return(ENXIO);
	}
	if (error = mountadosfs(mp, path, bdvp, &args, p))
		vrele(bdvp);
	return(error);
}

int
mountadosfs(mp, path, bdvp, args, p)
	struct mount *mp;
	char *path;
	struct vnode *bdvp;
	struct adosfs_args *args;
	struct proc *p;
{
	extern struct vnodeops adosfs_vnodeops;
	struct disklabel dl;
	struct partition *parp;
	struct amount *amp;
	struct statfs *sfp;	
	struct anode *rap;
	int error, nl, part, i;

#ifdef DISKPART
	part = DISKPART(bdvp->v_rdev);
#else
	part = minor(bdvp->v_rdev) % MAXPARTITIONS;
#error	just_for_now
#endif
	/*
	 * anything mounted on blkdev?
	 */
	if (error = mountedon(bdvp)) {
#ifdef ADOSFS_DIAGNOSTIC
		printf("mountadosfs: mountedon() failed\n");
#endif
		return(error);
	}
	/*
	 * XXX device currently in use? fail unless it is root XXX
	 */
#if yes_do_this
	if (vcount(bdvp) > 1 && bdvp != rootvp)
		return(EBUSY);
#endif	
	vinvalbuf(bdvp, 1);

	/* 
	 * opend blkdev and read root block
	 */
	if (error = VOP_OPEN(bdvp, FREAD, NOCRED, p)) {
#ifdef ADOSFS_DIAGNOSTIC
		printf("mountadosfs: VOP_OPEN() failed\n");
#endif
		return(error);
	}

	amp = malloc(sizeof(struct amount), M_ADOSFSMNT, M_WAITOK);
	
	if (error = VOP_IOCTL(bdvp,DIOCGDINFO,(caddr_t)&dl, FREAD, NOCRED, p)) {
#ifdef ADOSFS_DIAGNOSTIC
		printf("mountadosfs: VOP_IOCTL() failed\n");
#endif
		goto fail;
	}

	if (part >= dl.d_npartitions) {
#ifdef DIAGNOSTIC
		printf("adosfs_mount: %n with only %d partitions on drive\n",
		    part, dl.d_npartitions);
#endif
		error = EINVAL;		/* XXX correct? */
		goto fail;
	}
	if (dl.d_partitions[part].p_fstype != FS_ADOS) {
#ifdef DIAGNOSTIC
		printf("adosfs_mount: not an ados partition\n");
#endif
		error = EINVAL;
		goto fail;
	}

	parp = &dl.d_partitions[part];
	amp->mp = mp;
	amp->startb = parp->p_offset;
	amp->endb = parp->p_offset + parp->p_size;
	amp->bsize = dl.d_secsize;
	amp->nwords = amp->bsize >> 2;
	amp->devvp = bdvp;
	amp->rootb = (parp->p_size - 1 + 2) >> 1;
	amp->uid = args->uid;	/* XXX check? */
	amp->gid = args->gid;	/* XXX check? */
	amp->mask = args->mask;
	/*
	 * copy in stat information
	 */
	sfp = &mp->mnt_stat;
	error = copyinstr(path, sfp->f_mntonname,sizeof(sfp->f_mntonname),&nl);
	if (error) {
#ifdef ADOSFS_DIAGNOSTIC
		printf("mountadosfs: copyinstr() failed\n");
#endif
		goto fail;
	}
	bzero(&sfp->f_mntonname[nl], sizeof(sfp->f_mntonname) - nl);
	bzero(sfp->f_mntfromname, sizeof(sfp->f_mntfromname));
	bcopy("adosfs", sfp->f_mntfromname, strlen("adosfs"));
	
	bdvp->v_specflags |= SI_MOUNTEDON;
	mp->mnt_data = (qaddr_t)amp;
	mp->mnt_flag |= MNT_LOCAL | MNT_RDONLY;
        mp->mnt_stat.f_fsid.val[0] = (long)bdvp->v_rdev;
        mp->mnt_stat.f_fsid.val[1] = makefstype(MOUNT_ADOSFS);

	/*
	 * init anode table.
	 */
	for (i = 0; i < ANODEHASHSZ; i++) 
		LIST_INIT(&amp->anodetab[i]);

	/*
	 * get the root anode, if not a valid fs this will fail.
	 */
	if (error = aget(mp, amp->rootb, &rap)) {
#ifdef ADOSFS_DIAGNOSTIC
		printf("mountadosfs: aget() failed\n");
#endif
		goto fail;
	}
	amp->rootvp = ATOV(rap);
	AUNLOCK(rap);
	(void)adosfs_statfs(mp, &mp->mnt_stat, p);

	return(0);
fail:
	VOP_CLOSE(bdvp, FREAD, NOCRED, p);
	free(amp, M_ADOSFSMNT);
	return(error);
}

int
adosfs_start(mp, flags, p)
	struct mount *mp;
	int flags;
	struct proc *p;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("adstart(%x, %x, %x)\n", mp, flags, p);
#endif
	return(0);
}

int
adosfs_unmount(mp, flags, p)
	struct mount *mp;
	int flags;
	struct proc *p;
{
	extern int doforce;
	struct amount *amp;
	struct vnode *bdvp, *rvp;
	int error;

#ifdef ADOSFS_DIAGNOSTIC
	printf("adumount(%x, %x, %x)\n", mp, flags, p);
#endif
	amp = VFSTOADOSFS(mp);
	rvp = amp->rootvp;
	bdvp = amp->devvp;

	if (flags & MNT_FORCE) {
		if (doforce == 0)
			return(EINVAL);	/*XXX*/
		flags |= FORCECLOSE;
	}

	/*
	 * clean out cached stuff 
	 */
	mntflushbuf(mp, 0);
	if (mntinvalbuf(mp))
		return(EBUSY);
	if (rvp->v_usecount > 1)
		return(EBUSY);
	error = vflush(mp, rvp, flags);
	if (error)
		return(error);
	/* 
	 * release block device we are mounted on.
	 */
	bdvp->v_specflags &= ~SI_MOUNTEDON;
	VOP_CLOSE(bdvp, FREAD, NOCRED, p);
	vrele(amp->devvp);
	free(amp, M_ADOSFSMNT);
	mp->mnt_data = 0;

	return(0);
}

int
adosfs_root(mp, vpp)
	struct mount *mp;
	struct vnode **vpp;
{
	struct vnode *rvp;

#ifdef ADOSFS_DIAGNOSTIC
	printf("adroot(%x, %x)\n", mp, vpp);
#endif
	rvp = VFSTOADOSFS(mp)->rootvp;
	VREF(rvp);
	VOP_LOCK(rvp);
	*vpp = rvp;
	return(0);
}

int
adosfs_quotactl(mp, cmds, uid, arg, p)
	struct mount *mp;
	int cmds;
	uid_t uid;
	caddr_t arg;
	struct proc *p;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("adquatactl(%x, %x, %x, %x, %x)\n", mp, cmds, uid, arg, p);
#endif
	return(EOPNOTSUPP);
}

int
adosfs_statfs(mp, sbp, p)
	struct mount *mp;
	struct statfs *sbp;
	struct proc *p;
{
	struct amount *amp;
#ifdef ADOSFS_DIAGNOSTIC
	printf("adstatfs(%x)\n", mp);
#endif
	amp = VFSTOADOSFS(mp);
	sbp->f_type = 0;
	sbp->f_bsize = amp->bsize;
	sbp->f_iosize = amp->bsize;
	sbp->f_blocks = 2;		/* XXX */
	sbp->f_bfree = 0;		/* none */
	sbp->f_bavail = 0;		/* none */
	sbp->f_files = 0;		/* who knows */
	sbp->f_ffree = 0;		/* " " */
	if (sbp != &mp->mnt_stat) {
		bcopy(&mp->mnt_stat.f_mntonname, sbp->f_mntonname, 
		    sizeof(sbp->f_mntonname));
		bcopy(&mp->mnt_stat.f_mntfromname, sbp->f_mntfromname, 
		    sizeof(sbp->f_mntfromname));
	}
	strncpy(sbp->f_fstypename, mp->mnt_op->vfs_name, MFSNAMELEN);
	sbp->f_fstypename[MFSNAMELEN] = 0;
	return(0);
}

int
adosfs_sync(mp, waitfor)
	struct mount *mp;
	int waitfor;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("ad_sync(%x, %x)\n", mp, waitfor);
#endif
	return(0);
}

int
adosfs_fhtovp(mp, fhp, vpp)
	struct mount *mp;
	struct fid *fhp;
	struct vnode **vpp;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("adfhtovp(%x, %x, %x)\n", mp, fhp, vpp);
#endif
	return(0);
}

int
adosfs_vptofh(vp, fhp)
	struct vnode *vp;
	struct fid *fhp;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("advptofh(%x, %x)\n", vp, fhp);
#endif
	return(0);
}

int
adosfs_init()
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("adinit()\n");
#endif
	return(0);
}

/*
 * vfs generic function call table
 */
struct vfsops adosfs_vfsops = {
	MOUNT_ADOSFS,
	adosfs_mount,
	adosfs_start,
	adosfs_unmount,
	adosfs_root,
	adosfs_quotactl,                
	adosfs_statfs,                  
	adosfs_sync,                    
	adosfs_fhtovp,                  
	adosfs_vptofh,                  
	adosfs_init,                    
};           

