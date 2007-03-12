/*	$NetBSD: mount.h,v 1.152.2.1 2007/03/12 06:00:53 rmind Exp $	*/

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
 *	@(#)mount.h	8.21 (Berkeley) 5/20/95
 */

#ifndef _SYS_MOUNT_H_
#define _SYS_MOUNT_H_

#ifndef _KERNEL
#include <sys/featuretest.h>
#include <sys/ucred.h>
#if defined(_NETBSD_SOURCE)
#include <sys/stat.h>
#endif /* _NETBSD_SOURCE */
#endif
#include <sys/fstypes.h>
#include <sys/queue.h>
#include <sys/lock.h>
#include <sys/statvfs.h>
#include <sys/specificdata.h>

/*
 * file system statistics
 */

#define	MFSNAMELEN	16	/* length of fs type name, including nul */
#define	MNAMELEN	90	/* length of buffer for returned name */

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
#define	MOUNT_CODA	MOUNT_CFS	/* Coda Filesystem */
#define	MOUNT_FILECORE	"filecore"	/* Acorn Filecore Filesystem */
#define	MOUNT_NTFS	"ntfs"		/* Windows/NT Filesystem */
#define	MOUNT_SMBFS	"smbfs"		/* CIFS (SMB) */
#define	MOUNT_PTYFS	"ptyfs"		/* Pseudo tty filesystem */
#define	MOUNT_TMPFS	"tmpfs"		/* Efficient memory file-system */
#define MOUNT_UDF	"udf"		/* UDF CD/DVD filesystem */
#define	MOUNT_SYSVBFS	"sysvbfs"	/* System V Boot Filesystem */
#define MOUNT_PUFFS	"puffs"		/* Pass-to-Userspace filesystem */
#define MOUNT_HFS	"hfs"		/* Apple HFS+ Filesystem */

/*
 * Structure per mounted file system.  Each mounted file system has an
 * array of operations and an instance record.  The file systems are
 * put on a doubly linked list.
 */
TAILQ_HEAD(vnodelst, vnode);

struct mount {
	CIRCLEQ_ENTRY(mount) mnt_list;		/* mount list */
	struct vfsops	*mnt_op;		/* operations on fs */
	struct vnode	*mnt_vnodecovered;	/* vnode we mounted on */
	struct vnode	*mnt_syncer;		/* syncer vnode */
	struct vnodelst	mnt_vnodelist;		/* list of vnodes this mount */
	struct lock	mnt_lock;		/* mount structure lock */
	int		mnt_flag;		/* flags */
	int		mnt_iflag;		/* internal flags */
	int		mnt_fs_bshift;		/* offset shift for lblkno */
	int		mnt_dev_bshift;		/* shift for device sectors */
	struct statvfs	mnt_stat;		/* cache of filesystem stats */
	void		*mnt_data;		/* private data */
	int		mnt_wcnt;		/* count of vfs_busy waiters */
	struct lwp	*mnt_unmounter;		/* who is unmounting */
	int		mnt_writeopcountupper;	/* upper writeops in progress */
	int		mnt_writeopcountlower;	/* lower writeops in progress */
	struct simplelock mnt_slock;		/* mutex for wcnt and
						   writeops counters */
	struct mount	*mnt_leaf;		/* leaf fs we mounted on */
	specificdata_reference
			mnt_specdataref;	/* subsystem specific data */
};

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
#define	VFS_MAGICLINKS  4		/* expand 'magic' symlinks */
#define	VFSGEN_MAXID	5		/* number of valid vfs.generic ids */

#ifndef _STANDALONE
/*
 * USE THE SAME NAMES AS MOUNT_*!
 *
 * Only need to add new entry here if the filesystem actually supports
 * sysctl(2).
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
	{ "magiclinks", CTLTYPE_INT }, \
}

/*
 * Operations supported on mounted file system.
 */
#ifdef _KERNEL

#if __STDC__
struct nameidata;
struct mbuf;
struct vnodeopv_desc;
struct kauth_cred;
#endif

#define VFS_PROTOS(fsname)						\
int	fsname##_mount(struct mount *, const char *, void *,		\
		struct nameidata *, struct lwp *);			\
int	fsname##_start(struct mount *, int, struct lwp *);		\
int	fsname##_unmount(struct mount *, int, struct lwp *);		\
int	fsname##_root(struct mount *, struct vnode **);			\
int	fsname##_quotactl(struct mount *, int, uid_t, void *,		\
		struct lwp *);						\
int	fsname##_statvfs(struct mount *, struct statvfs *,		\
		struct lwp *);						\
