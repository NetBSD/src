/*	$NetBSD: dump.h,v 1.28 2001/08/14 05:44:44 lukem Exp $	*/

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
 *	@(#)dump.h	8.2 (Berkeley) 4/28/95
 */

#include <machine/bswap.h>

#define MAXINOPB	(MAXBSIZE / sizeof(struct dinode))
#define MAXNINDIR	(MAXBSIZE / sizeof(daddr_t))

/*
 * Filestore-independent UFS data, so code can be more easily shared
 * between ffs, lfs, and maybe ext2fs and others as well.
 */
struct ufsi {
	int64_t ufs_dsize;	/* file system size, in sectors */
	int32_t ufs_bsize;	/* block size */
	int32_t ufs_bshift;	/* log2(ufs_bsize) */
	int32_t ufs_fsize;	/* fragment size */
	int32_t ufs_frag;       /* block size / frag size */
	int32_t ufs_fsatoda;	/* disk address conversion constant */
	int32_t	ufs_nindir;	/* disk addresses per indirect block */
	int32_t ufs_inopb;      /* inodes per block */
	int32_t ufs_maxsymlinklen; /* max symlink length */
	int32_t ufs_bmask;      /* block mask */
	int32_t ufs_fmask;      /* frag mask */
	int64_t ufs_qbmask;     /* ~ufs_bmask */
	int64_t ufs_qfmask;     /* ~ufs_fmask */
};
#define fsatoda(u,a) ((a) << (u)->ufs_fsatoda)
#define ufs_fragroundup(u,size) /* calculates roundup(size, ufs_fsize) */ \
        (((size) + (u)->ufs_qfmask) & (u)->ufs_fmask)
#define ufs_blkoff(u,loc)   /* calculates (loc % u->ufs_bsize) */ \
        ((loc) & (u)->ufs_qbmask)
#define ufs_dblksize(u,d,b) \
	((((b) >= NDADDR || (d)->di_size >= ((b)+1) << (u)->ufs_bshift \
		? (u)->ufs_bsize \
		: (ufs_fragroundup((u), ufs_blkoff(u, (d)->di_size))))))
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
char	*tape;		/* name of the tape file */
char	*dumpdates;	/* name of the file containing dump date information*/
char	*temp;		/* name of the file for doing rewrite of dumpdates */
char	lastlevel;	/* dump level of previous dump */
char	level;		/* dump level of this dump */
int	uflag;		/* update flag */
int	eflag;		/* eject flag */
int	diskfd;		/* disk file descriptor */
int	tapefd;		/* tape file descriptor */
int	pipeout;	/* true => output to standard output */
ino_t	curino;		/* current inumber; used globally */
int	newtape;	/* new tape flag */
long	tapesize;	/* estimated tape size, blocks */
long	tsize;		/* tape size in 0.1" units */
long	asize;		/* number of 0.1" units written on current tape */
int	etapes;		/* estimated number of tapes */
int	nonodump;	/* if set, do not honor UF_NODUMP user flags */

extern int	density;	/* density in 0.1" units */
extern int	notify;		/* notify operator flag */
extern int	blockswritten;	/* number of blocks written on current tape */
extern int	tapeno;		/* current tape number */

time_t	tstart_writing;	/* when started writing the first tape block */
int	xferrate;	/* averaged transfer rate of all volumes */
char	sblock_buf[MAXBSIZE]; /* buffer to hold the superblock */
extern long	dev_bsize;	/* block size of underlying disk device */
int	dev_bshift;	/* log2(dev_bsize) */
int	tp_bshift;	/* log2(TP_BSIZE) */
int needswap;	/* file system in swapped byte order */
/* some inline functs to help the byte-swapping mess */
static __inline u_int16_t iswap16(u_int16_t);
static __inline u_int32_t iswap32(u_int32_t);
static __inline u_int64_t iswap64(u_int64_t);

static __inline u_int16_t iswap16(u_int16_t x)
{
	if (needswap)
		return bswap16(x);
	else
		return x;
}

static __inline u_int32_t iswap32(u_int32_t x)
{
	if (needswap)
		return bswap32(x);
	else
		return x;
}

static __inline u_int64_t iswap64(u_int64_t x)
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

/* operator interface functions */
void	broadcast(char *);
void	lastdump(char);
void	msg(const char *fmt, ...) __attribute__((__format__(__printf__,1,2)));
void	msgtail(const char *fmt, ...) __attribute__((__format__(__printf__,1,2)));
int	query(char *);
void	quit(const char *fmt, ...) __attribute__((__format__(__printf__,1,2)));
void	set_operators(void);
time_t	do_stats(void);
void	statussig(int);
void	timeest(void);
time_t	unctime(char *);

/* mapping routines */
struct	dinode;
long	blockest(struct dinode *);
void	mapfileino(ino_t, long *, int *);
int	mapfiles(ino_t, long *, char *, char * const *);
int	mapdirs(ino_t, long *);

/* file dumping routines */
void	blksout(daddr_t *, int, ino_t);
void	dumpino(struct dinode *, ino_t);
void	dumpmap(char *, int, ino_t);
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
void	writerec(char *, int);

void	Exit(int);
void	dumpabort(int);
void	getfstab(void);

char	*rawname(char *);
struct	dinode *getino(ino_t);

void	*xcalloc(size_t, size_t);
void	*xmalloc(size_t);
char	*xstrdup(const char *);

/* rdump routines */
#if defined(RDUMP) || defined(RRESTORE)
void	rmtclose(void);
int	rmthost(char *);
int	rmtopen(char *, int);
int	rmtwrite(char *, int);
#endif /* RDUMP || RRESTORE */

void	interrupt(int);	/* in case operator bangs on console */

/*
 *	Exit status codes
 */
#define	X_FINOK		0	/* normal exit */
#define	X_REWRITE	2	/* restart writing from the check point */
#define	X_ABORT		3	/* abort dump; don't attempt checkpointing */

#define	OPGRENT	"operator"		/* group entry to notify */
#define DIALUP	"ttyd"			/* prefix for dialups */

struct	fstab *fstabsearch(const char *);	/* search fs_file and fs_spec */
struct	statfs *mntinfosearch(const char *key);

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
struct dumptime {
	struct	dumpdates dt_value;
	struct	dumptime *dt_next;
};

extern struct	dumptime *dthead;	/* head of the list version */
extern int	nddates;		/* number of records (might be zero) */
extern int	ddates_in;		/* we have read the increment file */
extern struct	dumpdates **ddatev;	/* the arrayfied version */

void	initdumptimes(void);
void	getdumptime(void);
void	putdumptime(void);
#define	ITITERATE(i, ddp) \
	for (ddp = ddatev[i = 0]; i < nddates; ddp = ddatev[++i])

void	sig(int signo);

/*
 * Compatibility with old systems.
 */
#ifdef COMPAT
#include <sys/file.h>
#define	strchr(a,b)	index(a,b)
#define	strrchr(a,b)	rindex(a,b)
extern char *strdup(), *ctime();
extern int read(), write();
extern int errno;
#endif

#ifndef	_PATH_UTMP
#define	_PATH_UTMP	"/etc/utmp"
#endif
#ifndef	_PATH_FSTAB
#define	_PATH_FSTAB	"/etc/fstab"
#endif
