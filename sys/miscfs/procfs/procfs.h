/*
 * Copyright (c) 1993 The Regents of the University of California.
 * Copyright (c) 1993 Jan-Simon Pendry
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
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
 * From:
 *	Id: procfs.h,v 4.1 1993/12/17 10:47:45 jsp Rel
 *
 *	$Id: procfs.h,v 1.1 1994/01/05 07:51:12 cgd Exp $
 */

/*
 * The different types of node in a procfs filesystem
 */
typedef enum {
	Proot,		/* the filesystem root */
	Pproc,		/* a process-specific sub-directory */
	Pfile,		/* the executable file */
	Pmem,		/* the process's memory image */
	Pregs,		/* the process's register set */
	Pctl,		/* process control */
	Pstatus,	/* process status */
	Pnote,		/* process notifier */
	Pnotepg		/* process group notifier */
} pfstype;

/*
 * control data for the proc file system.
 */
struct pfsnode {
	struct pfsnode	*pfs_next;	/* next on list */
	struct vnode	*pfs_vnode;	/* vnode associated with this pfsnode */
	pfstype		pfs_type;	/* type of procfs node */
	pid_t		pfs_pid;	/* associated process */
	u_short		pfs_mode;	/* mode bits for stat() */
	u_long		pfs_flags;	/* open flags */
	u_long		pfs_fileno;	/* unique file id */
};

#define PROCFS_NOTELEN	64	/* max length of a note (/proc/$pid/note) */
#define PROCFS_CTLLEN 	8	/* max length of a ctl msg (/proc/$pid/ctl */

/*
 * Kernel stuff follows
 */
#ifdef KERNEL

#ifndef VT_PROCFS
#define VT_PROCFS VT_UFS
#endif

#define NDEQ(ndp, s, len) \
	 ((ndp)->ni_namelen == (len) && \
	  (bcmp((s), (ndp)->ni_ptr, (len)) == 0))

/*
 * Format of a directory entry in /proc, ...
 * This must map onto struct dirent (see <dirent.h>)
 */
#define PROCFS_NAMELEN 8
struct pfsdent {
	u_long	d_fileno;
	u_short	d_reclen;
	u_short	d_namlen;
	char	d_name[PROCFS_NAMELEN];
};
#define UIO_MX sizeof(struct pfsdent)
#define PROCFS_FILENO(pid, type) \
	(((type) == Proot) ? \
			2 : \
			((((pid)+1) << 3) + ((int) (type))))

/*
 * Convert between pfsnode vnode
 */
#define VTOPFS(vp)	((struct pfsnode *)(vp)->v_data)
#define PFSTOV(pfs)	((pfs)->pfs_vnode)

typedef struct vfs_namemap vfs_namemap_t;
struct vfs_namemap {
	const char *nm_name;
	int nm_val;
};

extern int vfs_getuserstr __P((struct uio *, char *, int *));
extern vfs_namemap_t *vfs_findname __P((vfs_namemap_t *, char *, int));

struct reg;

#define PFIND(pid) ((pid) ? pfind(pid) : &proc0)
extern int procfs_freevp __P((struct vnode *));
extern int procfs_allocvp __P((struct mount *, struct vnode **, long, pfstype));
extern struct vnode *procfs_findtextvp __P((struct proc *));
extern int procfs_sstep __P((struct proc *));
extern int procfs_read_regs __P((struct proc *, struct reg *));
extern int procfs_write_regs __P((struct proc *, struct reg *));
extern int procfs_donote __P((struct proc *, struct proc *, struct pfsnode *pfsp, struct uio *uio));
extern int procfs_doregs __P((struct proc *, struct proc *, struct pfsnode *pfsp, struct uio *uio));
extern int procfs_domem __P((struct proc *, struct proc *, struct pfsnode *pfsp, struct uio *uio));
extern int procfs_doctl __P((struct proc *, struct proc *, struct pfsnode *pfsp, struct uio *uio));
extern int procfs_dostatus __P((struct proc *, struct proc *, struct pfsnode *pfsp, struct uio *uio));
extern int procfs_rw __P((struct vnode *, struct uio *, int, struct ucred *));

#define PROCFS_LOCKED	0x01
#define PROCFS_WANT	0x02

extern struct vnodeops procfs_vnodeops;
extern struct vfsops procfs_vfsops;

/*
 * Prototypes for procfs vnode ops
 */