int	fsname##_sync(struct mount *, int, struct kauth_cred *,		\
		struct lwp *);						\
int	fsname##_vget(struct mount *, ino_t, struct vnode **);		\
int	fsname##_fhtovp(struct mount *, struct fid *, struct vnode **);	\
int	fsname##_vptofh(struct vnode *, struct fid *);			\
void	fsname##_init(void);						\
void	fsname##_reinit(void);						\
void	fsname##_done(void);						\
int	fsname##_mountroot(void);					\
int	fsname##_snapshot(struct mount *, struct vnode *,		\
		struct timespec *);					\
int	fsname##_extattrctl(struct mount *, int, struct vnode *, int,	\
		const char *, struct lwp *);				\
int	fsname##_suspendctl(struct mount *, int);

struct vfsops {
	const char *vfs_name;
	int	(*vfs_mount)	(struct mount *, const char *, void *,
				    struct nameidata *, struct lwp *);
	int	(*vfs_start)	(struct mount *, int, struct lwp *);
	int	(*vfs_unmount)	(struct mount *, int, struct lwp *);
	int	(*vfs_root)	(struct mount *, struct vnode **);
	int	(*vfs_quotactl)	(struct mount *, int, uid_t, void *,
				    struct lwp *);
	int	(*vfs_statvfs)	(struct mount *, struct statvfs *,
				    struct lwp *);
	int	(*vfs_sync)	(struct mount *, int, struct kauth_cred *,
				    struct lwp *);
	int	(*vfs_vget)	(struct mount *, ino_t, struct vnode **);
	int	(*vfs_fhtovp)	(struct mount *, struct fid *,
				    struct vnode **);
	int	(*vfs_vptofh)	(struct vnode *, struct fid *, size_t *);
	void	(*vfs_init)	(void);
	void	(*vfs_reinit)	(void);
	void	(*vfs_done)	(void);
	int	(*vfs_mountroot)(void);
	int	(*vfs_snapshot)	(struct mount *, struct vnode *,
				    struct timespec *);
	int	(*vfs_extattrctl) (struct mount *, int,
				    struct vnode *, int, const char *,
				    struct lwp *);
	int	(*vfs_suspendctl) (struct mount *, int);
	const struct vnodeopv_desc * const *vfs_opv_descs;
	int	vfs_refcount;
	LIST_ENTRY(vfsops) vfs_list;
};

#define	VFS_ATTACH(vfs)		__link_set_add_data(vfsops, vfs)

#define VFS_MOUNT(MP, PATH, DATA, NDP, L) \
	(*(MP)->mnt_op->vfs_mount)(MP, PATH, DATA, NDP, L)
#define VFS_START(MP, FLAGS, L)	  (*(MP)->mnt_op->vfs_start)(MP, FLAGS, L)
#define VFS_UNMOUNT(MP, FORCE, L) (*(MP)->mnt_op->vfs_unmount)(MP, FORCE, L)
#define VFS_ROOT(MP, VPP)	  (*(MP)->mnt_op->vfs_root)(MP, VPP)
#define VFS_QUOTACTL(MP,C,U,A,L)  (*(MP)->mnt_op->vfs_quotactl)(MP, C, U, A, L)
#define VFS_STATVFS(MP, SBP, L)	  (*(MP)->mnt_op->vfs_statvfs)(MP, SBP, L)
#define VFS_SYNC(MP, WAIT, C, L)  (*(MP)->mnt_op->vfs_sync)(MP, WAIT, C, L)
#define VFS_VGET(MP, INO, VPP)    (*(MP)->mnt_op->vfs_vget)(MP, INO, VPP)
#define VFS_FHTOVP(MP, FIDP, VPP) (*(MP)->mnt_op->vfs_fhtovp)(MP, FIDP, VPP)
#define	VFS_VPTOFH(VP, FIDP, FIDSZP)  (*(VP)->v_mount->mnt_op->vfs_vptofh)(VP, FIDP, FIDSZP)
#define VFS_SNAPSHOT(MP, VP, TS)  (*(MP)->mnt_op->vfs_snapshot)(MP, VP, TS)
#define	VFS_EXTATTRCTL(MP, C, VP, AS, AN, L) \
	(*(MP)->mnt_op->vfs_extattrctl)(MP, C, VP, AS, AN, L)
#define VFS_SUSPENDCTL(MP, C)     (*(MP)->mnt_op->vfs_suspendctl)(MP, C)

