/*	$NetBSD: vnode.h,v 1.150 2005/12/25 14:48:59 yamt Exp $	*/

/*
 * Copyright (c) 1989, 1993
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
 *	@(#)vnode.h	8.17 (Berkeley) 5/20/95
 */

#ifndef _SYS_VNODE_H_
#define	_SYS_VNODE_H_

#include <sys/event.h>
#include <sys/lock.h>
#include <sys/queue.h>

/* XXX: clean up includes later */
#include <uvm/uvm_param.h>	/* XXX */
#include <uvm/uvm_pglist.h>	/* XXX */
#include <uvm/uvm_object.h>	/* XXX */
#include <uvm/uvm_extern.h>	/* XXX */

struct namecache;
struct uvm_ractx;

/*
 * The vnode is the focus of all file activity in UNIX.  There is a
 * unique vnode allocated for each active file, each current directory,
 * each mounted-on file, text file, and the root.
 */

/*
 * Vnode types.  VNON means no type.
 */
enum vtype	{ VNON, VREG, VDIR, VBLK, VCHR, VLNK, VSOCK, VFIFO, VBAD };

#define VNODE_TYPES \
    "VNON", "VREG", "VDIR", "VBLK", "VCHR", "VLNK", "VSOCK", "VFIFO", "VBAD"

/*
 * Vnode tag types.
 * These are for the benefit of external programs only (e.g., pstat)
 * and should NEVER be inspected by the kernel.
 */
enum vtagtype	{
	VT_NON, VT_UFS, VT_NFS, VT_MFS, VT_MSDOSFS, VT_LFS, VT_LOFS,
	VT_FDESC, VT_PORTAL, VT_NULL, VT_UMAP, VT_KERNFS, VT_PROCFS,
	VT_AFS, VT_ISOFS, VT_UNION, VT_ADOSFS, VT_EXT2FS, VT_CODA,
	VT_FILECORE, VT_NTFS, VT_VFS, VT_OVERLAY, VT_SMBFS, VT_PTYFS,
	VT_TMPFS, VT_UDF
};

#define VNODE_TAGS \
    "VT_NON", "VT_UFS", "VT_NFS", "VT_MFS", "VT_MSDOSFS", "VT_LFS", "VT_LOFS", \
    "VT_FDESC", "VT_PORTAL", "VT_NULL", "VT_UMAP", "VT_KERNFS", "VT_PROCFS", \
    "VT_AFS", "VT_ISOFS", "VT_UNION", "VT_ADOSFS", "VT_EXT2FS", "VT_CODA", \
    "VT_FILECORE", "VT_NTFS", "VT_VFS", "VT_OVERLAY", "VT_SMBFS", "VT_PTYFS", \
    "VT_TMPFS", "VT_UDF"

LIST_HEAD(buflists, buf);

/*
 * Reading or writing any of these items requires holding the appropriate lock.
 * v_freelist is locked by the global vnode_free_list simple lock.
 * v_mntvnodes is locked by the global mntvnodes simple lock.
 * v_flag, v_usecount, v_holdcount and v_writecount are
 *     locked by the v_interlock simple lock
 *
 * Each underlying filesystem allocates its own private area and hangs
 * it from v_data.
 */
struct vnode {
	struct uvm_object v_uobj;		/* the VM object */
#define	v_usecount	v_uobj.uo_refs
#define	v_interlock	v_uobj.vmobjlock
	voff_t		v_size;			/* size of file */
	int		v_flag;			/* flags */
	int		v_numoutput;		/* number of pending writes */
	long		v_writecount;		/* reference count of writers */
	long		v_holdcnt;		/* page & buffer references */
	struct mount	*v_mount;		/* ptr to vfs we are in */
	int		(**v_op)(void *);	/* vnode operations vector */
	TAILQ_ENTRY(vnode) v_freelist;		/* vnode freelist */
	LIST_ENTRY(vnode) v_mntvnodes;		/* vnodes for mount point */
	struct buflists	v_cleanblkhd;		/* clean blocklist head */
	struct buflists	v_dirtyblkhd;		/* dirty blocklist head */
	LIST_ENTRY(vnode) v_synclist;		/* vnodes with dirty buffers */
	LIST_HEAD(, namecache) v_dnclist;	/* namecaches for children */
	LIST_HEAD(, namecache) v_nclist;	/* namecaches for our parent */
	union {
		struct mount	*vu_mountedhere;/* ptr to mounted vfs (VDIR) */
		struct socket	*vu_socket;	/* unix ipc (VSOCK) */
		struct specinfo	*vu_specinfo;	/* device (VCHR, VBLK) */
		struct fifoinfo	*vu_fifoinfo;	/* fifo (VFIFO) */
		struct uvm_ractx *vu_ractx;	/* read-ahead context (VREG) */
	} v_un;
	struct nqlease	*v_lease;		/* Soft reference to lease */
	enum vtype	v_type;			/* vnode type */
	enum vtagtype	v_tag;			/* type of underlying data */
	struct lock	v_lock;			/* lock for this vnode */
	struct lock	*v_vnlock;		/* pointer to lock */
	void 		*v_data;		/* private data for fs */
	struct klist	v_klist;		/* knotes attached to vnode */
};
#define	v_mountedhere	v_un.vu_mountedhere
#define	v_socket	v_un.vu_socket
#define	v_specinfo	v_un.vu_specinfo
#define	v_fifoinfo	v_un.vu_fifoinfo
#define	v_ractx		v_un.vu_ractx

