/*	$NetBSD: cleanerd.c,v 1.19 2000/06/21 01:58:52 perseant Exp $	*/

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
__RCSID("$NetBSD: cleanerd.c,v 1.19 2000/06/21 01:58:52 perseant Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <ufs/ufs/dinode.h>
#include <ufs/lfs/lfs.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

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

#define	CLEAN_BYTES	0x1

/* function prototypes for system calls; not sure where they should go */
int	 lfs_segwait __P((fsid_t *, struct timeval *));
int	 lfs_segclean __P((fsid_t *, u_long));
int	 lfs_bmapv __P((fsid_t *, BLOCK_INFO *, int));
int	 lfs_markv __P((fsid_t *, BLOCK_INFO *, int));

/* function prototypes */
int	 bi_tossold __P((const void *, const void *, const void *));
int	 choose_segments __P((FS_INFO *, struct seglist *, 
	     unsigned long (*)(FS_INFO *, SEGUSE *)));
void	 clean_fs __P((FS_INFO	*, unsigned long (*)(FS_INFO *, SEGUSE *), int, long));
int	 clean_loop __P((FS_INFO *, int, long));
int	 clean_segment __P((FS_INFO *, struct seglist *));
unsigned long	 cost_benefit __P((FS_INFO *, SEGUSE *));
int	 cost_compare __P((const void *, const void *));
void	 sig_report __P((int));
int	 main __P((int, char *[]));

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
cost_benefit(fsp, su)
	FS_INFO *fsp;		/* file system information */
	SEGUSE *su;
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
                        syslog(LOG_NOTICE,"bad segusage count: %ld", live);
			live = 0;
		}
		return (lblkno(lfsp, seg_size(lfsp) - live) * age)
			/ lblkno(lfsp, seg_size(lfsp) + live);
	}
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	FS_INFO	*fsp;
	struct statfs *lstatfsp;	/* file system stats */
	struct timeval timeout;		/* sleep timeout */
	fsid_t fsid;
	long clean_opts;		/* cleaning options */
	int segs_per_clean;
	int opt, cmd_err;
	pid_t childpid;
	char *fs_name;			/* name of filesystem to clean */
	time_t now, lasttime;
	int loopcount;

	cmd_err = debug = do_quit = 0;
	clean_opts = 0;
	segs_per_clean = 1;
	while ((opt = getopt(argc, argv, "bdfl:mn:qr:st:")) != -1) {
		switch (opt) {
			case 'b':
				/*
				 * Use live bytes to determine
				 * how many segs to clean.
				 */
				clean_opts |= CLEAN_BYTES;
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
		err(1, "usage: lfs_cleanerd [-bdms] [-l load] [-n nsegs] [-r report_freq] [-t timeout] fs_name");

	fs_name = argv[0];

	if (fs_getmntinfo(&lstatfsp, fs_name, MOUNT_LFS) == 0) {
		/* didn't find the filesystem */
		err(1, "lfs_cleanerd: filesystem %s isn't an LFS!", fs_name);
	}

        /* should we become a daemon, chdir to / & close fd's */
	if (debug == 0) {
		if (daemon(0, 0) == -1)
			err(1, "lfs_cleanerd: couldn't become a daemon!");
		lasttime=0;
		loopcount=0;
	    loop:
		if((childpid=fork())<0) {
			syslog(LOG_NOTICE,"%s: couldn't fork, exiting: %m",
				fs_name);
			exit(1);
		}
		if(childpid != 0) {
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
				syslog(LOG_INFO,"lfs_cleanerd: fs %s unmounted, exiting", fs_name);
				exit(0);
			}
			goto loop;
		}
                openlog("lfs_cleanerd", LOG_NDELAY|LOG_PID, LOG_DAEMON);
        } else {
                openlog("lfs_cleanerd", LOG_NDELAY|LOG_PID|LOG_PERROR,
                        LOG_DAEMON);
        }

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
		if (lfs_segwait(&fsid, &timeout) < 0)
			syslog(LOG_WARNING,"lfs_segwait returned error.");
                if(debug > 1)
                    syslog(LOG_DEBUG,"Cleaner waking up.");
	}
}

