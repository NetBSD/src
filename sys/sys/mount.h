/*	$NetBSD: mount.h,v 1.85 2000/06/10 18:44:44 sommerfeld Exp $	*/

/*
 * Copyright (c) 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)mount.h	8.21 (Berkeley) 5/20/95
 */

#ifndef _SYS_MOUNT_H_
#define _SYS_MOUNT_H_

#ifndef _KERNEL
#include <sys/ucred.h>
#if !defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE)
#include <sys/stat.h>
#endif /* !_POSIX_C_SOURCE */
#endif
#include <sys/queue.h>
#include <sys/lock.h>

typedef struct { int32_t val[2]; } fsid_t;	/* file system id type */

/*
 * File identifier.
 * These are unique per filesystem on a single machine.
 */
#define	MAXFIDSZ	16

struct fid {
	u_short		fid_len;		/* length of data in bytes */
	u_short		fid_reserved;		/* force longword alignment */
	char		fid_data[MAXFIDSZ];	/* data (variable length) */
};

/*
 * file system statistics
 */

#define	MFSNAMELEN	16	/* length of fs type name, including nul */
#define	MNAMELEN	90	/* length of buffer for returned name */

struct statfs {
	short	f_type;			/* type of file system */
	u_short	f_oflags;		/* deprecated copy of mount flags */
	long	f_bsize;		/* fundamental file system block size */
	long	f_iosize;		/* optimal transfer block size */
	long	f_blocks;		/* total data blocks in file system */
	long	f_bfree;		/* free blocks in fs */
	long	f_bavail;		/* free blocks avail to non-superuser */
	long	f_files;		/* total file nodes in file system */
	long	f_ffree;		/* free file nodes in fs */
	fsid_t	f_fsid;			/* file system id */
	uid_t	f_owner;		/* user that mounted the file system */
	long	f_flags;		/* copy of mount flags */
	long	f_syncwrites;		/* count of sync writes since mount */
	long	f_asyncwrites;		/* count of async writes since mount */
	long	f_spare[1];		/* spare for later */
	char	f_fstypename[MFSNAMELEN]; /* fs type name */
	char	f_mntonname[MNAMELEN];	  /* directory on which mounted */
	char	f_mntfromname[MNAMELEN];  /* mounted file system */
};

/*
 * File system types.
 */
#define	MOUNT_FFS	"ffs"		/* UNIX "Fast" Filesystem */
#define	MOUNT_UFS	MOUNT_FFS	/* for compatibility */
#define	MOUNT_NFS	"nfs"		/* Network Filesystem */
#define	MOUNT_MFS	"mfs"		/* Memory Filesystem */
#define	MOUNT_MSDOS	"msdos"		/* MSDOS Filesystem */
#define	MOUNT_LFS	"lfs"		/* Log-based Filesystem */
#define	MOUNT_FDESC	"fdesc"		/* File Descriptor Filesystem */
#define	MOUNT_PORTAL	"portal"	/* Portal Filesystem */
#define	MOUNT_NULL	"null"		/* Minimal Filesystem Layer */
#define	MOUNT_OVERLAY	"overlay"	/* Minimal Overlay Filesystem Layer */
#define	MOUNT_UMAP	"umap"	/* User/Group Identifier Remapping Filesystem */
#define	MOUNT_KERNFS	"kernfs"	/* Kernel Information Filesystem */
#define	MOUNT_PROCFS	"procfs"	/* /proc Filesystem */
#define	MOUNT_AFS	"afs"		/* Andrew Filesystem */
#define	MOUNT_CD9660	"cd9660"	/* ISO9660 (aka CDROM) Filesystem */
#define	MOUNT_UNION	"union"		/* Union (translucent) Filesystem */
#define	MOUNT_ADOSFS	"adosfs"	/* AmigaDOS Filesystem */
#define	MOUNT_EXT2FS	"ext2fs"	/* Second Extended Filesystem */
#define	MOUNT_CFS	"coda"		/* Coda Filesystem */
#define	MOUNT_CODA	"coda"		/* Coda Filesystem */
#define	MOUNT_FILECORE	"filecore"	/* Acorn Filecore Filesystem */
#define	MOUNT_NTFS	"ntfs"		/* Windows/NT Filesystem */