/*
 * All vnode locking operations should use vp->v_vnlock. For leaf filesystems
 * (such as ffs, lfs, msdosfs, etc), vp->v_vnlock = &vp->v_lock. For
 * stacked filesystems, vp->v_vnlock may equal lowervp->v_vnlock.
 *
 * vp->v_vnlock may also be NULL, which indicates that a leaf node does not
 * export a struct lock for vnode locking. Stacked filesystems (such as
 * nullfs) must call the underlying fs for locking. See layerfs_ routines
 * for examples.
 *
 * All filesystems must (pretend to) understand lockmanager flags.
 */

/*
 * Vnode flags.
 */
#define	VROOT		0x0001	/* root of its file system */
#define	VTEXT		0x0002	/* vnode is a pure text prototype */
	/* VSYSTEM only used to skip vflush()ing quota files */
#define	VSYSTEM		0x0004	/* vnode being used by kernel */
	/* VISTTY used when reading dead vnodes */
#define	VISTTY		0x0008	/* vnode represents a tty */
#define	VEXECMAP	0x0010	/* vnode has PROT_EXEC mappings */
#define	VWRITEMAP	0x0020	/* might have PROT_WRITE user mappings */
#define	VWRITEMAPDIRTY	0x0040	/* might have dirty pages due to VWRITEMAP */
#define	VLOCKSWORK	0x0080	/* FS supports locking discipline */
#define	VXLOCK		0x0100	/* vnode is locked to change underlying type */
#define	VXWANT		0x0200	/* process is waiting for vnode */
#define	VBWAIT		0x0400	/* waiting for output to complete */
#define	VALIASED	0x0800	/* vnode has an alias */
#define	VDIROP		0x1000	/* LFS: vnode is involved in a directory op */
#define	VLAYER		0x2000	/* vnode is on a layer filesystem */
#define	VONWORKLST	0x4000	/* On syncer work-list */
#define	VFREEING	0x8000	/* vnode is being freed */

#define VNODE_FLAGBITS \
    "\20\1ROOT\2TEXT\3SYSTEM\4ISTTY\5EXECMAP\6WRITEMAP\7WRITEMAPDIRTY" \
    "\10LOCKSWORK\11XLOCK\12XWANT\13BWAIT\14ALIASED" \
    "\15DIROP\16LAYER\17ONWORKLIST\20FREEING"

#define	VSIZENOTSET	((voff_t)-1)

/*
 * Use a global lock for all v_numoutput updates.
 * Define a convenience macro to increment by one.
 * Note: the only place where v_numoutput is decremented is in vwakeup().
 */
extern struct simplelock global_v_numoutput_slock;
#define V_INCR_NUMOUTPUT(vp) do {			\
	simple_lock(&global_v_numoutput_slock);		\
	(vp)->v_numoutput++;				\
	simple_unlock(&global_v_numoutput_slock);	\
} while (/*CONSTCOND*/ 0)

/*
 * Vnode attributes.  A field value of VNOVAL represents a field whose value
 * is unavailable (getattr) or which is not to be changed (setattr).
 */
