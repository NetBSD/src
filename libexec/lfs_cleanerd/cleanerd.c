/*	$NetBSD: cleanerd.c,v 1.47 2003/04/02 10:39:22 fvdl Exp $	*/

/*-
 * Copyright (c) 1992, 1993
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
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1992, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#if 0
static char sccsid[] = "@(#)cleanerd.c	8.5 (Berkeley) 6/10/95";
#else
__RCSID("$NetBSD: cleanerd.c,v 1.47 2003/04/02 10:39:22 fvdl Exp $");
#endif
#endif /* not lint */

#include <sys/fcntl.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>

#include <ufs/ufs/dinode.h>
#include <ufs/lfs/lfs.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <util.h>
#include <errno.h>
#include <err.h>

#include <syslog.h>

#include "clean.h"
char *special = "cleanerd";
int do_small = 0;
int do_mmap = 0;
int do_quit = 0;
int stat_report = 0;
int debug = 0;
int segwait_timeout = 5*60; /* Five minutes */
double load_threshold = 0.2;
int use_fs_idle = 0;
struct cleaner_stats {
	double	util_tot;
	double	util_sos;
	int	blocks_read;
	int	blocks_written;
	int	segs_cleaned;
	int	segs_empty;
	int	segs_error;
} cleaner_stats;

struct seglist { 
	unsigned long sl_id;	/* segment number */
	unsigned long sl_cost; 	/* cleaning cost */
	unsigned long sl_bytes;	/* bytes in segment */
	unsigned long sl_age;	/* age in seconds */
};

struct tossstruct {
	struct lfs *lfs;
	int seg;
};

typedef struct {
	int nsegs;      /* number of segments */
	struct seglist **segs; /* segment numbers, costs, etc */
	int nb;         /* total number of blocks */
	BLOCK_INFO *ba; /* accumulated block_infos */
	caddr_t *buf;   /* segment buffers */
} SEGS_AND_BLOCKS;

#define	CLEAN_BYTES	0x1

/* function prototypes */
int	 bi_tossold(const void *, const void *, const void *);
int	 choose_segments(FS_INFO *, struct seglist *, 
	     unsigned long (*)(FS_INFO *, SEGUSE *));
void	 clean_fs(FS_INFO	*, unsigned long (*)(FS_INFO *, SEGUSE *), int, long);
int	 clean_loop(FS_INFO *, int, long);
int	 add_segment(FS_INFO *, struct seglist *, SEGS_AND_BLOCKS *);
int	 clean_segments(FS_INFO *, SEGS_AND_BLOCKS *);
unsigned long	 cost_benefit(FS_INFO *, SEGUSE *);
int	 cost_compare(const void *, const void *);
void	 sig_report(int);
void	 just_exit(int);
int	 main(int, char *[]);

/*
 * Emulate lfs_{bmapv,markv,segwait} using fcntl calls.
 * NOTE: the old system calls still use BLOCK_INFO_15,
 * while the fcntls use BLOCK_INFO.
 */
int
lfs_markv_emul(int fd, BLOCK_INFO *blkiov, int blkcnt)
{
	struct lfs_fcntl_markv /* {
		BLOCK_INFO *blkiov;
		int blkcnt;
	} */ lim;

	lim.blkiov = blkiov;
	lim.blkcnt = blkcnt;
	return fcntl(fd, LFCNMARKV, &lim);
}

int
lfs_bmapv_emul(int fd, BLOCK_INFO *blkiov, int blkcnt)
{
	struct lfs_fcntl_markv /* {
		BLOCK_INFO *blkiov;
		int blkcnt;
	} */ lim;

	lim.blkiov = blkiov;
	lim.blkcnt = blkcnt;
	return fcntl(fd, LFCNBMAPV, &lim);
}

int
lfs_segwait_emul(int fd, struct timeval *tv)
{
	return fcntl(fd, LFCNSEGWAIT, tv);
}

/*
 * Cleaning Cost Functions:
 *
 * These return the cost of cleaning a segment.  The higher the cost value
 * the better it is to clean the segment, so empty segments have the highest
 * cost.  (It is probably better to think of this as a priority value
 * instead).
 *
 * This is the cost-benefit policy simulated and described in Rosenblum's
 * 1991 SOSP paper.
 */

