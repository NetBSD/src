/* $NetBSD: main.c,v 1.43.8.2 2014/08/20 00:02:25 tls Exp $	 */

/*
 * Copyright (c) 1980, 1986, 1993
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
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/mount.h>
#include <ufs/lfs/lfs.h>

#include <fstab.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <err.h>
#include <util.h>
#include <signal.h>

#include "fsck.h"
#include "extern.h"
#include "fsutil.h"
#include "exitvalues.h"

volatile sig_atomic_t returntosingle = 0;

static int argtoi(int, const char *, const char *, int);
static int checkfilesys(const char *, char *, long, int);
static void usage(void);
static void efun(int, const char *, ...);
extern void (*panic_func)(int, const char *, va_list);

static void
efun(int eval, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	verr(EEXIT, fmt, ap);
	va_end(ap);
}

int
main(int argc, char **argv)
{
	int ch;
	int ret = FSCK_EXIT_OK;
	const char *optstring = "b:dfi:m:npPqUy";
	bool reallypreen;

	reallypreen = false;
	ckfinish = ckfini;
	skipclean = 1;
	exitonfail = 0;
	idaddr = 0x0;
	panic_func = vmsg;
	esetfunc(efun);
	while ((ch = getopt(argc, argv, optstring)) != -1) {
		switch (ch) {
		case 'b':
			skipclean = 0;
			bflag = argtoi('b', "number", optarg, 0);
			printf("Alternate super block location: %d\n", bflag);
			break;
		case 'd':
			debug++;
			break;
		case 'e':
			exitonfail++;
			break;
		case 'f':
			skipclean = 0;
			reallypreen = true;
			break;
		case 'i':
			idaddr = strtol(optarg, NULL, 0);
			break;
		case 'm':
			lfmode = argtoi('m', "mode", optarg, 8);
			if (lfmode & ~07777)
				err(1, "bad mode to -m: %o\n", lfmode);
			printf("** lost+found creation mode %o\n", lfmode);
			break;

		case 'n':
			nflag++;
			yflag = 0;
			break;

		case 'p':
			preen++;
			break;

		case 'P':		/* Progress meter not implemented. */
			break;

		case 'q':
			quiet++;
			break;
#ifndef SMALL
		case 'U':
			Uflag++;
			break;
#endif
		case 'y':
			yflag++;
			nflag = 0;
			break;

		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (!argc)
		usage();

	/*
	 * Don't do anything in preen mode. This is a replacement for
	 * version 1.111 of src/distrib/utils/sysinst/disks.c, which
	 * disabled fsck on installer-generated lfs partitions. That
	 * isn't the right way to do it; better to run fsck but have
	 * it not do anything, so that when the issues in fsck get
	 * resolved it can be turned back on.
	 *
	 * If you really want to run fsck in preen mode you can do:
	 *    fsck_lfs -p -f image
	 *
	 * This was prompted by
	 * http://mail-index.netbsd.org/tech-kern/2010/02/09/msg007306.html.
	 *
	 * It would be nice if someone prepared a more detailed report
	 * of the problems.
	 *
	 * XXX.
	 */
	if (preen && !reallypreen) {
		return ret;
	}

	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void) signal(SIGINT, catch);
	if (preen)
		(void) signal(SIGQUIT, catchquit);

	while (argc-- > 0) {
		int nret = checkfilesys(blockcheck(*argv++), 0, 0L, 0);
		if (ret < nret)
			ret = nret;
	}

	return returntosingle ? FSCK_EXIT_UNRESOLVED : ret;
}

static int
argtoi(int flag, const char *req, const char *str, int base)
{
	char *cp;
	int ret;

	ret = (int) strtol(str, &cp, base);
	if (cp == str || *cp)
		err(FSCK_EXIT_USAGE, "-%c flag requires a %s\n", flag, req);
	return (ret);
}

/*
 * Check the specified filesystem.
 */

