/*	$NetBSD: fsdbutil.c,v 1.13 2003/04/02 10:39:29 fvdl Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__RCSID("$NetBSD: fsdbutil.c,v 1.13 2003/04/02 10:39:29 fvdl Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/mount.h>
#include <ctype.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <err.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>

#include "fsdb.h"
#include "fsck.h"

char  **
crack(line, argc)
	char   *line;
	int    *argc;
{
	static char *argv[8];
	int     i;
	char   *p, *val;
	for (p = line, i = 0; p != NULL && i < 8; i++) {
		while ((val = strsep(&p, " \t\n")) != NULL && *val == '\0')
			 /**/ ;
		if (val)
			argv[i] = val;
		else
			break;
	}
	*argc = i;
	return argv;
}

int
argcount(cmdp, argc, argv)
	struct cmdtable *cmdp;
	int     argc;
	char   *argv[];
{
	if (cmdp->minargc == cmdp->maxargc)
		warnx("command `%s' takes %u arguments", cmdp->cmd,
		    cmdp->minargc - 1);
	else
		warnx("command `%s' takes from %u to %u arguments",
		    cmdp->cmd, cmdp->minargc - 1, cmdp->maxargc - 1);

	warnx("usage: %s: %s", cmdp->cmd, cmdp->helptxt);
	return 1;
}

void
printstat(cp, inum, dp)
	const char *cp;
	ino_t   inum;
	union dinode *dp;
{
	struct group *grp;
	struct passwd *pw;
	time_t  t;
	char   *p;
	uint64_t size, blocks;
	uint16_t mode;
	uint32_t rdev;

	size = iswap64(DIP(dp, size));
	blocks = is_ufs2 ? iswap64(DIP(dp, blocks)) : iswap32(DIP(dp, blocks));
	mode = iswap16(DIP(dp, mode));
	rdev = iswap32(DIP(dp, rdev));

	printf("%s: ", cp);
	switch (mode & IFMT) {
	case IFDIR:
		puts("directory");
		break;
	case IFREG:
		puts("regular file");
		break;
	case IFBLK:
		printf("block special (%d,%d)", major(rdev), minor(rdev));
		break;
	case IFCHR:
		printf("character special (%d,%d)", major(rdev), minor(rdev));
		break;
	case IFLNK:
		fputs("symlink", stdout);
		if (size > 0 && size < sblock->fs_maxsymlinklen &&
		    DIP(dp, blocks) == 0) {
			p = is_ufs2 ? (char *)dp->dp2.di_db :
			    (char *)dp->dp1.di_db;
			printf(" to `%.*s'\n", (int)size, p);
		} else
			putchar('\n');
		break;
	case IFSOCK:
		puts("socket");
		break;
	case IFIFO:
		puts("fifo");
		break;
	}
	printf("I=%u MODE=%o SIZE=%llu", inum, mode,
	    (unsigned long long)size);
	t = is_ufs2 ? iswap64(dp->dp2.di_mtime) : iswap32(dp->dp1.di_mtime);
	p = ctime(&t);
	printf("\n\tMTIME=%15.15s %4.4s [%d nsec]", &p[4], &p[20],
	    iswap32(DIP(dp, mtimensec)));
	t = is_ufs2 ? iswap64(dp->dp2.di_ctime) : iswap32(dp->dp1.di_ctime);
	p = ctime(&t);
	printf("\n\tCTIME=%15.15s %4.4s [%d nsec]", &p[4], &p[20],
	    iswap32(DIP(dp, ctimensec)));
	t = is_ufs2 ? iswap64(dp->dp2.di_atime) : iswap32(dp->dp1.di_atime);
	p = ctime(&t);
	printf("\n\tATIME=%15.15s %4.4s [%d nsec]\n", &p[4], &p[20],
	    iswap32(DIP(dp,atimensec)));

	if ((pw = getpwuid(iswap32(DIP(dp, uid)))) != NULL)
		printf("OWNER=%s ", pw->pw_name);
	else
		printf("OWNUID=%u ", iswap32(DIP(dp, uid)));
	if ((grp = getgrgid(iswap32(DIP(dp, gid)))) != NULL)
		printf("GRP=%s ", grp->gr_name);
	else
		printf("GID=%u ", iswap32(DIP(dp, gid)));

	printf("LINKCNT=%hd FLAGS=0x%#x BLKCNT=0x%llx GEN=0x%x\n",
		iswap16(DIP(dp, nlink)),
	    iswap32(DIP(dp, flags)), (unsigned long long)blocks,
		iswap32(DIP(dp, gen)));
}

int
checkactive()
{
	if (!curinode) {
		warnx("no current inode");
		return 0;
	}
	return 1;
}

int
checkactivedir()
{
	if (!curinode) {
		warnx("no current inode");
		return 0;
	}
	if ((iswap16(DIP(curinode, mode)) & IFMT) != IFDIR) {
		warnx("inode %d not a directory", curinum);
		return 0;
	}
	return 1;
}

int
printactive()
{
	uint16_t mode;

	if (!checkactive())
		return 1;
	mode = iswap16(DIP(curinode, mode));
	switch (mode & IFMT) {
	case IFDIR:
	case IFREG:
	case IFBLK:
	case IFCHR:
	case IFLNK:
	case IFSOCK:
	case IFIFO:
		printstat("current inode", curinum, curinode);
		break;
	case 0:
		printf("current inode %d: unallocated inode\n", curinum);
		break;
	default:
		printf("current inode %d: screwy itype 0%o (mode 0%o)?\n",
		    curinum, mode & IFMT, mode);
		break;
	}
	return 0;
}