unsigned long
cost_benefit(FS_INFO *fsp, SEGUSE *su)
{
	struct lfs *lfsp;
	struct timeval t;
	time_t age;
	unsigned long live;

	gettimeofday(&t, NULL);

	live = su->su_nbytes;	
	age = t.tv_sec < su->su_lastmod ? 0 : t.tv_sec - su->su_lastmod;
	
	lfsp = &fsp->fi_lfs;
	if (live == 0) { /* No cost, only benefit. */
		return lblkno(lfsp, seg_size(lfsp)) * t.tv_sec;
	} else if (su->su_flags & SEGUSE_ERROR) {
		/* No benefit: don't even try */
		return 0;
	} else {
		/* 
		 * from lfsSegUsage.c (Mendel's code).
		 * priority calculation is done using INTEGER arithmetic.
		 * sizes are in BLOCKS (that is why we use lblkno below).
		 * age is in seconds.
		 *
		 * priority = ((seg_size - live) * age) / (seg_size + live)
		 */
		if (live < 0 || live > seg_size(lfsp)) {
			syslog(LOG_WARNING,"bad segusage count: %ld", live);
			live = 0;
		}
		return (lblkno(lfsp, seg_size(lfsp) - live) * age)
			/ lblkno(lfsp, seg_size(lfsp) + live);
	}
}

int
main(int argc, char **argv)
{
	FS_INFO	*fsp;
	struct statfs *lstatfsp;	/* file system stats */
	struct timeval timeout;		/* sleep timeout */
	fsid_t fsid;
	long clean_opts;		/* cleaning options */
	int segs_per_clean;
	int opt, cmd_err;
	int do_coaleace;
	pid_t childpid;
	char *fs_name;			/* name of filesystem to clean */
	time_t now, lasttime;
	int loopcount;
	char *pidname;                  /* Name of pid file base */
	char *cp;

	cmd_err = debug = do_quit = 0;
	do_coaleace = clean_opts = 0;
	segs_per_clean = 1;
	while ((opt = getopt(argc, argv, "bcdfl:mn:qr:st:")) != -1) {
		switch (opt) {
			case 'b':
				/*
				 * Use live bytes to determine
				 * how many segs to clean.
				 */
				clean_opts |= CLEAN_BYTES;
				break;
			case 'c':
				debug++;
				do_coaleace++;
				break;
			case 'd':	/* Debug mode. */
				debug++;
				break;
			case 'f':
				use_fs_idle = 1;
				break;
			case 'l':       /* Load below which to clean */
				load_threshold = atof(optarg);
				break;
			case 'm':
				do_mmap = 1;
				break;
			case 'n':	/* How many segs to clean at once */
				segs_per_clean = atoi(optarg);
				break;
			case 'q':	/* Quit after one run */
				do_quit = 1;
				break;
			case 'r':	/* Report every stat_report segments */
				stat_report = atoi(optarg);
				break;
			case 's':	/* small writes */
				do_small = 1;
				break;
			case 't':
				segwait_timeout = atoi(optarg);
				break;
			default:
				++cmd_err;
		}
	}
	argc -= optind;
	argv += optind;
	if (cmd_err || (argc != 1))
		errx(1, "usage: lfs_cleanerd [-bdms] [-l load] [-n nsegs] [-r report_freq] [-t timeout] fs_name");

	fs_name = argv[0];

	if (fs_getmntinfo(&lstatfsp, fs_name, MOUNT_LFS) == 0) {
		/* didn't find the filesystem */
		errx(1, "lfs_cleanerd: filesystem %s isn't an LFS!", fs_name);
	}

	/* should we become a daemon, chdir to / & close fd's */
	if (debug == 0) {
		if (daemon(0, 0) == -1)
			err(1, "lfs_cleanerd: couldn't become a daemon!");
		openlog("lfs_cleanerd", LOG_NDELAY | LOG_PID | 0, LOG_DAEMON);
		lasttime=0;
		loopcount=0;
	    loop:
		if((childpid=fork())<0) {
			syslog(LOG_ERR,"%s: couldn't fork, exiting: %m",
				fs_name);
			exit(1);
		}
		if(childpid == 0) {
			/* Record child's pid */
			pidname = malloc(strlen(fs_name) + 16);
			sprintf(pidname, "lfs_cleanerd:s:%s", fs_name);
			while((cp = strchr(pidname, '/')) != NULL)
				*cp = '|';
			pidfile(pidname);
			/* The cleaner wants to stay in core, really */
			if (mlockall(MCL_FUTURE) != 0) {
				syslog(LOG_WARNING, "mlockall failed: %m");
			}
		} else {
			/* Record parent's pid */
			pidname = malloc(strlen(fs_name) + 16);
			sprintf(pidname, "lfs_cleanerd:m:%s", fs_name);
			while((cp = strchr(pidname, '/')) != NULL)
				*cp = '|';
			pidfile(pidname);
			signal(SIGINT, just_exit);

			wait(NULL);
			/* If the child is looping, give up */
			++loopcount;
			if((now=time(NULL)) - lasttime > TIME_THRESHOLD) {
				loopcount=0;
			}
			lasttime = now;
			if(loopcount > LOOP_THRESHOLD) {
				syslog(LOG_ERR,"%s: cleanerd looping, exiting",
					fs_name);
				exit(1);
			}
			if (fs_getmntinfo(&lstatfsp, fs_name, MOUNT_LFS) == 0) {
				/* fs has been unmounted(?); exit quietly */
				syslog(LOG_ERR,"lfs_cleanerd: fs %s unmounted, exiting", fs_name);
				exit(0);
			}
			goto loop;
		}
	} else
		openlog("lfs_cleanerd", LOG_NDELAY | LOG_PID | LOG_PERROR,
		    LOG_DAEMON);

	signal(SIGINT, sig_report);
	signal(SIGUSR1, sig_report);
	signal(SIGUSR2, sig_report);

	if(debug)
		syslog(LOG_INFO,"Cleaner starting on filesystem %s", fs_name);

	timeout.tv_sec = segwait_timeout;
	timeout.tv_usec = 0;
	fsid.val[0] = 0;
	fsid.val[1] = 0;

	for (fsp = get_fs_info(lstatfsp, do_mmap); ;
	    reread_fs_info(fsp, do_mmap)) {
		/*
		 * If the user specified '-F', he doesn't want us
		 * to do regular cleaning, only coalesce.
		 */
		if (do_coaleace) {
			clean_all_inodes(fsp);
			exit(0);
		}

		/*
		 * clean the filesystem, and, if it needed cleaning
		 * (i.e. it returned nonzero) try it again
		 * to make sure that some nasty process hasn't just
		 * filled the disk system up.
		 */
		if (clean_loop(fsp, segs_per_clean, clean_opts))
			continue;

		fsid = lstatfsp->f_fsid;
		if(debug > 1)
			syslog(LOG_DEBUG,"Cleaner going to sleep.");
		if (lfs_segwait_emul(ifile_fd, &timeout) < 0)
			syslog(LOG_WARNING,"LFCNSEGWAIT: %m");
		if(debug > 1)
			syslog(LOG_DEBUG,"Cleaner waking up.");
	}
}

