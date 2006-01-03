/*	$NetBSD: nfs.h,v 1.53 2006/01/03 11:41:03 yamt Exp $	*/
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
 *	@(#)nfs.h	8.4 (Berkeley) 5/1/95
 */

#ifndef _NFS_NFS_H_
#define _NFS_NFS_H_

/*
 * Tunable constants for nfs
 */

#define	NFS_MAXIOVEC	34
#define NFS_TICKINTVL	5		/* Desired time for a tick (msec) */
#define NFS_HZ		(hz / nfs_ticks) /* Ticks/sec */
#define	NFS_TIMEO	(3 * NFS_HZ)	/* Default timeout = 3 seconds */
#define	NFS_MINTIMEO	(1 * NFS_HZ)	/* Min timeout to use */
#define	NFS_MAXTIMEO	(60 * NFS_HZ)	/* Max timeout to backoff to */
#define	NFS_MINIDEMTIMEO (5 * NFS_HZ)	/* Min timeout for non-idempotent ops*/
#define	NFS_MAXREXMIT	100		/* Stop counting after this many */
#define	NFS_MAXWINDOW	1024		/* Max number of outstanding requests */
#define	NFS_RETRANS	10		/* Num of retrans for soft mounts */
#define	NFS_MAXGRPS	16		/* Max. size of groups list */
#ifndef NFS_MINATTRTIMO
#define	NFS_MINATTRTIMO 5		/* Attribute cache timeout in sec */
#endif
#ifndef NFS_MAXATTRTIMO
#define	NFS_MAXATTRTIMO 60
#endif
#define	NFS_TRYLATERDEL	1		/* Initial try later delay (sec) */
#define	NFS_TRYLATERDELMAX (1*60)	/* Maximum try later delay (sec) */
#define	NFS_TRYLATERDELMUL 2		/* Exponential backoff multiplier */

/*
 * These can be overridden through <machine/param.h>, included via
 * <sys/param.h>. This means that <sys/param.h> should always be
 * included before this file.
 */
#ifndef NFS_WSIZE
#define	NFS_WSIZE	8192		/* Def. write data size */
#endif
#ifndef NFS_RSIZE
#define	NFS_RSIZE	8192		/* Def. read data size */
#endif
#ifndef NFS_READDIRSIZE
#define NFS_READDIRSIZE	8192		/* Def. readdir size */
#endif

/*
 * NFS client IO daemon threads. May be overridden by config options.
 */
#ifndef NFS_MAXASYNCDAEMON
#define	NFS_MAXASYNCDAEMON 	128	/* Max. number async_daemons runable */
#endif

/*
 * NFS client read-ahead. May be overridden by config options.
 * Should be no more than NFS_MAXASYNCDAEMON as each read-ahead operation
 * requires one IO thread.
 */
#ifndef NFS_MAXRAHEAD
#define	NFS_MAXRAHEAD	32		/* Max. read ahead # blocks */
#endif
#define	NFS_DEFRAHEAD	2		/* Def. read ahead # blocks */

#define	NFS_MAXUIDHASH	64		/* Max. # of hashed uid entries/mp */

#ifdef _KERNEL
extern int nfs_niothreads;              /* Number of async_daemons desired */
#ifndef NFS_DEFAULT_NIOTHREADS
#define NFS_DEFAULT_NIOTHREADS 4
#endif
#endif
#define NFS_MAXGATHERDELAY	100	/* Max. write gather delay (msec) */
#ifndef NFS_GATHERDELAY
#define NFS_GATHERDELAY		10	/* Default write gather delay (msec) */
#endif

/*
 * NFS_DIRBLKSIZ is the size of buffers in the buffer cache used for
 * NFS directory vnodes. NFS_DIRFRAGSIZ is the minimum aligned amount
 * of data in those buffers, and thus the minimum amount of data
 * that you can request. NFS_DIRFRAGSIZ should be no smaller than
 * DIRBLKSIZ.
 */

#define	NFS_DIRBLKSIZ	8192		/* Must be a multiple of DIRBLKSIZ */
#define NFS_DIRFRAGSIZ	 512		/* Same as DIRBLKSIZ, generally */

/*
 * Maximum number of directory entries cached per NFS node, to avoid
 * having this grow without bounds on very large directories. The
 * minimum size to get reasonable performance for emulated binaries
 * is the maximum number of entries that fits in NFS_DIRBLKSIZ.
 * For NFS_DIRBLKSIZ = 512, this would be 512 / 14 = 36.
 */
#define NFS_MAXDIRCACHE	(NFS_DIRBLKSIZ / 14)

/*
 * Oddballs
 */
#define NFS_CMPFH(n, f, s) \
	((n)->n_fhsize == (s) && !memcmp((caddr_t)(n)->n_fhp,  (caddr_t)(f),  (s)))
#ifdef NFS_V2_ONLY
#define NFS_ISV3(v)	(0)
#else
#define NFS_ISV3(v)	(VFSTONFS((v)->v_mount)->nm_flag & NFSMNT_NFSV3)
#endif
#define NFS_SRVMAXDATA(n) \
		(((n)->nd_flag & ND_NFSV3) ? (((n)->nd_nam2) ? \
		 NFS_MAXDGRAMDATA : NFS_MAXDATA) : NFS_V2MAXDATA)

/*
 * Use the vm_page flag reserved for pager use to indicate pages
 * which have been written to the server but not yet committed.
 */
#define PG_NEEDCOMMIT PG_PAGER1

/*
 * The IO_METASYNC flag should be implemented for local file systems.
 * (Until then, it is nothin at all.)
 */
#ifndef IO_METASYNC
#define IO_METASYNC	0
#endif

/*
 * Set the attribute timeout based on how recently the file has been modified.
 */
#define	NFS_ATTRTIMEO(nmp, np) \
    ((nmp->nm_flag & NFSMNT_NOAC) ? 0 : \
	((((np)->n_flag & NMODIFIED) || \
	 (time.tv_sec - (np)->n_mtime.tv_sec) / 10 < NFS_MINATTRTIMO) ? NFS_MINATTRTIMO : \
	 ((time.tv_sec - (np)->n_mtime.tv_sec) / 10 > NFS_MAXATTRTIMO ? NFS_MAXATTRTIMO : \
	  (time.tv_sec - (np)->n_mtime.tv_sec) / 10)))

/*
 * Export arguments for local filesystem mount calls.
 * Keep in mind that changing this structure modifies nfssvc(2)'s ABI (see
 * 'struct mountd_exports_list' below).
 * When modifying this structure, take care to also edit the
 * nfs_update_exports_30 function in nfs_export.c accordingly to convert
 * export_args to export_args30.
 */
struct export_args {
	int	ex_flags;		/* export related flags */
	uid_t	ex_root;		/* mapping for root uid */
	struct	uucred ex_anon;		/* mapping for anonymous user */
	struct	sockaddr *ex_addr;	/* net address to which exported */
	int	ex_addrlen;		/* and the net address length */
	struct	sockaddr *ex_mask;	/* mask of valid bits in saddr */
	int	ex_masklen;		/* and the smask length */
	char	*ex_indexfile;		/* index file for WebNFS URLs */
};

/*
 * Structures for the nfssvc(2) syscall. Not that anyone but mountd, nfsd and
 * mount_nfs should ever try and use it.
 */
struct nfsd_args {
	int	sock;		/* Socket to serve */
	caddr_t	name;		/* Client addr for connection based sockets */
	int	namelen;	/* Length of name */
};

struct nfsd_srvargs {
	struct nfsd	*nsd_nfsd;	/* Pointer to in kernel nfsd struct */
	uid_t		nsd_uid;	/* Effective uid mapped to cred */
	u_int32_t	nsd_haddr;	/* Ip address of client */
	struct uucred	nsd_cr;		/* Cred. uid maps to */
	int		nsd_authlen;	/* Length of auth string (ret) */
	u_char		*nsd_authstr;	/* Auth string (ret) */
	int		nsd_verflen;	/* and the verfier */
	u_char		*nsd_verfstr;
	struct timeval	nsd_timestamp;	/* timestamp from verifier */
	u_int32_t	nsd_ttl;	/* credential ttl (sec) */
	NFSKERBKEY_T	nsd_key;	/* Session key */
};

struct nfsd_cargs {
	char		*ncd_dirp;	/* Mount dir path */
	uid_t		ncd_authuid;	/* Effective uid */
	int		ncd_authtype;	/* Type of authenticator */
	u_int		ncd_authlen;	/* Length of authenticator string */
	u_char		*ncd_authstr;	/* Authenticator string */
	u_int		ncd_verflen;	/* and the verifier */
	u_char		*ncd_verfstr;
	NFSKERBKEY_T	ncd_key;	/* Session key */
};

struct mountd_exports_list {
	const char		*mel_path;
	size_t			mel_nexports;
	struct export_args	*mel_exports;
};

/*
 * Stats structure
 */
struct nfsstats {
	int	attrcache_hits;
	int	attrcache_misses;
	int	lookupcache_hits;
	int	lookupcache_misses;
	int	direofcache_hits;
	int	direofcache_misses;
	int	biocache_reads;
	int	read_bios;
	int	read_physios;
	int	biocache_writes;
	int	write_bios;
	int	write_physios;
	int	biocache_readlinks;
	int	readlink_bios;
	int	biocache_readdirs;
	int	readdir_bios;
	int	rpccnt[NFS_NPROCS];
	int	rpcretries;
	int	srvrpccnt[NFS_NPROCS];
	int	srvrpc_errs;
	int	srv_errs;
	int	rpcrequests;
	int	rpctimeouts;
	int	rpcunexpected;
	int	rpcinvalid;
	int	srvcache_inproghits;
	int	srvcache_idemdonehits;
	int	srvcache_nonidemdonehits;
	int	srvcache_misses;
	int	srvnqnfs_leases;
	int	srvnqnfs_maxleases;
	int	srvnqnfs_getleases;
	int	srvvop_writes;
};

/*
 * Flags for nfssvc() system call.
 */
#define	NFSSVC_BIOD	0x002
#define	NFSSVC_NFSD	0x004
#define	NFSSVC_ADDSOCK	0x008
#define	NFSSVC_AUTHIN	0x010
#define	NFSSVC_GOTAUTH	0x040
#define	NFSSVC_AUTHINFAIL 0x080
#define	NFSSVC_MNTD	0x100
#define	NFSSVC_SETEXPORTSLIST	0x200

/*
 * fs.nfs sysctl(3) identifiers
 */
#define NFS_NFSSTATS	1		/* struct: struct nfsstats */
#define NFS_IOTHREADS	2		/* number of io threads */
#define	NFS_MAXID	3

#define NFS_NAMES { \
	{ 0, 0 }, \
	{ "nfsstats", CTLTYPE_STRUCT }, \
	{ "iothreads", CTLTYPE_INT }, \
}

