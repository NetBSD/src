/*	$NetBSD: ufs_ls.c,v 1.3 2003/02/01 14:52:13 dsl Exp $	 */

/*
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1996
 *	Matthias Drochner.  All rights reserved.
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


#include <sys/param.h>
#include <string.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>

#include "stand.h"
#include "ufs.h"

#define NELEM(x) (sizeof (x) / sizeof(*x))

typedef struct entry_t entry_t;
struct entry_t {
	entry_t	*e_next;
	ino_t	e_ino;
	uint8_t	e_type;
	char	e_name[1];
};

static const char    *typestr[] = {
	"unknown",
	"FIFO",
	"CHR",
	0,
	"DIR",
	0,
	"BLK",
	0,
	"REG",
	0,
	"LNK",
	0,
	"SOCK",
	0,
	"WHT"
};

static int
fn_match(const char *fname, const char *pattern)
{
	char fc, pc;

	do {
		fc = *fname++;
		pc = *pattern++;
		if (!fc && !pc)
			return 1;
		if (pc == '?' && fc)
			pc = fc;
	} while (fc == pc);

	if (pc != '*')
		return 0;
	/* Too hard (and unnecessary really) too check for "*?name" etc....
	   "**" will look for a '*' and "*?" a '?' */
	pc = *pattern++;
	if (!pc)
		return 1;
	while ((fname = strchr(fname, pc)))
		if (fn_match(++fname, pattern))
			return 1;
	return 0;
}

void 
ufs_ls(const char *path)
{
	int             fd;
	struct stat     sb;
	size_t          size;
	char            dirbuf[DIRBLKSIZ];
	const char	*fname = 0;
	char		*p;
	entry_t		*names = 0, *n, **np;

	if ((fd = open(path, 0)) < 0
	    || fstat(fd, &sb) < 0
	    || (sb.st_mode & IFMT) != IFDIR) {
		/* Path supplied isn't a directory, open parent
		   directory and list matching files. */
		if (fd >= 0)
			close(fd);
		fname = strrchr(path, '/');
		if (fname) {
			size = fname - path;
			p = alloc(size + 1);
			if (!p)
				goto out;
			memcpy(p, path, size);
			p[size] = 0;
			fd = open(p, 0);
			free(p, size + 1);
		} else {
			fd = open("", 0);
			fname = path;
		}

		if (fd < 0) {
			printf("ls: %s\n", strerror(errno));
			return;
		}
		if (fstat(fd, &sb) < 0) {
			printf("stat: %s\n", strerror(errno));
			goto out;
		}
		if ((sb.st_mode & IFMT) != IFDIR) {
			printf("%s: %s\n", path, strerror(ENOTDIR));
			goto out;
		}
	}

	while ((size = read(fd, dirbuf, DIRBLKSIZ)) == DIRBLKSIZ) {
		struct direct  *dp, *edp;

		dp = (struct direct *) dirbuf;
		edp = (struct direct *) (dirbuf + size);

		for (; dp < edp; dp = (void *)((char *)dp + dp->d_reclen)) {
			const char *t;
			if (dp->d_ino ==  0)
				continue;

			if (dp->d_type >= NELEM(typestr) ||
			    !(t = typestr[dp->d_type])) {
				/*
				 * This does not handle "old"
				 * filesystems properly. On little
				 * endian machines, we get a bogus
				 * type name if the namlen matches a
				 * valid type identifier. We could
				 * check if we read namlen "0" and
				 * handle this case specially, if
				 * there were a pressing need...
				 */
				printf("bad dir entry\n");
				goto out;
			}
			if (fname && !fn_match(dp->d_name, fname))
				continue;
			n = alloc(sizeof *n + strlen(dp->d_name));
			if (!n) {
				printf("%d: %s (%s)\n",
					dp->d_ino, dp->d_name, t);
				continue;
			}
			n->e_ino = dp->d_ino;
			n->e_type = dp->d_type;
			strcpy(n->e_name, dp->d_name);
			for (np = &names; *np; np = &(*np)->e_next) {
				if (strcmp(n->e_name, (*np)->e_name) < 0)
					break;
			}
			n->e_next = *np;
			*np = n;
		}
	}

	if (names) {
		do {
			n = names;
			printf("%d: %s (%s)\n",
				n->e_ino, n->e_name, typestr[n->e_type]);
			names = n->e_next;
			free(n, 0);
		} while (names);
	} else {
		printf( "%s not found\n", path );
	}
out:
	close(fd);
}
