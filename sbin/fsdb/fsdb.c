/*	$NetBSD: fsdb.c,v 1.44.2.1 2013/02/25 00:28:07 tls Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by John T. Kohl.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: fsdb.c,v 1.44.2.1 2013/02/25 00:28:07 tls Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/mount.h>
#include <ctype.h>
#include <fcntl.h>
#include <grp.h>
#include <histedit.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <err.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

#include "fsdb.h"
#include "fsck.h"
#include "extern.h"

__dead static void usage(void);
static int cmdloop(void);
static char *prompt(EditLine *);
static int scannames(struct inodesc *);
static int dolookup(char *);
static int chinumfunc(struct inodesc *);
static int chnamefunc(struct inodesc *);
static int dotime(char *, int32_t *, int32_t *);
static void print_blks32(int32_t *buf, int size, uint64_t *blknum);
static void print_blks64(int64_t *buf, int size, uint64_t *blknum);
static void print_indirblks32(uint32_t blk, int ind_level,
    uint64_t *blknum);
static void print_indirblks64(uint64_t blk, int ind_level,
    uint64_t *blknum);
static int compare_blk32(uint32_t *, uint32_t);
static int compare_blk64(uint64_t *, uint64_t);
static int founddatablk(uint64_t);
static int find_blks32(uint32_t *buf, int size, uint32_t *blknum);
static int find_blks64(uint64_t *buf, int size, uint64_t *blknum);
static int find_indirblks32(uint32_t blk, int ind_level,
						uint32_t *blknum);
static int find_indirblks64(uint64_t blk, int ind_level,
						uint64_t *blknum);

union dinode *curinode;
ino_t   curinum;

static void
usage(void)
{
	errx(1, "usage: %s [-dFn] -f <fsname>", getprogname());
}
/*
 * We suck in lots of fsck code, and just pick & choose the stuff we want.
 *
 * fsreadfd is set up to read from the file system, fswritefd to write to
 * the file system.
 */
int
main(int argc, char *argv[])
{
	int     ch, rval;
	char   *fsys = NULL;

	forceimage = 0;
	debug = 0;
	isappleufs = 0;
	while ((ch = getopt(argc, argv, "dFf:n")) != -1) {
		switch (ch) {
		case 'd':
			debug++;
			break;
		case 'F':
			forceimage = 1;
			break;
		case 'f':
			fsys = optarg;
			break;
		case 'n':
			nflag++;
			break;
		default:
			usage();
		}
	}
	if (fsys == NULL)
		usage();
	endian = 0;
	if (setup(fsys, fsys) <= 0)
		errx(1, "cannot set up file system `%s'", fsys);
	printf("Editing file system `%s'\nLast Mounted on %s\n", fsys,
	    sblock->fs_fsmnt);
	rval = cmdloop();
	if (nflag)
		exit(rval);
	sblock->fs_clean = 0;	/* mark it dirty */
	sbdirty();
	markclean = 0;
	ckfini(1);
	printf("*** FILE SYSTEM MARKED DIRTY\n");
	printf("*** BE SURE TO RUN FSCK TO CLEAN UP ANY DAMAGE\n");
	printf("*** IF IT WAS MOUNTED, RE-MOUNT WITH -u -o reload\n");
	exit(rval);
}

#define CMDFUNC(func) static int func (int argc, char *argv[])

CMDFUNC(helpfn);
CMDFUNC(focus);			/* focus on inode */
CMDFUNC(active);		/* print active inode */
CMDFUNC(focusname);		/* focus by name */
CMDFUNC(zapi);			/* clear inode */
CMDFUNC(uplink);		/* incr link */
CMDFUNC(downlink);		/* decr link */
CMDFUNC(linkcount);		/* set link count */
CMDFUNC(quit);			/* quit */
CMDFUNC(ls);			/* list directory */
CMDFUNC(blks);			/* list blocks */
CMDFUNC(findblk);		/* find block */
CMDFUNC(rm);			/* remove name */
CMDFUNC(ln);			/* add name */
CMDFUNC(newtype);		/* change type */
CMDFUNC(chmode);		/* change mode */
CMDFUNC(chlen);			/* change length */
CMDFUNC(chaflags);		/* change flags */
CMDFUNC(chgen);			/* change generation */
CMDFUNC(chowner);		/* change owner */
CMDFUNC(chgroup);		/* Change group */
CMDFUNC(back);			/* pop back to last ino */
CMDFUNC(chmtime);		/* Change mtime */
CMDFUNC(chctime);		/* Change ctime */
CMDFUNC(chatime);		/* Change atime */
CMDFUNC(chinum);		/* Change inode # of dirent */
CMDFUNC(chname);		/* Change dirname of dirent */

