/*	$NetBSD: osf1_mount.c,v 1.11 1999/04/26 03:10:58 cgd Exp $	*/

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include "fs_nfs.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/syscallargs.h>

#include <compat/osf1/osf1.h>
#include <compat/osf1/osf1_syscallargs.h>
#include <compat/osf1/osf1_util.h>

#include <net/if.h>
#include <netinet/in.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsmount.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/ufsmount.h>

#include <machine/vmparam.h>

#define	OSF1_MNT_WAIT		0x1
#define	OSF1_MNT_NOWAIT		0x2

#define	OSF1_MNT_FORCE		0x1
#define	OSF1_MNT_NOFORCE	0x2

/* acceptable flags for various calls */
#define	OSF1_GETFSSTAT_FLAGS	(OSF1_MNT_WAIT|OSF1_MNT_NOWAIT)
#define	OSF1_MOUNT_FLAGS	0xffffffff			/* XXX */
#define	OSF1_UNMOUNT_FLAGS	(OSF1_MNT_FORCE|OSF1_MNT_NOFORCE)


void bsd2osf_statfs __P((struct statfs *, struct osf1_statfs *));
int osf1_mount_mfs __P((struct proc *, struct osf1_sys_mount_args *,
			struct sys_mount_args *));
int osf1_mount_nfs __P((struct proc *, struct osf1_sys_mount_args *,
			struct sys_mount_args *));



void
bsd2osf_statfs(bsfs, osfs)
	struct statfs *bsfs;
	struct osf1_statfs *osfs;
{

	memset(osfs, 0, sizeof (struct osf1_statfs));
	if (!strncmp(MOUNT_FFS, bsfs->f_fstypename, MFSNAMELEN))
		osfs->f_type = OSF1_MOUNT_UFS;
	else if (!strncmp(MOUNT_NFS, bsfs->f_fstypename, MFSNAMELEN))
		osfs->f_type = OSF1_MOUNT_NFS;
	else if (!strncmp(MOUNT_MFS, bsfs->f_fstypename, MFSNAMELEN))
		osfs->f_type = OSF1_MOUNT_MFS;
	else
		/* uh oh...  XXX = PC, CDFS, PROCFS, etc. */
		osfs->f_type = OSF1_MOUNT_ADDON;
	osfs->f_flags = bsfs->f_flags;		/* XXX translate */
	osfs->f_fsize = bsfs->f_bsize;
	osfs->f_bsize = bsfs->f_iosize;
	osfs->f_blocks = bsfs->f_blocks;
	osfs->f_bfree = bsfs->f_bfree;
	osfs->f_bavail = bsfs->f_bavail;
	osfs->f_files = bsfs->f_files;
	osfs->f_ffree = bsfs->f_ffree;
	memcpy(&osfs->f_fsid, &bsfs->f_fsid,
	    max(sizeof bsfs->f_fsid, sizeof osfs->f_fsid));
	/* osfs->f_spare zeroed above */
	memcpy(osfs->f_mntonname, bsfs->f_mntonname,
	    max(sizeof bsfs->f_mntonname, sizeof osfs->f_mntonname));
	memcpy(osfs->f_mntfromname, bsfs->f_mntfromname,
	    max(sizeof bsfs->f_mntfromname, sizeof osfs->f_mntfromname));
	/* XXX osfs->f_xxx should be filled in... */
}

int
osf1_sys_statfs(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_statfs_args *uap = v;
	struct mount *mp;
	struct statfs *sp;
	struct osf1_statfs osfs;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
	if ((error = namei(&nd)))
		return (error);
	mp = nd.ni_vp->v_mount;
	sp = &mp->mnt_stat;
	vrele(nd.ni_vp);
	if ((error = VFS_STATFS(mp, sp, p)))
		return (error);
	sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
	bsd2osf_statfs(sp, &osfs);
	return copyout(&osfs, SCARG(uap, buf), min(sizeof osfs,
	    SCARG(uap, len)));
}

int
osf1_sys_fstatfs(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_fstatfs_args *uap = v;
	struct file *fp;
	struct mount *mp;
	struct statfs *sp;
	struct osf1_statfs osfs;
	int error;

	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)))
		return (error);
	mp = ((struct vnode *)fp->f_data)->v_mount;
	sp = &mp->mnt_stat;
	if ((error = VFS_STATFS(mp, sp, p)))
		return (error);
	sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
	bsd2osf_statfs(sp, &osfs);
	return copyout(&osfs, SCARG(uap, buf), min(sizeof osfs,
	    SCARG(uap, len)));
}

