/*	$NetBSD: nfs_vfsops.c,v 1.91.2.1 2000/12/14 23:37:22 he Exp $	*/

/*
 * Copyright (c) 1989, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Macklem at The University of Guelph.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *
 *	@(#)nfs_vfsops.c	8.12 (Berkeley) 5/20/95
 */

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/device.h>
#include <sys/vnode.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <vm/vm.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfsnode.h>
#include <nfs/nfs.h>
#include <nfs/nfsmount.h>
#include <nfs/xdr_subs.h>
#include <nfs/nfsm_subs.h>
#include <nfs/nfsdiskless.h>
#include <nfs/nqnfs.h>
#include <nfs/nfs_var.h>

extern struct nfsstats nfsstats;
extern int nfs_ticks;

int nfs_sysctl __P((int *, u_int, void *, size_t *, void *, size_t,
		      struct proc *));

/*
 * nfs vfs operations.
 */

extern struct vnodeopv_desc nfsv2_vnodeop_opv_desc;
extern struct vnodeopv_desc spec_nfsv2nodeop_opv_desc;
extern struct vnodeopv_desc fifo_nfsv2nodeop_opv_desc;

struct vnodeopv_desc *nfs_vnodeopv_descs[] = {
	&nfsv2_vnodeop_opv_desc,
	&spec_nfsv2nodeop_opv_desc,
	&fifo_nfsv2nodeop_opv_desc,
	NULL,
};

struct vfsops nfs_vfsops = {
	MOUNT_NFS,
	nfs_mount,
	nfs_start,
	nfs_unmount,
	nfs_root,
	nfs_quotactl,
	nfs_statfs,
	nfs_sync,
	nfs_vget,
	nfs_fhtovp,
	nfs_vptofh,
	nfs_vfs_init,
	nfs_vfs_done,
	nfs_sysctl,
	nfs_mountroot,
	nfs_checkexp,
	nfs_vnodeopv_descs,
};

extern u_int32_t nfs_procids[NFS_NPROCS];
extern u_int32_t nfs_prog, nfs_vers;

static int nfs_mount_diskless __P((struct nfs_dlmount *, const char *,
    struct mount **, struct vnode **, struct proc *));

#define TRUE	1
#define	FALSE	0

/*
 * nfs statfs call
 */