static struct cmdtable cmds[] = {
	{"help", "Print out help", 1, 1, helpfn},
	{"?", "Print out help", 1, 1, helpfn},
	{"inode", "Set active inode to INUM", 2, 2, focus},
	{"clri", "Clear inode INUM", 2, 2, zapi},
	{"lookup", "Set active inode by looking up NAME", 2, 2, focusname},
	{"cd", "Set active inode by looking up NAME", 2, 2, focusname},
	{"back", "Go to previous active inode", 1, 1, back},
	{"active", "Print active inode", 1, 1, active},
	{"print", "Print active inode", 1, 1, active},
	{"uplink", "Increment link count", 1, 1, uplink},
	{"downlink", "Decrement link count", 1, 1, downlink},
	{"linkcount", "Set link count to COUNT", 2, 2, linkcount},
	{"ls", "List current inode as directory", 1, 1, ls},
	{"blks", "List current inode's data blocks", 1, 1, blks},
	{"findblk", "Find inode owning disk block(s)", 2, 33, findblk},
	{"rm", "Remove NAME from current inode directory", 2, 2, rm},
	{"del", "Remove NAME from current inode directory", 2, 2, rm},
	{"ln", "Hardlink INO into current inode directory as NAME", 3, 3, ln},
	{"chinum", "Change dir entry number INDEX to INUM", 3, 3, chinum},
	{"chname", "Change dir entry number INDEX to NAME", 3, 3, chname},
	{"chtype", "Change type of current inode to TYPE", 2, 2, newtype},
	{"chmod", "Change mode of current inode to MODE", 2, 2, chmode},
	{"chown", "Change owner of current inode to OWNER", 2, 2, chowner},
	{"chlen", "Change length of current inode to LENGTH", 2, 2, chlen},
	{"chgrp", "Change group of current inode to GROUP", 2, 2, chgroup},
	{"chflags", "Change flags of current inode to FLAGS", 2, 2, chaflags},
	{"chgen", "Change generation number of current inode to GEN", 2, 2,
		    chgen},
	{"mtime", "Change mtime of current inode to MTIME", 2, 2, chmtime},
	{"ctime", "Change ctime of current inode to CTIME", 2, 2, chctime},
	{"atime", "Change atime of current inode to ATIME", 2, 2, chatime},
	{"quit", "Exit", 1, 1, quit},
	{"q", "Exit", 1, 1, quit},
	{"exit", "Exit", 1, 1, quit},
	{ .cmd = NULL},
};

static int
helpfn(int argc, char *argv[])
{
	struct cmdtable *cmdtp;

	printf("Commands are:\n%-10s %5s %5s   %s\n",
	    "command", "min argc", "max argc", "what");

	for (cmdtp = cmds; cmdtp->cmd; cmdtp++)
		printf("%-10s %5u %5u   %s\n",
		    cmdtp->cmd, cmdtp->minargc, cmdtp->maxargc, cmdtp->helptxt);
	return 0;
}

static char *
prompt(EditLine *el)
{
	static char pstring[64];
	snprintf(pstring, sizeof(pstring), "fsdb (inum: %llu)> ",
	    (unsigned long long)curinum);
	return pstring;
}


static int
cmdloop(void)
{
	char   *line;
	const char *elline;
	int     cmd_argc, rval = 0, known;
#define scratch known
	char  **cmd_argv;
	struct cmdtable *cmdp;
	History *hist;
	HistEvent he;
	EditLine *elptr;

	curinode = ginode(UFS_ROOTINO);
	curinum = UFS_ROOTINO;
	printactive();

	hist = history_init();
	history(hist, &he, H_SETSIZE, 100);	/* 100 elt history buffer */

	elptr = el_init(getprogname(), stdin, stdout, stderr);
	el_set(elptr, EL_EDITOR, "emacs");
	el_set(elptr, EL_PROMPT, prompt);
	el_set(elptr, EL_HIST, history, hist);
	el_source(elptr, NULL);

	while ((elline = el_gets(elptr, &scratch)) != NULL && scratch != 0) {
		if (debug)
			printf("command `%s'\n", elline);

		history(hist, &he, H_ENTER, elline);

		line = strdup(elline);
		cmd_argv = crack(line, &cmd_argc);
		if (cmd_argc) {
			/*
		         * el_parse returns -1 to signal that it's not been
		         * handled internally.
		         */
			if (el_parse(elptr, cmd_argc, (void *)cmd_argv) != -1)
				continue;
			known = 0;
			for (cmdp = cmds; cmdp->cmd; cmdp++) {
				if (!strcmp(cmdp->cmd, cmd_argv[0])) {
					if (cmd_argc >= cmdp->minargc &&
					    cmd_argc <= cmdp->maxargc)
						rval =
						    (*cmdp->handler)(cmd_argc,
							cmd_argv);
					else
						rval = argcount(cmdp, cmd_argc,
						    cmd_argv);
					known = 1;
					break;
				}
			}
			if (!known)
				warnx("unknown command `%s'", cmd_argv[0]),
				    rval = 1;
		} else
			rval = 0;
		free(line);
		if (rval < 0)
			return rval;
		if (rval)
			warnx("rval was %d", rval);
	}
	el_end(elptr);
	history_end(hist);
	return rval;
}