/* return the number of segments cleaned */
int
clean_loop(fsp, nsegs, options)
	FS_INFO	*fsp;	/* file system information */
	int nsegs;
	long options;
{
	double loadavg[MAXLOADS];
	time_t	now;
	u_long max_free_segs;
	u_long db_per_seg;

        /*
	 * Compute the maximum possible number of free segments, given the
	 * number of free blocks.
	 */
	db_per_seg = fragstodb(&fsp->fi_lfs, fsp->fi_lfs.lfs_ssize);
	max_free_segs = (fsp->fi_statfsp->f_bfree / fsp->fi_lfs.lfs_ssize) >> fsp->fi_lfs.lfs_fbshift;
	
	/* 
	 * We will clean if there are not enough free blocks or total clean
	 * space is less than BUSY_LIM % of possible clean space.
	 */
	now = time((time_t *)NULL);

        if(debug > 1) {
            syslog(LOG_DEBUG, "db_per_seg = %lu max_free_segs = %lu, bfree = %u avail = %d ",
                   db_per_seg, max_free_segs, fsp->fi_lfs.lfs_bfree,
                   fsp->fi_lfs.lfs_avail);
	    syslog(LOG_DEBUG, "clean segs = %d, max_free_segs = %ld",
		   fsp->fi_cip->clean, max_free_segs);
            syslog(LOG_DEBUG, "clean = %d", fsp->fi_cip->clean);
        }

	if ((fsp->fi_lfs.lfs_bfree - fsp->fi_lfs.lfs_avail > db_per_seg &&
	     fsp->fi_lfs.lfs_avail < db_per_seg) ||
	    (fsp->fi_cip->clean < max_free_segs &&
	     (fsp->fi_cip->clean <= MIN_SEGS(&fsp->fi_lfs) ||
	      fsp->fi_cip->clean < max_free_segs * BUSY_LIM)))
	{
                if(debug)
			syslog(LOG_DEBUG, "Cleaner Running  at %s (%d of %lu segments available)",
			       ctime(&now), fsp->fi_cip->clean, max_free_segs);
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
		 * the total clean space is less then IDLE_LIM % of possible
		 * clean space.
		 */
		if (getloadavg(loadavg, MAXLOADS) == -1) {
			perror("getloadavg: failed");
			return (-1);
		}
		if (loadavg[ONE_MIN] < load_threshold
		    && fsp->fi_cip->clean < max_free_segs * IDLE_LIM)
		{
                        if(debug)
				syslog(LOG_DEBUG, "Cleaner Running  at %s "
				       "(system load %.1f, %d of %lu segments available)",
					ctime(&now), loadavg[ONE_MIN],
					fsp->fi_cip->clean, max_free_segs);
		        clean_fs(fsp, cost_benefit, nsegs, options);
			if(do_quit) {
				if(debug)
					syslog(LOG_INFO,"Cleaner shutting down");
				exit(0);
			}
			return (1);
		}
	}
        if(debug > 1)
            syslog(LOG_DEBUG, "Cleaner Not Running at %s", ctime(&now));
	return (0);
}


