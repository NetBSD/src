/*	$NetBSD: denode.h,v 1.5 1994/07/16 21:33:17 cgd Exp $	*/

/*
 * Written by Paul Popelka (paulp@uts.amdahl.com)
 * 
 * You can do anything you want with this software, just don't say you wrote
 * it, and don't remove this notice.
 * 
 * This software is provided "as is".
 * 
 * The author supplies this software to be publicly redistributed on the
 * understanding that the author is not responsible for the correct
 * functioning of this software in any circumstances and is not liable for
 * any damages caused by this software.
 * 
 * October 1992
 */

/*
 * This is the pc filesystem specific portion of the vnode structure.
 *
 * To describe a file uniquely the de_dirclust, de_diroffset, and
 * de_StartCluster fields are used.
 *
 * de_dirclust contains the cluster number of the directory cluster
 *	containing the entry for a file or directory.
 * de_diroffset is the index into the cluster for the entry describing
 *	a file or directory.
 * de_StartCluster is the number of the first cluster of the file or directory.
 *
 * Now to describe the quirks of the pc filesystem.
 * - Clusters 0 and 1 are reserved.
 * - The first allocatable cluster is 2.
 * - The root directory is of fixed size and all blocks that make it up
 *   are contiguous.
 * - Cluster 0 refers to the root directory when it is found in the
 *   startcluster field of a directory entry that points to another directory.
 * - Cluster 0 implies a 0 length file when found in the start cluster field
 *   of a directory entry that points to a file.
 * - You can't use the cluster number 0 to derive the address of the root
 *   directory.
 * - Multiple directory entries can point to a directory. The entry in the
 *   parent directory points to a child directory.  Any directories in the
 *   child directory contain a ".." entry that points back to the parent.
 *   The child directory itself contains a "." entry that points to itself.
 * - The root directory does not contain a "." or ".." entry.
 * - Directory entries for directories are never changed once they are created
 *   (except when removed).  The size stays 0, and the last modification time
 *   is never changed.  This is because so many directory entries can point to
 *   the physical clusters that make up a directory.  It would lead to an
 *   update nightmare.
 * - The length field in a directory entry pointing to a directory contains 0
 *   (always).  The only way to find the end of a directory is to follow the
 *   cluster chain until the "last cluster" marker is found.
 *
 * My extensions to make this house of cards work.  These apply only to the in
 * memory copy of the directory entry.
 * - A reference count for each denode will be kept since dos doesn't keep such
 *   things.
 */

/*
 * Internal pseudo-offset for (nonexistent) directory entry for the root
 * dir in the root dir
 */
#define	MSDOSFSROOT_OFS	0x1fffffff

/*
 * The fat cache structure. fc_fsrcn is the filesystem relative cluster
 * number that corresponds to the file relative cluster number in this
 * structure (fc_frcn).
 */
struct fatcache {
	u_short fc_frcn;	/* file relative cluster number */
	u_short fc_fsrcn;	/* filesystem relative cluster number */
};

/*
 * The fat entry cache as it stands helps make extending files a "quick"
 * operation by avoiding having to scan the fat to discover the last
 * cluster of the file. The cache also helps sequential reads by
 * remembering the last cluster read from the file.  This also prevents us
 * from having to rescan the fat to find the next cluster to read.  This
 * cache is probably pretty worthless if a file is opened by multiple
 * processes.
 */
#define	FC_SIZE		2	/* number of entries in the cache */
#define	FC_LASTMAP	0	/* entry the last call to pcbmap() resolved
				 * to */
#define	FC_LASTFC	1	/* entry for the last cluster in the file */

#define	FCE_EMPTY	0xffff	/* doesn't represent an actual cluster # */

/*
 * Set a slot in the fat cache.
 */
#define	fc_setcache(dep, slot, frcn, fsrcn) \
	(dep)->de_fc[slot].fc_frcn = frcn; \
	(dep)->de_fc[slot].fc_fsrcn = fsrcn;

/*
 * This is the in memory variant of a dos directory entry.  It is usually
 * contained within a vnode.
 */
struct denode {
	struct denode *de_next;	/* Hash chain forward */
	struct denode **de_prev; /* Hash chain back */
	struct vnode *de_vnode;	/* addr of vnode we are part of */
	struct vnode *de_devvp;	/* vnode of blk dev we live on */
	u_long de_flag;		/* flag bits */
	dev_t de_dev;		/* device where direntry lives */
	u_long de_dirclust;	/* cluster of the directory file containing this entry */
	u_long de_diroffset;	/* ordinal of this entry in the directory */
	u_long de_fndclust;	/* cluster of found dir entry */
	u_long de_fndoffset;	/* offset of found dir entry */
	long de_refcnt;		/* reference count */
	struct msdosfsmount *de_pmp;	/* addr of our mount struct */
	struct lockf *de_lockf;	/* byte level lock list */
	pid_t de_lockholder;	/* current lock holder */
	pid_t de_lockwaiter;	/* lock wanter */
	/* the next two fields must be contiguous in memory... */
	u_char de_Name[8];	/* name, from directory entry */
	u_char de_Extension[3];	/* extension, from directory entry */
	u_char de_Attributes;	/* attributes, from directory entry */
	u_short de_Time;	/* creation time */
	u_short de_Date;	/* creation date */
	u_short de_StartCluster; /* starting cluster of file */
	u_long de_FileSize;	/* size of file in bytes */
	struct fatcache de_fc[FC_SIZE];	/* fat cache */
};