static ino_t ocurrent;

#define GETINUM(ac,inum)    inum = strtoull(argv[ac], &cp, 0); \
    if (inum < UFS_ROOTINO || inum >= maxino || cp == argv[ac] || *cp != '\0' ) { \
	printf("inode %llu out of range; range is [%llu,%llu]\n", \
	   (unsigned long long)inum, (unsigned long long)UFS_ROOTINO, \
	   (unsigned long long)maxino); \
	return 1; \
    }

/*
 * Focus on given inode number
 */
CMDFUNC(focus)
{
	ino_t   inum;
	char   *cp;

	GETINUM(1, inum);
	curinode = ginode(inum);
	ocurrent = curinum;
	curinum = inum;
	printactive();
	return 0;
}

CMDFUNC(back)
{
	curinum = ocurrent;
	curinode = ginode(curinum);
	printactive();
	return 0;
}

CMDFUNC(zapi)
{
	ino_t   inum;
	union dinode *dp;
	char   *cp;

	GETINUM(1, inum);
	dp = ginode(inum);
	clearinode(dp);
	inodirty();
	if (curinode)		/* re-set after potential change */
		curinode = ginode(curinum);
	return 0;
}

CMDFUNC(active)
{
	printactive();
	return 0;
}

CMDFUNC(quit)
{
	return -1;
}

CMDFUNC(uplink)
{
	int16_t nlink;

	if (!checkactive())
		return 1;
	nlink = iswap16(DIP(curinode, nlink));
	nlink++;
	DIP_SET(curinode, nlink, iswap16(nlink));
	printf("inode %llu link count now %d\n", (unsigned long long)curinum,
	    nlink);
	inodirty();
	return 0;
}

CMDFUNC(downlink)
{
	int16_t nlink;

	if (!checkactive())
		return 1;
	nlink = iswap16(DIP(curinode, nlink));
	nlink--;
	DIP_SET(curinode, nlink, iswap16(nlink));
	printf("inode %llu link count now %d\n", (unsigned long long)curinum,
	    nlink);
	inodirty();
	return 0;
}

static const char *typename[] = {
	"unknown",
	"fifo",
	"char special",
	"unregistered #3",
	"directory",
	"unregistered #5",
	"blk special",
	"unregistered #7",
	"regular",
	"unregistered #9",
	"symlink",
	"unregistered #11",
	"socket",
	"unregistered #13",
	"whiteout",
};

static int slot;

static int
scannames(struct inodesc *idesc)
{
	struct direct *dirp = idesc->id_dirp;

	printf("slot %d ino %d reclen %d: %s, `%.*s'\n",
	    slot++, iswap32(dirp->d_ino), iswap16(dirp->d_reclen),
		typename[dirp->d_type],
	    dirp->d_namlen, dirp->d_name);
	return (KEEPON);
}

CMDFUNC(ls)
{
	struct inodesc idesc;
	checkactivedir();	/* let it go on anyway */

	slot = 0;
	idesc.id_number = curinum;
	idesc.id_func = scannames;
	idesc.id_type = DATA;
	idesc.id_fix = IGNORE;
	ckinode(curinode, &idesc);
	curinode = ginode(curinum);

	return 0;
}

CMDFUNC(blks)
{
	uint64_t blkno = 0;
	int i, type;
	if (!curinode) {
		warnx("no current inode");
		return 0;
	}
	type = iswap16(DIP(curinode, mode)) & IFMT;
	if (type != IFDIR && type != IFREG) {
		warnx("inode %llu not a file or directory",
		    (unsigned long long)curinum);
		return 0;
	}
	if (is_ufs2) {
		printf("I=%llu %lld blocks\n", (unsigned long long)curinum,
		    (long long)(iswap64(curinode->dp2.di_blocks)));
	} else {
		printf("I=%llu %d blocks\n", (unsigned long long)curinum,
		    iswap32(curinode->dp1.di_blocks));
	}
	printf("Direct blocks:\n");
	if (is_ufs2)
		print_blks64(curinode->dp2.di_db, UFS_NDADDR, &blkno);
	else
		print_blks32(curinode->dp1.di_db, UFS_NDADDR, &blkno);

	if (is_ufs2) {
		for (i = 0; i < UFS_NIADDR; i++)
			print_indirblks64(iswap64(curinode->dp2.di_ib[i]), i,
			    &blkno);
	} else {
		for (i = 0; i < UFS_NIADDR; i++)
			print_indirblks32(iswap32(curinode->dp1.di_ib[i]), i,
			    &blkno);
	}
	return 0;
}

