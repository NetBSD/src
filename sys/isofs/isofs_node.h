/*
 *	$Id: isofs_node.h,v 1.7.2.1 1993/11/26 22:43:12 mycroft Exp $
 */

typedef	struct	{
	struct timeval	iso_atime;	/* time of last access */
	struct timeval	iso_mtime;	/* time of last modification */
	struct timeval	iso_ctime;	/* time file changed */
	u_short		iso_mode;	/* files access mode and type */
	uid_t		iso_uid;	/* owner user id */
	gid_t		iso_gid;	/* owner group id */
	short		iso_links;	/* links of file */
	dev_t		iso_rdev;	/* Major/Minor number for special */
} ISO_RRIP_INODE;

#ifdef	ISODEVMAP
/*
 * FOr device# (major,minor) translation table
 */
struct iso_dnode {
	struct iso_dnode *d_chain[2];	/* hash chain, MUST be first */
	dev_t		i_dev;		/* device where dnode resides */
	ino_t		i_number;	/* the identity of the inode */
	dev_t		d_dev;		/* device # for translation */
};
#define	d_forw		d_chain[0]
#define	d_back		d_chain[1]
#endif

struct iso_node {
	struct	iso_node *i_chain[2]; /* hash chain, MUST be first */
	struct	vnode *i_vnode;	/* vnode associated with this inode */
	struct	vnode *i_devvp;	/* vnode for block I/O */
	u_long	i_flag;		/* see below */
	dev_t	i_dev;		/* device where inode resides */
	ino_t	i_number;	/* the identity of the inode */
				/* we use the actual starting block of the file */
	struct	iso_mnt *i_mnt;	/* filesystem associated with this inode */
	struct	lockf *i_lockf;	/* head of byte-level lock list */
	long	i_diroff;	/* offset in dir, where we found last entry */
	off_t	i_endoff;	/* end of useful stuff in directory */
	long	i_spare0;
	long	i_spare1;

	long iso_extent;
	long i_size;
	long iso_start;		/* actual start of data of file (may be */
				/* different from iso_extent, if file has */
				/* extended attributes) */
	ISO_RRIP_INODE  inode;
};

#define	i_forw		i_chain[0]
#define	i_back		i_chain[1]

/* flags */
#define	ILOCKED		0x0001		/* inode is locked */
#define	IWANT		0x0002		/* some process waiting on lock */
#define	IACC		0x0020		/* inode access time to be updated */

#define VTOI(vp) ((struct iso_node *)(vp)->v_data)
#define ITOV(ip) ((ip)->i_vnode)

#define ISO_ILOCK(ip)	iso_ilock(ip)
#define ISO_IUNLOCK(ip)	iso_iunlock(ip)

/*
 * Prototypes for ISOFS vnode operations
 */
int isofs_lookup __P((struct vnode *vp, struct nameidata *ndp, struct proc *p));
int isofs_open __P((struct vnode *vp, int mode, struct ucred *cred,
	struct proc *p));
int isofs_close __P((struct vnode *vp, int fflag, struct ucred *cred,
	struct proc *p));
int isofs_access __P((struct vnode *vp, int mode, struct ucred *cred,
	struct proc *p));
int isofs_getattr __P((struct vnode *vp, struct vattr *vap, struct ucred *cred,
	struct proc *p));
int isofs_read __P((struct vnode *vp, struct uio *uio, int ioflag,
	struct ucred *cred));
int isofs_ioctl __P((struct vnode *vp, int command, caddr_t data, int fflag,
	struct ucred *cred, struct proc *p));
int isofs_select __P((struct vnode *vp, int which, int fflags, struct ucred *cred,
	struct proc *p));
int isofs_mmap __P((struct vnode *vp, int fflags, struct ucred *cred,
	struct proc *p));
int isofs_seek __P((struct vnode *vp, off_t oldoff, off_t newoff,
	struct ucred *cred));
int isofs_readdir __P((struct vnode *vp, struct uio *uio, struct ucred *cred,
	int *eofflagp, u_int *cookies, int ncookies));
int isofs_abortop __P((struct nameidata *ndp));
int isofs_inactive __P((struct vnode *vp, struct proc *p));
int isofs_reclaim __P((struct vnode *vp));
int isofs_lock __P((struct vnode *vp));
int isofs_unlock __P((struct vnode *vp));
int isofs_strategy __P((struct buf *bp));
void isofs_print __P((struct vnode *vp));
int isofs_islocked __P((struct vnode *vp));
void isofs_defattr __P((struct iso_directory_record *isodir,
			struct iso_node *inop, struct buf *bp));
void isofs_deftstamp __P((struct iso_directory_record *isodir,
			struct iso_node *inop, struct buf *bp));
#ifdef	ISODEVMAP
struct iso_dnode *iso_dmap __P((dev_t dev, ino_t ino, int create));
void iso_dunmap __P((dev_t dev));
#endif
