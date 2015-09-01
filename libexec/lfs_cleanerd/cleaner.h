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
	union {
		struct dlfs u_32;
		struct dlfs64 u_64;
	} lfs_dlfs_u;
	unsigned lfs_is64 : 1,
		lfs_dobyteswap : 1,
		lfs_hasolddirfmt : 1;

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
 * Get lfs accessors that use struct clfs. This must come after the
 * definition of struct clfs. (blah)
 */
#define STRUCT_LFS struct clfs
#include <ufs/lfs/lfs_accessors.h>

/*
 * Fraction of the could-be-clean segments required to be clean.
 */
#define BUSY_LIM 0.5
#define IDLE_LIM 0.9

__BEGIN_DECLS

/* lfs_cleanerd.c */
void pwarn(const char *, ...);
void calc_cb(struct clfs *, int, struct clfs_seguse *);
void dlog(const char *, ...);
void handle_error(struct clfs **, int);
int init_fs(struct clfs *, char *);
int invalidate_segment(struct clfs *, int);
void lfs_ientry(IFILE **, struct clfs *, ino_t, struct ubuf **);
int load_segment(struct clfs *, int, BLOCK_INFO **, int *);
int reinit_fs(struct clfs *);
void reload_ifile(struct clfs *);
void toss_old_blocks(struct clfs *, BLOCK_INFO **, blkcnt_t *, int *);

/* cleansrv.c */
void check_control_socket(void);
void try_to_become_master(int, char **);

/* coalesce.c */
int clean_all_inodes(struct clfs *);
int fork_coalesce(struct clfs *);

__END_DECLS

#endif /* CLEANER_H_ */