struct vattr {
	enum vtype	va_type;	/* vnode type (for create) */
	mode_t		va_mode;	/* files access mode and type */
	nlink_t		va_nlink;	/* number of references to file */
	uid_t		va_uid;		/* owner user id */
	gid_t		va_gid;		/* owner group id */
	long		va_fsid;	/* file system id (dev for now) */
	ino_t		va_fileid;	/* file id */
	u_quad_t	va_size;	/* file size in bytes */
	long		va_blocksize;	/* blocksize preferred for i/o */
	struct timespec	va_atime;	/* time of last access */
	struct timespec	va_mtime;	/* time of last modification */
	struct timespec	va_ctime;	/* time file changed */
	struct timespec va_birthtime;	/* time file created */
	u_long		va_gen;		/* generation number of file */
	u_long		va_flags;	/* flags defined for file */
	dev_t		va_rdev;	/* device the special file represents */
	u_quad_t	va_bytes;	/* bytes of disk space held by file */
	u_quad_t	va_filerev;	/* file modification number */
	u_int		va_vaflags;	/* operations flags, see below */
	long		va_spare;	/* remain quad aligned */
};

/*
 * Flags for va_vaflags.
 */
#define	VA_UTIMES_NULL	0x01		/* utimes argument was NULL */
#define	VA_EXCLUSIVE	0x02		/* exclusive create request */

#ifdef _KERNEL

/*
 * Flags for ioflag.
 */
#define	IO_UNIT		0x00010		/* do I/O as atomic unit */
#define	IO_APPEND	0x00020		/* append write to end */
#define	IO_SYNC		(0x04|IO_DSYNC)	/* sync I/O file integrity completion */
#define	IO_NODELOCKED	0x00080		/* underlying node already locked */
#define	IO_NDELAY	0x00100		/* FNDELAY flag set in file table */
#define	IO_DSYNC	0x00200		/* sync I/O data integrity completion */
#define	IO_ALTSEMANTICS	0x00400		/* use alternate i/o semantics */
#define	IO_NORMAL	0x00800		/* operate on regular data */
#define	IO_EXT		0x01000		/* operate on extended attributes */
#define	IO_ADV_MASK	0x00003		/* access pattern hint */

#define	IO_ADV_SHIFT	0
#define	IO_ADV_ENCODE(adv)	(((adv) << IO_ADV_SHIFT) & IO_ADV_MASK)
#define	IO_ADV_DECODE(ioflag)	(((ioflag) & IO_ADV_MASK) >> IO_ADV_SHIFT)

/*
 *  Modes.
 */
#define	VREAD	00004		/* read, write, execute permissions */
#define	VWRITE	00002
#define	VEXEC	00001

/*
 * Token indicating no attribute value yet assigned.
 */
#define	VNOVAL	(-1)

/*
 * Convert between vnode types and inode formats (since POSIX.1
 * defines mode word of stat structure in terms of inode formats).
 */
extern const enum vtype	iftovt_tab[];
extern const int	vttoif_tab[];
#define	IFTOVT(mode)	(iftovt_tab[((mode) & S_IFMT) >> 12])
#define	VTTOIF(indx)	(vttoif_tab[(int)(indx)])
#define	MAKEIMODE(indx, mode)	(int)(VTTOIF(indx) | (mode))

/*
 * Flags to various vnode functions.
 */
#define	SKIPSYSTEM	0x0001		/* vflush: skip vnodes marked VSYSTEM */
#define	FORCECLOSE	0x0002		/* vflush: force file closeure */
#define	WRITECLOSE	0x0004		/* vflush: only close writable files */
#define	DOCLOSE		0x0008		/* vclean: close active files */
#define	V_SAVE		0x0001		/* vinvalbuf: sync file first */
					/* vn_start_write: */
#define	V_WAIT		0x0001		/*  sleep for suspend */
#define	V_NOWAIT	0x0002		/*  don't sleep for suspend */
#define	V_SLEEPONLY	0x0004		/*  just return after sleep */
#define V_PCATCH	0x0008		/*  sleep witch PCATCH set */
#define V_LOWER		0x0010		/*  lower level operation */

/*
 * Flags to various vnode operations.
 */
#define	REVOKEALL	0x0001		/* revoke: revoke all aliases */

#define	FSYNC_WAIT	0x0001		/* fsync: wait for completion */
#define	FSYNC_DATAONLY	0x0002		/* fsync: hint: sync file data only */
#define	FSYNC_RECLAIM	0x0004		/* fsync: hint: vnode is being reclaimed */
#define	FSYNC_LAZY	0x0008		/* fsync: lazy sync (trickle) */
#define	FSYNC_CACHE	0x0100		/* fsync: flush disk caches too */

#define	UPDATE_WAIT	0x0001		/* update: wait for completion */
#define	UPDATE_DIROP	0x0002		/* update: hint to fs to wait or not */
#define	UPDATE_CLOSE	0x0004		/* update: clean up on close */