/* return the number of segments cleaned */
int
clean_loop(FS_INFO *fsp, int nsegs, long options)
{
	struct lfs *lfsp;
	double loadavg[MAXLOADS];
	time_t	now;
	u_long max_free_segs;
	u_long fsb_per_seg;

	lfsp = &fsp->fi_lfs;
	/*
	 * Compute the maximum possible number of free segments, given the
	 * number of free blocks.
	 */
	fsb_per_seg = segtod(lfsp, 1);
	max_free_segs = fsp->fi_cip->bfree / fsb_per_seg + lfsp->lfs_minfreeseg;
	
	/* 
	 * We will clean if there are not enough free blocks or total clean
	 * space is less than BUSY_LIM % of possible clean space.
	 */
	now = time((time_t *)NULL);

	if(debug > 1) {
		syslog(LOG_DEBUG, "fsb_per_seg = %lu bfree = %u avail = %d",
		       fsb_per_seg, fsp->fi_cip->bfree, fsp->fi_cip->avail);
		syslog(LOG_DEBUG, "clean segs = %d, max_free_segs = %ld",
		       fsp->fi_cip->clean, max_free_segs);
	}

	if ((fsp->fi_cip->bfree - fsp->fi_cip->avail > fsb_per_seg &&
	     fsp->fi_cip->avail < (long)fsb_per_seg &&
	     fsp->fi_cip->bfree > (long)fsb_per_seg) ||
	    (fsp->fi_cip->clean < max_free_segs &&
	     (fsp->fi_cip->clean <= lfsp->lfs_minfreeseg ||
	      fsp->fi_cip->clean < max_free_segs * BUSY_LIM)))
	{
		if(debug)
			syslog(LOG_DEBUG, "Cleaner Running  at %s"
			       " (%d of %lu segments available, avail = %d,"
			       " bfree = %u)",
			       ctime(&now), fsp->fi_cip->clean, max_free_segs,
			       fsp->fi_cip->avail, fsp->fi_cip->bfree);
		clean_fs(fsp, cost_benefit, nsegs, options);
		if(do_quit) {
			if(debug)
				syslog(LOG_INFO,"Cleaner shutting down");
			exit(0);
		}
		return (1);
	} else if(use_fs_idle) {
		/*
		 * If we're using "filesystem idle" instead of system idle,
		 * clean if the fs has not been modified in segwait_timeout
		 * seconds.
		 */
		if(now-fsp->fi_fs_tstamp > segwait_timeout
		   && fsp->fi_cip->clean < max_free_segs * IDLE_LIM) {
			if(debug) {
				syslog(LOG_DEBUG, "Cleaner Running  at %s: "
				       "fs idle time %ld sec; %d of %lu segments available)",
				       ctime(&now), (long)now-fsp->fi_fs_tstamp,
				       fsp->fi_cip->clean, max_free_segs);
				syslog(LOG_DEBUG, "  filesystem idle since %s", ctime(&(fsp->fi_fs_tstamp)));
			}
			clean_fs(fsp, cost_benefit, nsegs, options);
			if(do_quit) {
				if(debug)
					syslog(LOG_INFO,"Cleaner shutting down");
				exit(0);
			}
		return (1);
		}
	} else {
	        /* 
		 * We will also clean if the system is reasonably idle and
		 * the total clean space is less than IDLE_LIM % of possible
		 * clean space.
		 */
		if (getloadavg(loadavg, MAXLOADS) == -1) {
			perror("getloadavg: failed");
			return (-1);
		}
		if (loadavg[ONE_MIN] < load_threshold
		    && fsp->fi_cip->clean < max_free_segs * IDLE_LIM)
		{
			if (debug)
				syslog(LOG_DEBUG, "Cleaner Running  at %s "
				       "(system load %.1f, %d of %lu segments available)",
					ctime(&now), loadavg[ONE_MIN],
					fsp->fi_cip->clean, max_free_segs);
		        clean_fs(fsp, cost_benefit, nsegs, options);
			if (do_quit) {
				if(debug)
					syslog(LOG_INFO,"Cleaner shutting down");
				exit(0);
			}
			return (1);
		}
	}
	if (debug > 1) {
		if (fsp->fi_cip->bfree - fsp->fi_cip->avail <= fsb_per_seg)
			syslog(LOG_DEBUG, "condition 1 false");
		if (fsp->fi_cip->avail >= (long)fsb_per_seg)
			syslog(LOG_DEBUG, "condition 2 false");
		if (fsp->fi_cip->clean >= max_free_segs)
			syslog(LOG_DEBUG, "condition 3 false");
		if (fsp->fi_cip->clean > lfsp->lfs_minfreeseg)
			syslog(LOG_DEBUG, "condition 4 false");
		if (fsp->fi_cip->clean >= max_free_segs * BUSY_LIM)
			syslog(LOG_DEBUG, "condition 5 false");

		syslog(LOG_DEBUG, "Cleaner Not Running at %s", ctime(&now));
	}
	return (0);
}