static int findblk_numtofind;
static int wantedblksize;
CMDFUNC(findblk)
{
	ino_t   inum, inosused;
	uint32_t *wantedblk32 = NULL;
	uint64_t *wantedblk64 = NULL;
	struct cg *cgp = cgrp;
	int i, c;

	ocurrent = curinum;
	wantedblksize = (argc - 1);
	if (is_ufs2) {
		wantedblk64 = malloc(sizeof(uint64_t) * wantedblksize);
		if (wantedblk64 == NULL) {
			perror("malloc");
			return 1;
		}
		memset(wantedblk64, 0, sizeof(uint64_t) * wantedblksize);
		for (i = 1; i < argc; i++)
			wantedblk64[i - 1] =
			    dbtofsb(sblock, strtoull(argv[i], NULL, 0)); 
	} else {
		wantedblk32 = malloc(sizeof(uint32_t) * wantedblksize);
		if (wantedblk32 == NULL) {
			perror("malloc");
			return 1;
		}
		memset(wantedblk32, 0, sizeof(uint32_t) * wantedblksize);
		for (i = 1; i < argc; i++)
			wantedblk32[i - 1] =
			    dbtofsb(sblock, strtoull(argv[i], NULL, 0)); 
	}
	findblk_numtofind = wantedblksize;
	for (c = 0; c < sblock->fs_ncg; c++) {
		inum = c * sblock->fs_ipg;
		getblk(&cgblk, cgtod(sblock, c), sblock->fs_cgsize);
		memcpy(cgp, cgblk.b_un.b_cg, sblock->fs_cgsize);
		if (needswap)
			ffs_cg_swap(cgblk.b_un.b_cg, cgp, sblock);
		if (is_ufs2)
			inosused = cgp->cg_initediblk;
		else
			inosused = sblock->fs_ipg;
		for (; inosused > 0; inum++, inosused--) {
			if (inum < UFS_ROOTINO)
				continue;
			if (is_ufs2 ? compare_blk64(wantedblk64,
			        ino_to_fsba(sblock, inum)) :
			    compare_blk32(wantedblk32,
			        ino_to_fsba(sblock, inum))) {
				printf("block %llu: inode block (%llu-%llu)\n",
				    (unsigned long long)fsbtodb(sblock,
					ino_to_fsba(sblock, inum)),
				    (unsigned long long)
				    (inum / INOPB(sblock)) * INOPB(sblock),
				    (unsigned long long)
				    (inum / INOPB(sblock) + 1) * INOPB(sblock));
				findblk_numtofind--;
				if (findblk_numtofind == 0)
					goto end;
			}
			curinum = inum;
			curinode = ginode(inum);
			switch (iswap16(DIP(curinode, mode)) & IFMT) {
			case IFDIR:
			case IFREG:
				if (DIP(curinode, blocks) == 0)
					continue;
				break;
			case IFLNK:
				{
				uint64_t size = iswap64(DIP(curinode, size));
				if (size > 0 &&
				    size < (uint64_t)sblock->fs_maxsymlinklen &&
				    DIP(curinode, blocks) == 0)
					continue;
				else
					break;
				}
			default:
				continue;
			}
			if (is_ufs2 ?
			    find_blks64(curinode->dp2.di_db, UFS_NDADDR,
				wantedblk64) : 
			    find_blks32(curinode->dp1.di_db, UFS_NDADDR,
				wantedblk32))
				goto end;
			for (i = 0; i < UFS_NIADDR; i++) {
				if (is_ufs2 ?
				    compare_blk64(wantedblk64,
					iswap64(curinode->dp2.di_ib[i])) :
				    compare_blk32(wantedblk32,
					iswap32(curinode->dp1.di_ib[i])))
					if (founddatablk(is_ufs2 ?
					    iswap64(curinode->dp2.di_ib[i]) :
					    iswap32(curinode->dp1.di_ib[i])))
						goto end;
				if (is_ufs2 ? (curinode->dp2.di_ib[i] != 0) :
				    (curinode->dp1.di_ib[i] != 0))
					if (is_ufs2 ?
					    find_indirblks64(
						iswap64(curinode->dp2.di_ib[i]),
						i, wantedblk64) :
					    find_indirblks32(
						iswap32(curinode->dp1.di_ib[i]),
						i, wantedblk32))
						goto end;
			}
		}
	}
end:
	if (wantedblk32)
		free(wantedblk32);
	if (wantedblk64)
		free(wantedblk64);
	curinum = ocurrent;
	curinode = ginode(curinum);
	return 0;
}

static int
compare_blk32(uint32_t *wantedblk, uint32_t curblk)
{
	int i;
	for (i = 0; i < wantedblksize; i++) {
		if (wantedblk[i] != 0 && wantedblk[i] == curblk) {
			wantedblk[i] = 0;
			return 1;
		}
	}
	return 0;
}

static int
compare_blk64(uint64_t *wantedblk, uint64_t curblk)
{
	int i;
	for (i = 0; i < wantedblksize; i++) {
		if (wantedblk[i] != 0 && wantedblk[i] == curblk) {
			wantedblk[i] = 0;
			return 1;
		}
	}
	return 0;
}

