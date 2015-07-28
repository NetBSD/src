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
	struct dlfs lfs_dlfs;

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

// XXX temporary
#include <ufs/lfs/lfs_accessors.h>

/* ugh... */
#define CLFS_DEF_SB_ACCESSOR(type, field) \
	static __unused inline type				\
	clfs_sb_get##field(struct clfs *fs)			\
	{							\
		return fs->lfs_dlfs.dlfs_##field;		\
	}							\
	static __unused inline void				\
	clfs_sb_set##field(struct clfs *fs, type val)		\
	{							\
		fs->lfs_dlfs.dlfs_##field = val;		\
	}							\
	static __unused inline void				\
	clfs_sb_add##field(struct clfs *fs, type val)		\
	{							\
		type *p = &fs->lfs_dlfs.dlfs_##field;		\
		*p += val;					\
	}

/* more ugh... */
CLFS_DEF_SB_ACCESSOR(u_int32_t, ssize);
CLFS_DEF_SB_ACCESSOR(u_int32_t, bsize);
CLFS_DEF_SB_ACCESSOR(u_int32_t, fsize);
CLFS_DEF_SB_ACCESSOR(u_int32_t, frag);
CLFS_DEF_SB_ACCESSOR(u_int32_t, ifile);
CLFS_DEF_SB_ACCESSOR(u_int32_t, inopb);
CLFS_DEF_SB_ACCESSOR(u_int32_t, ifpb);
CLFS_DEF_SB_ACCESSOR(u_int32_t, sepb);
CLFS_DEF_SB_ACCESSOR(u_int32_t, nseg);
CLFS_DEF_SB_ACCESSOR(u_int32_t, cleansz);
CLFS_DEF_SB_ACCESSOR(u_int32_t, segtabsz);
CLFS_DEF_SB_ACCESSOR(u_int64_t, bmask);
CLFS_DEF_SB_ACCESSOR(u_int32_t, bshift);
CLFS_DEF_SB_ACCESSOR(u_int64_t, ffmask);
CLFS_DEF_SB_ACCESSOR(u_int32_t, ffshift);
CLFS_DEF_SB_ACCESSOR(u_int32_t, fbshift);
CLFS_DEF_SB_ACCESSOR(u_int32_t, blktodb);
CLFS_DEF_SB_ACCESSOR(u_int32_t, minfreeseg);
CLFS_DEF_SB_ACCESSOR(u_int32_t, sumsize);
CLFS_DEF_SB_ACCESSOR(u_int32_t, ibsize);
CLFS_DEF_SB_ACCESSOR(int32_t, s0addr);
static __unused inline int32_t
clfs_sb_getsboff(struct clfs *fs, unsigned n)
{
	assert(n < LFS_MAXNUMSB);
	return fs->lfs_dlfs.dlfs_sboffs[n];
}
static __unused inline const char *
clfs_sb_getfsmnt(struct clfs *fs)
{
	return (const char *)fs->lfs_dlfs.dlfs_fsmnt;
}

/* still more ugh... */
#define lfs_sb_getssize(fs) clfs_sb_getssize(fs)
#define lfs_sb_getbsize(fs) clfs_sb_getbsize(fs)
#define lfs_sb_getfsize(fs) clfs_sb_getfsize(fs)
#define lfs_sb_getfrag(fs) clfs_sb_getfrag(fs)
#define lfs_sb_getinopb(fs) clfs_sb_getinopb(fs)
#define lfs_sb_getifpb(fs) clfs_sb_getifpb(fs)
#define lfs_sb_getsepb(fs) clfs_sb_getsepb(fs)
#define lfs_sb_getnseg(fs) clfs_sb_getnseg(fs)
#define lfs_sb_getcleansz(fs) clfs_sb_getcleansz(fs)
#define lfs_sb_getsegtabsz(fs) clfs_sb_getsegtabsz(fs)
#define lfs_sb_getbmask(fs) clfs_sb_getbmask(fs)
#define lfs_sb_getbshift(fs) clfs_sb_getbshift(fs)
#define lfs_sb_getffmask(fs) clfs_sb_getffmask(fs)
#define lfs_sb_getffshift(fs) clfs_sb_getffshift(fs)
#define lfs_sb_getfbshift(fs) clfs_sb_getfbshift(fs)
#define lfs_sb_getblktodb(fs) clfs_sb_getblktodb(fs)
#define lfs_sb_getminfreeseg(fs) clfs_sb_getminfreeseg(fs)
#define lfs_sb_getsumsize(fs) clfs_sb_getsumsize(fs)
#define lfs_sb_getibsize(fs) clfs_sb_getibsize(fs)
#define lfs_sb_gets0addr(fs) clfs_sb_gets0addr(fs)
#define lfs_sb_getsboff(fs, n) clfs_sb_getsboff(fs, n)
#define lfs_sb_getfsmnt(fs) clfs_sb_getfsmnt(fs)

/*
 * This needs to come after the definition of struct clfs. (XXX blah)
 */
//#define STRUCT_LFS struct clfs
//#include <ufs/lfs/lfs_accessors.h>

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