void
clean_fs(FS_INFO *fsp, unsigned long (*cost_func)(FS_INFO *, SEGUSE *),
	 int nsegs, long options)
{
	struct seglist *segs, *sp;
	long int to_clean, cleaned_bytes;
	unsigned long i, j, total;
	struct rusage ru;
	fsid_t *fsidp;
	int error;
	SEGS_AND_BLOCKS *sbp;

	fsidp = &fsp->fi_statfsp->f_fsid;

	if ((segs =
	    malloc(fsp->fi_lfs.lfs_nseg * sizeof(struct seglist))) == NULL) {
		syslog(LOG_WARNING,"malloc failed: %m");
		return;
	}
	total = i = choose_segments(fsp, segs, cost_func);

	/*
	 * If we can get lots of cleaning for free, do it now.
	 * The theory is that if empty dirty segments exist, we
	 * can afford to write a bunch of inodes, directory blocks,
	 * and Ifile blocks in order to clean them.  (If things are
	 * so bad that we don't have enough segments to do this, though,
	 * we're in trouble.)
	 */
	sp = segs;
	for (j = 0; j < total && sp->sl_bytes == 0; j++) {
		if (debug)
			syslog(LOG_DEBUG,"Not cleaning empty segment %ld",
				sp->sl_id);
		++cleaner_stats.segs_empty;
		sp++;
		i--;
	}
	if (j > 0) {
		/* Call limited checkpoint to help clean empty segs */
		fcntl(ifile_fd, LFCNRECLAIM, 0);
	}
	if (j > nsegs) {
		free(segs);
		return;
	}

#if 0
	/* If we relly need to clean a lot, do it now */
	if(fsp->fi_cip->clean < 2 * fsp->fi_lfs.lfs_minfreeseg)
		nsegs = MAX(nsegs, fsp->fi_lfs.lfs_minfreeseg);
#endif
	/* But back down if we haven't got that many free to clean into */
	if(fsp->fi_cip->clean < nsegs)
		nsegs = fsp->fi_cip->clean;

	if (debug > 1)
		syslog(LOG_DEBUG, "clean_fs: found %ld segments to clean in %s",
			i, fsp->fi_statfsp->f_mntonname);

	if (i) {
		sbp = (SEGS_AND_BLOCKS *)malloc(sizeof(SEGS_AND_BLOCKS));
		memset(sbp, 0, sizeof(SEGS_AND_BLOCKS));

		/* Check which cleaning algorithm to use. */
		if (options & CLEAN_BYTES) {
			/* Count bytes */
			cleaned_bytes = 0;
			to_clean = nsegs * fsp->fi_lfs.lfs_ssize;
			for (; i && cleaned_bytes < to_clean; i--, ++sp) {
				if (add_segment(fsp, sp, sbp) < 0) {
					syslog(LOG_WARNING,"add_segment failed"
					       " segment %ld: %m", sp->sl_id);
					if (sbp->nsegs == 0 && errno != ENOENT)
						continue;
					else
						break;
				}
				cleaned_bytes += sp->sl_bytes;
			}
		} else {
			/* Count segments */
			for (i = MIN(i, nsegs); i-- ; ++sp) {
				total--;
				syslog(LOG_DEBUG, "Cleaning segment %ld"
				       " (of %ld choices)", sp->sl_id, i + 1);
				if (add_segment(fsp, sp, sbp) != 0) {
					syslog(LOG_WARNING,"add_segment failed"
					       " segment %ld: %m", sp->sl_id);
					if (sbp->nsegs == 0 && errno != ENOENT)
						continue;
					else
						break;
				}
			}
		}
		clean_segments(fsp, sbp);

		if (sbp->buf)
			free(sbp->buf);
		if (sbp->segs)
			free(sbp->segs);
		free(sbp);
	}
	free(segs);
	if(debug) {
		error = getrusage(RUSAGE_SELF, &ru);
		if(error) {
			syslog(LOG_WARNING, "getrusage returned error: %m");
		} else {
			syslog(LOG_DEBUG, "Current usage: maxrss=%ld,"
			       " idrss=%ld, isrss=%ld", ru.ru_maxrss,
			       ru.ru_idrss, ru.ru_isrss);
		}
	}
}