static int
founddatablk(uint64_t blk)
{
	printf("%llu: data block of inode %llu\n",
	    (unsigned long long)fsbtodb(sblock, blk),
	    (unsigned long long)curinum);
	findblk_numtofind--;
	if (findblk_numtofind == 0)
		return 1;
	return 0;
}

static int
find_blks32(uint32_t *buf, int size, uint32_t *wantedblk)
{
	int blk;
	for(blk = 0; blk < size; blk++) {
		if (buf[blk] == 0)
			continue;
		if (compare_blk32(wantedblk, iswap32(buf[blk]))) {
			if (founddatablk(iswap32(buf[blk])))
				return 1;
		}
	}
	return 0;
}

static int
find_indirblks32(uint32_t blk, int ind_level, uint32_t *wantedblk)
{
#define MAXNINDIR	(MAXBSIZE / sizeof(uint32_t))
	uint32_t idblk[MAXNINDIR];
	size_t i;

	bread(fsreadfd, (char *)idblk, fsbtodb(sblock, blk),
	    (int)sblock->fs_bsize);
	if (ind_level <= 0) {
		if (find_blks32(idblk,
		    sblock->fs_bsize / sizeof(uint32_t), wantedblk))
			return 1;
	} else {
		ind_level--;
		for (i = 0; i < sblock->fs_bsize / sizeof(uint32_t); i++) {
			if (compare_blk32(wantedblk, iswap32(idblk[i]))) {
				if (founddatablk(iswap32(idblk[i])))
					return 1;
			}
			if(idblk[i] != 0)
				if (find_indirblks32(iswap32(idblk[i]),
				    ind_level, wantedblk))
				return 1;
		}
	}
#undef MAXNINDIR
	return 0;
}


static int
find_blks64(uint64_t *buf, int size, uint64_t *wantedblk)
{
	int blk;
	for(blk = 0; blk < size; blk++) {
		if (buf[blk] == 0)
			continue;
		if (compare_blk64(wantedblk, iswap64(buf[blk]))) {
			if (founddatablk(iswap64(buf[blk])))
				return 1;
		}
	}
	return 0;
}

static int
find_indirblks64(uint64_t blk, int ind_level, uint64_t *wantedblk)
{
#define MAXNINDIR	(MAXBSIZE / sizeof(uint64_t))
	uint64_t idblk[MAXNINDIR];
	size_t i;

	bread(fsreadfd, (char *)idblk, fsbtodb(sblock, blk),
	    (int)sblock->fs_bsize);
	if (ind_level <= 0) {
		if (find_blks64(idblk,
		    sblock->fs_bsize / sizeof(uint64_t), wantedblk))
			return 1;
	} else {
		ind_level--;
		for (i = 0; i < sblock->fs_bsize / sizeof(uint64_t); i++) {
			if (compare_blk64(wantedblk, iswap64(idblk[i]))) {
				if (founddatablk(iswap64(idblk[i])))
					return 1;
			}
			if(idblk[i] != 0)
				if (find_indirblks64(iswap64(idblk[i]),
				    ind_level, wantedblk))
				return 1;
		}
	}
#undef MAXNINDIR
	return 0;
}


#define CHARS_PER_LINES 70

static void
print_blks32(int32_t *buf, int size, uint64_t *blknum)
{
	int chars;
	char prbuf[CHARS_PER_LINES+1];
	int blk;
 
	chars = 0;
	for(blk = 0; blk < size; blk++, (*blknum)++) {
		if (buf[blk] == 0)
			continue;
		snprintf(prbuf, CHARS_PER_LINES, "%d ", iswap32(buf[blk]));
		if ((chars + strlen(prbuf)) > CHARS_PER_LINES) {
			printf("\n");
			chars = 0;
		}
		if (chars == 0)
			printf("%" PRIu64 ": ", *blknum);
		printf("%s", prbuf);
		chars += strlen(prbuf);
	}
	printf("\n");
}

static void
print_blks64(int64_t *buf, int size, uint64_t *blknum)
{
	int chars;
	char prbuf[CHARS_PER_LINES+1];
	int blk;
 
	chars = 0;
	for(blk = 0; blk < size; blk++, (*blknum)++) {
		if (buf[blk] == 0)
			continue;
		snprintf(prbuf, CHARS_PER_LINES, "%lld ",
		    (long long)iswap64(buf[blk]));
		if ((chars + strlen(prbuf)) > CHARS_PER_LINES) {
			printf("\n");
			chars = 0;
		}
		if (chars == 0)
			printf("%" PRIu64 ": ", *blknum);
		printf("%s", prbuf);
		chars += strlen(prbuf);
	}
	printf("\n");
}

#undef CHARS_PER_LINES