/*
 * The set of signals the interrupt an I/O in progress for NFSMNT_INT mounts.
 * What should be in this set is open to debate, but I believe that since
 * I/O system calls on ufs are never interrupted by signals the set should
 * be minimal. My reasoning is that many current programs that use signals
 * such as SIGALRM will not expect file I/O system calls to be interrupted
 * by them and break.
 */
#ifdef _KERNEL

struct uio; struct buf; struct vattr; struct nameidata;	/* XXX */

/*
 * Socket errors ignored for connectionless sockets??
 * For now, ignore them all
 */
#define	NFSIGNORE_SOERROR(s, e) \
		((e) != EINTR && (e) != ERESTART && (e) != EWOULDBLOCK && \
		((s) & PR_CONNREQUIRED) == 0)

/*
 * Nfs outstanding request list element
 */
struct nfsreq {
	TAILQ_ENTRY(nfsreq) r_chain;
	struct mbuf	*r_mreq;
	struct mbuf	*r_mrep;
	struct mbuf	*r_md;
	caddr_t		r_dpos;
	struct nfsmount *r_nmp;
	u_int32_t	r_xid;
	int		r_flags;	/* flags on request, see below */
	int		r_retry;	/* max retransmission count */
	int		r_rexmit;	/* current retrans count */
	int		r_timer;	/* tick counter on reply */
	u_int32_t	r_procnum;	/* NFS procedure number */
	int		r_rtt;		/* RTT for rpc */
	struct lwp	*r_lwp;		/* LWP that did I/O system call */
};