/*
 * Segment with the highest priority get sorted to the beginning of the
 * list.  This sort assumes that empty segments always have a higher
 * cost/benefit than any used segment.
 */
int
cost_compare(const void *a, const void *b)
{
	return ((struct seglist *)b)->sl_cost < ((struct seglist *)a)->sl_cost ? -1 : 1;
}


/*
 * Returns the number of segments to be cleaned with the elements of seglist
 * filled in.
 */
int
choose_segments(FS_INFO *fsp, struct seglist *seglist, unsigned long (*cost_func)(FS_INFO *, SEGUSE *))
{
	struct lfs *lfsp;
	struct seglist *sp;
	SEGUSE *sup;
	int i, nsegs;

	lfsp = &fsp->fi_lfs;

	if (debug > 1)
		syslog(LOG_DEBUG,"Entering choose_segments");
	dump_super(lfsp);
	dump_cleaner_info(fsp->fi_cip);

	for (sp = seglist, i = 0; i < lfsp->lfs_nseg; ++i) {
		if (debug > 1) {
			printf("%d...", i);
			fflush(stdout);
		}
		sup = SEGUSE_ENTRY(lfsp, fsp->fi_segusep, i);
		if (debug > 2)
			PRINT_SEGUSE(sup, i);
		if (!(sup->su_flags & SEGUSE_DIRTY) ||
		    sup->su_flags & SEGUSE_ACTIVE)
			continue;
		if (debug > 2)
			syslog(LOG_DEBUG, "\tchoosing segment %d", i);
		sp->sl_cost = (*cost_func)(fsp, sup);
		sp->sl_id = i;
		sp->sl_bytes = sup->su_nbytes;
		sp->sl_age = time(NULL) - sup->su_lastmod;
		++sp;
	}
	nsegs = sp - seglist;
	if (debug > 1) {
		putchar('\n');
		syslog(LOG_DEBUG, "Sorting...");
	}
	qsort(seglist, nsegs, sizeof(struct seglist), cost_compare);
	if (debug > 2)
		for(i = 0; i < nsegs; i++) {
			syslog(LOG_DEBUG, "%d: segment %lu age %lu"
			       " contains %lu priority %lu\n", i,
			       seglist[i].sl_age, seglist[i].sl_id,
			       seglist[i].sl_bytes, seglist[i].sl_cost);
		}

	if (debug > 1)
		syslog(LOG_DEBUG,"Returning %d segments", nsegs);

	return (nsegs);
}

/*
 * Add still-valid blocks from the given segment to the block array,
 * in preparation for sending through markv.
 */