static void
print_indirblks32(uint32_t blk, int ind_level, uint64_t *blknum)
{
#define MAXNINDIR	(MAXBSIZE / sizeof(int32_t))
	const int ptrperblk_shift = sblock->fs_bshift - 2;
	const int ptrperblk = 1 << ptrperblk_shift;
	int32_t idblk[MAXNINDIR];
	int i;

	if (blk == 0) {
		*blknum += (uint64_t)ptrperblk << (ptrperblk_shift * ind_level);
		return;
	}
 
	printf("Indirect block %lld (level %d):\n", (long long)blk,
	    ind_level+1);
	bread(fsreadfd, (char *)idblk, fsbtodb(sblock, blk),
	    (int)sblock->fs_bsize);
	if (ind_level <= 0) {
		print_blks32(idblk, ptrperblk, blknum);
	} else {
		ind_level--;
		for (i = 0; i < ptrperblk; i++)
			print_indirblks32(iswap32(idblk[i]), ind_level, blknum);
	}
#undef MAXNINDIR
}

static void
print_indirblks64(uint64_t blk, int ind_level, uint64_t *blknum)
{
#define MAXNINDIR	(MAXBSIZE / sizeof(int64_t))
	const int ptrperblk_shift = sblock->fs_bshift - 3;
	const int ptrperblk = 1 << ptrperblk_shift;
	int64_t idblk[MAXNINDIR];
	int i;

	if (blk == 0) {
		*blknum += (uint64_t)ptrperblk << (ptrperblk_shift * ind_level);
		return;
	}
 
	printf("Indirect block %lld (level %d):\n", (long long)blk,
	    ind_level+1);
	bread(fsreadfd, (char *)idblk, fsbtodb(sblock, blk),
	    (int)sblock->fs_bsize);
	if (ind_level <= 0) {
		print_blks64(idblk, ptrperblk, blknum);
	} else {
		ind_level--;
		for (i = 0; i < ptrperblk; i++)
			print_indirblks64(iswap64(idblk[i]), ind_level, blknum);
	}
#undef MAXNINDIR
}

static int
dolookup(char *name)
{
	struct inodesc idesc;

	if (!checkactivedir())
		return 0;
	idesc.id_number = curinum;
	idesc.id_func = findino;
	idesc.id_name = name;
	idesc.id_type = DATA;
	idesc.id_fix = IGNORE;
	if (ckinode(curinode, &idesc) & FOUND) {
		curinum = idesc.id_parent;
		curinode = ginode(curinum);
		printactive();
		return 1;
	} else {
		warnx("name `%s' not found in current inode directory", name);
		return 0;
	}
}

CMDFUNC(focusname)
{
	char   *p, *val;

	if (!checkactive())
		return 1;

	ocurrent = curinum;

	if (argv[1][0] == '/') {
		curinum = UFS_ROOTINO;
		curinode = ginode(UFS_ROOTINO);
	} else {
		if (!checkactivedir())
			return 1;
	}
	for (p = argv[1]; p != NULL;) {
		while ((val = strsep(&p, "/")) != NULL && *val == '\0');
		if (val) {
			printf("component `%s': ", val);
			fflush(stdout);
			if (!dolookup(val)) {
				curinode = ginode(curinum);
				return (1);
			}
		}
	}
	return 0;
}

CMDFUNC(ln)
{
	ino_t   inum;
	int     rval;
	char   *cp;

	GETINUM(1, inum);

	if (!checkactivedir())
		return 1;
	rval = makeentry(curinum, inum, argv[2]);
	if (rval)
		printf("Ino %llu entered as `%s'\n", (unsigned long long)inum,
		    argv[2]);
	else
		printf("could not enter name? weird.\n");
	curinode = ginode(curinum);
	return rval;
}

CMDFUNC(rm)
{
	int     rval;

	if (!checkactivedir())
		return 1;
	rval = changeino(curinum, argv[1], 0);
	if (rval & ALTERED) {
		printf("Name `%s' removed\n", argv[1]);
		return 0;
	} else {
		printf("could not remove name? weird.\n");
		return 1;
	}
}

static long slotcount, desired;

static int
chinumfunc(struct inodesc *idesc)
{
	struct direct *dirp = idesc->id_dirp;

	if (slotcount++ == desired) {
		dirp->d_ino = iswap32(idesc->id_parent);
		return STOP | ALTERED | FOUND;
	}
	return KEEPON;
}

CMDFUNC(chinum)
{
	char   *cp;
	ino_t   inum;
	struct inodesc idesc;

	slotcount = 0;
	if (!checkactivedir())
		return 1;
	GETINUM(2, inum);

	desired = strtol(argv[1], &cp, 0);
	if (cp == argv[1] || *cp != '\0' || desired < 0) {
		printf("invalid slot number `%s'\n", argv[1]);
		return 1;
	}
	idesc.id_number = curinum;
	idesc.id_func = chinumfunc;
	idesc.id_fix = IGNORE;
	idesc.id_type = DATA;
	idesc.id_parent = inum;	/* XXX convenient hiding place */

	if (ckinode(curinode, &idesc) & FOUND)
		return 0;
	else {
		warnx("no %sth slot in current directory", argv[1]);
		return 1;
	}
}