/*
 * Queue head for nfsreq's
 */
extern TAILQ_HEAD(nfsreqhead, nfsreq) nfs_reqq;

/* Flag values for r_flags */
#define R_TIMING	0x01		/* timing request (in mntp) */
#define R_SENT		0x02		/* request has been sent */
#define	R_SOFTTERM	0x04		/* soft mnt, too many retries */
#define	R_INTR		0x08		/* intr mnt, signal pending */
#define	R_SOCKERR	0x10		/* Fatal error on socket */
#define	R_TPRINTFMSG	0x20		/* Did a tprintf msg. */
#define	R_MUSTRESEND	0x40		/* Must resend request */
#define	R_GETONEREP	0x80		/* Probe for one reply only */
#define	R_REXMITTED	0x100		/* retransmitted after reconnect */

/*
 * A list of nfssvc_sock structures is maintained with all the sockets
 * that require service by the nfsd.
 * The nfsuid structs hang off of the nfssvc_sock structs in both lru
 * and uid hash lists.
 */
#ifndef NFS_UIDHASHSIZ
#define	NFS_UIDHASHSIZ	29	/* Tune the size of nfssvc_sock with this */
#endif
#define	NUIDHASH(sock, uid) \
	(&(sock)->ns_uidhashtbl[(uid) % NFS_UIDHASHSIZ])