/*
 * Structure per mounted file system.  Each mounted file system has an
 * array of operations and an instance record.  The file systems are
 * put on a doubly linked list.
 */
LIST_HEAD(vnodelst, vnode);

struct mount {
	CIRCLEQ_ENTRY(mount) mnt_list;		/* mount list */
	struct vfsops	*mnt_op;		/* operations on fs */
	struct vnode	*mnt_vnodecovered;	/* vnode we mounted on */
	struct vnode	*mnt_syncer;		/* syncer vnode */
	struct vnodelst	mnt_vnodelist;		/* list of vnodes this mount */
	struct lock	mnt_lock;		/* mount structure lock */
	int		mnt_flag;		/* flags */
	int		mnt_maxsymlinklen;	/* max size of short symlink */
	struct statfs	mnt_stat;		/* cache of filesystem stats */
	qaddr_t		mnt_data;		/* private data */
	int		mnt_wcnt;		/* count of vfs_busy waiters */
	struct proc	*mnt_unmounter;		/* who is unmounting */
};

/*
 * Mount flags.  XXX BEWARE: these are not in numerical order!
 *
 * Unmount uses MNT_FORCE flag.
 *
 * Note that all mount flags are listed here.  if you need to add one, take
 * one of the __MNT_UNUSED flags.
 */

#define __MNT_UNUSED1	0x00100000
#define __MNT_UNUSED2	0x00400000
#define __MNT_UNUSED3	0x00800000

#define	MNT_RDONLY	0x00000001	/* read only filesystem */
#define	MNT_SYNCHRONOUS	0x00000002	/* file system written synchronously */
#define	MNT_NOEXEC	0x00000004	/* can't exec from filesystem */
#define	MNT_NOSUID	0x00000008	/* don't honor setuid bits on fs */
#define	MNT_NODEV	0x00000010	/* don't interpret special files */
#define	MNT_UNION	0x00000020	/* union with underlying filesystem */
#define	MNT_ASYNC	0x00000040	/* file system written asynchronously */
#define	MNT_NOCOREDUMP	0x00008000	/* don't write core dumps to this FS */
#define MNT_NOATIME	0x04000000	/* Never update access times in fs */
#define MNT_SYMPERM	0x20000000	/* recognize symlink permission */
#define MNT_NODEVMTIME	0x40000000	/* Never update mod times for devs */
#define MNT_SOFTDEP	0x80000000	/* Use soft dependencies */

/*
 * exported mount flags.
 */
#define	MNT_EXRDONLY	0x00000080	/* exported read only */
#define	MNT_EXPORTED	0x00000100	/* file system is exported */
#define	MNT_DEFEXPORTED	0x00000200	/* exported to the world */
#define	MNT_EXPORTANON	0x00000400	/* use anon uid mapping for everyone */
#define	MNT_EXKERB	0x00000800	/* exported with Kerberos uid mapping */
#define MNT_EXNORESPORT	0x08000000	/* don't enforce reserved ports (NFS) */
#define MNT_EXPUBLIC	0x10000000	/* public export (WebNFS) */

/*
 * Flags set by internal operations.
 */
#define	MNT_LOCAL	0x00001000	/* filesystem is stored locally */
#define	MNT_QUOTA	0x00002000	/* quotas are enabled on filesystem */
#define	MNT_ROOTFS	0x00004000	/* identifies the root filesystem */

/*
 * Mask of flags that are visible to statfs()
 */
#define	MNT_VISFLAGMASK	0xfc00ffff

/*
 * External filesystem control flags.
 *
 * MNT_MLOCK lock the mount entry so that name lookup cannot proceed
 * past the mount point.  This keeps the subtree stable during mounts
 * and unmounts.
 */
#define	MNT_UPDATE	0x00010000	/* not a real mount, just an update */
#define	MNT_DELEXPORT	0x00020000	/* delete export host lists */
#define	MNT_RELOAD	0x00040000	/* reload filesystem data */
#define	MNT_FORCE	0x00080000	/* force unmount or readonly change */

/*
 * Internal filesystem control flags.
 *
 * MNT_UNMOUNT locks the mount entry so that name lookup cannot proceed
 * past the mount point.  This keeps the subtree stable during mounts
 * and unmounts.
 */
