/*	$NetBSD: spec.c,v 1.24 2001/09/10 03:22:24 lukem Exp $	*/

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
static char sccsid[] = "@(#)spec.c	8.2 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: spec.c,v 1.24 2001/09/10 03:22:24 lukem Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>

#include <ctype.h>
#include <errno.h>
#include <fts.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <util.h>
#include <vis.h>

#include "mtree.h"
#include "extern.h"

size_t lineno;				/* Current spec line number. */

static void	 set(char *, NODE *);
static void	 unset(char *, NODE *);

NODE *
spec(void)
{
	NODE *centry, *last;
	char *p;
	NODE ginfo, *root;
	char *buf;

	root = NULL;
	centry = last = NULL;
	memset(&ginfo, 0, sizeof(ginfo));
	for (lineno = 0;
	    (buf = fparseln(stdin, NULL, &lineno, NULL,
		FPARSELN_UNESCCOMM | FPARSELN_UNESCCONT | FPARSELN_UNESCESC));
	    free(buf)) {
		/* Skip leading whitespace. */
		for (p = buf; *p && isspace((unsigned char)*p); ++p)
			continue;

		/* If nothing but whitespace, continue. */
		if (!*p)
			continue;

#ifdef DEBUG
		(void)fprintf(stderr, "line %d: {%s}\n", lineno, p);
#endif

		/* Grab file name, "$", "set", or "unset". */
		if ((p = strtok(p, "\n\t ")) == NULL)
			mtree_err("missing field");

		if (p[0] == '/') {
			if (strcmp(p + 1, "set") == 0)
				set(NULL, &ginfo);
			else if (strcmp(p + 1, "unset") == 0)
				unset(NULL, &ginfo);
			else
				mtree_err("invalid specification `%s'", p);
			continue;
		}

		if (strchr(p, '/'))
			mtree_err("slash character in file name");

		if (!strcmp(p, "..")) {
			/* Don't go up, if haven't gone down. */
			if (!root)
				goto noparent;
			if (last->type != F_DIR || last->flags & F_DONE) {
				if (last == root)
					goto noparent;
				last = last->parent;
			}
			last->flags |= F_DONE;
			continue;

noparent:		mtree_err("no parent node");
		}

		if ((centry = calloc(1, sizeof(NODE) + strlen(p))) == NULL)
			mtree_err("%s", strerror(errno));
		*centry = ginfo;
                if (strunvis(centry->name, p) == -1)
			mtree_err("strunvis failed on `%s'", p);
#define	MAGIC	"?*["
		if (strpbrk(p, MAGIC))
			centry->flags |= F_MAGIC;
		set(NULL, centry);

		if (!root) {
			last = root = centry;
			root->parent = root;
		} else if (last->type == F_DIR && !(last->flags & F_DONE)) {
			centry->parent = last;
			last = last->child = centry;
		} else {
			centry->parent = last->parent;
			centry->prev = last;
			last = last->next = centry;
		}
	}
	return (root);
}

static void
set(char *t, NODE *ip)
{
	int type;
	char *kw, *val, *md;
	struct group *gr;
	struct passwd *pw;
	void *m;
	int value;
	char *ep;

	val = NULL;
	for (; (kw = strtok(t, "= \t\n")) != NULL; t = NULL) {
		ip->flags |= type = parsekey(kw, &value);
		if (value && (val = strtok(NULL, " \t\n")) == NULL)
			mtree_err("missing value");
		switch(type) {
		case F_CKSUM:
			ip->cksum = strtoul(val, &ep, 10);
			if (*ep)
				mtree_err("invalid checksum `%s'", val);
			break;
		case F_FLAGS:
			if (strcmp("none", val) == 0)
				ip->st_flags = 0;
			else if (string_to_flags(&val, &ip->st_flags, NULL) != 0)
				mtree_err("invalid flag `%s'", val);
			break;
		case F_GID:
			ip->st_gid = (gid_t)strtoul(val, &ep, 10);
			if (*ep)
				mtree_err("invalid gid `%s'", val);
			break;
		case F_GNAME:
			if ((gr = getgrnam(val)) == NULL)
			    mtree_err("unknown group `%s'", val);
			ip->st_gid = gr->gr_gid;
			break;
		case F_IGN:
			/* just set flag bit */
			break;
		case F_MD5:
			if (val[0]=='0' && val[1]=='x')
				md=&val[2];
			else
				md=val;
			if ((ip->md5sum = strdup(md)) == NULL)
				mtree_err("memory allocation error");
			break;
		case F_MODE:
			if ((m = setmode(val)) == NULL)
				mtree_err("invalid file mode `%s'", val);
			ip->st_mode = getmode(m, 0);
			free(m);
			break;
		case F_NLINK:
			ip->st_nlink = (nlink_t)strtoul(val, &ep, 10);
			if (*ep)
				mtree_err("invalid link count `%s'", val);
			break;
		case F_SIZE:
			ip->st_size = (off_t)strtoq(val, &ep, 10);
			if (*ep)
				mtree_err("invalid size `%s'", val);
			break;
		case F_SLINK:
			if ((ip->slink = strdup(val)) == NULL)
				mtree_err("memory allocation error");
			break;
		case F_TIME:
			ip->st_mtimespec.tv_sec =
			    (time_t)strtoul(val, &ep, 10);
			if (*ep != '.')
				mtree_err("invalid time `%s'", val);
			val = ep + 1;
			ip->st_mtimespec.tv_nsec = strtol(val, &ep, 10);
			if (*ep)
				mtree_err("invalid time `%s'", val);
			break;
		case F_TYPE:
			ip->type = parsetype(val);
			break;
		case F_UID:
			ip->st_uid = (uid_t)strtoul(val, &ep, 10);
			if (*ep)
				mtree_err("invalid uid `%s'", val);
			break;
		case F_UNAME:
			if ((pw = getpwnam(val)) == NULL)
			    mtree_err("unknown user `%s'", val);
			ip->st_uid = pw->pw_uid;
			break;
		}
	}
}

static void
unset(char *t, NODE *ip)
{
	char *p;

	while ((p = strtok(t, "\n\t ")) != NULL)
		ip->flags &= ~parsekey(p, NULL);
}