void
clean_fs(fsp, cost_func, nsegs, options)
	FS_INFO	*fsp;	/* file system information */
	unsigned long (*cost_func) __P((FS_INFO *, SEGUSE *));
	int nsegs;
	long options;
{
	struct seglist *segs, *sp;
	long int to_clean, cleaned_bytes;
	unsigned long i, j, total;
	struct rusage ru;
	int error;

	if ((segs =
	    malloc(fsp->fi_lfs.lfs_nseg * sizeof(struct seglist))) == NULL) {
		syslog(LOG_WARNING,"malloc failed: %m");
		return;
	}
	total = i = choose_segments(fsp, segs, cost_func);

	/* If we can get lots of cleaning for free, do it now */
	sp=segs;
	for(j=0; j < total && sp->sl_bytes == 0; j++) {
		if(debug)
			syslog(LOG_DEBUG,"Wiping empty segment %ld",sp->sl_id);
		if(lfs_segclean(&fsp->fi_statfsp->f_fsid, sp->sl_id) < 0)
			syslog(LOG_NOTICE,"lfs_segclean failed empty segment %ld: %m", sp->sl_id);
                ++cleaner_stats.segs_empty;
		sp++;
		i--;
	}
	if(j > nsegs) {
		free(segs);
		return;
	}

	/* If we relly need to clean a lot, do it now */
	if(fsp->fi_cip->clean < 2*MIN_FREE_SEGS)
		nsegs = MAX(nsegs,MIN_FREE_SEGS);
	/* But back down if we haven't got that many free to clean into */
	if(fsp->fi_cip->clean < nsegs)
		nsegs = fsp->fi_cip->clean;

        if(debug > 1)
            syslog(LOG_DEBUG, "clean_fs: found %ld segments to clean in file system %s",
                   i, fsp->fi_statfsp->f_mntonname);

	if (i) {
		/* Check which cleaning algorithm to use. */
		if (options & CLEAN_BYTES) {
			cleaned_bytes = 0;
			to_clean = nsegs << fsp->fi_lfs.lfs_segshift;
			for (; i && cleaned_bytes < to_clean;
			    i--, ++sp) {
				if (clean_segment(fsp, sp) < 0)
					syslog(LOG_NOTICE,"clean_segment failed segment %ld: %m", sp->sl_id);
				else if (lfs_segclean(&fsp->fi_statfsp->f_fsid,
						      sp->sl_id) < 0)
					syslog(LOG_NOTICE,"lfs_segclean failed segment %ld: %m", sp->sl_id);
				else {
					if(debug) {
						syslog(LOG_DEBUG,
						       "Cleaned segment %ld (%ld->%ld/%ld)",
						       sp->sl_id,
						       (1<<fsp->fi_lfs.lfs_segshift) - sp->sl_bytes,
						       cleaned_bytes + (1<<fsp->fi_lfs.lfs_segshift) - sp->sl_bytes,
						       to_clean);
					}
					cleaned_bytes += (1<<fsp->fi_lfs.lfs_segshift) - sp->sl_bytes;
				}
			}
		} else
			for (i = MIN(i, nsegs); i-- ; ++sp) {
				total--;
				syslog(LOG_DEBUG,"Cleaning segment %ld (of %ld choices)", sp->sl_id, i+1);
				if (clean_segment(fsp, sp) < 0) {
					syslog(LOG_NOTICE,"clean_segment failed segment %ld: %m", sp->sl_id);
					if(total)
						i++;
				}
				else if (lfs_segclean(&fsp->fi_statfsp->f_fsid,
				    sp->sl_id) < 0)
					syslog(LOG_NOTICE,"lfs_segclean failed segment %ld: %m", sp->sl_id);
				else if(debug)
					syslog(LOG_DEBUG,"Completed cleaning segment %ld (of %ld choices)", sp->sl_id, i+1);
			}
	}
	free(segs);
	if(debug) {
		error=getrusage(RUSAGE_SELF,&ru);
		if(error) {
			syslog(LOG_INFO,"getrusage returned error: %m");
		} else {
			syslog(LOG_DEBUG,"Current usage: maxrss=%ld, idrss=%ld, isrss=%ld",
			       ru.ru_maxrss,ru.ru_idrss,ru.ru_isrss);
		}
	}
}

/*
 * Segment with the highest priority get sorted to the beginning of the
 * list.  This sort assumes that empty segments always have a higher
 * cost/benefit than any utilized segment.
 */
int
cost_compare(a, b)
	const void *a;
	const void *b;
{
	return ((struct seglist *)b)->sl_cost < ((struct seglist *)a)->sl_cost ? -1 : 1;
}


/*
 * Returns the number of segments to be cleaned with the elements of seglist
 * filled in.
 */
int
choose_segments(fsp, seglist, cost_func)
	FS_INFO *fsp;
	struct seglist *seglist;
	unsigned long (*cost_func) __P((FS_INFO *, SEGUSE *));
{
	struct lfs *lfsp;
	struct seglist *sp;
	SEGUSE *sup;
	int i, nsegs;

	lfsp = &fsp->fi_lfs;

        if(debug > 1)
            syslog(LOG_DEBUG,"Entering choose_segments");
	dump_super(lfsp);
	dump_cleaner_info(fsp->fi_cip);

	for (sp = seglist, i = 0; i < lfsp->lfs_nseg; ++i) {
		sup = SEGUSE_ENTRY(lfsp, fsp->fi_segusep, i);
                if(debug > 1)
                    PRINT_SEGUSE(sup, i);
		if (!(sup->su_flags & SEGUSE_DIRTY) ||
		    sup->su_flags & SEGUSE_ACTIVE)
			continue;
                if(debug > 1)
                    syslog(LOG_DEBUG, "\tchoosing segment %d", i);
		sp->sl_cost = (*cost_func)(fsp, sup);
		sp->sl_id = i;
		sp->sl_bytes = sup->su_nbytes;
		sp->sl_age = time(NULL) - sup->su_lastmod;
		++sp;
	}
	nsegs = sp - seglist;
	qsort(seglist, nsegs, sizeof(struct seglist), cost_compare);
#if 0
	for(i=0; i<nsegs; i++) {
		printf("%d: segment %lu age %lu contains %lu priority %lu\n", i,
			seglist[i].sl_age, seglist[i].sl_id, seglist[i].sl_bytes,
			seglist[i].sl_cost);
	}
#endif
        if(debug > 1)
            syslog(LOG_DEBUG,"Returning %d segments", nsegs);

	return (nsegs);
}