/* ARGSUSED */
static int
checkfilesys(const char *filesys, char *mntpt, long auxdata, int child)
{
	struct dups *dp;
	struct zlncnt *zlnp;

	if (preen && child)
		(void) signal(SIGQUIT, voidquit);
	setcdevname(filesys, preen);
	if (debug && preen)
		pwarn("starting\n");
	switch (setup(filesys)) {
	case 0:
		if (preen)
			pfatal("CAN'T CHECK FILE SYSTEM.");
	case -1:
		return FSCK_EXIT_OK;
	}

	/*
	 * For LFS, "preen" means "roll forward".  We don't check anything
	 * else.
	 */
	if (preen == 0) {
		printf("** Last Mounted on %s\n", fs->lfs_fsmnt);
		if (hotroot())
			printf("** Root file system\n");
		/*
		 * 0: check segment checksums, inode ranges
		 */
		printf("** Phase 0 - Check Inode Free List\n");
	}

	/*
	 * Check inode free list - we do this even if idaddr is set,
	 * since if we're writing we don't want to write a bad list.
	 */
	pass0();

	if (preen == 0) {
		/*
		 * 1: scan inodes tallying blocks used
		 */
		printf("** Phase 1 - Check Blocks and Sizes\n");
		pass1();

		/*
		 * 2: traverse directories from root to mark all connected directories
		 */
		printf("** Phase 2 - Check Pathnames\n");
		pass2();

		/*
		 * 3: scan inodes looking for disconnected directories
		 */
		printf("** Phase 3 - Check Connectivity\n");
		pass3();

		/*
		 * 4: scan inodes looking for disconnected files; check reference counts
		 */
		printf("** Phase 4 - Check Reference Counts\n");
		pass4();
	}

	/*
	 * 5: check segment byte totals and dirty flags, and cleanerinfo
	 */
	if (!preen)
		printf("** Phase 5 - Check Segment Block Accounting\n");
	pass5();

	if (debug && !preen) {
		if (duplist != NULL) {
			printf("The following duplicate blocks remain:");
			for (dp = duplist; dp; dp = dp->next)
				printf(" %lld,", (long long) dp->dup);
			printf("\n");
		}
		if (zlnhead != NULL) {
			printf("The following zero link count inodes remain:");
			for (zlnp = zlnhead; zlnp; zlnp = zlnp->next)
				printf(" %llu,",
				    (unsigned long long)zlnp->zlncnt);
			printf("\n");
		}
	}

	if (!rerun) {
		if (!preen) {
			if (reply("ROLL FILESYSTEM FORWARD") == 1) {
				printf("** Phase 6 - Roll Forward\n");
				pass6();
			}
		}
		else {
			pass6();
		}
	}
	zlnhead = (struct zlncnt *) 0;
	orphead = (struct zlncnt *) 0;
	duplist = (struct dups *) 0;
	muldup = (struct dups *) 0;
	inocleanup();

	/*
	 * print out summary statistics
	 */
	pwarn("%llu files, %lld used, %lld free\n",
	    (unsigned long long)n_files, (long long) n_blks,
	    (long long) fs->lfs_bfree);

	ckfini(1);

	free(blockmap);
	free(statemap);
	free((char *)lncntp);
	if (!fsmodified) {
		return FSCK_EXIT_OK;
	}
	if (!preen)
		printf("\n***** FILE SYSTEM WAS MODIFIED *****\n");
	if (rerun)
		printf("\n***** PLEASE RERUN FSCK *****\n");
	if (hotroot()) {
		struct statvfs stfs_buf;
		/*
		 * We modified the root.  Do a mount update on
		 * it, unless it is read-write, so we can continue.
		 */
		if (statvfs("/", &stfs_buf) == 0) {
			long flags = stfs_buf.f_flag;
			struct ulfs_args args;

			if (flags & MNT_RDONLY) {
				args.fspec = 0;
				flags |= MNT_UPDATE | MNT_RELOAD;
				if (mount(MOUNT_LFS, "/", flags,
				    &args, sizeof args) == 0)
					return FSCK_EXIT_OK;
			}
		}
		if (!preen)
			printf("\n***** REBOOT NOW *****\n");
		sync();
		return FSCK_EXIT_ROOT_CHANGED;
	}
	return FSCK_EXIT_OK;
}

static void
usage(void)
{

	(void) fprintf(stderr,
	    "Usage: %s [-dfpqU] [-b block] [-m mode] [-y | -n] filesystem ...\n",
	    getprogname());
	exit(FSCK_EXIT_USAGE);
}