struct vfs_hooks {
	void	(*vh_unmount)(struct mount *);
};
#define	VFS_HOOKS_ATTACH(hooks)	__link_set_add_data(vfs_hooks, hooks)

void	vfs_hooks_unmount(struct mount *);

#endif /* _KERNEL */

/*
 * Export arguments for local filesystem mount calls.
 *
 * This structure is deprecated and is only provided for compatibility
 * reasons with old binary utilities; several file systems expose an
 * instance of this structure in their mount arguments structure, thus
 * needing a padding in place of the old values.  This definition cannot
 * change in the future due to this reason.
 * XXX: This should be moved to the compat subtree but cannot be done
 * until we can move the mount args structures themselves.
 *
 * The current export_args structure can be found in nfs/nfs.h.
 */
struct export_args30 {
	int	ex_flags;		/* export related flags */
	uid_t	ex_root;		/* mapping for root uid */
	struct	uucred ex_anon;		/* mapping for anonymous user */
	struct	sockaddr *ex_addr;	/* net address to which exported */
	int	ex_addrlen;		/* and the net address length */
	struct	sockaddr *ex_mask;	/* mask of valid bits in saddr */
	int	ex_masklen;		/* and the smask length */
	char	*ex_indexfile;		/* index file for WebNFS URLs */
};

#ifdef _KERNEL
#include <sys/mallocvar.h>
MALLOC_DECLARE(M_MOUNT);

/*
 * exported VFS interface (see vfssubr(9))
 */
struct	mount *vfs_getvfs(fsid_t *);    /* return vfs given fsid */
int	vfs_composefh(struct vnode *, fhandle_t *, size_t *);
int	vfs_composefh_alloc(struct vnode *, fhandle_t **);
void	vfs_composefh_free(fhandle_t *);
int	vfs_fhtovp(fhandle_t *, struct vnode **);
int	vfs_mountedon(struct vnode *);/* is a vfs mounted on vp */
int	vfs_mountroot(void);
void	vfs_shutdown(void);	    /* unmount and sync file systems */
void	vfs_unmountall(struct lwp *);	    /* unmount file systems */
int 	vfs_busy(struct mount *, int, struct simplelock *);
int	vfs_rootmountalloc(const char *, const char *, struct mount **);
void	vfs_unbusy(struct mount *);
int	vfs_attach(struct vfsops *);
int	vfs_detach(struct vfsops *);
void	vfs_reinit(void);
struct vfsops *vfs_getopsbyname(const char *);

int	vfs_stdextattrctl(struct mount *, int, struct vnode *,
	    int, const char *, struct lwp *);
int	vfs_stdsuspendctl(struct mount *, int);

extern	CIRCLEQ_HEAD(mntlist, mount) mountlist;	/* mounted filesystem list */
extern	struct vfsops *vfssw[];			/* filesystem type table */
extern	int nvfssw;
extern	struct simplelock mountlist_slock;
extern	struct simplelock spechash_slock;
long	makefstype(const char *);
int	dounmount(struct mount *, int, struct lwp *);
void	vfsinit(void);
void	vfs_opv_init(const struct vnodeopv_desc * const *);
void	vfs_opv_free(const struct vnodeopv_desc * const *);
#ifdef DEBUG
void	vfs_bufstats(void);
#endif

int	mount_specific_key_create(specificdata_key_t *, specificdata_dtor_t);
void	mount_specific_key_delete(specificdata_key_t);
void 	mount_initspecific(struct mount *);
void 	mount_finispecific(struct mount *);
void *	mount_getspecific(struct mount *, specificdata_key_t);
void	mount_setspecific(struct mount *, specificdata_key_t, void *);

LIST_HEAD(vfs_list_head, vfsops);
extern struct vfs_list_head vfs_list;

#else /* _KERNEL */

#include <sys/cdefs.h>

__BEGIN_DECLS
#if !defined(__LIBC12_SOURCE__) && !defined(_STANDALONE)
int	getfh(const char *, void *, size_t *)
	__RENAME(__getfh30);
#endif

int	mount(const char *, const char *, int, void *);
int	unmount(const char *, int);
#if defined(_NETBSD_SOURCE)
#ifndef __LIBC12_SOURCE__
int	fhopen(const void *, size_t, int) __RENAME(__fhopen40);
int	fhstat(const void *, size_t, struct stat *) __RENAME(__fhstat40);
#endif
#endif /* _NETBSD_SOURCE */
__END_DECLS

#endif /* _KERNEL */
#endif /* !_STANDALONE */

#endif /* !_SYS_MOUNT_H_ */
