/*	$NetBSD: spec.c,v 1.63 2006/10/08 18:09:47 martin Exp $	*/

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

/*-
 * Copyright (c) 2001-2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn of Wasabi Systems.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID) && !defined(lint)
#if 0
static char sccsid[] = "@(#)spec.c	8.2 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: spec.c,v 1.63 2006/10/08 18:09:47 martin Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>

#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vis.h>
#include <util.h>

#include "extern.h"
#include "pack_dev.h"

size_t	mtree_lineno;			/* Current spec line number */
int	mtree_Mflag;			/* Merge duplicate entries */
int	mtree_Wflag;			/* Don't "whack" permissions */

static	dev_t	parsedev(char *);
static	void	replacenode(NODE *, NODE *);
static	void	set(char *, NODE *);
static	void	unset(char *, NODE *);

#define REPLACEPTR(x,v)	do { if ((x)) free((x)); (x) = (v); } while (0)

NODE *
spec(FILE *fp)
{
	NODE *centry, *last, *pathparent, *cur;
	char *p, *e, *next;
	NODE ginfo, *root;
	char *buf, *tname, *ntname;
	size_t tnamelen, plen;

	root = NULL;
	centry = last = NULL;
	tname = NULL;
	tnamelen = 0;
	memset(&ginfo, 0, sizeof(ginfo));
	for (mtree_lineno = 0;
	    (buf = fparseln(fp, NULL, &mtree_lineno, NULL,
		FPARSELN_UNESCCOMM));
	    free(buf)) {
		/* Skip leading whitespace. */
		for (p = buf; *p && isspace((unsigned char)*p); ++p)
			continue;

		/* If nothing but whitespace, continue. */
		if (!*p)
			continue;

#ifdef DEBUG
		fprintf(stderr, "line %lu: {%s}\n",
		    (u_long)mtree_lineno, p);
#endif
		/* Grab file name, "$", "set", or "unset". */
		next = buf;
		while ((p = strsep(&next, " \t")) != NULL && *p == '\0')
			continue;
		if (p == NULL)
			mtree_err("missing field");

		if (p[0] == '/') {
			if (strcmp(p + 1, "set") == 0)
				set(next, &ginfo);
			else if (strcmp(p + 1, "unset") == 0)
				unset(next, &ginfo);
			else
				mtree_err("invalid specification `%s'", p);
			continue;
		}

		if (strcmp(p, "..") == 0) {
			/* Don't go up, if haven't gone down. */
			if (root == NULL)
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

		plen = strlen(p) + 1;
		if (plen > tnamelen) {
			if ((ntname = realloc(tname, plen)) == NULL)
				mtree_err("realloc: %s", strerror(errno));
			tname = ntname;
			tnamelen = plen;
		}
		if (strunvis(tname, p) == -1)
			mtree_err("strunvis failed on `%s'", p);
		p = tname;

		pathparent = NULL;
		if (strchr(p, '/') != NULL) {
			cur = root;
			for (; (e = strchr(p, '/')) != NULL; p = e+1) {
				if (p == e)
					continue;	/* handle // */
				*e = '\0';
				if (strcmp(p, ".") != 0) {
					while (cur &&
					    strcmp(cur->name, p) != 0) {
						cur = cur->next;
					}
				}
				if (cur == NULL || cur->type != F_DIR) {
					mtree_err("%s: %s", tname,
					    strerror(ENOENT));
				}
				*e = '/';
				pathparent = cur;
				cur = cur->child;
			}
			if (*p == '\0')
				mtree_err("%s: empty leaf element", tname);
		}

		if ((centry = calloc(1, sizeof(NODE) + strlen(p))) == NULL)
			mtree_err("%s", strerror(errno));
		*centry = ginfo;
		centry->lineno = mtree_lineno;
		strcpy(centry->name, p);
#define	MAGIC	"?*["
		if (strpbrk(p, MAGIC))
			centry->flags |= F_MAGIC;
		set(next, centry);

		if (root == NULL) {
				/*
				 * empty tree
				 */
			if (strcmp(centry->name, ".") != 0 ||
			    centry->type != F_DIR)
				mtree_err(
				    "root node must be the directory `.'");
			last = root = centry;
			root->parent = root;
		} else if (pathparent != NULL) {
				/*
				 * full path entry
				 */
			centry->parent = pathparent;
			cur = pathparent->child;
			if (cur == NULL) {
				pathparent->child = centry;
				last = centry;
			} else {
				for (; cur != NULL; cur = cur->next) {
					if (strcmp(cur->name, centry->name)
					    == 0) {
						/* existing entry; replace */
						replacenode(cur, centry);
						break;
					}
					if (cur->next == NULL) {
						/* last entry; add new */
						cur->next = centry;
						centry->prev = cur;
						break;
					}
				}
				last = cur;
				while (last->next != NULL)
					last = last->next;
			}
		} else if (strcmp(centry->name, ".") == 0) {
				/*
				 * duplicate "." entry; always replace
				 */
			replacenode(root, centry);
		} else if (last->type == F_DIR && !(last->flags & F_DONE)) {
				/*
				 * new relative child
				 * (no duplicate check)
				 */
			centry->parent = last;
			last = last->child = centry;
		} else {
				/*
				 * relative entry, up one directory
				 * (no duplicate check)
				 */
			centry->parent = last->parent;
			centry->prev = last;
			last = last->next = centry;
		}
	}
	return (root);
}

void
free_nodes(NODE *root)
{
	NODE	*cur, *next;

	if (root == NULL)
		return;

	next = NULL;
	for (cur = root; cur != NULL; cur = next) {
		next = cur->next;
		free_nodes(cur->child);
		REPLACEPTR(cur->slink, NULL);
		REPLACEPTR(cur->md5digest, NULL);
		REPLACEPTR(cur->rmd160digest, NULL);
		REPLACEPTR(cur->sha1digest, NULL);
		REPLACEPTR(cur->sha256digest, NULL);
		REPLACEPTR(cur->sha384digest, NULL);
		REPLACEPTR(cur->sha512digest, NULL);
		REPLACEPTR(cur->tags, NULL);
		REPLACEPTR(cur, NULL);
	}
}

/*
 * dump_nodes --
 *	dump the NODEs from `cur', based in the directory `dir'.
 *	if pathlast is none zero, print the path last, otherwise print
 *	it first.
 */
void
dump_nodes(const char *dir, NODE *root, int pathlast)
{
	NODE	*cur;
	char	path[MAXPATHLEN];
	const char *name;

	for (cur = root; cur != NULL; cur = cur->next) {
		if (cur->type != F_DIR && !matchtags(cur))
			continue;

		if (snprintf(path, sizeof(path), "%s%s%s",
		    dir, *dir ? "/" : "", cur->name)
		    >= sizeof(path))
			mtree_err("Pathname too long.");

		if (!pathlast)
			printf("%s ", vispath(path));

#define MATCHFLAG(f)	((keys & (f)) && (cur->flags & (f)))
		if (MATCHFLAG(F_TYPE))
			printf("type=%s ", nodetype(cur->type));
		if (MATCHFLAG(F_UID | F_UNAME)) {
			if (keys & F_UNAME &&
			    (name = user_from_uid(cur->st_uid, 1)) != NULL)
				printf("uname=%s ", name);
			else
				printf("uid=%u ", cur->st_uid);
		}
		if (MATCHFLAG(F_GID | F_GNAME)) {
			if (keys & F_GNAME &&
			    (name = group_from_gid(cur->st_gid, 1)) != NULL)
				printf("gname=%s ", name);
			else
				printf("gid=%u ", cur->st_gid);
		}
		if (MATCHFLAG(F_MODE))
			printf("mode=%#o ", cur->st_mode);
		if (MATCHFLAG(F_DEV) &&
		    (cur->type == F_BLOCK || cur->type == F_CHAR))
			printf("device=%#x ", cur->st_rdev);
		if (MATCHFLAG(F_NLINK))
			printf("nlink=%d ", cur->st_nlink);
		if (MATCHFLAG(F_SLINK))
			printf("link=%s ", vispath(cur->slink));
		if (MATCHFLAG(F_SIZE))
			printf("size=%lld ", (long long)cur->st_size);
		if (MATCHFLAG(F_TIME))
			printf("time=%ld.%ld ", (long)cur->st_mtimespec.tv_sec,
			    cur->st_mtimespec.tv_nsec);
		if (MATCHFLAG(F_CKSUM))
			printf("cksum=%lu ", cur->cksum);
		if (MATCHFLAG(F_MD5))
			printf("md5=%s ", cur->md5digest);
		if (MATCHFLAG(F_RMD160))
			printf("rmd160=%s ", cur->rmd160digest);
		if (MATCHFLAG(F_SHA1))
			printf("sha1=%s ", cur->sha1digest);
		if (MATCHFLAG(F_SHA256))
			printf("sha256=%s ", cur->sha256digest);
		if (MATCHFLAG(F_SHA384))
			printf("sha384=%s ", cur->sha384digest);
		if (MATCHFLAG(F_SHA512))
			printf("sha512=%s ", cur->sha512digest);
		if (MATCHFLAG(F_FLAGS))
			printf("flags=%s ",
			    flags_to_string(cur->st_flags, "none"));
		if (MATCHFLAG(F_IGN))
			printf("ignore ");
		if (MATCHFLAG(F_OPT))
			printf("optional ");
		if (MATCHFLAG(F_TAGS))
			printf("tags=%s ", cur->tags);
		puts(pathlast ? vispath(path) : "");

		if (cur->child)
			dump_nodes(path, cur->child, pathlast);
	}
}

/*
 * vispath --
 *	strsvis(3) encodes path, which must not be longer than MAXPATHLEN
 *	characters long, and returns a pointer to a static buffer containing
 *	the result.
 */
char *
vispath(const char *path)
{
	const char extra[] = { ' ', '\t', '\n', '\\', '#', '\0' };
	static char pathbuf[4*MAXPATHLEN + 1];

	strsvis(pathbuf, path, VIS_CSTYLE, extra);
	return(pathbuf);
}


static dev_t
parsedev(char *arg)
{
#define MAX_PACK_ARGS	3
	u_long	numbers[MAX_PACK_ARGS];
	char	*p, *ep, *dev;
	int	argc;
	pack_t	*pack;
	dev_t	result;
	const char *error = NULL;

	if ((dev = strchr(arg, ',')) != NULL) {
		*dev++='\0';
		if ((pack = pack_find(arg)) == NULL)
			mtree_err("unknown format `%s'", arg);
		argc = 0;
		while ((p = strsep(&dev, ",")) != NULL) {
			if (*p == '\0')
				mtree_err("missing number");
			numbers[argc++] = strtoul(p, &ep, 0);
			if (*ep != '\0')
				mtree_err("invalid number `%s'",
				    p);
			if (argc > MAX_PACK_ARGS)
				mtree_err("too many arguments");
		}
		if (argc < 2)
			mtree_err("not enough arguments");
		result = (*pack)(argc, numbers, &error);
		if (error != NULL)
			mtree_err(error);
	} else {
		result = (dev_t)strtoul(arg, &ep, 0);
		if (*ep != '\0')
			mtree_err("invalid device `%s'", arg);
	}
	return (result);
}

static void
replacenode(NODE *cur, NODE *new)
{

#define REPLACE(x)	cur->x = new->x
#define REPLACESTR(x)	REPLACEPTR(cur->x,new->x)

	if (cur->type != new->type) {
		if (mtree_Mflag) {
				/*
				 * merge entries with different types; we
				 * don't want children retained in this case.
				 */
			REPLACE(type);
			free_nodes(cur->child);
			cur->child = NULL;
		} else {
			mtree_err(
			    "existing entry for `%s', type `%s'"
			    " does not match type `%s'",
			    cur->name, nodetype(cur->type),
			    nodetype(new->type));
		}
	}

	REPLACE(st_size);
	REPLACE(st_mtimespec);
	REPLACESTR(slink);
	if (cur->slink != NULL) {
		if ((cur->slink = strdup(new->slink)) == NULL)
			mtree_err("memory allocation error");
		if (strunvis(cur->slink, new->slink) == -1)
			mtree_err("strunvis failed on `%s'", new->slink);
		free(new->slink);
	}
	REPLACE(st_uid);
	REPLACE(st_gid);
	REPLACE(st_mode);
	REPLACE(st_rdev);
	REPLACE(st_flags);
	REPLACE(st_nlink);
	REPLACE(cksum);
	REPLACESTR(md5digest);
	REPLACESTR(rmd160digest);
	REPLACESTR(sha1digest);
	REPLACESTR(sha256digest);
	REPLACESTR(sha384digest);
	REPLACESTR(sha512digest);
	REPLACESTR(tags);
	REPLACE(lineno);
	REPLACE(flags);
	free(new);
}

static void
set(char *t, NODE *ip)
{
	int	type, value, len;
	gid_t	gid;
	uid_t	uid;
	char	*kw, *val, *md, *ep;
	void	*m;

	while ((kw = strsep(&t, "= \t")) != NULL) {
		if (*kw == '\0')
			continue;
		if (strcmp(kw, "all") == 0)
			mtree_err("invalid keyword `all'");
		ip->flags |= type = parsekey(kw, &value);
		if (!value)
			/* Just set flag bit (F_IGN and F_OPT) */
			continue;
		while ((val = strsep(&t, " \t")) != NULL && *val == '\0')
			continue;
		if (val == NULL)
			mtree_err("missing value");
		switch (type) {
		case F_CKSUM:
			ip->cksum = strtoul(val, &ep, 10);
			if (*ep)
				mtree_err("invalid checksum `%s'", val);
			break;
		case F_DEV:
			ip->st_rdev = parsedev(val);
			break;
		case F_FLAGS:
			if (strcmp("none", val) == 0)
				ip->st_flags = 0;
			else if (string_to_flags(&val, &ip->st_flags, NULL)
			    != 0)
				mtree_err("invalid flag `%s'", val);
			break;
		case F_GID:
			ip->st_gid = (gid_t)strtoul(val, &ep, 10);
			if (*ep)
				mtree_err("invalid gid `%s'", val);
			break;
		case F_GNAME:
			if (mtree_Wflag)	/* don't parse if whacking */
				break;
			if (gid_from_group(val, &gid) == -1)
				mtree_err("unknown group `%s'", val);
			ip->st_gid = gid;
			break;
		case F_MD5:
			if (val[0]=='0' && val[1]=='x')
				md=&val[2];
			else
				md=val;
			if ((ip->md5digest = strdup(md)) == NULL)
				mtree_err("memory allocation error");
			break;
		case F_MODE:
			if ((m = setmode(val)) == NULL)
				mtree_err("cannot set file mode `%s' (%s)",
				    val, strerror(errno));
			ip->st_mode = getmode(m, 0);
			free(m);
			break;
		case F_NLINK:
			ip->st_nlink = (nlink_t)strtoul(val, &ep, 10);
			if (*ep)
				mtree_err("invalid link count `%s'", val);
			break;
		case F_RMD160:
			if (val[0]=='0' && val[1]=='x')
				md=&val[2];
			else
				md=val;
			if ((ip->rmd160digest = strdup(md)) == NULL)
				mtree_err("memory allocation error");
			break;
		case F_SHA1:
			if (val[0]=='0' && val[1]=='x')
				md=&val[2];
			else
				md=val;
			if ((ip->sha1digest = strdup(md)) == NULL)
				mtree_err("memory allocation error");
			break;
		case F_SIZE:
			ip->st_size = (off_t)strtoll(val, &ep, 10);
			if (*ep)
				mtree_err("invalid size `%s'", val);
			break;
		case F_SLINK:
			if ((ip->slink = strdup(val)) == NULL)
				mtree_err("memory allocation error");
			if (strunvis(ip->slink, val) == -1)
				mtree_err("strunvis failed on `%s'", val);
			break;
		case F_TAGS:
			len = strlen(val) + 3;	/* "," + str + ",\0" */
			if ((ip->tags = malloc(len)) == NULL)
				mtree_err("memory allocation error");
			snprintf(ip->tags, len, ",%s,", val);
			break;
		case F_TIME:
			ip->st_mtimespec.tv_sec =
			    (time_t)strtoul(val, &ep, 10);
			if (*ep != '.')
				mtree_err("invalid time `%s'", val);
			val = ep + 1;
			ip->st_mtimespec.tv_nsec = strtoul(val, &ep, 10);
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
			if (mtree_Wflag)	/* don't parse if whacking */
				break;
			if (uid_from_user(val, &uid) == -1)
				mtree_err("unknown user `%s'", val);
			ip->st_uid = uid;
			break;
		case F_SHA256:
			if (val[0]=='0' && val[1]=='x')
				md=&val[2];
			else
				md=val;
			if ((ip->sha256digest = strdup(md)) == NULL)
				mtree_err("memory allocation error");
			break;
		case F_SHA384:
			if (val[0]=='0' && val[1]=='x')
				md=&val[2];
			else
				md=val;
			if ((ip->sha384digest = strdup(md)) == NULL)
				mtree_err("memory allocation error");
			break;
		case F_SHA512:
			if (val[0]=='0' && val[1]=='x')
				md=&val[2];
			else
				md=val;
			if ((ip->sha512digest = strdup(md)) == NULL)
				mtree_err("memory allocation error");
			break;
		default:
			mtree_err(
			    "set(): unsupported key type 0x%x (INTERNAL ERROR)",
			    type);
			/* NOTREACHED */
		}
	}
}

static void
unset(char *t, NODE *ip)
{
	char *p;

	while ((p = strsep(&t, " \t")) != NULL) {
		if (*p == '\0')
			continue;
		ip->flags &= ~parsekey(p, NULL);
	}
}