int
nfs_statfs(mp, sbp, p)
	struct mount *mp;
	struct statfs *sbp;
	struct proc *p;
{
	struct vnode *vp;
	struct nfs_statfs *sfp;
	caddr_t cp;
	u_int32_t *tl;
	int32_t t1, t2;
	caddr_t bpos, dpos, cp2;
	struct nfsmount *nmp = VFSTONFS(mp);
	int error = 0, v3 = (nmp->nm_flag & NFSMNT_NFSV3), retattr;
	struct mbuf *mreq, *mrep = NULL, *md, *mb, *mb2;
	struct ucred *cred;
	struct nfsnode *np;
	u_quad_t tquad;

#ifndef nolint
	sfp = (struct nfs_statfs *)0;
#endif
	error = nfs_nget(mp, (nfsfh_t *)nmp->nm_fh, nmp->nm_fhsize, &np);
	if (error)
		return (error);
	vp = NFSTOV(np);
	cred = crget();
	cred->cr_ngroups = 0;
	if (v3 && (nmp->nm_iflag & NFSMNT_GOTFSINFO) == 0)
		(void)nfs_fsinfo(nmp, vp, cred, p);
	nfsstats.rpccnt[NFSPROC_FSSTAT]++;
	nfsm_reqhead(vp, NFSPROC_FSSTAT, NFSX_FH(v3));
	nfsm_fhtom(vp, v3);
	nfsm_request(vp, NFSPROC_FSSTAT, p, cred);
	if (v3)
		nfsm_postop_attr(vp, retattr);
	if (error) {
		if (mrep != NULL)
			m_free(mrep);
		goto nfsmout;
	}
	nfsm_dissect(sfp, struct nfs_statfs *, NFSX_STATFS(v3));
#ifdef COMPAT_09
	sbp->f_type = 2;
#else
	sbp->f_type = 0;
#endif
	sbp->f_flags = nmp->nm_flag;
	sbp->f_iosize = min(nmp->nm_rsize, nmp->nm_wsize);
	if (v3) {
		sbp->f_bsize = NFS_FABLKSIZE;
		tquad = fxdr_hyper(&sfp->sf_tbytes);
		sbp->f_blocks = (long)((quad_t)tquad / (quad_t)NFS_FABLKSIZE);
		tquad = fxdr_hyper(&sfp->sf_fbytes);
		sbp->f_bfree = (long)((quad_t)tquad / (quad_t)NFS_FABLKSIZE);
		tquad = fxdr_hyper(&sfp->sf_abytes);
		sbp->f_bavail = (long)((quad_t)tquad / (quad_t)NFS_FABLKSIZE);
		tquad = fxdr_hyper(&sfp->sf_tfiles);
		sbp->f_files = (long)tquad;
		tquad = fxdr_hyper(&sfp->sf_ffiles);
		sbp->f_ffree = (long)tquad;
	} else {
		sbp->f_bsize = fxdr_unsigned(int32_t, sfp->sf_bsize);
		sbp->f_blocks = fxdr_unsigned(int32_t, sfp->sf_blocks);
		sbp->f_bfree = fxdr_unsigned(int32_t, sfp->sf_bfree);
		sbp->f_bavail = fxdr_unsigned(int32_t, sfp->sf_bavail);
		sbp->f_files = 0;
		sbp->f_ffree = 0;
	}
	if (sbp != &mp->mnt_stat) {
		memcpy(sbp->f_mntonname, mp->mnt_stat.f_mntonname, MNAMELEN);
		memcpy(sbp->f_mntfromname, mp->mnt_stat.f_mntfromname, MNAMELEN);
	}
	strncpy(&sbp->f_fstypename[0], mp->mnt_op->vfs_name, MFSNAMELEN);
	nfsm_reqdone;
	vrele(vp);
	crfree(cred);
	return (error);
}

/*
 * nfs version 3 fsinfo rpc call
 */
int
nfs_fsinfo(nmp, vp, cred, p)
	struct nfsmount *nmp;
	struct vnode *vp;
	struct ucred *cred;
	struct proc *p;
{
	struct nfsv3_fsinfo *fsp;
	caddr_t cp;
	int32_t t1, t2;
	u_int32_t *tl, pref, max;
	caddr_t bpos, dpos, cp2;
	int error = 0, retattr;
	struct mbuf *mreq, *mrep, *md, *mb, *mb2;
	u_int64_t maxfsize;

	nfsstats.rpccnt[NFSPROC_FSINFO]++;
	nfsm_reqhead(vp, NFSPROC_FSINFO, NFSX_FH(1));
	nfsm_fhtom(vp, 1);
	nfsm_request(vp, NFSPROC_FSINFO, p, cred);
	nfsm_postop_attr(vp, retattr);
	if (!error) {
		nfsm_dissect(fsp, struct nfsv3_fsinfo *, NFSX_V3FSINFO);
		pref = fxdr_unsigned(u_int32_t, fsp->fs_wtpref);
		if (pref < nmp->nm_wsize && pref >= NFS_FABLKSIZE)
			nmp->nm_wsize = (pref + NFS_FABLKSIZE - 1) &
				~(NFS_FABLKSIZE - 1);
		max = fxdr_unsigned(u_int32_t, fsp->fs_wtmax);
		if (max < nmp->nm_wsize && max > 0) {
			nmp->nm_wsize = max & ~(NFS_FABLKSIZE - 1);
			if (nmp->nm_wsize == 0)
				nmp->nm_wsize = max;
		}
		pref = fxdr_unsigned(u_int32_t, fsp->fs_rtpref);
		if (pref < nmp->nm_rsize && pref >= NFS_FABLKSIZE)
			nmp->nm_rsize = (pref + NFS_FABLKSIZE - 1) &
				~(NFS_FABLKSIZE - 1);
		max = fxdr_unsigned(u_int32_t, fsp->fs_rtmax);
		if (max < nmp->nm_rsize && max > 0) {
			nmp->nm_rsize = max & ~(NFS_FABLKSIZE - 1);
			if (nmp->nm_rsize == 0)
				nmp->nm_rsize = max;
		}
		pref = fxdr_unsigned(u_int32_t, fsp->fs_dtpref);
		if (pref < nmp->nm_readdirsize && pref >= NFS_DIRFRAGSIZ)
			nmp->nm_readdirsize = (pref + NFS_DIRFRAGSIZ - 1) &
				~(NFS_DIRFRAGSIZ - 1);
		if (max < nmp->nm_readdirsize && max > 0) {
			nmp->nm_readdirsize = max & ~(NFS_DIRFRAGSIZ - 1);
			if (nmp->nm_readdirsize == 0)
				nmp->nm_readdirsize = max;
		}
		/* XXX */
		nmp->nm_maxfilesize = (u_int64_t)0x80000000 * DEV_BSIZE - 1;
		maxfsize = fxdr_hyper(&fsp->fs_maxfilesize);
		if (maxfsize > 0 && maxfsize < nmp->nm_maxfilesize)
			nmp->nm_maxfilesize = maxfsize;
		nmp->nm_iflag |= NFSMNT_GOTFSINFO;
	}
	nfsm_reqdone;
	return (error);
}