int
clean_segment(fsp, slp)
	FS_INFO *fsp;	/* file system information */
	struct seglist *slp; /* segment info */
{
	int id=slp->sl_id;
	BLOCK_INFO *block_array, *bp, *_bip;
	SEGUSE *sp;
	struct lfs *lfsp;
	struct tossstruct t;
	struct dinode *dip;
	double util;
	caddr_t seg_buf;
	daddr_t seg_addr;
	int num_blocks, maxblocks, clean_blocks, i, j, error;
	int seg_isempty=0;
        unsigned long *lp;

	lfsp = &fsp->fi_lfs;
	sp = SEGUSE_ENTRY(lfsp, fsp->fi_segusep, id);
	seg_addr = sntoda(lfsp,id);
	error = 0;

        syslog(LOG_DEBUG, "cleaning segment %d: contains %lu bytes", id,
                   (unsigned long)sp->su_nbytes);

	/* XXX could add debugging to verify that segment is really empty */
	if (sp->su_nbytes == 0) {
		++cleaner_stats.segs_empty;
		++seg_isempty;
	}

	/* map the segment into a buffer */
	if (mmap_segment(fsp, id, &seg_buf, do_mmap) < 0) {
		syslog(LOG_WARNING,"clean_segment: mmap_segment failed: %m");
		++cleaner_stats.segs_error;
		return (-1);
	}
	/* get a list of blocks that are contained by the segment */
	if ((error = lfs_segmapv(fsp, id, seg_buf, &block_array,
                                 &num_blocks)) < 0) {
		syslog(LOG_WARNING,
		       "clean_segment: lfs_segmapv failed for segment %d", id);
		goto out;
	}
	cleaner_stats.blocks_read += fsp->fi_lfs.lfs_ssize;

        if(debug > 1)
            syslog(LOG_DEBUG, "lfs_segmapv returned %d blocks", num_blocks);

	/* get the current disk address of blocks contained by the segment */
	if ((error = lfs_bmapv(&fsp->fi_statfsp->f_fsid, block_array,
			       num_blocks)) < 0) {
		perror("clean_segment: lfs_bmapv failed");
		goto out;
	}

	/* Now toss any blocks not in the current segment */
	t.lfs = lfsp;
	t.seg = id;
	toss(block_array, &num_blocks, sizeof(BLOCK_INFO), bi_tossold, &t);
	/* Check if last element should be tossed */
	if (num_blocks && bi_tossold(&t, block_array + num_blocks - 1, NULL))
		--num_blocks;

	if(seg_isempty) {
		if(num_blocks)
			syslog(LOG_WARNING,"segment %d was supposed to be empty, but has %d live blocks!", id, num_blocks);
		else
			syslog(LOG_DEBUG,"segment %d is empty, as claimed", id);
	}
	/* XXX KS - check for misplaced blocks */
	for(i=0; i<num_blocks; i++) {
		if(block_array[i].bi_daddr
		   && ((char *)(block_array[i].bi_bp) - seg_buf) != (block_array[i].bi_daddr - seg_addr) * DEV_BSIZE
		   && datosn(&(fsp->fi_lfs),block_array[i].bi_daddr) == id)
		{
			if(debug > 1) {
				syslog(LOG_DEBUG, "seg %d, ino %d lbn %d, 0x%x != 0x%lx (fixed)",
					id,
				       block_array[i].bi_inode,
				       block_array[i].bi_lbn,
				       block_array[i].bi_daddr,
				       (long)seg_addr + ((char *)(block_array[i].bi_bp) - seg_buf)/DEV_BSIZE);
			}
			/*
			 * XXX KS - have to be careful here about Inodes;
			 * if lfs_bmapv shows them somewhere else in the
			 * segment from where we thought, we need to reload
			 * the *right* inode, not the first one in the block.
			 */
			if(block_array[i].bi_lbn == LFS_UNUSED_LBN) {
				dip = (struct dinode *)(seg_buf + (block_array[i].bi_daddr - seg_addr) * DEV_BSIZE);
				for(j=INOPB(lfsp)-1;j>=0;j--) {
					if(dip[j].di_u.inumber == block_array[i].bi_inode) {
						block_array[i].bi_bp = (char *)(dip+j);
						break;
					}
				}
				if(j<0) {
					syslog(LOG_NOTICE, "lost inode %d in the shuffle! (blk %d)",
					       block_array[i].bi_inode, block_array[i].bi_daddr);
					syslog(LOG_DEBUG, "inode numbers found were:");
					for(j=INOPB(lfsp)-1;j>=0;j--) {
						syslog(LOG_DEBUG, "%d", dip[j].di_u.inumber);
					}
					err(1,"lost inode");
				} else if(debug>1) {
					syslog(LOG_DEBUG,"Ino %d corrected to 0x%x+%d",
					       block_array[i].bi_inode,
					       block_array[i].bi_daddr,
					       (int)((caddr_t)(block_array[i].bi_bp) - (caddr_t)(long)seg_addr) % DEV_BSIZE);
				}
			} else {
				block_array[i].bi_bp = seg_buf + (block_array[i].bi_daddr - seg_addr) * DEV_BSIZE;
			}
		}
	}