static int
chnamefunc(struct inodesc *idesc)
{
	struct direct *dirp = idesc->id_dirp;
	struct direct testdir;

	if (slotcount++ == desired) {
		/* will name fit? */
		testdir.d_namlen = strlen(idesc->id_name);
		if (DIRSIZ(NEWDIRFMT, &testdir, 0) <= iswap16(dirp->d_reclen)) {
			dirp->d_namlen = testdir.d_namlen;
			strlcpy(dirp->d_name, idesc->id_name,
			    sizeof(dirp->d_name));
			return STOP | ALTERED | FOUND;
		} else
			return STOP | FOUND;	/* won't fit, so give up */
	}
	return KEEPON;
}

CMDFUNC(chname)
{
	int     rval;
	char   *cp;
	struct inodesc idesc;

	slotcount = 0;
	if (!checkactivedir())
		return 1;

	desired = strtoul(argv[1], &cp, 0);
	if (cp == argv[1] || *cp != '\0') {
		printf("invalid slot number `%s'\n", argv[1]);
		return 1;
	}
	idesc.id_number = curinum;
	idesc.id_func = chnamefunc;
	idesc.id_fix = IGNORE;
	idesc.id_type = DATA;
	idesc.id_name = argv[2];

	rval = ckinode(curinode, &idesc);
	if ((rval & (FOUND | ALTERED)) == (FOUND | ALTERED))
		return 0;
	else
		if (rval & FOUND) {
			warnx("new name `%s' does not fit in slot %s",
			    argv[2], argv[1]);
			return 1;
		} else {
			warnx("no %sth slot in current directory", argv[1]);
			return 1;
		}
}

static struct typemap {
	const char *typename;
	int     typebits;
}       typenamemap[] = {
	{ "file", IFREG },
	{ "dir", IFDIR },
	{ "socket", IFSOCK },
	{ "fifo", IFIFO },
};

CMDFUNC(newtype)
{
	int     type;
	uint16_t mode;
	struct typemap *tp;

	if (!checkactive())
		return 1;
	mode = iswap16(DIP(curinode, mode));
	type = mode & IFMT;
	for (tp = typenamemap;
	    tp < &typenamemap[sizeof(typenamemap) / sizeof(*typenamemap)];
	    tp++) {
		if (!strcmp(argv[1], tp->typename)) {
			printf("setting type to %s\n", tp->typename);
			type = tp->typebits;
			break;
		}
	}
	if (tp == &typenamemap[sizeof(typenamemap) / sizeof(*typenamemap)]) {
		warnx("type `%s' not known", argv[1]);
		warnx("try one of `file', `dir', `socket', `fifo'");
		return 1;
	}
	DIP_SET(curinode, mode, iswap16((mode & ~IFMT) | type));
	inodirty();
	printactive();
	return 0;
}

CMDFUNC(chmode)
{
	long    modebits;
	char   *cp;
	uint16_t mode;

	if (!checkactive())
		return 1;

	modebits = strtol(argv[1], &cp, 8);
	if (cp == argv[1] || *cp != '\0') {
		warnx("bad modebits `%s'", argv[1]);
		return 1;
	}
	mode = iswap16(DIP(curinode, mode));
	DIP_SET(curinode, mode, iswap16((mode & ~07777) | modebits));
	inodirty();
	printactive();
	return 0;
}

CMDFUNC(chlen)
{
	long    len;
	char   *cp;

	if (!checkactive())
		return 1;

	len = strtol(argv[1], &cp, 0);
	if (cp == argv[1] || *cp != '\0' || len < 0) {
		warnx("bad length '%s'", argv[1]);
		return 1;
	}
	DIP_SET(curinode, size, iswap64(len));
	inodirty();
	printactive();
	return 0;
}

CMDFUNC(chaflags)
{
	u_long  flags;
	char   *cp;

	if (!checkactive())
		return 1;

	flags = strtoul(argv[1], &cp, 0);
	if (cp == argv[1] || *cp != '\0') {
		warnx("bad flags `%s'", argv[1]);
		return 1;
	}
	if (flags > UINT_MAX) {
		warnx("flags set beyond 32-bit range of field (0x%lx)",
		    flags);
		return (1);
	}
	DIP_SET(curinode, flags, iswap32(flags));
	inodirty();
	printactive();
	return 0;
}

CMDFUNC(chgen)
{
	long    gen;
	char   *cp;

	if (!checkactive())
		return 1;

	gen = strtol(argv[1], &cp, 0);
	if (cp == argv[1] || *cp != '\0') {
		warnx("bad gen `%s'", argv[1]);
		return 1;
	}
	if (gen > INT_MAX || gen < INT_MIN) {
		warnx("gen set beyond 32-bit range of field (0x%lx)", gen);
		return (1);
	}
	DIP_SET(curinode, gen, iswap32(gen));
	inodirty();
	printactive();
	return 0;
}