/*
 * Mount a remote root fs via. NFS.  It goes like this:
 * - Call nfs_boot_init() to fill in the nfs_diskless struct
 * - build the rootfs mount point and call mountnfs() to do the rest.
 */
int
nfs_mountroot()
{
	struct nfs_diskless *nd;
	struct vattr attr;
	struct mount *mp;
	struct vnode *vp;
	struct proc *procp;
	long n;
	int error;

	procp = curproc; /* XXX */

	if (root_device->dv_class != DV_IFNET)
		return (ENODEV);

	/*
	 * XXX time must be non-zero when we init the interface or else
	 * the arp code will wedge.  [Fixed now in if_ether.c]
	 * However, the NFS attribute cache gives false "hits" when
	 * time.tv_sec < NFS_ATTRTIMEO(np) so keep this in for now.
	 */
	if (time.tv_sec < NFS_MAXATTRTIMO)
		time.tv_sec = NFS_MAXATTRTIMO;

	/*
	 * Call nfs_boot_init() to fill in the nfs_diskless struct.
	 * Side effect:  Finds and configures a network interface.
	 */
	nd = malloc(sizeof(*nd), M_NFSMNT, M_WAITOK);
	memset((caddr_t)nd, 0, sizeof(*nd));
	error = nfs_boot_init(nd, procp);
	if (error) {
		free(nd, M_NFSMNT);
		return (error);
	}

	/*
	 * Create the root mount point.
	 */
	error = nfs_mount_diskless(&nd->nd_root, "/", &mp, &vp, procp);
	if (error)
		goto out;
	printf("root on %s\n", nd->nd_root.ndm_host);

	/*
	 * Link it into the mount list.
	 */
	simple_lock(&mountlist_slock);
	CIRCLEQ_INSERT_TAIL(&mountlist, mp, mnt_list);
	simple_unlock(&mountlist_slock);
	rootvp = vp;
	mp->mnt_vnodecovered = NULLVP;
	vfs_unbusy(mp);

	/* Get root attributes (for the time). */
	error = VOP_GETATTR(vp, &attr, procp->p_ucred, procp);
	if (error)
		panic("nfs_mountroot: getattr for root");
	n = attr.va_atime.tv_sec;
#ifdef	DEBUG
	printf("root time: 0x%lx\n", n);
#endif
	inittodr(n);

out:
	if (error)
		nfs_boot_cleanup(nd, procp);
	free(nd, M_NFSMNT);
	return (error);
}

/*
 * Internal version of mount system call for diskless setup.
 * Separate function because we used to call it twice.
 * (once for root and once for swap)
 */
static int
nfs_mount_diskless(ndmntp, mntname, mpp, vpp, p)
	struct nfs_dlmount *ndmntp;
	const char *mntname;	/* mount point name */
	struct mount **mpp;
	struct vnode **vpp;
	struct proc *p;
{
	struct mount *mp;
	struct mbuf *m;
	int error;

	vfs_rootmountalloc(MOUNT_NFS, (char *)mntname, &mp);

	mp->mnt_op = &nfs_vfsops;