int
osf1_sys_getfsstat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_getfsstat_args *uap = v;
	struct mount *mp, *nmp;
	struct statfs *sp;
	struct osf1_statfs osfs;
	caddr_t osf_sfsp;
	long count, maxcount, error;

	if (SCARG(uap, flags) & ~OSF1_GETFSSTAT_FLAGS)
		return (EINVAL);

	maxcount = SCARG(uap, bufsize) / sizeof(struct osf1_statfs);
	osf_sfsp = (caddr_t)SCARG(uap, buf);
	for (count = 0, mp = mountlist.cqh_first; mp != (void *)&mountlist;
	    mp = nmp) {
		nmp = mp->mnt_list.cqe_next;
		if (osf_sfsp && count < maxcount) {
			sp = &mp->mnt_stat;
			/*
			 * If OSF1_MNT_NOWAIT is specified, do not refresh the
			 * fsstat cache.  OSF1_MNT_WAIT overrides
			 * OSF1_MNT_NOWAIT.
			 */
			if (((SCARG(uap, flags) & OSF1_MNT_NOWAIT) == 0 ||
			    (SCARG(uap, flags) & OSF1_MNT_WAIT)) &&
			    (error = VFS_STATFS(mp, sp, p)))
				continue;
			sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
			bsd2osf_statfs(sp, &osfs);
			if ((error = copyout(&osfs, osf_sfsp,
			    sizeof (struct osf1_statfs))))
				return (error);
			osf_sfsp += sizeof (struct osf1_statfs);
		}
		count++;
	}
	if (osf_sfsp && count > maxcount)
		*retval = maxcount;
	else
		*retval = count;
	return (0);
}

int
osf1_sys_unmount(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_unmount_args *uap = v;
	struct sys_unmount_args a;

	SCARG(&a, path) = SCARG(uap, path);

	if (SCARG(uap, flags) & ~OSF1_UNMOUNT_FLAGS)
		return (EINVAL);
	SCARG(&a, flags) = 0;
	if ((SCARG(uap, flags) & OSF1_MNT_FORCE) &&
	    (SCARG(uap, flags) & OSF1_MNT_NOFORCE) == 0)
		SCARG(&a, flags) |= MNT_FORCE;

	return sys_unmount(p, &a, retval);
}

int
osf1_sys_mount(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct osf1_sys_mount_args *uap = v;
	struct sys_mount_args a;
	int error;

	SCARG(&a, path) = SCARG(uap, path);

	if (SCARG(uap, flags) & ~OSF1_MOUNT_FLAGS)
		return (EINVAL);
	SCARG(&a, flags) = SCARG(uap, flags);		/* XXX - xlate */

	switch (SCARG(uap, type)) {
	case OSF1_MOUNT_NFS:
		if ((error = osf1_mount_nfs(p, uap, &a)))
			return error;
		break;

	case OSF1_MOUNT_MFS:
		if ((error = osf1_mount_mfs(p, uap, &a)))
			return error;
		break;

	default:
		return (EINVAL);
	}

	return sys_mount(p, &a, retval);
}

int
osf1_mount_mfs(p, osf_argp, bsd_argp)
	struct proc *p;
	struct osf1_sys_mount_args *osf_argp;
	struct sys_mount_args *bsd_argp;
{
	struct osf1_mfs_args osf_ma;
	struct mfs_args bsd_ma;
	caddr_t sg = stackgap_init(p->p_emul);
	int error, len;
	static const char mfs_name[] = MOUNT_MFS;

	if ((error = copyin(SCARG(osf_argp, data), &osf_ma, sizeof osf_ma)))
		return error;

	memset(&bsd_ma, 0, sizeof bsd_ma);
	bsd_ma.fspec = osf_ma.name;
	/* XXX export args */
	bsd_ma.base = osf_ma.base;
	bsd_ma.size = osf_ma.size;

	SCARG(bsd_argp, data) = stackgap_alloc(&sg, sizeof bsd_ma);
	if ((error = copyout(&bsd_ma, SCARG(bsd_argp, data), sizeof bsd_ma)))
		return error;

	len = strlen(mfs_name) + 1;
	SCARG(bsd_argp, type) = stackgap_alloc(&sg, len);
	if ((error = copyout(mfs_name, (void *)SCARG(bsd_argp, type), len)))
		return error;

	return 0;
}