#define	MNT_GONE	0x00200000	/* filesystem is gone.. */
#define MNT_UNMOUNT	0x01000000	/* unmount in progress */
#define MNT_WANTRDWR	0x02000000	/* upgrade to read/write requested */

/*
 * Sysctl CTL_VFS definitions.
 *
 * Second level identifier specifies which filesystem. Second level
 * identifier VFS_GENERIC returns information about all filesystems.
 *
 * Note the slightly non-flat nature of these sysctl numbers.  Oh for
 * a better sysctl interface.
 */
#define VFS_GENERIC	0		/* generic filesystem information */
#define VFS_MAXTYPENUM	1		/* int: highest defined fs type */
#define VFS_CONF	2		/* struct: vfsconf for filesystem given
					   as next argument */
#define VFS_USERMOUNT	3		/* enable/disable fs mnt by non-root */
#define	VFSGEN_MAXID	4		/* number of valid vfs.generic ids */

/*
 * XXX NOTE!  These must be in the order of mountcompatnames[] in
 * XXX sys/kern/vfs_syscalls.c!
 *
 * USE THE SAME NAMES AS MOUNT_*!
 */
#define	CTL_VFS_NAMES { \
	{ "generic", CTLTYPE_NODE }, \
	{ MOUNT_FFS, CTLTYPE_NODE }, \
	{ MOUNT_NFS, CTLTYPE_NODE }, \
	{ MOUNT_MFS, CTLTYPE_NODE }, \
	{ MOUNT_MSDOS, CTLTYPE_NODE }, \
	{ MOUNT_LFS, CTLTYPE_NODE }, \
	{ 0, 0 }, 			/* MOUNT_LOFS */ \
	{ MOUNT_FDESC, CTLTYPE_NODE }, \
	{ MOUNT_PORTAL, CTLTYPE_NODE }, \
	{ MOUNT_NULL, CTLTYPE_NODE }, \
	{ MOUNT_UMAP, CTLTYPE_NODE }, \
	{ MOUNT_KERNFS, CTLTYPE_NODE }, \
	{ MOUNT_PROCFS, CTLTYPE_NODE }, \
	{ MOUNT_AFS, CTLTYPE_NODE }, \
	{ MOUNT_CD9660, CTLTYPE_NODE }, \
	{ MOUNT_UNION, CTLTYPE_NODE }, \
	{ MOUNT_ADOSFS, CTLTYPE_NODE }, \
	{ MOUNT_EXT2FS, CTLTYPE_NODE }, \
	{ MOUNT_CODA, CTLTYPE_NODE }, \
	{ MOUNT_FILECORE, CTLTYPE_NODE }, \
	{ MOUNT_NTFS, CTLTYPE_NODE }, \
}

#define	VFS_MAXID	20		/* number of valid vfs ids */

#define	CTL_VFSGENCTL_NAMES { \
	{ 0, 0 }, \
	{ "maxtypenum", CTLTYPE_INT }, \
	{ "conf", CTLTYPE_NODE }, 	/* Special */ \
	{ "usermount", CTLTYPE_INT }, \
}

/*
 * Operations supported on mounted file system.
 */
#ifdef _KERNEL

#if defined(COMPAT_09) || defined(COMPAT_43) || defined(COMPAT_44)

/*
 * Filesystem configuration information. Not used by NetBSD, but
 * defined here to provide a compatible sysctl interface to Lite2.
 */
struct vfsconf {
	struct	vfsops *vfc_vfsops;	/* filesystem operations vector */
	char	vfc_name[MFSNAMELEN]; 	/* filesystem type name */
	int	vfc_typenum;		/* historic filesystem type number */
	int  	vfc_refcount;		/* number mounted of this type */
	int	vfc_flags;		/* permanent flags */
	int	(*vfc_mountroot)(void);	/* if != NULL, routine to mount root */
	struct	vfsconf *vfc_next; 	/* next in list */
};

#endif

#ifdef __STDC__
struct nameidata;
struct mbuf;
struct vnodeopv_desc;
#endif