	/*
	 * Historical practice expects NFS root file systems to
	 * be initially mounted r/w.
	 */
	mp->mnt_flag &= ~MNT_RDONLY;

	/* Get mbuf for server sockaddr. */
	m = m_get(M_WAIT, MT_SONAME);
	if (m == NULL)
		panic("nfs_mountroot: mget soname for %s", mntname);
	memcpy(mtod(m, caddr_t), (caddr_t)ndmntp->ndm_args.addr,
	      (m->m_len = ndmntp->ndm_args.addr->sa_len));

	error = mountnfs(&ndmntp->ndm_args, mp, m, mntname,
			 ndmntp->ndm_args.hostname, vpp, p);
	if (error) {
		mp->mnt_op->vfs_refcount--;
		vfs_unbusy(mp);
		printf("nfs_mountroot: mount %s failed: %d\n",
		       mntname, error);
		free(mp, M_MOUNT);
	} else
		*mpp = mp;

	return (error);
}

void
nfs_decode_args(nmp, argp)
	struct nfsmount *nmp;
	struct nfs_args *argp;
{
	int s;
	int adjsock;
	int maxio;

	s = splsoftnet();

	/*
	 * Silently clear NFSMNT_NOCONN if it's a TCP mount, it makes
	 * no sense in that context.
	 */
	if (argp->sotype == SOCK_STREAM)
		argp->flags &= ~NFSMNT_NOCONN;

	/*
	 * Cookie translation is not needed for v2, silently ignore it.
	 */
	if ((argp->flags & (NFSMNT_XLATECOOKIE|NFSMNT_NFSV3)) ==
	    NFSMNT_XLATECOOKIE)
		argp->flags &= ~NFSMNT_XLATECOOKIE;

	/* Re-bind if rsrvd port requested and wasn't on one */
	adjsock = !(nmp->nm_flag & NFSMNT_RESVPORT)
		  && (argp->flags & NFSMNT_RESVPORT);
	/* Also re-bind if we're switching to/from a connected UDP socket */
	adjsock |= ((nmp->nm_flag & NFSMNT_NOCONN) !=
		    (argp->flags & NFSMNT_NOCONN));

	/* Update flags atomically.  Don't change the lock bits. */
	nmp->nm_flag = argp->flags | nmp->nm_flag;
	splx(s);

	if ((argp->flags & NFSMNT_TIMEO) && argp->timeo > 0) {
		nmp->nm_timeo = (argp->timeo * NFS_HZ + 5) / 10;
		if (nmp->nm_timeo < NFS_MINTIMEO)
			nmp->nm_timeo = NFS_MINTIMEO;
		else if (nmp->nm_timeo > NFS_MAXTIMEO)
			nmp->nm_timeo = NFS_MAXTIMEO;
	}

	if ((argp->flags & NFSMNT_RETRANS) && argp->retrans > 1) {
		nmp->nm_retry = argp->retrans;
		if (nmp->nm_retry > NFS_MAXREXMIT)
			nmp->nm_retry = NFS_MAXREXMIT;
	}

	if (argp->flags & NFSMNT_NFSV3) {
		if (argp->sotype == SOCK_DGRAM)
			maxio = NFS_MAXDGRAMDATA;
		else
			maxio = NFS_MAXDATA;
	} else
		maxio = NFS_V2MAXDATA;

	if ((argp->flags & NFSMNT_WSIZE) && argp->wsize > 0) {
		int osize = nmp->nm_wsize;
		nmp->nm_wsize = argp->wsize;
		/* Round down to multiple of blocksize */
		nmp->nm_wsize &= ~(NFS_FABLKSIZE - 1);
		if (nmp->nm_wsize <= 0)
			nmp->nm_wsize = NFS_FABLKSIZE;
		adjsock |= (nmp->nm_wsize != osize);
	}
	if (nmp->nm_wsize > maxio)
		nmp->nm_wsize = maxio;
	if (nmp->nm_wsize > MAXBSIZE)
		nmp->nm_wsize = MAXBSIZE;