int
add_segment(FS_INFO *fsp, struct seglist *slp, SEGS_AND_BLOCKS *sbp)
{
	int id = slp->sl_id;
	BLOCK_INFO *tba, *_bip;
	SEGUSE *sp;
	struct lfs *lfsp;
	struct tossstruct t;
	struct ufs1_dinode *dip;
	caddr_t seg_buf;
	caddr_t cmp_buf, cmp_dp;
	size_t size;
	daddr_t seg_addr;
	int num_blocks, i, j, error;
	int seg_isempty=0;
	unsigned long *lp;

	lfsp = &fsp->fi_lfs;
	sp = SEGUSE_ENTRY(lfsp, fsp->fi_segusep, id);
	seg_addr = sntod(lfsp,id);
	error = 0;
	tba = NULL;

	syslog(LOG_DEBUG, "adding segment %d: contains %lu bytes", id,
	       (unsigned long)sp->su_nbytes);

	/* XXX could add debugging to verify that segment is really empty */
	if (sp->su_nbytes == 0) {
		++cleaner_stats.segs_empty;
		++seg_isempty;
	}

	/* Add a new segment to the accumulated list */
	sbp->nsegs++;
	sbp->segs = (struct seglist **)realloc(sbp->segs, sizeof(struct seglist *) * sbp->nsegs);
	sbp->buf = (caddr_t *)realloc(sbp->buf, sizeof(caddr_t) * sbp->nsegs);
	sbp->segs[sbp->nsegs - 1] = slp;

	/* map the segment into a buffer */
	if (mmap_segment(fsp, id, &seg_buf, do_mmap) < 0) {
		syslog(LOG_WARNING,"add_segment: mmap_segment failed: %m");
		++cleaner_stats.segs_error;
		--sbp->nsegs;
		return (-1);
	}
	sbp->buf[sbp->nsegs - 1] = seg_buf;
	/* get a list of blocks that are contained by the segment */
	if ((error = lfs_segmapv(fsp, id, seg_buf, &tba, &num_blocks)) < 0) {
		syslog(LOG_WARNING,
		       "add_segment: lfs_segmapv failed for segment %d", id);
		goto out;
	}
	cleaner_stats.blocks_read += segtod(lfsp, 1);

	if (debug > 1)
		syslog(LOG_DEBUG, "lfs_segmapv returned %d blocks", num_blocks);

	/* get the current disk address of blocks contained by the segment */
	if ((error = lfs_bmapv_emul(ifile_fd, tba, num_blocks)) < 0) {
		syslog(LOG_WARNING, "add_segment: LFCNBMAPV failed");
		goto out;
	}

	/* Now toss any blocks not in the current segment */
	t.lfs = lfsp;
	t.seg = id;
	toss(tba, &num_blocks, sizeof(BLOCK_INFO), bi_tossold, &t);
	/* Check if last element should be tossed */
	if (num_blocks && bi_tossold(&t, tba + num_blocks - 1, NULL))
		--num_blocks;

	if(seg_isempty) {
		if(num_blocks)
			syslog(LOG_WARNING,"segment %d was supposed to be empty, but has %d live blocks!", id, num_blocks);
		else
			syslog(LOG_DEBUG,"segment %d is empty, as claimed", id);
	}
	/* XXX KS - check for misplaced blocks */
	/* XXX ondisk32 */
	for(i=0; i<num_blocks; i++) {
		if(tba[i].bi_daddr
		   && tba[i].bi_daddr != seg_addr +
				btofsb(lfsp, (char *)(tba[i].bi_bp) - seg_buf)
		   && dtosn(&(fsp->fi_lfs), tba[i].bi_daddr) == id)
		{
			syslog(LOG_ERR, "bi_daddr = 0x%x = %lldb; %p - %p = %ld",
				tba[i].bi_daddr,
				(long long)fsbtob(lfsp, tba[i].bi_daddr - seg_addr),
				tba[i].bi_bp, seg_buf,
				(long)(((char *)(tba[i].bi_bp) - seg_buf)));
			syslog(LOG_ERR, "seg %d (0x%llx), ino %d lbn %d, 0x%x != 0x%llx",
			       id, (long long)seg_addr,
			       tba[i].bi_inode,
			       tba[i].bi_lbn,
			       tba[i].bi_daddr,
			       (long long)(seg_addr +
			       btofsb(lfsp, (char *)(tba[i].bi_bp) - seg_buf)));
			error = EFAULT;
			goto out;

			/*
			 * XXX KS - have to be careful here about Inodes;
			 * if bmapv shows them somewhere else in the
			 * segment from where we thought, we need to reload
			 * the *right* inode, not the first one in the block.
			 */
			/* XXX ondisk32 */
			if(tba[i].bi_lbn == LFS_UNUSED_LBN) {
				dip = (struct ufs1_dinode *)(seg_buf + fsbtob(lfsp, tba[i].bi_daddr - seg_addr));
				for(j=INOPB(lfsp)-1;j>=0;j--) {
					if(dip[j].di_u.inumber == tba[i].bi_inode) {
						tba[i].bi_bp = (char *)(dip+j);
						break;
					}
				}
				if(j<0) {
					syslog(LOG_ERR, "lost inode %d in the shuffle! (blk %d)",
					       tba[i].bi_inode, tba[i].bi_daddr);
					if (debug) {
						syslog(LOG_DEBUG,
						   "inode numbers found were:");
						for(j=INOPB(lfsp)-1;j>=0;j--) {
							syslog(LOG_DEBUG, "%d",
							   dip[j].di_u.inumber);
						}
					}
					errx(1, "lost inode");
				} else if (debug > 1) {
					syslog(LOG_DEBUG,"Ino %d corrected to 0x%x",
					       tba[i].bi_inode,
					       tba[i].bi_daddr);
				}
			} else {
				tba[i].bi_bp = seg_buf + fsbtob(lfsp, tba[i].bi_daddr - seg_addr);
			}
		}
	}

	/* Update live bytes calc - XXX KS */
	slp->sl_bytes = 0;
	for(i=0; i<num_blocks; i++)
		if(tba[i].bi_lbn == LFS_UNUSED_LBN)
			slp->sl_bytes += sizeof(struct ufs1_dinode);
		else
			slp->sl_bytes += tba[i].bi_size;

	if(debug > 1) {
		syslog(LOG_DEBUG, "after bmapv still have %d blocks",
			num_blocks);
		if (num_blocks)
			syslog(LOG_DEBUG, "BLOCK INFOS");
		for (_bip = tba, i=0; i < num_blocks; ++_bip, ++i) {
			PRINT_BINFO(_bip);
			/* XXX ondisk32? */
			lp = (u_long *)_bip->bi_bp;
		}
	}

	/* Compress segment buffer, if necessary */
	if (!do_mmap && slp->sl_bytes < seg_size(lfsp) / 2) {
		if (debug > 1)
			syslog(LOG_DEBUG, "compressing: %d < %d",
				(int)slp->sl_bytes, seg_size(lfsp) / 2);
		cmp_buf = malloc(slp->sl_bytes); /* XXX could do in-place */
		if (cmp_buf == NULL) {
			if (debug)
				syslog(LOG_DEBUG, "can't compress segment: %m");
		} else {
			cmp_dp = cmp_buf;
			for (i = 0; i < num_blocks; i++) {
				if(tba[i].bi_lbn == LFS_UNUSED_LBN)
					size = sizeof(struct ufs1_dinode);
				else
					size = tba[i].bi_size;
				memcpy(cmp_dp, tba[i].bi_bp, size);
				tba[i].bi_bp = cmp_dp;
				cmp_dp += size;
			}
			free(seg_buf);
			seg_buf = cmp_buf;
			sbp->buf[sbp->nsegs - 1] = seg_buf;
		}
	}

	/* Add these blocks to the accumulated list */
	sbp->ba = realloc(sbp->ba, (sbp->nb + num_blocks) * sizeof(BLOCK_INFO));
	memcpy(sbp->ba + sbp->nb, tba, num_blocks * sizeof(BLOCK_INFO));
	sbp->nb += num_blocks;

	free(tba);
	return (0);

    out:
	--sbp->nsegs;
	if (tba)
		free(tba);
	if (error) {
		sp->su_flags |= SEGUSE_ERROR;
		++cleaner_stats.segs_error;
	}
	munmap_segment(fsp, sbp->buf[sbp->nsegs], do_mmap);
	if (stat_report && cleaner_stats.segs_cleaned % stat_report == 0)
		sig_report(SIGUSR1);
	return (error);
}

