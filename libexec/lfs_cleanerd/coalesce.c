/*      $NetBSD: coalesce.c,v 1.34 2026/04/23 16:26:05 perseant Exp $  */

/*-
 * Copyright (c) 2002, 2005, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Konrad E. Schroder <perseant@hhhh.org>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>

#include <ufs/lfs/lfs.h>

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <util.h>
#include <errno.h>
#include <err.h>
#include <assert.h>

#include <syslog.h>

#include "bufcache.h"
#include "vnode.h"
#include "cleaner.h"
#include "kernelops.h"

extern int debug, do_mmap;

enum coalesce_returncodes {
	COALESCE_OK = 0,
	COALESCE_NOINODE,
	COALESCE_TOOSMALL,
	COALESCE_BADSIZE,
	COALESCE_BADBLOCKSIZE,
	COALESCE_NOMEM,
	COALESCE_BADBMAPV,
	COALESCE_BADMARKV,
	COALESCE_NOTWORTHIT,
	COALESCE_NOTHINGLEFT,
	COALESCE_EIO,

	COALESCE_MAXERROR
};

const char *coalesce_return[] = {
	"Successfully coalesced",
	"File not in use or inode not found",
	"Not large enough to coalesce",
	"Negative size",
	"Not enough blocks to account for size",
	"Malloc failed",
	"LFCNBMAPV failed",
	"Not broken enough to fix",
	"Too many blocks not found",
	"Too many blocks found in active segments",
	"I/O error",

	"No such error"
};

#define INODES_AT_A_TIME 1024

static void flush(struct clfs *fs, struct lfs_inode_array *liap, int *totals)
{
	int j;
	
	if (kops.ko_fcntl(fs->clfs_ifilefd, LFCNREWRITEFILE, liap) < 0) { 
		syslog(LOG_WARNING, "%s: coalesce: LFCNREWRITEFILE: %m",
		       lfs_sb_getfsmnt(fs));
		++totals[COALESCE_BADBMAPV];
	}
	/* XXX stats */
	
	for (j = 0; j < liap->len; j++) {
		/* XXX check individual errors */
		++totals[COALESCE_OK];
	}
	
	liap->len = 0;
}

/*
 * Try coalescing every inode in the filesystem.
 * Return the number of inodes actually altered.
 */
int clean_all_inodes(struct clfs *fs)
{
	ino_t i, base, maxino;
	int totals[COALESCE_MAXERROR];
	struct stat st;
	struct lfs_filestat_req lfr;
	struct lfs_filestats fss[INODES_AT_A_TIME];
	struct lfs_inode_array lia;
	ino_t ia[INODES_AT_A_TIME];
	int blocksperseg;

	memset(totals, 0, sizeof(totals));
	lia.inodes = &ia[0];
	lfr.fss = &fss[0];
	blocksperseg = lfs_sb_getssize(fs) / lfs_sb_getbsize(fs);
	
	fstat(fs->clfs_ifilefd, &st);
	maxino = lfs_sb_getifpb(fs) * (st.st_size >> lfs_sb_getbshift(fs)) -
		lfs_sb_getsegtabsz(fs) - lfs_sb_getcleansz(fs);

	for (base = 0; base < maxino; base += INODES_AT_A_TIME) {
		lia.len = 0;
		for (i = 0; base + i < maxino; i++)
			ia[lia.len++] = base + i;

		/* See which files need treatment */
		if (kops.ko_fcntl(fs->clfs_ifilefd, LFCNFILESTATS, &lfr) < 0) { 
			syslog(LOG_WARNING, "%s: coalesce: LFCNFILESTATS: %m",
			       lfs_sb_getfsmnt(fs));
			++totals[COALESCE_BADBMAPV];
		}

		for (i = 0; i < INODES_AT_A_TIME; i++) {
			if (fss[i].nblk < 2) {
				++totals[COALESCE_TOOSMALL];
				continue;
			}
			if (fss[i].dc_count < 2 * howmany(fss[i].nblk,
							  blocksperseg)) {
				++totals[COALESCE_NOTWORTHIT];
				continue;
			}
			ia[lia.len++] = fss[i].ino;
			if (lia.len == INODES_AT_A_TIME)
				flush(fs, &lia, totals);
		}
	}
	if (lia.len > 0)
		flush(fs, &lia, totals);
	
	for (i = 0; i < COALESCE_MAXERROR; i++)
		if (totals[i])
			syslog(LOG_DEBUG, "%s: %d", coalesce_return[i],
			       totals[i]);
	
	return totals[COALESCE_OK];
}

/*
 * Fork a child process to coalesce this fs.
 */
int
fork_coalesce(struct clfs *fs)
{
	static pid_t childpid;
	int num;

	/*
	 * If already running a coalescing child, don't start a new one.
	 */
	if (childpid) {
		if (waitpid(childpid, NULL, WNOHANG) == childpid)
			childpid = 0;
	}
	if (childpid && kill(childpid, 0) >= 0) {
		/* already running a coalesce process */
		if (debug)
			syslog(LOG_DEBUG, "coalescing already in progress");
		return 0;
	}

	/*
	 * Fork a child and let the child coalease
	 */
	childpid = fork();
	if (childpid < 0) {
		syslog(LOG_ERR, "%s: fork to coaleasce: %m", lfs_sb_getfsmnt(fs));
		return 0;
	} else if (childpid == 0) {
		syslog(LOG_NOTICE, "%s: new coalescing process, pid %d",
		       lfs_sb_getfsmnt(fs), getpid());
		num = clean_all_inodes(fs);
		syslog(LOG_NOTICE, "%s: coalesced %d discontiguous inodes",
		       lfs_sb_getfsmnt(fs), num);
		exit(0);
	}

	return 0;
}
