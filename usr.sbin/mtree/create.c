/*	$NetBSD: create.c,v 1.31 2001/10/04 04:51:27 lukem Exp $	*/

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
static char sccsid[] = "@(#)create.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: create.c,v 1.31 2001/10/04 04:51:27 lukem Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <grp.h>
#include <md5.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <vis.h>

#include "mtree.h"
#include "extern.h"

#define	INDENTNAMELEN	15
#define	MAXLINELEN	80
#define VISFLAGS        VIS_CSTYLE

static gid_t gid;
static uid_t uid;
static mode_t mode;
static u_long flags;
static char codebuf[4*MAXPATHLEN + 1];
static const char extra[] = { ' ', '\t', '\n', '\\', '#', '\0' };

static int	dsort(const FTSENT **, const FTSENT **);
static void	output(int *, const char *, ...)
	__attribute__((__format__(__printf__, 2, 3)));
static int	statd(FTS *, FTSENT *, uid_t *, gid_t *, mode_t *, u_long *);
static void	statf(FTSENT *);

void
cwalk(void)
{
	FTS *t;
	FTSENT *p;
	time_t clock;
	char *argv[2], host[MAXHOSTNAMELEN + 1];

	(void)time(&clock);
	(void)gethostname(host, sizeof(host));
	host[sizeof(host) - 1] = '\0';
	(void)printf(
	    "#\t   user: %s\n#\tmachine: %s\n#\t   tree: %s\n#\t   date: %s",
	    getlogin(), host, fullpath, ctime(&clock));

	argv[0] = ".";
	argv[1] = NULL;
	if ((t = fts_open(argv, ftsoptions, dsort)) == NULL)
		mtree_err("fts_open: %s", strerror(errno));
	while ((p = fts_read(t)) != NULL)
		switch(p->fts_info) {
		case FTS_D:
			(void)printf("\n# %s\n", p->fts_path);
			statd(t, p, &uid, &gid, &mode, &flags);
			statf(p);
			break;
		case FTS_DP:
			if (p->fts_level > 0)
				(void)printf("# %s\n..\n\n", p->fts_path);
			break;
		case FTS_DNR:
		case FTS_ERR:
		case FTS_NS:
			(void)fprintf(stderr,
			    "mtree: %s: %s\n", p->fts_path, strerror(errno));
			break;
		default:
			if (!dflag)
				statf(p);
			break;
			
		}
	(void)fts_close(t);
	if (sflag && keys & F_CKSUM)
		(void)fprintf(stderr,
		    "mtree: %s checksum: %u\n", fullpath, crc_total);
}

static void
statf(FTSENT *p)
{
	u_int32_t len, val;
	int fd, indent;
	char md5buf[33], *md5cp;
	const char *name;

	strsvis(codebuf, p->fts_name, VISFLAGS, extra);
	if (S_ISDIR(p->fts_statp->st_mode))
		indent = printf("%s", codebuf); 
	else
		indent = printf("    %s", codebuf);

	if (indent > INDENTNAMELEN)
		indent = MAXLINELEN;
	else
		indent += printf("%*s", INDENTNAMELEN - indent, "");

	if (!S_ISREG(p->fts_statp->st_mode))
		output(&indent, "type=%s", inotype(p->fts_statp->st_mode));
	if (keys & (F_UID | F_UNAME) && p->fts_statp->st_uid != uid) {
		if (keys & F_UNAME &&
		    (name = user_from_uid(p->fts_statp->st_uid, 1)) != NULL)
			output(&indent, "uname=%s", name);
		else /* if (keys & F_UID) */
			output(&indent, "uid=%u", p->fts_statp->st_uid);
	}
	if (keys & (F_GID | F_GNAME) && p->fts_statp->st_gid != gid) {
		if (keys & F_GNAME &&
		    (name = group_from_gid(p->fts_statp->st_gid, 1)) != NULL)
			output(&indent, "gname=%s", name);
		else /* if (keys & F_GID) */
			output(&indent, "gid=%u", p->fts_statp->st_gid);
	}
	if (keys & F_MODE && (p->fts_statp->st_mode & MBITS) != mode)
		output(&indent, "mode=%#o", p->fts_statp->st_mode & MBITS);
	if (keys & F_NLINK && p->fts_statp->st_nlink != 1)
		output(&indent, "nlink=%u", p->fts_statp->st_nlink);
	if (keys & F_SIZE && S_ISREG(p->fts_statp->st_mode))
		output(&indent, "size=%lld", (long long)p->fts_statp->st_size);
#ifdef BSD4_4
	if (keys & F_TIME)
		output(&indent, "time=%ld.%ld",
		    (long)p->fts_statp->st_mtimespec.tv_sec,
		    p->fts_statp->st_mtimespec.tv_nsec);
#else
		output(&indent, "time=%ld.%ld",
		    p->fts_statp->st_mtime, 0);
#endif
	if (keys & F_CKSUM && S_ISREG(p->fts_statp->st_mode)) {
		if ((fd = open(p->fts_accpath, O_RDONLY, 0)) < 0 ||
		    crc(fd, &val, &len))
			mtree_err("%s: %s", p->fts_accpath, strerror(errno));
		(void)close(fd);
		output(&indent, "cksum=%lu", (long)val);
	}
	if (keys & F_MD5 && S_ISREG(p->fts_statp->st_mode)) {
		if ((md5cp = MD5File(p->fts_accpath, md5buf)) == NULL)
			mtree_err("%s: %s", p->fts_accpath, "MD5File");
				  output(&indent, "md5=%s", md5cp);
	}
	if (keys & F_SLINK &&
	    (p->fts_info == FTS_SL || p->fts_info == FTS_SLNONE))
		output(&indent, "link=%s", rlink(p->fts_accpath));
	if (keys & F_FLAGS && p->fts_statp->st_flags != flags)
		output(&indent, "flags=%s",
		    flags_to_string(p->fts_statp->st_flags, "none"));
	(void)putchar('\n');
}