#define	HOLDRELE(vp)	holdrele(vp)
#define	VHOLD(vp)	vhold(vp)
#define	VREF(vp)	vref(vp)
TAILQ_HEAD(freelst, vnode);
extern struct freelst	vnode_hold_list; /* free vnodes referencing buffers */
extern struct freelst	vnode_free_list; /* vnode free list */
extern struct simplelock vnode_free_list_slock;

#ifdef DIAGNOSTIC
#define	ilstatic
#else
#define	ilstatic static
#endif

ilstatic void holdrelel(struct vnode *);
ilstatic void vholdl(struct vnode *);
ilstatic void vref(struct vnode *);

static inline void holdrele(struct vnode *) __attribute__((__unused__));
static inline void vhold(struct vnode *) __attribute__((__unused__));

#ifdef DIAGNOSTIC
#define	VATTR_NULL(vap)	vattr_null(vap)
#else
#define	VATTR_NULL(vap)	(*(vap) = va_null)	/* initialize a vattr */

/*
 * decrease buf or page ref
 *
 * called with v_interlock held
 */
static inline void
holdrelel(struct vnode *vp)
{

	vp->v_holdcnt--;
	if ((vp->v_freelist.tqe_prev != (struct vnode **)0xdeadb) &&
	    vp->v_holdcnt == 0 && vp->v_usecount == 0) {
		simple_lock(&vnode_free_list_slock);
		TAILQ_REMOVE(&vnode_hold_list, vp, v_freelist);
		TAILQ_INSERT_TAIL(&vnode_free_list, vp, v_freelist);
		simple_unlock(&vnode_free_list_slock);
	}
}

/*
 * increase buf or page ref
 *
 * called with v_interlock held
 */
static inline void
vholdl(struct vnode *vp)
{

	if ((vp->v_freelist.tqe_prev != (struct vnode **)0xdeadb) &&
	    vp->v_holdcnt == 0 && vp->v_usecount == 0) {
		simple_lock(&vnode_free_list_slock);
		TAILQ_REMOVE(&vnode_free_list, vp, v_freelist);
		TAILQ_INSERT_TAIL(&vnode_hold_list, vp, v_freelist);
		simple_unlock(&vnode_free_list_slock);
	}
	vp->v_holdcnt++;
}

/*
 * increase reference
 */
static inline void
vref(struct vnode *vp)
{

	simple_lock(&vp->v_interlock);
	vp->v_usecount++;
	simple_unlock(&vp->v_interlock);
}
#endif /* DIAGNOSTIC */

/*
 * decrease buf or page ref
 */
static inline void
holdrele(struct vnode *vp)
{

	simple_lock(&vp->v_interlock);
	holdrelel(vp);
	simple_unlock(&vp->v_interlock);
}

/*
 * increase buf or page ref
 */
static inline void
vhold(struct vnode *vp)
{

	simple_lock(&vp->v_interlock);
	vholdl(vp);
	simple_unlock(&vp->v_interlock);
}

#define	NULLVP	((struct vnode *)NULL)

#define	VN_KNOTE(vp, b)		KNOTE(&vp->v_klist, (b))

/*
 * Global vnode data.
 */
extern struct vnode	*rootvnode;	/* root (i.e. "/") vnode */
extern int		desiredvnodes;	/* number of vnodes desired */
extern long		numvnodes;	/* current number of vnodes */
extern time_t		syncdelay;	/* max time to delay syncing data */
extern time_t		filedelay;	/* time to delay syncing files */
extern time_t		dirdelay;	/* time to delay syncing directories */
extern time_t		metadelay;	/* time to delay syncing metadata */
extern struct vattr	va_null;	/* predefined null vattr structure */

/*
 * Macro/function to check for client cache inconsistency w.r.t. leasing.
 */
#define	LEASE_READ	0x1		/* Check lease for readers */
#define	LEASE_WRITE	0x2		/* Check lease for modifiers */

#endif /* _KERNEL */


/*
 * Mods for exensibility.
 */

/*
 * Flags for vdesc_flags:
 */