#ifndef NFS_WDELAYHASHSIZ
#define	NFS_WDELAYHASHSIZ 16	/* and with this */
#endif
#define	NWDELAYHASH(sock, f) \
	(&(sock)->ns_wdelayhashtbl[(*((u_int32_t *)(f))) % NFS_WDELAYHASHSIZ])
#ifndef NFS_MUIDHASHSIZ
#define NFS_MUIDHASHSIZ	63	/* Tune the size of nfsmount with this */
#endif
#define	NMUIDHASH(nmp, uid) \
	(&(nmp)->nm_uidhashtbl[(uid) % NFS_MUIDHASHSIZ])
#define	NFSNOHASH(fhsum) ((fhsum) & nfsnodehash)

#ifndef NFS_DIRHASHSIZ
#define NFS_DIRHASHSIZ 64
#endif
#define NFSDIRHASH(np, off) \
	(&np->n_dircache[(nfs_dirhash((off)) & nfsdirhashmask)])

/*
 * Macros for storing/retrieving cookies into directory buffers.
 */
#define NFS_STASHCOOKIE(dp,off) \
	*((off_t *)((caddr_t)(dp) + (dp)->d_reclen - sizeof (off_t))) = off
#define NFS_GETCOOKIE(dp) \
	(*((off_t *)((caddr_t)(dp) + (dp)->d_reclen - sizeof (off_t))))
#define NFS_STASHCOOKIE32(dp, val) \
	*((u_int32_t *)((caddr_t)(dp) + (dp)->d_reclen - sizeof (off_t) - \
	    sizeof (int))) = val
#define NFS_GETCOOKIE32(dp) \
	(*((u_int32_t *)((caddr_t)(dp) + (dp)->d_reclen - sizeof (off_t) - \
	    sizeof (int))))

/*
 * Flags passed to nfs_bioread().
 */
#define NFSBIO_CACHECOOKIES	0x0001	/* Cache dir offset cookies */

/*
 * Network address hash list element
 */
union nethostaddr {
	u_int32_t had_inetaddr;
	struct mbuf *had_nam;
};

struct nfsuid {
	TAILQ_ENTRY(nfsuid) nu_lru;	/* LRU chain */
	LIST_ENTRY(nfsuid) nu_hash;	/* Hash list */
	int		nu_flag;	/* Flags */
	union nethostaddr nu_haddr;	/* Host addr. for dgram sockets */
	struct ucred	nu_cr;		/* Cred uid mapped to */
	int		nu_expire;	/* Expiry time (sec) */
	struct timeval	nu_timestamp;	/* Kerb. timestamp */
	u_int32_t	nu_nickname;	/* Nickname on server */
	NFSKERBKEY_T	nu_key;		/* and session key */
};