/* Call markv and clean up */
int
clean_segments(FS_INFO *fsp, SEGS_AND_BLOCKS *sbp)
{
	int maxblocks, clean_blocks, icount, extra, ebytes, nbytes;
	BLOCK_INFO *bp;
	int i, error;
	double util;
	ino_t ino, inino;

	error = 0;

	cleaner_stats.segs_cleaned += sbp->nsegs;
	cleaner_stats.blocks_written += sbp->nb;
	/*
	 * Count up the number of indirect blocks and inodes we'll
	 * have to write to take care of this (if we are asked to do this).
	 * XXX this only cares about single indirect blocks.
	 */
	icount = 0;
	ino = inino = 0;
	extra = 0;
	nbytes = 0;
	for (i = sbp->nb, bp = sbp->ba; i > 0; bp++, i--) {
		if (bp->bi_lbn != LFS_UNUSED_LBN)
			nbytes += bp->bi_size;
		if (ino != bp->bi_inode) {
			ino = bp->bi_inode;
			++icount;
		}
		if (bp->bi_lbn == -NDADDR)
			inino = ino;
		if (inino != ino && bp->bi_lbn > 0 && bp->bi_lbn > NDADDR) {
			++extra;
			inino = ino;
		}
	}
	ebytes =  0 + INOPB(&fsp->fi_lfs) * fsp->fi_lfs.lfs_ibsize ;
	ebytes += extra * fsp->fi_lfs.lfs_bsize;
	if (debug) {
		fprintf(stderr, "clean_segment: %d inodes %d indirect -> %d bytes + %d = %d total (to save %d)\n",
			icount, extra, ebytes, nbytes, ebytes + nbytes,
			fsp->fi_lfs.lfs_fsize * fsp->fi_lfs.lfs_fsbpseg * sbp->nsegs);
	}
	/* If we're writing more than we're saving, try coalescing */
	if (ebytes + nbytes > fsp->fi_lfs.lfs_fsize * fsp->fi_lfs.lfs_fsbpseg * sbp->nsegs) {
		fork_coalesce(fsp);
	}

	util = ((double)sbp->nb / segtod(&fsp->fi_lfs, 1));
	cleaner_stats.util_tot += util;
	cleaner_stats.util_sos += util * util;
	if (do_small)
		maxblocks = MAXPHYS / fsp->fi_lfs.lfs_bsize - 1;
	else
		maxblocks = sbp->nb;

	for (bp = sbp->ba; sbp->nb > 0; bp += clean_blocks) {
		clean_blocks = maxblocks < sbp->nb ? maxblocks : sbp->nb;
		if ((error = lfs_markv_emul(ifile_fd, bp, clean_blocks)) < 0) {
			syslog(LOG_WARNING,"clean_segment: LFCNMARKV failed: %m");
			++cleaner_stats.segs_error;
			if (errno == ENOENT) break;
		}
		sbp->nb -= clean_blocks;
	}

	/* Clean up */
	if (sbp->ba)
		free(sbp->ba);
	if (error)
		++cleaner_stats.segs_error;
	for (i = 0; i < sbp->nsegs; i++)
		munmap_segment(fsp, sbp->buf[i], do_mmap);
	if (stat_report && cleaner_stats.segs_cleaned % stat_report == 0)
		sig_report(SIGUSR1);
	return (error);
}


