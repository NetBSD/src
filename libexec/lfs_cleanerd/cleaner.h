#ifndef CLEANER_H_
#define CLEANER_H_

/*
 * An abbreviated version of the SEGUSE data structure.
 */
struct clfs_seguse {
	u_int32_t nbytes;
	u_int32_t nsums;
	u_int32_t flags;
	u_int64_t lastmod;
	u_int64_t priority;
};

/*
 * The cleaner's view of the superblock data structure.
 */
struct clfs {
	struct dlfs lfs_dlfs;	   /* Leverage LFS lfs_* defines here */

	/* Ifile */
	int clfs_ifilefd;	   /* Ifile file descriptor */
	struct uvnode *lfs_ivnode; /* Ifile vnode */
	struct lfs_fhandle clfs_ifilefh;	   /* Ifile file handle */

	/* Device */
	int clfs_devfd;		   /* Device file descriptor */
	struct uvnode *clfs_devvp; /* Device vnode */
	char *clfs_dev;		   /* Name of device */

	/* Cache of segment status */
	struct clfs_seguse  *clfs_segtab;  /* Abbreviated seguse table */
	struct clfs_seguse **clfs_segtabp; /* pointers to same */

	/* Progress status */
	int clfs_nactive;	   /* How many segments' blocks we have */
	int clfs_onhold;	   /* If cleaning this fs is on hold */
};

/*
 * Fraction of the could-be-clean segments required to be clean.
 */
#define BUSY_LIM 0.5
#define IDLE_LIM 0.9

__BEGIN_DECLS

/* lfs_cleanerd.c */
void pwarn(const char *, ...);
void calc_cb(struct clfs *, int, struct clfs_seguse *);
int clean_fs(struct clfs *, CLEANERINFO *);
void dlog(const char *, ...);
void handle_error(struct clfs **, int);
int init_fs(struct clfs *, char *);
int invalidate_segment(struct clfs *, int);
void lfs_ientry(IFILE **, struct clfs *, ino_t, struct ubuf **);
int load_segment(struct clfs *, int, BLOCK_INFO **, int *);
int needs_cleaning(struct clfs *, CLEANERINFO *);
int32_t parse_pseg(struct clfs *, daddr_t, BLOCK_INFO **, int *);
int reinit_fs(struct clfs *);
void reload_ifile(struct clfs *);
void toss_old_blocks(struct clfs *, BLOCK_INFO **, int *, int *);

/* cleansrv.c */
void check_control_socket(void);
void try_to_become_master(int, char **);

/* coalesce.c */
int log2int(int);
int clean_all_inodes(struct clfs *);
int fork_coalesce(struct clfs *);

__END_DECLS

#endif /* CLEANER_H_ */
