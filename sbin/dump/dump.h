/*	$NetBSD: dump.h,v 1.49 2013/01/13 23:45:35 dholland Exp $	*/

/*-
 * Copyright (c) 1980, 1993
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
 *	@(#)dump.h	8.2 (Berkeley) 4/28/95
 */

#include <machine/bswap.h>

union dinode {
	struct ufs1_dinode dp1;
	struct ufs2_dinode dp2;
};
#define DIP(dp, field) \
	(is_ufs2 ? (dp)->dp2.di_##field : (dp)->dp1.di_##field)

#define DIP_SET(dp, field, val) do {		\
	if (is_ufs2)				\
		(dp)->dp2.di_##field = (val);	\
	else					\
		(dp)->dp1.di_##field = (val);	\
} while (0)

/*
 * Filestore-independent UFS data, so code can be more easily shared
 * between ffs, lfs, and maybe ext2fs and others as well.
 */
struct ufsi {
	int64_t ufs_dsize;	/* file system size, in sectors */
	int32_t ufs_bsize;	/* block size */
	int32_t ufs_bshift;	/* log2(ufs_bsize) */
	int32_t ufs_fsize;	/* fragment size */
	int32_t ufs_frag;	/* block size / frag size */
	int32_t ufs_fsatoda;	/* disk address conversion constant */
	int32_t	ufs_nindir;	/* disk addresses per indirect block */
	int32_t ufs_inopb;	/* inodes per block */
	int32_t ufs_maxsymlinklen; /* max symlink length */
	int32_t ufs_bmask;	/* block mask */
	int32_t ufs_fmask;	/* frag mask */
	int64_t ufs_qbmask;	/* ~ufs_bmask */
	int64_t ufs_qfmask;	/* ~ufs_fmask */
};
#define fsatoda(u,a) ((a) << (u)->ufs_fsatoda)
#define ufs_fragroundup(u,size) /* calculates roundup(size, ufs_fsize) */ \
	(((size) + (u)->ufs_qfmask) & (u)->ufs_fmask)
#define ufs_blkoff(u,loc)   /* calculates (loc % u->ufs_bsize) */ \
	((loc) & (u)->ufs_qbmask)
#define ufs_dblksize(u,d,b) \
	((((b) >= NDADDR || DIP((d), size) >= ((b)+1) << (u)->ufs_bshift \
		? (u)->ufs_bsize \
		: (ufs_fragroundup((u), ufs_blkoff(u, DIP((d), size)))))))
struct ufsi *ufsib;

/*
 * Dump maps used to describe what is to be dumped.
 */
int	mapsize;	/* size of the state maps */
char	*usedinomap;	/* map of allocated inodes */
char	*dumpdirmap;	/* map of directories to be dumped */
char	*dumpinomap;	/* map of files to be dumped */
/*
 * Map manipulation macros.
 */
#define	SETINO(ino, map) \
	map[(u_int)((ino) - 1) / NBBY] |=  1 << ((u_int)((ino) - 1) % NBBY)
#define	CLRINO(ino, map) \
	map[(u_int)((ino) - 1) / NBBY] &=  ~(1 << ((u_int)((ino) - 1) % NBBY))
#define	TSTINO(ino, map) \
	(map[(u_int)((ino) - 1) / NBBY] &  (1 << ((u_int)((ino) - 1) % NBBY)))

/*
 *	All calculations done in 0.1" units!
 */
char	*disk;		/* name of the disk file */
char	*disk_dev;	/* name of the raw device we are dumping */
const char *tape;	/* name of the tape file */
const char *dumpdates;	/* name of the file containing dump date information*/
const char *temp;	/* name of the file for doing rewrite of dumpdates */
char	lastlevel;	/* dump level of previous dump */
char	level;		/* dump level of this dump */
int	uflag;		/* update flag */
int	eflag;		/* eject flag */
int	lflag;		/* autoload flag */
int	diskfd;		/* disk file descriptor */
int	tapefd;		/* tape file descriptor */
int	pipeout;	/* true => output to standard output */
int	trueinc;	/* true => "true incremental", i.e use last 9 as ref */
ino_t	curino;		/* current inumber; used globally */
int	newtape;	/* new tape flag */
u_int64_t	tapesize;	/* estimated tape size, blocks */
long	tsize;		/* tape size in 0.1" units */
long	asize;		/* number of 0.1" units written on current tape */
int	etapes;		/* estimated number of tapes */
int	nonodump;	/* if set, do not honor UF_NODUMP user flags */
int	unlimited;	/* if set, write to end of medium */

extern int	density;	/* density in 0.1" units */
extern int	notify;		/* notify operator flag */
extern int	timestamp;	/* timestamp messages */
extern int	blockswritten;	/* number of blocks written on current tape */
extern int	tapeno;		/* current tape number */
extern int	is_ufs2;