struct vfsops {
	const char *vfs_name;
	int	(*vfs_mount)	__P((struct mount *mp, const char *path,
				    void *data, struct nameidata *ndp,
				    struct proc *p));
	int	(*vfs_start)	__P((struct mount *mp, int flags,
				    struct proc *p));
	int	(*vfs_unmount)	__P((struct mount *mp, int mntflags,
				    struct proc *p));
	int	(*vfs_root)	__P((struct mount *mp, struct vnode **vpp));
	int	(*vfs_quotactl)	__P((struct mount *mp, int cmds, uid_t uid,
				    caddr_t arg, struct proc *p));
	int	(*vfs_statfs)	__P((struct mount *mp, struct statfs *sbp,
				    struct proc *p));
	int	(*vfs_sync)	__P((struct mount *mp, int waitfor,
				    struct ucred *cred, struct proc *p));
	int	(*vfs_vget)	__P((struct mount *mp, ino_t ino,
				    struct vnode **vpp));
	int	(*vfs_fhtovp)	__P((struct mount *mp, struct fid *fhp,
				    struct vnode **vpp));
	int	(*vfs_vptofh)	__P((struct vnode *vp, struct fid *fhp));
	void	(*vfs_init)	__P((void));
	void	(*vfs_done)	__P((void));
	int	(*vfs_sysctl)	__P((int *, u_int, void *, size_t *, void *,
				    size_t, struct proc *));
	int	(*vfs_mountroot) __P((void));
	int	(*vfs_checkexp) __P((struct mount *mp, struct mbuf *nam,
				    int *extflagsp, struct ucred **credanonp));
	struct vnodeopv_desc **vfs_opv_descs;
	int	vfs_refcount;
	LIST_ENTRY(vfsops) vfs_list;
};

#define VFS_MOUNT(MP, PATH, DATA, NDP, P) \
	(*(MP)->mnt_op->vfs_mount)(MP, PATH, DATA, NDP, P)
#define VFS_START(MP, FLAGS, P)	  (*(MP)->mnt_op->vfs_start)(MP, FLAGS, P)
#define VFS_UNMOUNT(MP, FORCE, P) (*(MP)->mnt_op->vfs_unmount)(MP, FORCE, P)
#define VFS_ROOT(MP, VPP)	  (*(MP)->mnt_op->vfs_root)(MP, VPP)
#define VFS_QUOTACTL(MP,C,U,A,P)  (*(MP)->mnt_op->vfs_quotactl)(MP, C, U, A, P)
#define VFS_STATFS(MP, SBP, P)	  (*(MP)->mnt_op->vfs_statfs)(MP, SBP, P)
#define VFS_SYNC(MP, WAIT, C, P)  (*(MP)->mnt_op->vfs_sync)(MP, WAIT, C, P)
#define VFS_VGET(MP, INO, VPP)	  (*(MP)->mnt_op->vfs_vget)(MP, INO, VPP)
#define VFS_FHTOVP(MP, FIDP, VPP) (*(MP)->mnt_op->vfs_fhtovp)(MP, FIDP, VPP)
#define VFS_CHECKEXP(MP, NAM, EXFLG, CRED) \
	(*(MP)->mnt_op->vfs_checkexp)(MP, NAM, EXFLG, CRED)
#define	VFS_VPTOFH(VP, FIDP)	  (*(VP)->v_mount->mnt_op->vfs_vptofh)(VP, FIDP)
#endif /* _KERNEL */

/*
 * Flags for various system call interfaces.
 *
 * waitfor flags to vfs_sync() and getfsstat()
 */
#define MNT_WAIT	1	/* synchronously wait for I/O to complete */
#define MNT_NOWAIT	2	/* start all I/O, but do not wait for it */
#define MNT_LAZY 	3	/* push data not written by filesystem syncer */

/*
 * Generic file handle
 */
struct fhandle {
	fsid_t	fh_fsid;	/* File system id of mount point */
	struct	fid fh_fid;	/* File sys specific id */
};
typedef struct fhandle	fhandle_t;

#ifdef _KERNEL
#include <net/radix.h>
#include <sys/socket.h>		/* XXX for AF_MAX */

/*
 * Network address lookup element
 */
struct netcred {
	struct	radix_node netc_rnodes[2];
	int	netc_exflags;
	struct	ucred netc_anon;
};

/*
 * Network export information
 */