#define	nu_inetaddr	nu_haddr.had_inetaddr
#define	nu_nam		nu_haddr.had_nam
/* Bits for nu_flag */
#define	NU_INETADDR	0x1
#define NU_NAM		0x2
#ifdef INET6
#define NU_NETFAM(u) \
	(((u)->nu_flag & NU_INETADDR) ? \
	(((u)->nu_flag & NU_NAM) ? AF_INET6 : AF_INET) : AF_ISO)
#else
#define NU_NETFAM(u)	(((u)->nu_flag & NU_INETADDR) ? AF_INET : AF_ISO)
#endif

struct nfssvc_sock {
	struct simplelock ns_lock;
	TAILQ_ENTRY(nfssvc_sock) ns_chain;	/* List of all nfssvc_sock's */
	TAILQ_ENTRY(nfssvc_sock) ns_pending;	/* List of pending sockets */
	TAILQ_HEAD(, nfsuid) ns_uidlruhead;
	struct file	*ns_fp;
	struct socket	*ns_so;
	struct mbuf	*ns_nam;
	struct mbuf	*ns_raw;
	struct mbuf	*ns_rawend;
	struct mbuf	*ns_rec;
	struct mbuf	*ns_recend;
	struct mbuf	*ns_frag;
	int		ns_flag;
	int		ns_solock;
	int		ns_cc;
	int		ns_reclen;
	int		ns_numuids;
	u_int32_t	ns_sref;
	LIST_HEAD(, nfsrv_descript) ns_tq;	/* Write gather lists */
	LIST_HEAD(, nfsuid) ns_uidhashtbl[NFS_UIDHASHSIZ];
	LIST_HEAD(nfsrvw_delayhash, nfsrv_descript) ns_wdelayhashtbl[NFS_WDELAYHASHSIZ];
};

/* Bits for "ns_flag" */
#define	SLP_VALID	0x01
#define	SLP_DOREC	0x02	/* on nfssvc_sockpending queue */
#define	SLP_NEEDQ	0x04
#define	SLP_DISCONN	0x08
#define	SLP_BUSY	0x10
#define	SLP_WANT	0x20
#define	SLP_LASTFRAG	0x40

extern TAILQ_HEAD(nfssvc_sockhead, nfssvc_sock) nfssvc_sockhead;
extern struct nfssvc_sockhead nfssvc_sockpending;
extern int nfssvc_sockhead_flag;
#define	SLP_INIT	0x01
#define	SLP_WANTINIT	0x02

int nfsdsock_lock(struct nfssvc_sock *, boolean_t);
void nfsdsock_unlock(struct nfssvc_sock *);
int nfsdsock_drain(struct nfssvc_sock *);

/*
 * One of these structures is allocated for each nfsd.
 */
struct nfsd {
	TAILQ_ENTRY(nfsd) nfsd_chain;	/* List of all nfsd's */
	SLIST_ENTRY(nfsd) nfsd_idle;	/* List of idle nfsd's */
	int		nfsd_flag;	/* NFSD_ flags */
	struct nfssvc_sock *nfsd_slp;	/* Current socket */
	int		nfsd_authlen;	/* Authenticator len */
	u_char		nfsd_authstr[RPCAUTH_MAXSIZ]; /* Authenticator data */
	int		nfsd_verflen;	/* and the Verifier */
	u_char		nfsd_verfstr[RPCVERF_MAXSIZ];
	struct proc	*nfsd_procp;	/* Proc ptr */
	struct nfsrv_descript *nfsd_nd;	/* Associated nfsrv_descript */
};

/* Bits for "nfsd_flag" */
#define	NFSD_NEEDAUTH	0x04
#define	NFSD_AUTHFAIL	0x08

/*
 * This structure is used by the server for describing each request.
 * Some fields are used only when write request gathering is performed.
 */