time_t	tstart_writing;	/* when started writing the first tape block */
time_t	tstart_volume;	/* when started writing the current volume */
int	xferrate;	/* averaged transfer rate of all volumes */
char	sblock_buf[MAXBSIZE]; /* buffer to hold the superblock */
extern long	dev_bsize;	/* block size of underlying disk device */
int	dev_bshift;	/* log2(dev_bsize) */
int	tp_bshift;	/* log2(TP_BSIZE) */
int needswap;	/* file system in swapped byte order */


/* some inline functions to help the byte-swapping mess */
static inline u_int16_t iswap16(u_int16_t);
static inline u_int32_t iswap32(u_int32_t);
static inline u_int64_t iswap64(u_int64_t);

static inline u_int16_t iswap16(u_int16_t x)
{
	if (needswap)
		return bswap16(x);
	else
		return x;
}

static inline u_int32_t iswap32(u_int32_t x)
{
	if (needswap)
		return bswap32(x);
	else
		return x;
}

static inline u_int64_t iswap64(u_int64_t x)
{
	if (needswap)
		return bswap64(x);
	else
		return x;
}

/* filestore-specific hooks */
int	fs_read_sblock(char *);
struct ufsi *fs_parametrize(void);
ino_t	fs_maxino(void);
void	fs_mapinodes(ino_t, u_int64_t *, int *);

/* operator interface functions */
void	broadcast(const char *);
void	lastdump(char);
void	msg(const char *fmt, ...) __printflike(1, 2);
void	msgtail(const char *fmt, ...) __printflike(1, 2);
int	query(const char *);
void	quit(const char *fmt, ...) __printflike(1, 2);
time_t	do_stats(void);
void	statussig(int);
void	timeest(void);
time_t	unctime(const char *);

/* mapping routines */
union	dinode;
int64_t	blockest(union dinode *);
void	mapfileino(ino_t, u_int64_t *, int *);
int	mapfiles(ino_t, u_int64_t *, char *, char * const *);
int	mapdirs(ino_t, u_int64_t *);

/* file dumping routines */
void	blksout32(int32_t *, int, ino_t);
void	blksout64(int64_t *, int, ino_t);
void	dumpino(union dinode *, ino_t);
#ifndef RRESTORE
void	dumpmap(char *, int, ino_t);
#endif
void	writeheader(ino_t);

/* data block caching */
void	bread(daddr_t, char *, int);
void	rawread(daddr_t, char *, int);
void	initcache(int, int);
void	printcachestats(void);

/* tape writing routines */
int	alloctape(void);
void	close_rewind(void);
void	dumpblock(daddr_t, int);
void	startnewtape(int);
void	trewind(int);
void	writerec(const char *, int);

void	Exit(int);
void	dumpabort(int);
void	getfstab(void);

char	*rawname(char *);
union	dinode *getino(ino_t);

void	*xcalloc(size_t, size_t);
void	*xmalloc(size_t);
char	*xstrdup(const char *);

/* LFS snapshot hooks */
#ifdef DUMP_LFS
int	lfs_wrap_stop(char *);
void	lfs_wrap_go(void);
#endif

/* rdump routines */
#if defined(RDUMP) || defined(RRESTORE)
void	rmtclose(void);
int	rmthost(const char *);
int	rmtopen(const char *, int, int);
int	rmtwrite(const char *, int);
int	rmtioctl(int, int);
#endif /* RDUMP || RRESTORE */

void	interrupt(int);	/* in case operator bangs on console */

/*
 *	Exit status codes
 */
#define	X_FINOK		0	/* normal exit */
#define	X_STARTUP	1	/* startup error */
#define	X_REWRITE	2	/* restart writing from the check point */
#define	X_ABORT		3	/* abort dump; don't attempt checkpointing */

#define	OPGRENT	"operator"		/* group entry to notify */
#define DIALUP	"ttyd"			/* prefix for dialups */

struct	fstab *fstabsearch(const char *);	/* search fs_file and fs_spec */
struct	statvfs *mntinfosearch(const char *key);

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

/*
 *	The contents of the file _PATH_DUMPDATES is maintained both on
 *	a linked list, and then (eventually) arrayified.
 */
struct dumpdates {
	char	dd_name[NAME_MAX+3];
	char	dd_level;
	time_t	dd_ddate;
};

extern int	nddates;		/* number of records (might be zero) */
extern struct	dumpdates **ddatev;	/* the arrayfied version */

void	initdumptimes(void);
void	getdumptime(void);
void	putdumptime(void);
#define	ITITERATE(i, ddp) \
	if (ddatev != NULL) \
		for (ddp = ddatev[i = 0]; i < nddates; ddp = ddatev[++i])

void	sig(int signo);

#ifndef	_PATH_FSTAB
#define	_PATH_FSTAB	"/etc/fstab"
#endif
