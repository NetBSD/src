/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
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
 *	from: @(#)nfs_vfsops.c	7.31 (Berkeley) 5/6/91
 *	$Id: nfs_vfsops.c,v 1.15 1994/04/18 06:18:22 glass Exp $
 */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>

#include <nfs/nfsv2.h>
#include <nfs/nfsnode.h>
#include <nfs/nfsmount.h>
#include <nfs/nfs.h>
#include <nfs/xdr_subs.h>
#include <nfs/nfsm_subs.h>
#include <nfs/nfsdiskless.h>

/*
 * nfs vfs operations.
 */
struct vfsops nfs_vfsops = {
	MOUNT_NFS,
	nfs_mount,
	nfs_start,
	nfs_unmount,
	nfs_root,
	nfs_quotactl,
	nfs_statfs,
	nfs_sync,
	nfs_fhtovp,
	nfs_vptofh,
	nfs_init,
};

static u_char nfs_mntid;
extern u_long nfs_procids[NFS_NPROCS];
extern u_long nfs_prog, nfs_vers;
void nfs_disconnect();

#define TRUE	1
#define	FALSE	0

/*
 * nfs statfs call
 */
nfs_statfs(mp, sbp, p)
	struct mount *mp;
	register struct statfs *sbp;
	struct proc *p;
{
	register struct vnode *vp;
	register struct nfsv2_statfs *sfp;
	register caddr_t cp;
	register long t1;
	caddr_t bpos, dpos, cp2;
	u_long xid;
	int error = 0;
	struct mbuf *mreq, *mrep, *md, *mb, *mb2;
	struct nfsmount *nmp;
	struct ucred *cred;
	struct nfsnode *np;

	nmp = VFSTONFS(mp);
	if (error = nfs_nget(mp, &nmp->nm_fh, &np))
		return (error);
	vp = NFSTOV(np);
	nfsstats.rpccnt[NFSPROC_STATFS]++;
	cred = crget();
	cred->cr_ngroups = 1;
	nfsm_reqhead(nfs_procids[NFSPROC_STATFS], cred, NFSX_FH);
	nfsm_fhtom(vp);
	nfsm_request(vp, NFSPROC_STATFS, p, 0);
	nfsm_disect(sfp, struct nfsv2_statfs *, NFSX_STATFS);
#ifdef COMPAT_09
	sbp->f_type = 2;
#else
	sbp->f_type = 0;
#endif
	sbp->f_flags = nmp->nm_flag;
	sbp->f_bsize = fxdr_unsigned(long, sfp->sf_tsize);
	sbp->f_fsize = fxdr_unsigned(long, sfp->sf_bsize);
	sbp->f_blocks = fxdr_unsigned(long, sfp->sf_blocks);
	sbp->f_bfree = fxdr_unsigned(long, sfp->sf_bfree);
	sbp->f_bavail = fxdr_unsigned(long, sfp->sf_bavail);
	sbp->f_files = 0;
	sbp->f_ffree = 0;
	if (sbp != &mp->mnt_stat) {
		bcopy(mp->mnt_stat.f_mntonname, sbp->f_mntonname, MNAMELEN);
		bcopy(mp->mnt_stat.f_mntfromname, sbp->f_mntfromname, MNAMELEN);
	}
	strncpy(&sbp->f_fstypename[0], mp->mnt_op->vfs_name, MFSNAMELEN);
	sbp->f_fstypename[MFSNAMELEN] = '\0';
	nfsm_reqdone;
	nfs_nput(vp);
	crfree(cred);
	return (error);
}

/*
 * Mount a remote root fs via. nfs.
 *
 * Configure up an interface
 * Initialize a nfs_diskless struct with:
 *                 ip addr
 *                 broadcast addr
 *                 netmask
 * as acquired and implied by RARP.
 *
 * Then call nfs_boot() to fill in the rest of the structure with enough
 * information to mount root and swap.
 */
nfs_mountroot()
{
	register struct mount *mp;
	register struct mbuf *m;
	struct socket *so;
	struct vnode *vp;
	struct ifnet *ifp;
	struct sockaddr_in *sin;
	struct in_addr myip;
	struct ifreq ireq;
	struct nfs_diskless diskless;
	int error, len;

	/*
	 * Find an interface, rarp for its ip address, stuff it, the
	 * implied broadcast addr, and netmask into a nfs_diskless struct.
	 */

	for (ifp = ifnet; ifp; ifp = ifp->if_next)
		if ((ifp->if_flags & (IFF_LOOPBACK|IFF_POINTOPOINT)) == 0)
			break;
	if (ifp == NULL)
		return ENETUNREACH;
	strcpy(ireq.ifr_name, ifp->if_name);
	len = strlen(ireq.ifr_name);
	ireq.ifr_name[len] = '0' + ifp->if_unit; /* XXX */
	ireq.ifr_name[len+1] = '\0';
	ireq.ifr_flags = IFF_UP;
	if (error = ifioctl(NULL, SIOCSIFFLAGS, &ireq, curproc))
		panic("nfs_mountroot, bringing interface %s up",
		      ireq.ifr_name);
	bzero((caddr_t) &diskless, sizeof(diskless));
	strcpy(diskless.myif.ifra_name, ireq.ifr_name);
	if (revarp_whoami(&myip, ifp))
		panic("revarp failed");
	sin = (struct sockaddr_in *) &ireq.ifr_addr;
	bzero((caddr_t) sin, sizeof(struct sockaddr_in));
	sin->sin_len = sizeof(struct sockaddr_in);
	sin->sin_family = AF_INET;
	sin->sin_addr = myip;

	/*
	 * Do enough of ifconfig(8) so that the chosen interface can
	 * talk to the server(s).
	 */
	if (socreate(AF_INET, &so, SOCK_DGRAM, 0))
		panic("nfs ifconf");
	if (ifioctl(so, SIOCSIFADDR, &ireq, curproc))
		panic("nfs_mountroot: setting interface address\n");
	bcopy((caddr_t) sin, (caddr_t) &diskless.myif.ifra_addr,
	      sizeof(struct sockaddr));
	if (ifioctl(so, SIOCGIFBRDADDR, &ireq, curproc))
		panic("nfs baddr");
	bcopy((caddr_t) &ireq.ifr_broadaddr,
	      (caddr_t) &diskless.myif.ifra_broadaddr,
	      sizeof (ireq.ifr_broadaddr));
	if (ifioctl(so, SIOCGIFNETMASK, &ireq, curproc))
		panic("nfs get netmask");
	soclose(so);

	if (error = nfs_boot(&diskless))
		return error;

	/*
	 * If the gateway field is filled in, set it as the default route.
	 */
#ifdef COMPAT_43
	if (diskless.mygateway.sa_family == AF_INET) {
		struct ortentry rt;
		struct sockaddr_in *sin;

		sin = (struct sockaddr_in *) &rt.rt_dst;
		sin->sin_len = sizeof (struct sockaddr_in);
		sin->sin_family = AF_INET;
		sin->sin_addr.s_addr = 0;	/* default */
		bcopy((caddr_t)&diskless.mygateway, (caddr_t)&rt.rt_gateway,
			sizeof (struct sockaddr_in));
		rt.rt_flags = (RTF_UP | RTF_GATEWAY);
		if (rtioctl(SIOCADDRT, (caddr_t)&rt, curproc))
			panic("nfs root route");
	}
#endif	/* COMPAT_43 */

	/*
	 * If swapping to an nfs node (indicated by swdevt[0].sw_dev == NODEV):
	 * Create a fake mount point just for the swap vnode so that the
	 * swap file can be on a different server from the rootfs.
	 */
	if (swdevt[0].sw_dev == NODEV) {
		mp = (struct mount *)malloc((u_long)sizeof(struct mount),
			M_MOUNT, M_NOWAIT);
		if (mp == NULL)
			panic("nfs root mount");
		mp->mnt_op = &nfs_vfsops;
		mp->mnt_flag = 0;
		mp->mnt_exroot = 0;
		mp->mnt_mounth = NULLVP;
	
		/*
		 * Set up the diskless nfs_args for the swap mount point
		 * and then call mountnfs() to mount it.
		 * Since the swap file is not the root dir of a file system,
		 * hack it to a regular file.
		 */
		diskless.swap_args.fh = (nfsv2fh_t *)diskless.swap_fh;
		MGET(m, MT_SONAME, M_DONTWAIT);
		if (m == NULL)
			panic("nfs root mbuf");
		bcopy((caddr_t)&diskless.swap_saddr, mtod(m, caddr_t),
			diskless.swap_saddr.sa_len);
		m->m_len = diskless.swap_saddr.sa_len;
		if (mountnfs(&diskless.swap_args, mp, m, "/swap",
			diskless.swap_hostnam, &vp))
			panic("nfs swap");
		vp->v_type = VREG;
		vp->v_flag = 0;
		swapdev_vp = vp;
		VREF(vp);
		swdevt[0].sw_vp = vp;
		{
			struct vattr attr;

			if (nfs_dogetattr(vp,&attr,NOCRED,0,0)) {
			    panic("nfs swap getattr");
			}
			swdevt[0].sw_nblks = attr.va_size / DEV_BSIZE;
		}
	}

	/*
	 * Create the rootfs mount point.
	 */
	mp = (struct mount *)malloc((u_long)sizeof(struct mount),
		M_MOUNT, M_NOWAIT);
	if (mp == NULL)
		panic("nfs root mount2");
	mp->mnt_op = &nfs_vfsops;
	mp->mnt_flag = MNT_RDONLY;
	mp->mnt_exroot = 0;
	mp->mnt_mounth = NULLVP;

	/*
	 * Set up the root fs args and call mountnfs() to do the rest.
	 */
	diskless.root_args.fh = (nfsv2fh_t *)diskless.root_fh;
	MGET(m, MT_SONAME, M_DONTWAIT);
	if (m == NULL)
		panic("nfs root mbuf2");
	bcopy((caddr_t)&diskless.root_saddr, mtod(m, caddr_t),
		diskless.root_saddr.sa_len);
	m->m_len = diskless.root_saddr.sa_len;
	if (mountnfs(&diskless.root_args, mp, m, "/",
		diskless.root_hostnam, &vp))
		panic("nfs root");
	if (vfs_lock(mp))
		panic("nfs root2");
	rootfs = mp;
	mp->mnt_next = mp;
	mp->mnt_prev = mp;
	mp->mnt_vnodecovered = NULLVP;
	vfs_unlock(mp);
	rootvp = vp;
	inittodr((time_t)0);	/* There is no time in the nfs fsstat so ?? */
	return (0);
}

static void
nfs_decode_flags(argp, nmp)
struct nfs_args	*argp;
struct nfsmount	*nmp;
{
	int	s = splnet();

	/* Don't touch the lock flags */
	nmp->nm_flag = (argp->flags & ~(NFSMNT_LOCKBITS)) |
				(nmp->nm_flag & NFSMNT_LOCKBITS);
	splx(s);

	if ((argp->flags & NFSMNT_TIMEO) && argp->timeo > 0) {
		nmp->nm_rto = argp->timeo;
		/* NFS timeouts are specified in 1/10 sec. */
		nmp->nm_rto = (nmp->nm_rto * 10) / NFS_HZ;
		if (nmp->nm_rto < NFS_MINTIMEO)
			nmp->nm_rto = NFS_MINTIMEO;
		else if (nmp->nm_rto > NFS_MAXTIMEO)
			nmp->nm_rto = NFS_MAXTIMEO;
		nmp->nm_rttvar = nmp->nm_rto << 1;
	}

	if ((argp->flags & NFSMNT_RETRANS) && argp->retrans > 1) {
		nmp->nm_retry = argp->retrans;
		if (nmp->nm_retry > NFS_MAXREXMIT)
			nmp->nm_retry = NFS_MAXREXMIT;
	}

	if ((argp->flags & NFSMNT_WSIZE) && argp->wsize > 0) {
		nmp->nm_wsize = argp->wsize;
		/* Round down to multiple of blocksize */
		nmp->nm_wsize &= ~0x1ff;
		if (nmp->nm_wsize <= 0)
			nmp->nm_wsize = 512;
		else if (nmp->nm_wsize > NFS_MAXDATA)
			nmp->nm_wsize = NFS_MAXDATA;
	}
	if (nmp->nm_wsize > MAXBSIZE)
		nmp->nm_wsize = MAXBSIZE;

	if ((argp->flags & NFSMNT_RSIZE) && argp->rsize > 0) {
		nmp->nm_rsize = argp->rsize;
		/* Round down to multiple of blocksize */
		nmp->nm_rsize &= ~0x1ff;
		if (nmp->nm_rsize <= 0)
			nmp->nm_rsize = 512;
		else if (nmp->nm_rsize > NFS_MAXDATA)
			nmp->nm_rsize = NFS_MAXDATA;
	}
	if (nmp->nm_rsize > MAXBSIZE)
		nmp->nm_rsize = MAXBSIZE;
}

/*
 * VFS Operations.
 *
 * mount system call
 * It seems a bit dumb to copyinstr() the host and path here and then
 * bcopy() them in mountnfs(), but I wanted to detect errors before
 * doing the sockargs() call because sockargs() allocates an mbuf and
 * an error after that means that I have to release the mbuf.
 */
/* ARGSUSED */
nfs_mount(mp, path, data, ndp, p)
	struct mount *mp;
	char *path;
	caddr_t data;
	struct nameidata *ndp;
	struct proc *p;
{
	int error;
	struct nfs_args args;
	struct mbuf *nam;
	struct vnode *vp;
	char pth[MNAMELEN], hst[MNAMELEN];
	u_int len;
	nfsv2fh_t nfh;

	if (error = copyin(data, (caddr_t)&args, sizeof (struct nfs_args)))
		return (error);
	if (mp->mnt_flag & MNT_UPDATE) {
		register struct nfsmount *nmp = VFSTONFS(mp);

		if (nmp == NULL)
			return EIO;
		nfs_decode_flags(&args, nmp);
		return (0);
	}
	if (error = copyin((caddr_t)args.fh, (caddr_t)&nfh, sizeof (nfsv2fh_t)))
		return (error);
	if (error = copyinstr(path, pth, MNAMELEN-1, &len))
		return (error);
	bzero(&pth[len], MNAMELEN - len);
	if (error = copyinstr(args.hostname, hst, MNAMELEN-1, &len))
		return (error);
	bzero(&hst[len], MNAMELEN - len);
	/* sockargs() call must be after above copyin() calls */
	if (error = sockargs(&nam, (caddr_t)args.addr,
		sizeof (struct sockaddr), MT_SONAME))
		return (error);
	args.fh = &nfh;
	error = mountnfs(&args, mp, nam, pth, hst, &vp);
	return (error);
}

/*
 * Common code for mount and mountroot
 */
mountnfs(argp, mp, nam, pth, hst, vpp)
	register struct nfs_args *argp;
	register struct mount *mp;
	struct mbuf *nam;
	char *pth, *hst;
	struct vnode **vpp;
{
	register struct nfsmount *nmp;
	struct proc *p = curproc;		/* XXX */
	struct nfsnode *np;
	int error;
	fsid_t tfsid;

	MALLOC(nmp, struct nfsmount *, sizeof *nmp, M_NFSMNT, M_WAITOK);
	bzero((caddr_t)nmp, sizeof *nmp);
	mp->mnt_data = (qaddr_t)nmp;

	getnewfsid(mp, (int)MOUNT_NFS);
	nmp->nm_mountp = mp;
	nmp->nm_rto = NFS_TIMEO;
	nmp->nm_rtt = -1;
	nmp->nm_rttvar = nmp->nm_rto << 1;
	nmp->nm_retry = NFS_RETRANS;
	nmp->nm_wsize = NFS_WSIZE;
	nmp->nm_rsize = NFS_RSIZE;
	bcopy((caddr_t)argp->fh, (caddr_t)&nmp->nm_fh, sizeof(nfsv2fh_t));
	bcopy(hst, mp->mnt_stat.f_mntfromname, MNAMELEN);
	bcopy(pth, mp->mnt_stat.f_mntonname, MNAMELEN);
	nmp->nm_nam = nam;
	nfs_decode_flags(argp, nmp);

	/* Set up the sockets and per-host congestion */
	nmp->nm_sotype = argp->sotype;
	nmp->nm_soproto = argp->proto;
	if (error = nfs_connect(nmp))
		goto bad;

	if (error = nfs_statfs(mp, &mp->mnt_stat, p))
		goto bad;
	/*
	 * A reference count is needed on the nfsnode representing the
	 * remote root.  If this object is not persistent, then backward
	 * traversals of the mount point (i.e. "..") will not work if
	 * the nfsnode gets flushed out of the cache. Ufs does not have
	 * this problem, because one can identify root inodes by their
	 * number == ROOTINO (2).
	 */
	if (error = nfs_nget(mp, &nmp->nm_fh, &np))
		goto bad;
	/*
	 * Unlock it, but keep the reference count.
	 */
	nfs_unlock(NFSTOV(np));
	*vpp = NFSTOV(np);

	return (0);
bad:
	nfs_disconnect(nmp);
	FREE(nmp, M_NFSMNT);
	m_freem(nam);
	return (error);
}

/*
 * unmount system call
 */
nfs_unmount(mp, mntflags, p)
	struct mount *mp;
	int mntflags;
	struct proc *p;
{
	register struct nfsmount *nmp;
	struct nfsnode *np;
	struct vnode *vp;
	int error, flags = 0;
	extern int doforce;

	if (mntflags & MNT_FORCE) {
		if (!doforce || mp == rootfs)
			return (EINVAL);
		flags |= FORCECLOSE;
	}
	nmp = VFSTONFS(mp);
	/*
	 * Clear out the buffer cache
	 */
	mntflushbuf(mp, 0);
	if (mntinvalbuf(mp))
		return (EBUSY);
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
	if (error = nfs_nget(mp, &nmp->nm_fh, &np))
		return(error);
	vp = NFSTOV(np);
	if (vp->v_usecount > 2) {
		vput(vp);
		return (EBUSY);
	}
	if (error = vflush(mp, vp, flags)) {
		vput(vp);
		return (error);
	}
	/*
	 * Get rid of two reference counts, and unlock it on the second.
	 */
	vrele(vp);
	vput(vp);
	vgone(vp);
	nfs_disconnect(nmp);
	m_freem(nmp->nm_nam);
	free((caddr_t)nmp, M_NFSMNT);
	return (0);
}

/*
 * Return root of a filesystem
 */
nfs_root(mp, vpp)
	struct mount *mp;
	struct vnode **vpp;
{
	register struct vnode *vp;
	struct nfsmount *nmp;
	struct nfsnode *np;
	int error;
	struct vattr va;
	struct proc *p = curproc /* XXX */;

	nmp = VFSTONFS(mp);
	if (error = nfs_nget(mp, &nmp->nm_fh, &np))
		return (error);
	vp = NFSTOV(np);
	if (error = nfs_getattr(vp, &va, p->p_ucred, p))
		return (error);
	vp->v_type = va.va_type;
	vp->v_flag = VROOT;
	*vpp = vp;
	return (0);
}

extern int syncprt;

/*
 * Flush out the buffer cache
 */
/* ARGSUSED */
nfs_sync(mp, waitfor)
	struct mount *mp;
	int waitfor;
{
	if (syncprt)
		bufstats();
	/*
	 * Force stale buffer cache information to be flushed.
	 */
	mntflushbuf(mp, waitfor == MNT_WAIT ? B_SYNC : 0);
	return (0);
}

/*
 * At this point, this should never happen
 */
/* ARGSUSED */
nfs_fhtovp(mp, fhp, vpp)
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
nfs_quotactl(mp, cmd, uid, arg, p)
	struct mount *mp;
	int cmd;
	uid_t uid;
	caddr_t arg;
	struct proc *p;
{
#ifdef lint
	mp = mp; cmd = cmd; uid = uid; arg = arg;
#endif /* lint */
	return (EOPNOTSUPP);
}