struct nfsrv_descript {
	u_quad_t		nd_time;	/* Write deadline (usec) */
	off_t			nd_off;		/* Start byte offset */
	off_t			nd_eoff;	/* and end byte offset */
	LIST_ENTRY(nfsrv_descript) nd_hash;	/* Hash list */
	LIST_ENTRY(nfsrv_descript) nd_tq;		/* and timer list */
	LIST_HEAD(,nfsrv_descript) nd_coalesce;	/* coalesced writes */
	struct mbuf		*nd_mrep;	/* Request mbuf list */
	struct mbuf		*nd_md;		/* Current dissect mbuf */
	struct mbuf		*nd_mreq;	/* Reply mbuf list */
	struct mbuf		*nd_nam;	/* and socket addr */
	struct mbuf		*nd_nam2;	/* return socket addr */
	caddr_t			nd_dpos;	/* Current dissect pos */
	u_int32_t		nd_procnum;	/* RPC # */
	int			nd_stable;	/* storage type */
	int			nd_flag;	/* nd_flag */
	int			nd_len;		/* Length of this write */
	int			nd_repstat;	/* Reply status */
	u_int32_t		nd_retxid;	/* Reply xid */
	u_int32_t		nd_duration;	/* Lease duration */
	struct timeval		nd_starttime;	/* Time RPC initiated */
	fhandle_t		nd_fh;		/* File handle */
	struct ucred		nd_cr;		/* Credentials */
};

/* Bits for "nd_flag" */
#define	ND_READ		LEASE_READ
#define ND_WRITE	LEASE_WRITE
#define ND_CHECK	0x04
#define ND_LEASE	(ND_READ | ND_WRITE | ND_CHECK)
#define ND_NFSV3	0x08
#define ND_NQNFS	0x10
#define ND_KERBNICK	0x20
#define ND_KERBFULL	0x40
#define ND_KERBAUTH	(ND_KERBNICK | ND_KERBFULL)

extern struct simplelock nfsd_slock;
extern TAILQ_HEAD(nfsdhead, nfsd) nfsd_head;
extern SLIST_HEAD(nfsdidlehead, nfsd) nfsd_idle_head;
extern int nfsd_head_flag;
#define	NFSD_CHECKSLP	0x01

extern struct mowner nfs_mowner;
extern struct nfsstats nfsstats;
extern int nfs_numasync;

/*
 * These macros compare nfsrv_descript structures.
 */
#define NFSW_CONTIG(o, n) \
		((o)->nd_eoff >= (n)->nd_off && \
		 !memcmp((caddr_t)&(o)->nd_fh, (caddr_t)&(n)->nd_fh, NFSX_V3FH))

#define NFSW_SAMECRED(o, n) \
	(((o)->nd_flag & ND_KERBAUTH) == ((n)->nd_flag & ND_KERBAUTH) && \
 	 !memcmp((caddr_t)&(o)->nd_cr, (caddr_t)&(n)->nd_cr, \
		sizeof (struct ucred)))

/*
 * Defines for WebNFS
 */

#define WEBNFS_ESC_CHAR		'%'
#define WEBNFS_SPECCHAR_START	0x80

#define WEBNFS_NATIVE_CHAR	0x80
/*
 * ..
 * Possibly more here in the future.
 */

/*
 * Macro for converting escape characters in WebNFS pathnames.
 * Should really be in libkern.
 */

#define HEXTOC(c) \
	((c) >= 'a' ? ((c) - ('a' - 10)) : \
	    ((c) >= 'A' ? ((c) - ('A' - 10)) : ((c) - '0')))
#define HEXSTRTOI(p) \
	((HEXTOC(p[0]) << 4) + HEXTOC(p[1]))

/*
 * Structure holding information for a publicly exported filesystem
 * (WebNFS).  Currently the specs allow just for one such filesystem.
 */
struct nfs_public {
	int		np_valid;	/* Do we hold valid information */
	fhandle_t	np_handle;	/* Filehandle for pub fs (internal) */
	struct mount	*np_mount;	/* Mountpoint of exported fs */
	char		*np_index;	/* Index file */
};
#endif	/* _KERNEL */

#endif /* _NFS_NFS_H */
