/*
 *	%W% (Erasmus) %G%	- pk@cs.few.eur.nl
 */

/*
 * This structure defines the control data for the proc file system.
 */

struct pfsnode {
	struct	pfsnode	*pfs_next;	/* next on list */
	struct	vnode	*pfs_vnode;	/* vnode associated with this pfsnode */
	pid_t	pfs_pid;
	int	flags;
	long	pfs_spare[4];
};

struct pfsnode	*pfshead;

/*
 * Format of a directory entry in /proc
 */
struct pfsdent {
	unsigned long	d_fileno;
	unsigned short	d_reclen;
	unsigned short	d_namlen;
	char		d_nam[8];
};
#define PFSDENTSIZE	(sizeof(struct direct) - MAXNAMELEN + 8)

#ifndef DIRBLKSIZ
#define DIRBLKSIZ	DEV_BSIZE
#endif

#ifdef DEBUG
int pfs_debug;
#endif

/*
 * Convert between pfsnode pointers and vnode pointers
 */
#define VTOPFS(vp)	((struct pfsnode *)(vp)->v_data)
#define PFSTOV(pfsp)	((pfsp)->pfs_vnode)

/*
 * Prototypes for PFS operations on vnodes.
 */
int	pfs_badop();
int	pfs_doio();
int	pfs_lookup __P(( \
		struct vnode *vp, \
		struct nameidata *ndp, \
		struct proc *p));
#define pfs_create ((int (*) __P(( \
		struct nameidata *ndp, \
		struct vattr *vap, \
		struct proc *p))) pfs_badop)
#define pfs_mknod ((int (*) __P(( \
		struct nameidata *ndp, \
		struct vattr *vap, \
		struct ucred *cred, \
		struct proc *p))) pfs_badop)
int	pfs_open __P((
		struct vnode *vp,
		int mode,
		struct ucred *cred,
		struct proc *p));
int	pfs_close __P((
		struct vnode *vp,
		int fflag,
		struct ucred *cred,
		struct proc *p));
int	pfs_access __P((
		struct vnode *vp,
		int mode,
		struct ucred *cred,
		struct proc *p));
int	pfs_getattr __P((
		struct vnode *vp,
		struct vattr *vap,
		struct ucred *cred,
		struct proc *p));
#define pfs_setattr ((int (*) __P(( \
		struct vnode *vp, \
		struct vattr *vap, \
		struct ucred *cred, \
		struct proc *p))) pfs_badop)
#define	pfs_read ((int (*)  __P(( \
		struct vnode *vp, \
		struct uio *uio, \
		int ioflag, \
		struct ucred *cred))) pfs_doio)
#define	pfs_write ((int (*) __P(( \
		struct vnode *vp, \
		struct uio *uio, \
		int ioflag, \
		struct ucred *cred))) pfs_doio)
int	pfs_ioctl __P((
		struct vnode *vp,
		int command,
		caddr_t data,
		int fflag,
		struct ucred *cred,
		struct proc *p));
#define pfs_select ((int (*) __P(( \
		struct vnode *vp, \
		int which, \
		int fflags, \
		struct ucred *cred, \
		struct proc *p))) pfs_badop)
#define pfs_mmap ((int (*) __P(( \
		struct vnode *vp, \
		int fflags, \
		struct ucred *cred, \
		struct proc *p))) pfs_badop)
#define pfs_fsync ((int (*) __P(( \
		struct vnode *vp, \
		int fflags, \
		struct ucred *cred, \
		int waitfor, \
		struct proc *p))) pfs_badop)
#define pfs_seek ((int (*) __P(( \
		struct vnode *vp, \
		off_t oldoff, \
		off_t newoff, \
		struct ucred *cred))) pfs_badop)
#define pfs_remove ((int (*) __P(( \
		struct nameidata *ndp, \
		struct proc *p))) pfs_badop)
#define pfs_link ((int (*) __P(( \
		struct vnode *vp, \
		struct nameidata *ndp, \
		struct proc *p))) pfs_badop)
#define pfs_rename ((int (*) __P(( \
		struct nameidata *fndp, \
		struct nameidata *tdnp, \
		struct proc *p))) pfs_badop)
#define pfs_mkdir ((int (*) __P(( \
		struct nameidata *ndp, \
		struct vattr *vap, \
		struct proc *p))) pfs_badop)
#define pfs_rmdir ((int (*) __P(( \
		struct nameidata *ndp, \
		struct proc *p))) pfs_badop)
#define pfs_symlink ((int (*) __P(( \
		struct nameidata *ndp, \
		struct vattr *vap, \
		char *target, \
		struct proc *p))) pfs_badop)
int	pfs_readdir __P((
		struct vnode *vp,
		struct uio *uio,
		struct ucred *cred,
		int *eofflagp));
#define pfs_readlink ((int (*) __P(( \
		struct vnode *vp, \
		struct uio *uio, \
		struct ucred *cred))) pfs_badop)
#define pfs_abortop ((int (*) __P(( \
		struct nameidata *ndp))) pfs_badop)
int	pfs_inactive __P((
		struct vnode *vp,
		struct proc *p));
int	pfs_reclaim __P((
		struct vnode *vp));
#define pfs_lock ((int (*) __P(( \
		struct vnode *vp))) nullop)
#define pfs_unlock ((int (*) __P(( \
		struct vnode *vp))) nullop)
int	pfs_bmap __P((
		struct vnode *vp,
		daddr_t bn,
		struct vnode **vpp,
		daddr_t *bnp));
int	pfs_strategy __P((
		struct buf *bp));
void	pfs_print __P((
		struct vnode *vp));
#define pfs_islocked ((int (*) __P(( \
		struct vnode *vp))) nullop)
#define pfs_advlock ((int (*) __P(( \
		struct vnode *vp, \
		caddr_t id, \
		int op, \
		struct flock *fl, \
		int flags))) pfs_badop)