const struct emul_flags_xtab osf1_nfs_mount_flags_xtab[] = {
    {	OSF1_NFSMNT_SOFT,	OSF1_NFSMNT_SOFT,	NFSMNT_SOFT,	},
    {	OSF1_NFSMNT_WSIZE,	OSF1_NFSMNT_WSIZE,	NFSMNT_WSIZE,	},
    {	OSF1_NFSMNT_RSIZE,	OSF1_NFSMNT_RSIZE,	NFSMNT_RSIZE,	},
    {	OSF1_NFSMNT_TIMEO,	OSF1_NFSMNT_TIMEO,	NFSMNT_TIMEO,	},
    {	OSF1_NFSMNT_RETRANS,	OSF1_NFSMNT_RETRANS,	NFSMNT_RETRANS,	},
#if 0 /* no equivalent; needs special handling, see below */
    {	OSF1_NFSMNT_HOSTNAME,	OSF1_NFSMNT_HOSTNAME,	???,		},
#endif
    {	OSF1_NFSMNT_INT,	OSF1_NFSMNT_INT,	NFSMNT_INT,	},
    {	OSF1_NFSMNT_NOCONN,	OSF1_NFSMNT_NOCONN,	NFSMNT_NOCONN,	},
#if 0 /* no equivalents */
    {	OSF1_NFSMNT_NOAC,	OSF1_NFSMNT_NOAC,	???,		},
    {	OSF1_NFSMNT_ACREGMIN,	OSF1_NFSMNT_ACREGMIN,	???,		},
    {	OSF1_NFSMNT_ACREGMAX,	OSF1_NFSMNT_ACREGMAX,	???,		},
    {	OSF1_NFSMNT_ACDIRMIN,	OSF1_NFSMNT_ACDIRMIN,	???,		},
    {	OSF1_NFSMNT_ACDIRMAX,	OSF1_NFSMNT_ACDIRMAX,	???,		},
    {	OSF1_NFSMNT_NOCTO,	OSF1_NFSMNT_NOCTO,	???,		},
    {	OSF1_NFSMNT_POSIX,	OSF1_NFSMNT_POSIX,	???,		},
    {	OSF1_NFSMNT_AUTO,	OSF1_NFSMNT_AUTO,	???,		},
    {	OSF1_NFSMNT_SEC,	OSF1_NFSMNT_SEC,	???,		},
    {	OSF1_NFSMNT_TCP,	OSF1_NFSMNT_TCP,	???,		},
    {	OSF1_NFSMNT_PROPLIST,	OSF1_NFSMNT_PROPLIST,	???,		},
#endif
    {	0								}
};

int
osf1_mount_nfs(p, osf_argp, bsd_argp)
	struct proc *p;
	struct osf1_sys_mount_args *osf_argp;
	struct sys_mount_args *bsd_argp;
{
	struct osf1_nfs_args osf_na;
	struct nfs_args bsd_na;
	caddr_t sg = stackgap_init(p->p_emul);
	int error, len;
	static const char nfs_name[] = MOUNT_NFS;
	unsigned long leftovers;

	if ((error = copyin(SCARG(osf_argp, data), &osf_na, sizeof osf_na)))
		return error;

	memset(&bsd_na, 0, sizeof bsd_na);
	bsd_na.addr = (struct sockaddr *)osf_na.addr;
	bsd_na.addrlen = sizeof (struct sockaddr_in);
	bsd_na.fh = osf_na.fh;

        /* translate flags */
        bsd_na.flags = emul_flags_translate(osf1_nfs_mount_flags_xtab,
            osf_na.flags, &leftovers);
	if (leftovers & OSF1_NFSMNT_HOSTNAME) {
		leftovers &= ~OSF1_NFSMNT_HOSTNAME;
		bsd_na.hostname = osf_na.hostname;
	} else {
		/* XXX FILL IN HOST NAME WITH IPADDR? */
	}
	if (leftovers & OSF1_NFSMNT_TCP) {
		leftovers &= ~OSF1_NFSMNT_TCP;
		bsd_na.sotype = SOCK_DGRAM; 
		bsd_na.proto = 0; 
	} else {
		bsd_na.sotype = SOCK_STREAM; 
		bsd_na.proto = 0; 
	}
        if (leftovers != 0)
                return (EINVAL);

	/* copy structure elements based on flags */
	if (bsd_na.flags & NFSMNT_WSIZE)
		bsd_na.wsize = osf_na.wsize;
	if (bsd_na.flags & NFSMNT_RSIZE)
		bsd_na.rsize = osf_na.rsize;
	if (bsd_na.flags & NFSMNT_TIMEO)
		bsd_na.timeo = osf_na.timeo;
	if (bsd_na.flags & NFSMNT_RETRANS)
		bsd_na.retrans = osf_na.retrans;

	SCARG(bsd_argp, data) = stackgap_alloc(&sg, sizeof bsd_na);
	if ((error = copyout(&bsd_na, SCARG(bsd_argp, data), sizeof bsd_na)))
		return error;

	len = strlen(nfs_name) + 1;
	SCARG(bsd_argp, type) = stackgap_alloc(&sg, len);
	if ((error = copyout(MOUNT_NFS, (void *)SCARG(bsd_argp, type), len)))
		return error;

	return 0;
}