CMDFUNC(linkcount)
{
	int     lcnt;
	char   *cp;

	if (!checkactive())
		return 1;

	lcnt = strtol(argv[1], &cp, 0);
	if (cp == argv[1] || *cp != '\0') {
		warnx("bad link count `%s'", argv[1]);
		return 1;
	}
	if (lcnt > USHRT_MAX || lcnt < 0) {
		warnx("max link count is %d", USHRT_MAX);
		return 1;
	}
	DIP_SET(curinode, nlink, iswap16(lcnt));
	inodirty();
	printactive();
	return 0;
}

CMDFUNC(chowner)
{
	unsigned long uid;
	char   *cp;
	struct passwd *pwd;

	if (!checkactive())
		return 1;

	uid = strtoul(argv[1], &cp, 0);
	if (cp == argv[1] || *cp != '\0') {
		/* try looking up name */
		if ((pwd = getpwnam(argv[1])) != 0) {
			uid = pwd->pw_uid;
		} else {
			warnx("bad uid `%s'", argv[1]);
			return 1;
		}
	}
	if (!is_ufs2 && sblock->fs_old_inodefmt < FS_44INODEFMT)
		curinode->dp1.di_ouid = iswap32(uid);
	else
		DIP_SET(curinode, uid, iswap32(uid));
	inodirty();
	printactive();
	return 0;
}

CMDFUNC(chgroup)
{
	unsigned long gid;
	char   *cp;
	struct group *grp;

	if (!checkactive())
		return 1;

	gid = strtoul(argv[1], &cp, 0);
	if (cp == argv[1] || *cp != '\0') {
		if ((grp = getgrnam(argv[1])) != 0) {
			gid = grp->gr_gid;
		} else {
			warnx("bad gid `%s'", argv[1]);
			return 1;
		}
	}
	if (sblock->fs_old_inodefmt < FS_44INODEFMT)
		curinode->dp1.di_ogid = iswap32(gid);
	else
		DIP_SET(curinode, gid, iswap32(gid));
	inodirty();
	printactive();
	return 0;
}

static int
dotime(char *name, int32_t *rsec, int32_t *rnsec)
{
	char   *p, *val;
	struct tm t;
	int32_t sec;
	int32_t nsec;
	p = strchr(name, '.');
	if (p) {
		*p = '\0';
		nsec = strtoul(++p, &val, 0);
		if (val == p || *val != '\0' || nsec >= 1000000000 || nsec < 0) {
			warnx("invalid nanoseconds");
			goto badformat;
		}
	} else
		nsec = 0;
	if (strlen(name) != 14) {
badformat:
		warnx("date format: YYYYMMDDHHMMSS[.nsec]");
		return 1;
	}
	for (p = name; *p; p++)
		if (*p < '0' || *p > '9')
			goto badformat;

	p = name;
#define VAL() ((*p++) - '0')
	t.tm_year = VAL();
	t.tm_year = VAL() + t.tm_year * 10;
	t.tm_year = VAL() + t.tm_year * 10;
	t.tm_year = VAL() + t.tm_year * 10 - 1900;
	t.tm_mon = VAL();
	t.tm_mon = VAL() + t.tm_mon * 10 - 1;
	t.tm_mday = VAL();
	t.tm_mday = VAL() + t.tm_mday * 10;
	t.tm_hour = VAL();
	t.tm_hour = VAL() + t.tm_hour * 10;
	t.tm_min = VAL();
	t.tm_min = VAL() + t.tm_min * 10;
	t.tm_sec = VAL();
	t.tm_sec = VAL() + t.tm_sec * 10;
	t.tm_isdst = -1;

	sec = mktime(&t);
	if (sec == -1) {
		warnx("date/time out of range");
		return 1;
	}
	*rsec = iswap32(sec);
	*rnsec = iswap32(nsec);
	return 0;
}

CMDFUNC(chmtime)
{
	int32_t rsec, nsec;

	if (dotime(argv[1], &rsec, &nsec))
		return 1;
	DIP_SET(curinode, mtime, rsec);
	DIP_SET(curinode, mtimensec, nsec);
	inodirty();
	printactive();
	return 0;
}

CMDFUNC(chatime)
{
	int32_t rsec, nsec;

	if (dotime(argv[1], &rsec, &nsec))
		return 1;
	DIP_SET(curinode, atime, rsec);
	DIP_SET(curinode, atimensec, nsec);
	inodirty();
	printactive();
	return 0;
}

CMDFUNC(chctime)
{
	int32_t rsec, nsec;

	if (dotime(argv[1], &rsec, &nsec))
		return 1;
	DIP_SET(curinode, ctime, rsec);
	DIP_SET(curinode, ctimensec, nsec);
	inodirty();
	printactive();
	return 0;
}