struct netexport {
	struct	netcred ne_defexported;		      /* Default export */
	struct	radix_node_head *ne_rtable[AF_MAX+1]; /* Individual exports */
};
#endif /* _KERNEL */

/*
 * Export arguments for local filesystem mount calls.
 */
struct export_args {
	int	ex_flags;		/* export related flags */
	uid_t	ex_root;		/* mapping for root uid */
	struct	ucred ex_anon;		/* mapping for anonymous user */
	struct	sockaddr *ex_addr;	/* net address to which exported */
	int	ex_addrlen;		/* and the net address length */
	struct	sockaddr *ex_mask;	/* mask of valid bits in saddr */
	int	ex_masklen;		/* and the smask length */
	char	*ex_indexfile;		/* index file for WebNFS URLs */
};

/*
 * Structure holding information for a publicly exported filesystem
 * (WebNFS). Currently the specs allow just for one such filesystem.
 */
struct nfs_public {
	int		np_valid;	/* Do we hold valid information */
	fhandle_t	np_handle;	/* Filehandle for pub fs (internal) */
	struct mount	*np_mount;	/* Mountpoint of exported fs */
	char		*np_index;	/* Index file */
};

#ifdef _KERNEL
/*
 * exported vnode operations
 */
struct	mount *vfs_getvfs __P((fsid_t *));    /* return vfs given fsid */
int	vfs_export			    /* process mount export info */
	  __P((struct mount *, struct netexport *, struct export_args *));
struct	netcred *vfs_export_lookup	    /* lookup host in fs export list */
	  __P((struct mount *, struct netexport *, struct mbuf *));
int	vfs_setpublicfs			    /* set publicly exported fs */
	  __P((struct mount *, struct netexport *, struct export_args *));
int	vfs_lock __P((struct mount *));	    /* lock a vfs */
int	vfs_mountedon __P((struct vnode *));/* is a vfs mounted on vp */
int	vfs_mountroot __P((void));
void	vfs_shutdown __P((void));	    /* unmount and sync file systems */
void	vfs_unlock __P((struct mount *));   /* unlock a vfs */
void	vfs_unmountall __P((struct proc *));	    /* unmount file systems */
int 	vfs_busy __P((struct mount *, int, struct simplelock *));
int	vfs_rootmountalloc __P((char *, char *, struct mount **));
void	vfs_unbusy __P((struct mount *));
int	vfs_attach __P((struct vfsops *));
int	vfs_detach __P((struct vfsops *));
struct vfsops *vfs_getopsbyname __P((const char *));
int	vfs_sysctl __P((int *, u_int, void *, size_t *, void *, size_t,
			struct proc *));

extern	CIRCLEQ_HEAD(mntlist, mount) mountlist;	/* mounted filesystem list */
extern	struct vfsops *vfssw[];			/* filesystem type table */
extern	int nvfssw;
extern	struct nfs_public nfs_pub;
extern	struct simplelock mountlist_slock;
extern	struct simplelock spechash_slock;
long	makefstype __P((const char *));
int	dounmount __P((struct mount *, int, struct proc *));
void	vfsinit __P((void));
void	vfs_opv_init __P((struct vnodeopv_desc **));
void	vfs_opv_free __P((struct vnodeopv_desc **));
#ifdef DEBUG
void	vfs_bufstats __P((void));
#endif

LIST_HEAD(vfs_list_head, vfsops);
extern struct vfs_list_head vfs_list;

#else /* _KERNEL */

#include <sys/cdefs.h>

__BEGIN_DECLS
int	fstatfs __P((int, struct statfs *));
int	getfh __P((const char *, fhandle_t *));
int	getfsstat __P((struct statfs *, long, int));
int	getmntinfo __P((struct statfs **, int));
int	mount __P((const char *, const char *, int, void *));
int	statfs __P((const char *, struct statfs *));
int	unmount __P((const char *, int));
#if !defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE)
int	fhopen __P((const fhandle_t *, int));
int	fhstat __P((const fhandle_t *, struct stat *));
int	fhstatfs __P((const fhandle_t *, struct statfs *));
#endif /* !_POSIX_C_SOURCE */
__END_DECLS

#endif /* _KERNEL */

#endif /* !_SYS_MOUNT_H_ */