	if ((argp->flags & NFSMNT_RSIZE) && argp->rsize > 0) {
		int osize = nmp->nm_rsize;
		nmp->nm_rsize = argp->rsize;
		/* Round down to multiple of blocksize */
		nmp->nm_rsize &= ~(NFS_FABLKSIZE - 1);
		if (nmp->nm_rsize <= 0)
			nmp->nm_rsize = NFS_FABLKSIZE;
		adjsock |= (nmp->nm_rsize != osize);
	}
	if (nmp->nm_rsize > maxio)
		nmp->nm_rsize = maxio;
	if (nmp->nm_rsize > MAXBSIZE)
		nmp->nm_rsize = MAXBSIZE;

	if ((argp->flags & NFSMNT_READDIRSIZE) && argp->readdirsize > 0) {
		nmp->nm_readdirsize = argp->readdirsize;
		/* Round down to multiple of minimum blocksize */
		nmp->nm_readdirsize &= ~(NFS_DIRFRAGSIZ - 1);
		if (nmp->nm_readdirsize < NFS_DIRFRAGSIZ)
			nmp->nm_readdirsize = NFS_DIRFRAGSIZ;
		/* Bigger than buffer size makes no sense */
		if (nmp->nm_readdirsize > NFS_DIRBLKSIZ)
			nmp->nm_readdirsize = NFS_DIRBLKSIZ;
	} else if (argp->flags & NFSMNT_RSIZE)
		nmp->nm_readdirsize = nmp->nm_rsize;

	if (nmp->nm_readdirsize > maxio)
		nmp->nm_readdirsize = maxio;

	if ((argp->flags & NFSMNT_MAXGRPS) && argp->maxgrouplist >= 0 &&
		argp->maxgrouplist <= NFS_MAXGRPS)
		nmp->nm_numgrps = argp->maxgrouplist;
	if ((argp->flags & NFSMNT_READAHEAD) && argp->readahead >= 0 &&
		argp->readahead <= NFS_MAXRAHEAD)
		nmp->nm_readahead = argp->readahead;
	if ((argp->flags & NFSMNT_LEASETERM) && argp->leaseterm >= 2 &&
		argp->leaseterm <= NQ_MAXLEASE)
		nmp->nm_leaseterm = argp->leaseterm;
	if ((argp->flags & NFSMNT_DEADTHRESH) && argp->deadthresh >= 1 &&
		argp->deadthresh <= NQ_NEVERDEAD)
		nmp->nm_deadthresh = argp->deadthresh;

	adjsock |= ((nmp->nm_sotype != argp->sotype) ||
		    (nmp->nm_soproto != argp->proto));
	nmp->nm_sotype = argp->sotype;
	nmp->nm_soproto = argp->proto;

	if (nmp->nm_so && adjsock) {
		nfs_safedisconnect(nmp);
		if (nmp->nm_sotype == SOCK_DGRAM)
			while (nfs_connect(nmp, (struct nfsreq *)0)) {
				printf("nfs_args: retrying connect\n");
				(void) tsleep((caddr_t)&lbolt,
					      PSOCK, "nfscn3", 0);
			}
	}
}

/*
 * VFS Operations.
 *
 * mount system call
 * It seems a bit dumb to copyinstr() the host and path here and then
 * memcpy() them in mountnfs(), but I wanted to detect errors before
 * doing the sockargs() call because sockargs() allocates an mbuf and
 * an error after that means that I have to release the mbuf.
 */
/* ARGSUSED */
int
nfs_mount(mp, path, data, ndp, p)
	struct mount *mp;
	const char *path;
	void *data;
	struct nameidata *ndp;
	struct proc *p;
{
	int error;
	struct nfs_args args;
	struct mbuf *nam;
	struct vnode *vp;
	char pth[MNAMELEN], hst[MNAMELEN];
	size_t len;
	u_char nfh[NFSX_V3FHMAX];

