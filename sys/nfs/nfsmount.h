/*	$NetBSD: nfsmount.h,v 1.30 2004/04/21 01:05:43 christos Exp $	*/

/*
 * Copyright (c) 1989, 1993
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
 *
 *	@(#)nfsmount.h	8.3 (Berkeley) 3/30/95
 */


#ifndef _NFS_NFSMOUNT_H_
#define _NFS_NFSMOUNT_H_

/*
 * Arguments to mount NFS
 */
#define NFS_ARGSVERSION	3		/* change when nfs_args changes */
struct nfs_args {
	int		version;	/* args structure version number */
	struct sockaddr	*addr;		/* file server address */
	int		addrlen;	/* length of address */
	int		sotype;		/* Socket type */
	int		proto;		/* and Protocol */
	u_char		*fh;		/* File handle to be mounted */
	int		fhsize;		/* Size, in bytes, of fh */
	int		flags;		/* flags */
	int		wsize;		/* write size in bytes */
	int		rsize;		/* read size in bytes */
	int		readdirsize;	/* readdir size in bytes */
	int		timeo;		/* initial timeout in .1 secs */
	int		retrans;	/* times to retry send */
	int		maxgrouplist;	/* Max. size of group list */
	int		readahead;	/* # of blocks to readahead */
	int		leaseterm;	/* Term (sec) of lease */
	int		deadthresh;	/* Retrans threshold */
	char		*hostname;	/* server's name */
};

/*
 * NFS mount option flags (nm_flag)
 */
#define	NFSMNT_SOFT		0x00000001  /* soft mount (hard is default) */
#define	NFSMNT_WSIZE		0x00000002  /* set write size */
#define	NFSMNT_RSIZE		0x00000004  /* set read size */
#define	NFSMNT_TIMEO		0x00000008  /* set initial timeout */
#define	NFSMNT_RETRANS		0x00000010  /* set number of request retries */
#define	NFSMNT_MAXGRPS		0x00000020  /* set maximum grouplist size */
#define	NFSMNT_INT		0x00000040  /* allow interrupts on hard mount */
#define	NFSMNT_NOCONN		0x00000080  /* Don't Connect the socket */
#define	NFSMNT_NQNFS		0x00000100  /* Use Nqnfs protocol */
#define	NFSMNT_NFSV3		0x00000200  /* Use NFS Version 3 protocol */
#define	NFSMNT_KERB		0x00000400  /* Use Kerberos authentication */
#define	NFSMNT_DUMBTIMR		0x00000800  /* Don't estimate rtt dynamically */
#define	NFSMNT_LEASETERM	0x00001000  /* set lease term (nqnfs) */
#define	NFSMNT_READAHEAD	0x00002000  /* set read ahead */
#define	NFSMNT_DEADTHRESH	0x00004000  /* set dead server retry thresh */
#define	NFSMNT_RESVPORT		0x00008000  /* Allocate a reserved port */
#define	NFSMNT_RDIRPLUS		0x00010000  /* Use Readdirplus for V3 */
#define	NFSMNT_READDIRSIZE	0x00020000  /* Set readdir size */
#define NFSMNT_XLATECOOKIE	0x00040000  /* 32<->64 dir cookie xlation */

#define NFSMNT_BITS	"\177\20" \
    "b\00soft\0b\01wsize\0b\02rsize\0b\03timeo\0" \
    "b\04retrans\0b\05maxgrps\0b\06intr\0b\07noconn\0" \
    "b\10nqnfs\0b\11nfsv3\0b\12kerb\0b\13dumbtimr\0" \
    "b\14leaseterm\0b\15readahead\0b\16deadthresh\0b\17resvport\0" \
    "b\20rdirplus\0b\21readdirsize\0b\22xlatecookie\0"

/*
 * NFS internal flags (nm_iflag) */

#define NFSMNT_HASWRITEVERF	0x00000001  /* Has write verifier for V3 */
#define NFSMNT_GOTPATHCONF	0x00000002  /* Got the V3 pathconf info */
#define NFSMNT_GOTFSINFO	0x00000004  /* Got the V3 fsinfo */
#define	NFSMNT_MNTD		0x00000008  /* Mnt server for mnt point */
#define	NFSMNT_DISMINPROG	0x00000010  /* Dismount in progress */
#define	NFSMNT_DISMNT		0x00000020  /* Dismounted */
#define	NFSMNT_SNDLOCK		0x00000040  /* Send socket lock */
#define	NFSMNT_WANTSND		0x00000080  /* Want above */
#define	NFSMNT_RCVLOCK		0x00000100  /* Rcv socket lock */
#define	NFSMNT_WANTRCV		0x00000200  /* Want above */
#define	NFSMNT_WAITAUTH		0x00000400  /* Wait for authentication */
#define	NFSMNT_HASAUTH		0x00000800  /* Has authenticator */
#define	NFSMNT_WANTAUTH		0x00001000  /* Wants an authenticator */
#define	NFSMNT_AUTHERR		0x00002000  /* Authentication error */
#define NFSMNT_SWAPCOOKIE	0x00004000  /* XDR encode dir cookies */
#define NFSMNT_STALEWRITEVERF	0x00008000  /* Write verifier is changing */