int
bi_tossold(const void *client, const void *a, const void *b)
{
	const struct tossstruct *t;

	t = (struct tossstruct *)client;

	return (((BLOCK_INFO *)a)->bi_daddr == LFS_UNUSED_DADDR ||
	    dtosn(t->lfs, ((BLOCK_INFO *)a)->bi_daddr) != t->seg);
}

void
sig_report(int sig)
{
	double avg = 0.0;

	syslog(LOG_INFO, "lfs_cleanerd:\t%s%d\n\t\t%s%d\n\t\t%s%d\n\t\t%s%d\n\t\t%s%d",
		"blocks_read    ", cleaner_stats.blocks_read,
		"blocks_written ", cleaner_stats.blocks_written,
		"segs_cleaned   ", cleaner_stats.segs_cleaned,
		"segs_empty     ", cleaner_stats.segs_empty,
		"seg_error      ", cleaner_stats.segs_error);
	syslog(LOG_INFO, "\t\t%s%5.2f\n\t\t%s%5.2f",
		"util_tot       ", cleaner_stats.util_tot,
		"util_sos       ", cleaner_stats.util_sos);
	avg = cleaner_stats.util_tot / MAX(cleaner_stats.segs_cleaned, 1.0);
	syslog(LOG_INFO, "\t\tavg util: %4.2f std dev: %9.6f", avg,
		cleaner_stats.util_sos / MAX(cleaner_stats.segs_cleaned - avg * avg, 1.0));


	if (sig == SIGUSR2) {
		cleaner_stats.blocks_read = 0;
		cleaner_stats.blocks_written = 0;
		cleaner_stats.segs_cleaned = 0;
		cleaner_stats.segs_empty = 0;
		cleaner_stats.segs_error = 0;
		cleaner_stats.util_tot = 0.0;
		cleaner_stats.util_sos = 0.0;
	}
	if (sig == SIGINT)
		exit(0);
}

void
just_exit(int sig)
{
	exit(0);
}
