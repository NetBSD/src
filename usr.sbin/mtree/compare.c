/*	$NetBSD: compare.c,v 1.21 1999/07/06 15:11:14 christos Exp $	*/

/*-
 * Copyright (c) 1989, 1993
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
#if 0
static char sccsid[] = "@(#)compare.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: compare.c,v 1.21 1999/07/06 15:11:14 christos Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <fts.h>
#include <errno.h>
#include <md5.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "mtree.h"
#include "extern.h"

extern int iflag, mflag, tflag, uflag;

static char *ftype __P((u_int));

#define	INDENTNAMELEN	8
#define MARK                                                                  \
do {                                                                          \
	len = printf("%s: ", RP(p));                                          \
	if (len > INDENTNAMELEN) {                                            \
		tab = "\t";                                                   \
		(void)printf("\n");                                           \
	} else {                                                              \
		tab = "";                                                     \
		(void)printf("%*s", INDENTNAMELEN - (int)len, "");            \
	}                                                                     \
} while (0)
#define	LABEL if (!label++) MARK

#define CHANGEFLAGS(path, oflags) \
	if (flags != (oflags)) {                                              \
		if (!label) {                                                 \
			MARK;                                                 \
			(void)printf("%sflags (\"%s\"", tab,                  \
			    flags_to_string(p->fts_statp->st_flags, "none")); \
		}                                                             \
		if (chflags(path, flags)) {                                   \
			label++;                                              \
			(void)printf(", not modified: %s)\n",                 \
			    strerror(errno));                                 \
		} else                                                        \
			(void)printf(", modified to \"%s\")\n",               \
			     flags_to_string(flags, "none"));                 \
	}

/* SETFLAGS:
 * given pflags, additionally set those flags specified in sflags and
 * selected by mask (the other flags are left unchanged). oflags is
 * passed as reference to check if chflags is necessary.
 */
#define SETFLAGS(path, sflags, pflags, oflags, mask)                          \
do {                                                                          \
	flags = ((sflags) & (mask)) | (pflags);                               \
        CHANGEFLAGS(path, oflags);                                            \
} while (0)

/* CLEARFLAGS:
 * given pflags, reset the flags specified in sflags and selected by mask
 * (the other flags are left unchanged). oflags is
 * passed as reference to check if chflags is necessary.
 */
#define CLEARFLAGS(path, sflags, pflags, oflags, mask)                        \
do {                                                                          \
	flags = (~((sflags) & (mask)) & CH_MASK) & (pflags);                  \
        CHANGEFLAGS(path, oflags);                                            \
} while (0)