	/* Update live bytes calc - XXX KS */
	slp->sl_bytes = 0;
	for(i=0; i<num_blocks; i++)
		if(block_array[i].bi_lbn == LFS_UNUSED_LBN)
			slp->sl_bytes += sizeof(struct dinode);
		else
			slp->sl_bytes += block_array[i].bi_size;

        if(debug > 1) {
            syslog(LOG_DEBUG, "after bmapv still have %d blocks", num_blocks);
            if (num_blocks)
                syslog(LOG_DEBUG, "BLOCK INFOS");
            for (_bip = block_array, i=0; i < num_blocks; ++_bip, ++i) {
                PRINT_BINFO(_bip);
                lp = (u_long *)_bip->bi_bp;
            }
        }

	++cleaner_stats.segs_cleaned;
	cleaner_stats.blocks_written += num_blocks;
	util = ((double)num_blocks / fsp->fi_lfs.lfs_ssize);
	cleaner_stats.util_tot += util;
	cleaner_stats.util_sos += util * util;
	if (do_small)
		maxblocks = MAXPHYS / fsp->fi_lfs.lfs_bsize - 1;
	else
		maxblocks = num_blocks;

	for (bp = block_array; num_blocks > 0; bp += clean_blocks) {
		clean_blocks = maxblocks < num_blocks ? maxblocks : num_blocks;
		if ((error = lfs_markv(&fsp->fi_statfsp->f_fsid,
				       bp, clean_blocks)) < 0) {
			syslog(LOG_WARNING,"clean_segment: lfs_markv failed: %m");
			goto out;
		}
		num_blocks -= clean_blocks;
	}
		
    out:
	if (block_array)
		free(block_array);
	if (error)
		++cleaner_stats.segs_error;
	munmap_segment(fsp, seg_buf, do_mmap);
	if (stat_report && cleaner_stats.segs_cleaned % stat_report == 0)
		sig_report(SIGUSR1);
	return (error);
}


int
bi_tossold(client, a, b)
	const void *client;
	const void *a;
	const void *b;
{
	const struct tossstruct *t;

	t = (struct tossstruct *)client;

	return (((BLOCK_INFO *)a)->bi_daddr == LFS_UNUSED_DADDR ||
	    datosn(t->lfs, ((BLOCK_INFO *)a)->bi_daddr) != t->seg);
}

void
sig_report(sig)
	int sig;
{
	double avg = 0.0;

	syslog(LOG_DEBUG, "lfs_cleanerd:\t%s%d\n\t\t%s%d\n\t\t%s%d\n\t\t%s%d\n\t\t%s%d",
		"blocks_read    ", cleaner_stats.blocks_read,
		"blocks_written ", cleaner_stats.blocks_written,
		"segs_cleaned   ", cleaner_stats.segs_cleaned,
		"segs_empty     ", cleaner_stats.segs_empty,
		"seg_error      ", cleaner_stats.segs_error);
	syslog(LOG_DEBUG, "\t\t%s%5.2f\n\t\t%s%5.2f",
		"util_tot       ", cleaner_stats.util_tot,
		"util_sos       ", cleaner_stats.util_sos);
	syslog(LOG_DEBUG, "\t\tavg util: %4.2f std dev: %9.6f",
		avg = cleaner_stats.util_tot / cleaner_stats.segs_cleaned,
		cleaner_stats.util_sos / cleaner_stats.segs_cleaned - avg * avg);


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