int	procfs_badop();	/* varargs */
int	procfs_rw __P((
		struct vnode *vp,
		struct uio *uio,
		int ioflag,
		struct ucred *cred));
int	procfs_lookup __P((
		struct vnode *vp,
		struct nameidata *ndp,
		struct proc *p));
#define procfs_create ((int (*) __P(( \
		struct nameidata *ndp, \
		struct vattr *vap, \
		struct proc *p))) procfs_badop)
#define procfs_mknod ((int (*) __P(( \
		struct nameidata *ndp, \
		struct vattr *vap, \
		struct ucred *cred, \
		struct proc *p))) procfs_badop)
int	procfs_open __P((
		struct vnode *vp,
		int mode,
		struct ucred *cred,
		struct proc *p));
int	procfs_close __P((
		struct vnode *vp,
		int fflag,
		struct ucred *cred,
		struct proc *p));
int	procfs_access __P((
		struct vnode *vp,
		int mode,
		struct ucred *cred,
		struct proc *p));
int	procfs_getattr __P((
		struct vnode *vp,
		struct vattr *vap,
		struct ucred *cred,
		struct proc *p));
int	procfs_setattr __P((
		struct vnode *vp,
		struct vattr *vap,
		struct ucred *cred,
		struct proc *p));
#define	procfs_read procfs_rw
#define	procfs_write procfs_rw
int	procfs_ioctl __P((
		struct vnode *vp,
		int command,
		caddr_t data,
		int fflag,
		struct ucred *cred,
		struct proc *p));
#define procfs_select ((int (*) __P(( \
		struct vnode *vp, \
		int which, \
		int fflags, \
		struct ucred *cred, \
		struct proc *p))) procfs_badop)
#define procfs_mmap ((int (*) __P(( \
		struct vnode *vp, \
		int fflags, \
		struct ucred *cred, \
		struct proc *p))) procfs_badop)
#define procfs_fsync ((int (*) __P(( \
		struct vnode *vp, \
		int fflags, \
		struct ucred *cred, \
		int waitfor, \
		struct proc *p))) procfs_badop)
#define procfs_seek ((int (*) __P(( \
		struct vnode *vp, \
		off_t oldoff, \
		off_t newoff, \
		struct ucred *cred))) procfs_badop)
#define procfs_remove ((int (*) __P(( \
		struct nameidata *ndp, \
		struct proc *p))) procfs_badop)
#define procfs_link ((int (*) __P(( \
		struct vnode *vp, \
		struct nameidata *ndp, \
		struct proc *p))) procfs_badop)
#define procfs_rename ((int (*) __P(( \
		struct nameidata *fndp, \
		struct nameidata *tdnp, \
		struct proc *p))) procfs_badop)
#define procfs_mkdir ((int (*) __P(( \
		struct nameidata *ndp, \
		struct vattr *vap, \
		struct proc *p))) procfs_badop)
#define procfs_rmdir ((int (*) __P(( \
		struct nameidata *ndp, \
		struct proc *p))) procfs_badop)
#define procfs_symlink ((int (*) __P(( \
		struct nameidata *ndp, \
		struct vattr *vap, \
		char *target, \
		struct proc *p))) procfs_badop)
int	procfs_readdir __P((
		struct vnode *vp,
		struct uio *uio,
		struct ucred *cred,
		int *eofflagp,
		u_int *cookies,
		int ncookies));
#define procfs_readlink ((int (*) __P(( \
		struct vnode *vp, \
		struct uio *uio, \
		struct ucred *cred))) procfs_badop)
int	procfs_abortop __P((
		struct nameidata *ndp));
int	procfs_inactive __P((
		struct vnode *vp,
		struct proc *p));
int	procfs_reclaim __P((
		struct vnode *vp));
#define procfs_lock ((int (*) __P(( \
		struct vnode *vp))) nullop)
#define procfs_unlock ((int (*) __P(( \
		struct vnode *vp))) nullop)
int	procfs_bmap __P((
		struct vnode *vp,
		daddr_t bn,
		struct vnode **vpp,
		daddr_t *bnp));
#define	procfs_strategy ((int (*) __P(( \
		struct buf *bp))) procfs_badop)
int	procfs_print __P((
		struct vnode *vp));
#define procfs_islocked ((int (*) __P(( \
		struct vnode *vp))) nullop)
#define procfs_advlock ((int (*) __P(( \
		struct vnode *vp, \
		caddr_t id, \
		int op, \
		struct flock *fl, \
		int flags))) procfs_badop)

#endif /* KERNEL */