int
compare(name, s, p)
	char *name;
	NODE *s;
	FTSENT *p;
{
	u_int32_t len, val, flags;
	int fd, label;
	char *cp, *tab;
	char md5buf[35];

	tab = NULL;
	label = 0;
	switch(s->type) {
	case F_BLOCK:
		if (!S_ISBLK(p->fts_statp->st_mode))
			goto typeerr;
		break;
	case F_CHAR:
		if (!S_ISCHR(p->fts_statp->st_mode))
			goto typeerr;
		break;
	case F_DIR:
		if (!S_ISDIR(p->fts_statp->st_mode))
			goto typeerr;
		break;
	case F_FIFO:
		if (!S_ISFIFO(p->fts_statp->st_mode))
			goto typeerr;
		break;
	case F_FILE:
		if (!S_ISREG(p->fts_statp->st_mode))
			goto typeerr;
		break;
	case F_LINK:
		if (!S_ISLNK(p->fts_statp->st_mode))
			goto typeerr;
		break;
	case F_SOCK:
		if (!S_ISSOCK(p->fts_statp->st_mode)) {
typeerr:		LABEL;
			(void)printf("\ttype (%s, %s)\n",
			    ftype(s->type), inotype(p->fts_statp->st_mode));
		}
		break;
	}
	if (iflag && !uflag) {
		if (s->flags & F_FLAGS)
		    SETFLAGS(p->fts_accpath, s->st_flags,
			p->fts_statp->st_flags, p->fts_statp->st_flags,
			SP_FLGS);
		return (label);
        }
	if (mflag && !uflag) {
		if (s->flags & F_FLAGS)
		    CLEARFLAGS(p->fts_accpath, s->st_flags, 
			p->fts_statp->st_flags, p->fts_statp->st_flags,
			SP_FLGS);
		return (label);
        }
	/* Set the uid/gid first, then set the mode. */
	if (s->flags & (F_UID | F_UNAME) && s->st_uid != p->fts_statp->st_uid) {
		LABEL;
		(void)printf("%suser (%lu, %lu",
		    tab, (u_long)s->st_uid, (u_long)p->fts_statp->st_uid);
		if (uflag) {
			if (chown(p->fts_accpath, s->st_uid, -1))
				(void)printf(", not modified: %s)\n",
				    strerror(errno));
			else
				(void)printf(", modified)\n");
		} else
			(void)printf(")\n");
		tab = "\t";
	}
	if (s->flags & (F_GID | F_GNAME) && s->st_gid != p->fts_statp->st_gid) {
		LABEL;
		(void)printf("%sgid (%lu, %lu",
		    tab, (u_long)s->st_gid, (u_long)p->fts_statp->st_gid);
		if (uflag) {
			if (chown(p->fts_accpath, -1, s->st_gid))
				(void)printf(", not modified: %s)\n",
				    strerror(errno));
			else
				(void)printf(", modified)\n");
		}
		else
			(void)printf(")\n");
		tab = "\t";
	}
	if (s->flags & F_MODE &&
	    s->st_mode != (p->fts_statp->st_mode & MBITS)) {
		LABEL;
		(void)printf("%spermissions (%#lo, %#lo",
		    tab, (u_long)s->st_mode,
		    (u_long)p->fts_statp->st_mode & MBITS);
		if (uflag) {
			if (chmod(p->fts_accpath, s->st_mode))
				(void)printf(", not modified: %s)\n",
				    strerror(errno));
			else
				(void)printf(", modified)\n");
		}
		else
			(void)printf(")\n");
		tab = "\t";
	}
	if (s->flags & F_NLINK && s->type != F_DIR &&
	    s->st_nlink != p->fts_statp->st_nlink) {
		LABEL;
		(void)printf("%slink count (%lu, %lu)\n",
		    tab, (u_long)s->st_nlink, (u_long)p->fts_statp->st_nlink);
		tab = "\t";
	}
	if (s->flags & F_SIZE && s->st_size != p->fts_statp->st_size) {
		LABEL;
		(void)printf("%ssize (%qd, %qd)\n",
		    tab, (long long)s->st_size,
		    (long long)p->fts_statp->st_size);
		tab = "\t";
	}
	/*
	 * XXX
	 * Since utimes(2) only takes a timeval, there's no point in
	 * comparing the low bits of the timespec nanosecond field.  This
	 * will only result in mismatches that we can never fix.
	 *
	 * Doesn't display microsecond differences.
	 */
	if (s->flags & F_TIME) {
		struct timeval tv[2];
		struct stat *ps = p->fts_statp;
#ifdef BSD4_4
		time_t smtime = s->st_mtimespec.tv_sec;
		time_t pmtime = ps->st_mtimespec.tv_sec;

		TIMESPEC_TO_TIMEVAL(&tv[0], &s->st_mtimespec);
		TIMESPEC_TO_TIMEVAL(&tv[1], &ps->st_mtimespec);
#else
		time_t smtime = s->st_mtime;
		time_t pmtime = ps->st_mtime;

		tv[0].tv_sec = s->st_mtime;
		tv[0].tv_usec = 0;
		tv[1].tv_sec = ps->st_mtime;
		tv[1].tv_usec = 0;
#endif


		if (tv[0].tv_sec != tv[1].tv_sec ||
		    tv[0].tv_usec != tv[1].tv_usec) {
			LABEL;
			(void)printf("%smodification time (%.24s, ",
			    tab, ctime(&smtime));
			(void)printf("%.24s", ctime(&pmtime));
			if (tflag) {
				tv[1] = tv[0];
				if (utimes(p->fts_accpath, tv))
					(void)printf(", not modified: %s)\n",
					    strerror(errno));
				else
					(void)printf(", modified)\n");
			} else
				(void)printf(")\n");
			tab = "\t";
		}
	}
	/*
	 * XXX
	 * since chflags(2) will reset file times, the utimes() above
	 * may have been useless!  oh well, we'd rather have correct
	 * flags, rather than times?
	 */
        if ((s->flags & F_FLAGS) && ((s->st_flags != p->fts_statp->st_flags)
	    || mflag || iflag)) {
		if (s->st_flags != p->fts_statp->st_flags) {
			LABEL;
			(void)printf("%sflags (\"%s\" is not ", tab,
			    flags_to_string(s->st_flags, "none"));
			(void)printf("\"%s\"",
			    flags_to_string(p->fts_statp->st_flags, "none"));
		}
		if (uflag) {
			if (iflag)
				SETFLAGS(p->fts_accpath, s->st_flags,
				    0, p->fts_statp->st_flags, CH_MASK);
			else if (mflag)
				CLEARFLAGS(p->fts_accpath, s->st_flags,
				    0, p->fts_statp->st_flags, SP_FLGS);
			else
				SETFLAGS(p->fts_accpath, s->st_flags,
			     	    0, p->fts_statp->st_flags,
				    (~SP_FLGS & CH_MASK));
		} else
			(void)printf(")\n");
		tab = "\t";
	}
	if (s->flags & F_CKSUM) {
		if ((fd = open(p->fts_accpath, O_RDONLY, 0)) < 0) {
			LABEL;
			(void)printf("%scksum: %s: %s\n",
			    tab, p->fts_accpath, strerror(errno));
			tab = "\t";
		} else if (crc(fd, &val, &len)) {
			(void)close(fd);
			LABEL;
			(void)printf("%scksum: %s: %s\n",
			    tab, p->fts_accpath, strerror(errno));
			tab = "\t";
		} else {
			(void)close(fd);
			if (s->cksum != val) {
				LABEL;
				(void)printf("%scksum (%lu, %lu)\n", 
				    tab, s->cksum, (unsigned long)val);
			}
			tab = "\t";
		}
	}
	if (s->flags & F_MD5) {
		if (MD5File(p->fts_accpath, md5buf) == NULL) {
			LABEL;
			(void)printf("%smd5: %s: %s\n",
			    tab, p->fts_accpath, strerror(errno));
			tab = "\t";
		} else {
			if (strcmp(s->md5sum, md5buf)) {
				LABEL;
				(void)printf("%smd5 (0x%s, 0x%s)\n",
				    tab, s->md5sum, md5buf);
			}
			tab = "\t";
		}
	}

	if (s->flags & F_SLINK && strcmp(cp = rlink(name), s->slink)) {
		LABEL;
		(void)printf("%slink ref (%s, %s)\n", tab, cp, s->slink);
	}
	return (label);
}

char *
inotype(type)
	u_int type;
{
	switch(type & S_IFMT) {
	case S_IFBLK:
		return ("block");
	case S_IFCHR:
		return ("char");
	case S_IFDIR:
		return ("dir");
	case S_IFIFO:
		return ("fifo");
	case S_IFREG:
		return ("file");
	case S_IFLNK:
		return ("link");
	case S_IFSOCK:
		return ("socket");
	default:
		return ("unknown");
	}
	/* NOTREACHED */
}

static char *
ftype(type)
	u_int type;
{
	switch(type) {
	case F_BLOCK:
		return ("block");
	case F_CHAR:
		return ("char");
	case F_DIR:
		return ("dir");
	case F_FIFO:
		return ("fifo");
	case F_FILE:
		return ("file");
	case F_LINK:
		return ("link");
	case F_SOCK:
		return ("socket");
	default:
		return ("unknown");
	}
	/* NOTREACHED */
}

char *
rlink(name)
	char *name;
{
	static char lbuf[MAXPATHLEN];
	int len;

	if ((len = readlink(name, lbuf, sizeof(lbuf))) == -1)
		mtree_err("%s: %s", name, strerror(errno));
	lbuf[len] = '\0';
	return (lbuf);
}