/* XXX
 * FLAGS2INDEX will fail once the user and system settable bits need more
 * than one byte, respectively.
 */
#define FLAGS2INDEX(x)  (((x >> 8) & 0x0000ff00) | (x & 0x000000ff))

#define	MTREE_MAXGID	5000
#define	MTREE_MAXUID	5000
#define	MTREE_MAXMODE	(MBITS + 1)
#define	MTREE_MAXFLAGS  (FLAGS2INDEX(CH_MASK) + 1)   /* 1808 */
#define	MTREE_MAXS 16

static int
statd(FTS *t, FTSENT *parent, uid_t *puid, gid_t *pgid, mode_t *pmode,
      u_long *pflags)
{
	FTSENT *p;
	gid_t sgid;
	uid_t suid;
	mode_t smode;
	u_long sflags;
	const char *name;
	gid_t savegid;
	uid_t saveuid;
	mode_t savemode;
	u_long saveflags;
	u_short maxgid, maxuid, maxmode, maxflags;
	u_short g[MTREE_MAXGID], u[MTREE_MAXUID],
		m[MTREE_MAXMODE], f[MTREE_MAXFLAGS];

	savegid = 0;
	saveuid = 0;
	savemode = 0;
	saveflags = 0;
	if ((p = fts_children(t, 0)) == NULL) {
		if (errno)
			mtree_err("%s: %s", RP(parent), strerror(errno));
		return (1);
	}

	memset(g, 0, sizeof(g));
	memset(u, 0, sizeof(u));
	memset(m, 0, sizeof(m));
	memset(f, 0, sizeof(f));

	maxuid = maxgid = maxmode = maxflags = 0;
	for (; p; p = p->fts_link) {
		smode = p->fts_statp->st_mode & MBITS;
		if (smode < MTREE_MAXMODE && ++m[smode] > maxmode) {
			savemode = smode;
			maxmode = m[smode];
		}
		sgid = p->fts_statp->st_gid;
		if (sgid < MTREE_MAXGID && ++g[sgid] > maxgid) {
			savegid = sgid;
			maxgid = g[sgid];
		}
		suid = p->fts_statp->st_uid;
		if (suid < MTREE_MAXUID && ++u[suid] > maxuid) {
			saveuid = suid;
			maxuid = u[suid];
		}

		sflags = FLAGS2INDEX(p->fts_statp->st_flags);
		if (sflags < MTREE_MAXFLAGS && ++f[sflags] > maxflags) {
			saveflags = p->fts_statp->st_flags;
			maxflags = f[sflags];
		}
	}
	(void)printf("/set type=file");
	if (keys & F_GID)
		(void)printf(" gid=%lu", (u_long)savegid);
	if (keys & F_GNAME) {
		if ((name = group_from_gid(savegid, 1)) != NULL)
			(void)printf(" gname=%s", name);
		else
			(void)printf(" gid=%lu", (u_long)savegid);
	}
	if (keys & F_UNAME) {
		if ((name = user_from_uid(saveuid, 1)) != NULL)
			(void)printf(" uname=%s", name);
		else
			(void)printf(" uid=%lu", (u_long)saveuid);
	}
	if (keys & F_UID)
		(void)printf(" uid=%lu", (u_long)saveuid);
	if (keys & F_MODE)
		(void)printf(" mode=%#lo", (u_long)savemode);
	if (keys & F_NLINK)
		(void)printf(" nlink=1");
	if (keys & F_FLAGS)
		(void)printf(" flags=%s",
		    flags_to_string(saveflags, "none"));
	(void)printf("\n");
	*puid = saveuid;
	*pgid = savegid;
	*pmode = savemode;
	*pflags = saveflags;
	return (0);
}

static int
dsort(const FTSENT **a, const FTSENT **b)
{

	if (S_ISDIR((*a)->fts_statp->st_mode)) {
		if (!S_ISDIR((*b)->fts_statp->st_mode))
			return (1);
	} else if (S_ISDIR((*b)->fts_statp->st_mode))
		return (-1);
	return (strcmp((*a)->fts_name, (*b)->fts_name));
}

void
output(int *offset, const char *fmt, ...)
{
	va_list ap;
	char buf[1024];

	va_start(ap, fmt);
	(void)vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	if (*offset + strlen(buf) > MAXLINELEN - 3) {
		(void)printf(" \\\n%*s", INDENTNAMELEN, "");
		*offset = INDENTNAMELEN;
	}
	*offset += printf(" %s", buf) + 1;
}