/*
 * Values for the de_flag field of the denode.
 */
#define	DELOCKED	0x0001	/* directory entry is locked */
#define	DEWANT		0x0002	/* someone wants this de */
#define	DERENAME	0x0004	/* de is being renamed */
#define	DEUPD		0x0008	/* file has been modified */
#define	DESHLOCK	0x0010	/* file has shared lock */
#define	DEEXLOCK	0x0020	/* file has exclusive lock */
#define	DELWAIT		0x0040	/* someone waiting on file lock */
#define	DEMOD		0x0080	/* denode wants to be written back to disk */

/*
 * Transfer directory entries between internal and external form.
 * dep is a struct denode * (internal form),
 * dp is a struct direntry * (external form).
 */
#define DE_INTERNALIZE(dep, dp)			\
	(bcopy((dp)->deName, (dep)->de_Name, 11),	\
	 (dep)->de_Attributes = (dp)->deAttributes,	\
	 (dep)->de_Time = getushort((dp)->deTime),	\
	 (dep)->de_Date = getushort((dp)->deDate),	\
	 (dep)->de_StartCluster = getushort((dp)->deStartCluster), \
	 (dep)->de_FileSize = getulong((dp)->deFileSize))

#define DE_EXTERNALIZE(dp, dep)				\
	(bcopy((dep)->de_Name, (dp)->deName, 11),	\
	 (dp)->deAttributes = (dep)->de_Attributes,	\
	 putushort((dp)->deTime, (dep)->de_Time),	\
	 putushort((dp)->deDate, (dep)->de_Date),	\
	 putushort((dp)->deStartCluster, (dep)->de_StartCluster), \
	 putulong((dp)->deFileSize, (dep)->de_FileSize))

#define	de_forw		de_chain[0]
#define	de_back		de_chain[1]

#if defined(KERNEL)

#define	VTODE(vp)	((struct denode *)(vp)->v_data)
#define	DETOV(de)	((de)->de_vnode)

#define	DELOCK(de)	delock(de)
#define	DEUNLOCK(de)	deunlock(de)

#define	DEUPDAT(dep, t, waitfor) \
	if (dep->de_flag & DEUPD) \
		(void) deupdat(dep, t, waitfor);

#define	DETIMES(dep, t) \
	if (dep->de_flag & DEUPD) { \
		(dep)->de_flag |= DEMOD; \
		unix2dostime(t, &dep->de_Date, &dep->de_Time); \
		(dep)->de_flag &= ~DEUPD; \
	}

/*
 * This overlays the fid sturcture (see mount.h)
 */
struct defid {
	u_short defid_len;	/* length of structure */
	u_short defid_pad;	/* force long alignment */

	u_long defid_dirclust;	/* cluster this dir entry came from */
	u_long defid_dirofs;	/* index of entry within the cluster */

	/* u_long	defid_gen;	/* generation number */
};

/*
 * Prototypes for MSDOSFS vnode operations
 */
int msdosfs_lookup __P((struct vop_lookup_args *));
int msdosfs_create __P((struct vop_create_args *));
int msdosfs_mknod __P((struct vop_mknod_args *));
int msdosfs_open __P((struct vop_open_args *));
int msdosfs_close __P((struct vop_close_args *));
int msdosfs_access __P((struct vop_access_args *));
int msdosfs_getattr __P((struct vop_getattr_args *));
int msdosfs_setattr __P((struct vop_setattr_args *));
int msdosfs_read __P((struct vop_read_args *));
int msdosfs_write __P((struct vop_write_args *));
int msdosfs_ioctl __P((struct vop_ioctl_args *));
int msdosfs_select __P((struct vop_select_args *));
int msdosfs_mmap __P((struct vop_mmap_args *));
int msdosfs_fsync __P((struct vop_fsync_args *));
int msdosfs_seek __P((struct vop_seek_args *));
int msdosfs_remove __P((struct vop_remove_args *));
int msdosfs_link __P((struct vop_link_args *));
int msdosfs_rename __P((struct vop_rename_args *));
int msdosfs_mkdir __P((struct vop_mkdir_args *));
int msdosfs_rmdir __P((struct vop_rmdir_args *));
int msdosfs_symlink __P((struct vop_symlink_args *));
int msdosfs_readdir __P((struct vop_readdir_args *));
int msdosfs_readlink __P((struct vop_readlink_args *));
int msdosfs_abortop __P((struct vop_abortop_args *));
int msdosfs_inactive __P((struct vop_inactive_args *));
int msdosfs_reclaim __P((struct vop_reclaim_args *));
int msdosfs_lock __P((struct vop_lock_args *));
int msdosfs_unlock __P((struct vop_unlock_args *));
int msdosfs_bmap __P((struct vop_bmap_args *));
int msdosfs_strategy __P((struct vop_strategy_args *));
int msdosfs_print __P((struct vop_print_args *));
int msdosfs_islocked __P((struct vop_islocked_args *));
int msdosfs_advlock __P((struct vop_advlock_args *));
int msdosfs_reallocblks __P((struct vop_reallocblks_args *));

/*
 * Internal service routine prototypes.
 */
int deget __P((struct msdosfsmount * pmp, u_long dirclust, u_long diroffset, struct direntry * direntptr, struct denode ** depp));

static void deput __P((struct denode *dep));
static __inline void deput(dep)
struct denode *dep;
{
	vput(DETOV(dep));
}
#endif				/* defined(KERNEL) */