#ifdef _KERNEL
/*
 * Mount structure.
 * One allocated on every NFS mount.
 * Holds NFS specific information for mount.
 */
struct	nfsmount {
	struct simplelock nm_slock;	/* Lock for this structure */
	int	nm_flag;		/* Flags for soft/hard... */
	struct	mount *nm_mountp;	/* Vfs structure for this filesystem */
	int	nm_numgrps;		/* Max. size of groupslist */
	struct vnode *nm_vnode;
	struct	socket *nm_so;		/* Rpc socket */
	int	nm_sotype;		/* Type of socket */
	int	nm_soproto;		/* and protocol */
	int	nm_soflags;		/* pr_flags for socket protocol */
	struct	mbuf *nm_nam;		/* Addr of server */
	int	nm_timeo;		/* Init timer for NFSMNT_DUMBTIMR */
	int	nm_retry;		/* Max retries */
	int	nm_srtt[4];		/* Timers for rpcs */
	int	nm_sdrtt[4];
	int	nm_sent;		/* Request send count */
	int	nm_cwnd;		/* Request send window */
	int	nm_timeouts;		/* Request timeouts */
	int	nm_deadthresh;		/* Threshold of timeouts-->dead server*/
	int	nm_rsize;		/* Max size of read rpc */
	int	nm_wsize;		/* Max size of write rpc */
	int	nm_readdirsize;		/* Size of a readdir rpc */
	int	nm_readahead;		/* Num. of blocks to readahead */
	int	nm_leaseterm;		/* Term (sec) for NQNFS lease */
	CIRCLEQ_HEAD(, nfsnode) nm_timerhead; /* Head of lease timer queue */
	struct vnode *nm_inprog;	/* Vnode in prog by nqnfs_clientd() */
	uid_t	nm_authuid;		/* Uid for authenticator */
	int	nm_authtype;		/* Authenticator type */
	int	nm_authlen;		/* and length */
	char	*nm_authstr;		/* Authenticator string */
	char	*nm_verfstr;		/* and the verifier */
	int	nm_verflen;
	struct lock nm_writeverflock;	/* lock for below */
	u_char	nm_writeverf[NFSX_V3WRITEVERF]; /* V3 write verifier */
	NFSKERBKEY_T nm_key;		/* and the session key */
	int	nm_numuids;		/* Number of nfsuid mappings */
	TAILQ_HEAD(, nfsuid) nm_uidlruhead; /* Lists of nfsuid mappings */
	LIST_HEAD(, nfsuid) nm_uidhashtbl[NFS_MUIDHASHSIZ];
	TAILQ_HEAD(, buf) nm_bufq;      /* async io buffer queue */
	short	nm_bufqlen;		/* number of buffers in queue */
	short	nm_bufqwant;		/* process wants to add to the queue */
	int	nm_bufqiods;		/* number of iods processing queue */
	u_int64_t nm_maxfilesize;	/* maximum file size */
	int	nm_iflag;		/* internal flags */
	int	nm_waiters;		/* number of waiting listeners.. */
};

/*
 * Convert mount ptr to nfsmount ptr.
 */
#define VFSTONFS(mp)	((struct nfsmount *)((mp)->mnt_data))

/*
 * Prototypes for NFS mount operations
 */
int	nfs_mount __P((struct mount *mp, const char *path, void *data,
		struct nameidata *ndp, struct proc *p));
int	mountnfs __P((struct nfs_args *argp, struct mount *mp,
		struct mbuf *nam, const char *pth, const char *hst,
		struct vnode **vpp, struct proc *p));
int	nfs_mountroot __P((void));
void	nfs_decode_args __P((struct nfsmount *, struct nfs_args *));
int	nfs_start __P((struct mount *mp, int flags, struct proc *p));
int	nfs_unmount __P((struct mount *mp, int mntflags, struct proc *p));
int	nfs_root __P((struct mount *mp, struct vnode **vpp));
int	nfs_quotactl __P((struct mount *mp, int cmds, uid_t uid, caddr_t arg,
		struct proc *p));
int	nfs_statvfs __P((struct mount *mp, struct statvfs *sbp, struct proc *p));
int	nfs_sync __P((struct mount *mp, int waitfor, struct ucred *cred,
		struct proc *p));
int	nfs_vget __P((struct mount *, ino_t, struct vnode **));
int	nfs_fhtovp __P((struct mount *mp, struct fid *fhp, struct vnode **vpp));
int	nfs_checkexp __P((struct mount *mp, struct mbuf *nam, int *exflagsp,
		struct ucred **credanonp));
int	nfs_vptofh __P((struct vnode *vp, struct fid *fhp));
int	nfs_fsinfo __P((struct nfsmount *, struct vnode *, struct ucred *,
			struct proc *));
void	nfs_vfs_init __P((void));
void	nfs_vfs_reinit __P((void));
void	nfs_vfs_done __P((void));

/*
 * Prototypes for miscellaneous exported NFS functions.
 */
void	nfs_init __P((void));

#endif /* _KERNEL */

#endif