	error = copyin(data, (caddr_t)&args, sizeof (struct nfs_args));
	if (error)
		return (error);
	if (args.version != NFS_ARGSVERSION)
		return (EPROGMISMATCH);
	if (mp->mnt_flag & MNT_UPDATE) {
		struct nfsmount *nmp = VFSTONFS(mp);

		if (nmp == NULL)
			return (EIO);
		/*
		 * When doing an update, we can't change from or to
		 * v3 and/or nqnfs, or change cookie translation
		 */
		args.flags = (args.flags &
		    ~(NFSMNT_NFSV3|NFSMNT_NQNFS|NFSMNT_XLATECOOKIE)) |
		    (nmp->nm_flag &
			(NFSMNT_NFSV3|NFSMNT_NQNFS|NFSMNT_XLATECOOKIE));
		nfs_decode_args(nmp, &args);
		return (0);
	}
	error = copyin((caddr_t)args.fh, (caddr_t)nfh, args.fhsize);
	if (error)
		return (error);
	error = copyinstr(path, pth, MNAMELEN-1, &len);
	if (error)
		return (error);
	memset(&pth[len], 0, MNAMELEN - len);
	error = copyinstr(args.hostname, hst, MNAMELEN-1, &len);
	if (error)
		return (error);
	memset(&hst[len], 0, MNAMELEN - len);
	/* sockargs() call must be after above copyin() calls */
	error = sockargs(&nam, (caddr_t)args.addr, args.addrlen, MT_SONAME);
	if (error)
		return (error);
	args.fh = nfh;
	error = mountnfs(&args, mp, nam, pth, hst, &vp, p);
	return (error);
}

/*
 * Common code for mount and mountroot
 */
int
mountnfs(argp, mp, nam, pth, hst, vpp, p)
	struct nfs_args *argp;
	struct mount *mp;
	struct mbuf *nam;
	const char *pth, *hst;
	struct vnode **vpp;
	struct proc *p;
{
	struct nfsmount *nmp;
	struct nfsnode *np;
	int error;
	struct vattr attrs;
	struct ucred *cr;

	/* 
	 * If the number of nfs iothreads to use has never
	 * been set, create a reasonable number of them.
	 */

	if (nfs_niothreads < 0) {
		nfs_niothreads = 4;
		nfs_getset_niothreads(TRUE);
	}
	
	if (mp->mnt_flag & MNT_UPDATE) {
		nmp = VFSTONFS(mp);
		/* update paths, file handles, etc, here	XXX */
		m_freem(nam);
		return (0);
	} else {
		MALLOC(nmp, struct nfsmount *, sizeof (struct nfsmount),
		    M_NFSMNT, M_WAITOK);
		memset((caddr_t)nmp, 0, sizeof (struct nfsmount));
		mp->mnt_data = (qaddr_t)nmp;
		TAILQ_INIT(&nmp->nm_uidlruhead);
		TAILQ_INIT(&nmp->nm_bufq);
	}
	vfs_getnewfsid(mp);
	nmp->nm_mountp = mp;

	if (argp->flags & NFSMNT_NQNFS)
		/*
		 * We have to set mnt_maxsymlink to a non-zero value so
		 * that COMPAT_43 routines will know that we are setting
		 * the d_type field in directories (and can zero it for
		 * unsuspecting binaries).
		 */
		mp->mnt_maxsymlinklen = 1;

	if ((argp->flags & NFSMNT_NFSV3) == 0)
		/*
		 * V2 can only handle 32 bit filesizes. For v3, nfs_fsinfo
		 * will fill this in.
		 */
		nmp->nm_maxfilesize = 0xffffffffLL;

	nmp->nm_timeo = NFS_TIMEO;
	nmp->nm_retry = NFS_RETRANS;
	nmp->nm_wsize = NFS_WSIZE;
	nmp->nm_rsize = NFS_RSIZE;
	nmp->nm_readdirsize = NFS_READDIRSIZE;
	nmp->nm_numgrps = NFS_MAXGRPS;
	nmp->nm_readahead = NFS_DEFRAHEAD;
	nmp->nm_leaseterm = NQ_DEFLEASE;
	nmp->nm_deadthresh = NQ_DEADTHRESH;
	CIRCLEQ_INIT(&nmp->nm_timerhead);
	nmp->nm_inprog = NULLVP;
	nmp->nm_fhsize = argp->fhsize;
	memcpy((caddr_t)nmp->nm_fh, (caddr_t)argp->fh, argp->fhsize);
#ifdef COMPAT_09
	mp->mnt_stat.f_type = 2;
#else
	mp->mnt_stat.f_type = 0;
#endif
	strncpy(&mp->mnt_stat.f_fstypename[0], mp->mnt_op->vfs_name,
	    MFSNAMELEN);
	memcpy(mp->mnt_stat.f_mntfromname, hst, MNAMELEN);
	memcpy(mp->mnt_stat.f_mntonname, pth, MNAMELEN);
	nmp->nm_nam = nam;

