/*	$NetBSD: ls.c,v 1.3 1997/06/13 13:48:47 drochner Exp $	 */

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
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>

#include "stand.h"

static char    *typestr[] = {
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

void 
ls(path)
	char           *path;
{
	int             fd;
	struct stat     sb;
	size_t          size;
	char            dirbuf[DIRBLKSIZ];

	fd = open(path, 0);
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
	while ((size = read(fd, dirbuf, DIRBLKSIZ)) == DIRBLKSIZ) {
		struct direct  *dp, *edp;

		dp = (struct direct *) dirbuf;
		edp = (struct direct *) (dirbuf + size);

		while (dp < edp) {
			if (dp->d_ino != (ino_t) 0) {
				char           *t;

				if ((dp->d_namlen > MAXNAMLEN + 1) ||
				    (dp->d_type >
				      sizeof(typestr) / sizeof(char *) - 1) ||
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
				printf("%d: %s (%s)\n", dp->d_ino,
				    dp->d_name, t);
			}
			dp = (struct direct *) ((char *) dp + dp->d_reclen);
		}
	}
out:
	close(fd);
}