#define	VDESC_MAX_VPS		8
/* Low order 16 flag bits are reserved for willrele flags for vp arguments. */
#define	VDESC_VP0_WILLRELE	0x00000001
#define	VDESC_VP1_WILLRELE	0x00000002
#define	VDESC_VP2_WILLRELE	0x00000004
#define	VDESC_VP3_WILLRELE	0x00000008
#define	VDESC_VP0_WILLUNLOCK	0x00000100
#define	VDESC_VP1_WILLUNLOCK	0x00000200
#define	VDESC_VP2_WILLUNLOCK	0x00000400
#define	VDESC_VP3_WILLUNLOCK	0x00000800
#define	VDESC_VP0_WILLPUT	0x00000101
#define	VDESC_VP1_WILLPUT	0x00000202
#define	VDESC_VP2_WILLPUT	0x00000404
#define	VDESC_VP3_WILLPUT	0x00000808
#define	VDESC_NOMAP_VPP		0x00010000
#define	VDESC_VPP_WILLRELE	0x00020000

/*
 * VDESC_NO_OFFSET is used to identify the end of the offset list
 * and in places where no such field exists.
 */
#define	VDESC_NO_OFFSET -1

/*
 * This structure describes the vnode operation taking place.
 */
struct vnodeop_desc {
	int		vdesc_offset;	/* offset in vector--first for speed */
	const char	*vdesc_name;	/* a readable name for debugging */
	int		vdesc_flags;	/* VDESC_* flags */

	/*
	 * These ops are used by bypass routines to map and locate arguments.
	 * Creds and procs are not needed in bypass routines, but sometimes
	 * they are useful to (for example) transport layers.
	 * Nameidata is useful because it has a cred in it.
	 */
	const int	*vdesc_vp_offsets;	/* list ended by VDESC_NO_OFFSET */
	int		vdesc_vpp_offset;	/* return vpp location */
	int		vdesc_cred_offset;	/* cred location, if any */
	int		vdesc_proc_offset;	/* proc location, if any */
	int		vdesc_componentname_offset; /* if any */
	/*
	 * Finally, we've got a list of private data (about each operation)
	 * for each transport layer.  (Support to manage this list is not
	 * yet part of BSD.)
	 */
	caddr_t		*vdesc_transports;
};

#ifdef _KERNEL
#include <sys/mallocvar.h>
MALLOC_DECLARE(M_CACHE);
MALLOC_DECLARE(M_VNODE);

/*
 * A list of all the operation descs.
 */
extern struct vnodeop_desc	*vnodeop_descs[];

/*
 * Interlock for scanning list of vnodes attached to a mountpoint
 */
extern struct simplelock	mntvnode_slock;

/*
 * Union filesystem hook for vn_readdir().
 */
extern int (*vn_union_readdir_hook) (struct vnode **, struct file *, struct lwp *);

/*
 * This macro is very helpful in defining those offsets in the vdesc struct.
 *
 * This is stolen from X11R4.  I ingored all the fancy stuff for
 * Crays, so if you decide to port this to such a serious machine,
 * you might want to consult Intrisics.h's XtOffset{,Of,To}.
 */
#define	VOPARG_OFFSET(p_type,field) \
	((int) (((char *) (&(((p_type)NULL)->field))) - ((char *) NULL)))
#define	VOPARG_OFFSETOF(s_type,field) \
	VOPARG_OFFSET(s_type*,field)
#define	VOPARG_OFFSETTO(S_TYPE,S_OFFSET,STRUCT_P) \
	((S_TYPE)(((char*)(STRUCT_P))+(S_OFFSET)))


/*
 * This structure is used to configure the new vnodeops vector.
 */
struct vnodeopv_entry_desc {
	const struct vnodeop_desc *opve_op;	/* which operation this is */
	int (*opve_impl)(void *);	/* code implementing this operation */
};
struct vnodeopv_desc {
			/* ptr to the ptr to the vector where op should go */
	int (***opv_desc_vector_p)(void *);
	const struct vnodeopv_entry_desc *opv_desc_ops; /* null terminated list */
};

/*
 * A default routine which just returns an error.
 */
int vn_default_error(void *);

/*
 * A generic structure.
 * This can be used by bypass routines to identify generic arguments.
 */
struct vop_generic_args {
	struct vnodeop_desc *a_desc;
	/* other random data follows, presumably */
};

/*
 * VOCALL calls an op given an ops vector.  We break it out because BSD's
 * vclean changes the ops vector and then wants to call ops with the old
 * vector.
 */
/*
 * actually, vclean doesn't use it anymore, but nfs does,
 * for device specials and fifos.
 */
#define	VOCALL(OPSV,OFF,AP) (( *((OPSV)[(OFF)])) (AP))

/*
 * This call works for vnodes in the kernel.
 */