	/* Set up the sockets and per-host congestion */
	nmp->nm_sotype = argp->sotype;
	nmp->nm_soproto = argp->proto;

	nfs_decode_args(nmp, argp);

	/*
	 * For Connection based sockets (TCP,...) defer the connect until
	 * the first request, in case the server is not responding.
	 */
	if (nmp->nm_sotype == SOCK_DGRAM &&
		(error = nfs_connect(nmp, (struct nfsreq *)0)))
		goto bad;

	/*
	 * This is silly, but it has to be set so that vinifod() works.
	 * We do not want to do an nfs_statfs() here since we can get
	 * stuck on a dead server and we are holding a lock on the mount
	 * point.
	 */
	mp->mnt_stat.f_iosize = NFS_MAXDGRAMDATA;
	/*
	 * A reference count is needed on the nfsnode representing the
	 * remote root.  If this object is not persistent, then backward
	 * traversals of the mount point (i.e. "..") will not work if
	 * the nfsnode gets flushed out of the cache. Ufs does not have
	 * this problem, because one can identify root inodes by their
	 * number == ROOTINO (2).
	 */
	error = nfs_nget(mp, (nfsfh_t *)nmp->nm_fh, nmp->nm_fhsize, &np);
	if (error)
		goto bad;
	*vpp = NFSTOV(np);
	VOP_GETATTR(*vpp, &attrs, p->p_ucred, p);
	if ((nmp->nm_flag & NFSMNT_NFSV3) && ((*vpp)->v_type == VDIR)) {
		cr = crget();
		cr->cr_uid = attrs.va_uid;
		cr->cr_gid = attrs.va_gid;
		cr->cr_ngroups = 0;
		nfs_cookieheuristic(*vpp, &nmp->nm_iflag, p, cr);
		crfree(cr);
	}

	return (0);
bad:
	nfs_disconnect(nmp);
	free((caddr_t)nmp, M_NFSMNT);
	m_freem(nam);
	return (error);
}

/*
 * unmount system call
 */
int
nfs_unmount(mp, mntflags, p)
	struct mount *mp;
	int mntflags;
	struct proc *p;
{
	struct nfsmount *nmp;
	struct nfsnode *np;
	struct vnode *vp;
	int error, flags = 0;

	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;
	nmp = VFSTONFS(mp);
	/*
	 * Goes something like this..
	 * - Check for activity on the root vnode (other than ourselves).
	 * - Call vflush() to clear out vnodes for this file system,
	 *   except for the root vnode.
	 * - Decrement reference on the vnode representing remote root.
	 * - Close the socket
	 * - Free up the data structures
	 */
	/*
	 * We need to decrement the ref. count on the nfsnode representing
	 * the remote root.  See comment in mountnfs().  The VFS unmount()
	 * has done vput on this vnode, otherwise we would get deadlock!
	 */
	error = nfs_nget(mp, (nfsfh_t *)nmp->nm_fh, nmp->nm_fhsize, &np);
	if (error)
		return(error);
	vp = NFSTOV(np);
	if ((mntflags & MNT_FORCE) == 0 && vp->v_usecount > 2) {
		vput(vp);
		return (EBUSY);
	}

	/*
	 * Must handshake with nqnfs_clientd() if it is active.
	 */
	nmp->nm_iflag |= NFSMNT_DISMINPROG;
	while (nmp->nm_inprog != NULLVP)
		(void) tsleep((caddr_t)&lbolt, PSOCK, "nfsdism", 0);
	error = vflush(mp, vp, flags);
	if (error) {
		vput(vp);
		nmp->nm_iflag &= ~NFSMNT_DISMINPROG;
		return (error);
	}

	/*
	 * We are now committed to the unmount; mark the mount structure
	 * as doomed so that any sleepers kicked awake by nfs_disconnect
	 * will go away cleanly.
	 */
	nmp->nm_iflag |= NFSMNT_DISMNT;

	/*
	 * There are two reference counts to get rid of here.
	 */
	vrele(vp);
	vrele(vp);
	vgone(vp);
	nfs_disconnect(nmp);
	m_freem(nmp->nm_nam);

	/*
	 * For NQNFS, let the server daemon free the nfsmount structure.
	 */
	if ((nmp->nm_flag & (NFSMNT_NQNFS | NFSMNT_KERB)) == 0)
		free((caddr_t)nmp, M_NFSMNT);
	return (0);
}

/*
 * Return root of a filesystem
 */
int
nfs_root(mp, vpp)
	struct mount *mp;
	struct vnode **vpp;
{
	struct vnode *vp;
	struct nfsmount *nmp;
	struct nfsnode *np;
	int error;

	nmp = VFSTONFS(mp);
	error = nfs_nget(mp, (nfsfh_t *)nmp->nm_fh, nmp->nm_fhsize, &np);
	if (error)
		return (error);
	vp = NFSTOV(np);
	if (vp->v_type == VNON)
		vp->v_type = VDIR;
	vp->v_flag = VROOT;
	*vpp = vp;
	return (0);
}

extern int syncprt;

/*
 * Flush out the buffer cache
 */
/* ARGSUSED */
int
nfs_sync(mp, waitfor, cred, p)
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
		if (VOP_ISLOCKED(vp) || vp->v_dirtyblkhd.lh_first == NULL ||
		    waitfor == MNT_LAZY)
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

/*
 * NFS flat namespace lookup.
 * Currently unsupported.
 */
/* ARGSUSED */
int
nfs_vget(mp, ino, vpp)
	struct mount *mp;
	ino_t ino;
	struct vnode **vpp;
{

	return (EOPNOTSUPP);
}

/*
 * Do that sysctl thang...
 */
int
nfs_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	int rv;

	/*
	 * All names at this level are terminal.
	 */
	if(namelen > 1)
		return ENOTDIR;	/* overloaded */

	switch(name[0]) {
	case NFS_NFSSTATS:
		if(!oldp) {
			*oldlenp = sizeof nfsstats;
			return 0;
		}

		if(*oldlenp < sizeof nfsstats) {
			*oldlenp = sizeof nfsstats;
			return ENOMEM;
		}

		rv = copyout(&nfsstats, oldp, sizeof nfsstats);
		if(rv) return rv;

		if(newp && newlen != sizeof nfsstats)
			return EINVAL;

		if(newp) {
			return copyin(newp, &nfsstats, sizeof nfsstats);
		}
		return 0;

	case NFS_IOTHREADS:
		nfs_getset_niothreads(0);

		rv = (sysctl_int(oldp, oldlenp, newp, newlen,
		    &nfs_niothreads));

		if (newp)
			nfs_getset_niothreads(1);

		return rv;
                
	default:
		return EOPNOTSUPP;
	}
}


/*
 * At this point, this should never happen
 */
/* ARGSUSED */
int
nfs_fhtovp(mp, fhp, vpp)
	struct mount *mp;
	struct fid *fhp;
	struct vnode **vpp;
{

	return (EINVAL);
}

/* ARGSUSED */
int
nfs_checkexp(mp, nam, exflagsp, credanonp)
	struct mount *mp;
	struct mbuf *nam;
	int *exflagsp;
	struct ucred **credanonp;
{

	return (EINVAL);
}

/*
 * Vnode pointer to File handle, should never happen either
 */
/* ARGSUSED */
int
nfs_vptofh(vp, fhp)
	struct vnode *vp;
	struct fid *fhp;
{

	return (EINVAL);
}

/*
 * Vfs start routine, a no-op.
 */
/* ARGSUSED */
int
nfs_start(mp, flags, p)
	struct mount *mp;
	int flags;
	struct proc *p;
{

	return (0);
}

/*
 * Do operations associated with quotas, not supported
 */
/* ARGSUSED */
int
nfs_quotactl(mp, cmd, uid, arg, p)
	struct mount *mp;
	int cmd;
	uid_t uid;
	caddr_t arg;
	struct proc *p;
{

	return (EOPNOTSUPP);
}