#define	VCALL(VP,OFF,AP) VOCALL((VP)->v_op,(OFF),(AP))
#define	VDESC(OP) (& __CONCAT(OP,_desc))
#define	VOFFSET(OP) (VDESC(OP)->vdesc_offset)

/*
 * Functions to gate filesystem write operations. Declared static inline
 * here because they usually go into time critical code paths.
 */
#include <sys/mount.h>

int vn_start_write(struct vnode *, struct mount **, int);
void vn_finished_write(struct mount *, int);

/*
 * Finally, include the default set of vnode operations.
 */
#include <sys/vnode_if.h>

/*
 * Public vnode manipulation functions.
 */
struct file;
struct filedesc;
struct nameidata;
struct proc;
struct stat;
struct ucred;
struct uio;
struct vattr;
struct vnode;

/* see vnode(9) */
int 	bdevvp(dev_t, struct vnode **);
int 	cdevvp(dev_t, struct vnode **);
struct vnode *
	checkalias(struct vnode *, dev_t, struct mount *);
int 	getnewvnode(enum vtagtype, struct mount *, int (**)(void *),
	    struct vnode **);
void	ungetnewvnode(struct vnode *);
int	vaccess(enum vtype, mode_t, uid_t, gid_t, mode_t, struct ucred *);
void 	vattr_null(struct vattr *);
int 	vcount(struct vnode *);
void	vdevgone(int, int, int, enum vtype);
int	vfinddev(dev_t, enum vtype, struct vnode **);
int	vflush(struct mount *, struct vnode *, int);
void	vflushbuf(struct vnode *, int);
int 	vget(struct vnode *, int);
void 	vgone(struct vnode *);
void	vgonel(struct vnode *, struct lwp *);
int	vinvalbuf(struct vnode *, int, struct ucred *,
	    struct lwp *, int, int);
void	vprint(const char *, struct vnode *);
void 	vput(struct vnode *);
int	vrecycle(struct vnode *, struct simplelock *, struct lwp *);
void 	vrele(struct vnode *);
int	vtruncbuf(struct vnode *, daddr_t, int, int);
void	vwakeup(struct buf *);

/* see vnsubr(9) */
int	vn_bwrite(void *);
int 	vn_close(struct vnode *, int, struct ucred *, struct lwp *);
int	vn_isunder(struct vnode *, struct vnode *, struct lwp *);
int	vn_lock(struct vnode *, int);
void	vn_markexec(struct vnode *);
int	vn_marktext(struct vnode *);
int 	vn_open(struct nameidata *, int, int);
int 	vn_rdwr(enum uio_rw, struct vnode *, caddr_t, int, off_t, enum uio_seg,
	    int, struct ucred *, size_t *, struct lwp *);
int	vn_readdir(struct file *, char *, int, u_int, int *, struct lwp *,
	    off_t **, int *);
void	vn_restorerecurse(struct vnode *, u_int);
u_int	vn_setrecurse(struct vnode *);
int	vn_stat(struct vnode *, struct stat *, struct lwp *);
int	vn_kqfilter(struct file *, struct knote *);
int	vn_writechk(struct vnode *);
int	vn_extattr_get(struct vnode *, int, int, const char *, size_t *,
	    void *, struct lwp *);
int	vn_extattr_set(struct vnode *, int, int, const char *, size_t,
	    const void *, struct lwp *);
int	vn_extattr_rm(struct vnode *, int, int, const char *, struct lwp *);
int	vn_cow_establish(struct vnode *, int (*)(void *, struct buf *),
            void *);
int	vn_cow_disestablish(struct vnode *, int (*)(void *, struct buf *),
            void *);
void	vn_ra_allocctx(struct vnode *);

/* initialise global vnode management */
void	vntblinit(void);

/* misc stuff */
void	vn_syncer_add_to_worklist(struct vnode *, int);
void	vn_syncer_remove_from_worklist(struct vnode *);
int	speedup_syncer(void);

/* from vfs_syscalls.c - abused by compat code */
int	getvnode(struct filedesc *, int, struct file **);

/* see vfssubr(9) */
void	vfs_getnewfsid(struct mount *);
int	vfs_drainvnodes(long target, struct lwp *);
void	vfs_write_resume(struct mount *);
int	vfs_write_suspend(struct mount *, int, int);
#ifdef DDB
void	vfs_vnode_print(struct vnode *, int, void (*)(const char *, ...));
void	vfs_mount_print(struct mount *, int, void (*)(const char *, ...));
#endif /* DDB */
#endif /* _KERNEL */

#endif /* !_SYS_VNODE_H_ */
