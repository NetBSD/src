/*	$NetBSD: t_glob.c,v 1.10 2020/03/13 23:27:54 rillig Exp $	*/
/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_glob.c,v 1.10 2020/03/13 23:27:54 rillig Exp $");

#include <atf-c.h>

#include <sys/param.h>
#include <sys/stat.h>

#include <dirent.h>
#include <glob.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "h_macros.h"


#ifdef DEBUG
#define DPRINTF(a) printf a
#else
#define DPRINTF(a)
#endif

struct vfs_file {
	char type;		/* 'd' or '-', like in ls(1) */
	const char *name;
};

static struct vfs_file a[] = {
	{ '-', "1" },
	{ 'd', "b" },
	{ '-', "3" },
	{ '-', "4" },
};

static struct vfs_file b[] = {
	{ '-', "x" },
	{ '-', "y" },
	{ '-', "z" },
	{ '-', "w" },
};

static struct vfs_file hidden_dir[] = {
	{ '-', "visible-file" },
	{ '-', ".hidden-file" },
};

static struct vfs_file dot[] = {
	{ 'd', "a" },
	{ 'd', ".hidden-dir" },
};

struct vfs_dir {
	const char *name;	/* full directory name */
	const struct vfs_file *entries;
	size_t entries_len;
	size_t pos;		/* only between opendir/closedir */
};

#define VFS_DIR_INIT(name, entries) \
    { name, entries, __arraycount(entries), 0 }

static struct vfs_dir d[] = {
	VFS_DIR_INIT("a", a),
	VFS_DIR_INIT("a/b", b),
	VFS_DIR_INIT(".", dot),
	VFS_DIR_INIT(".hidden-dir", hidden_dir),
};

static void
trim(char *buf, size_t len, const char *name)
{
	char *path = buf, *epath = buf + len;
	while (path < epath && (*path++ = *name++) != '\0')
		continue;
	path--;
	while (path > buf && *--path == '/')
		*path = '\0';
}

static void *
vfs_opendir(const char *dir)
{
	size_t i;
	char buf[MAXPATHLEN];
	trim(buf, sizeof(buf), dir);

	for (i = 0; i < __arraycount(d); i++)
		if (strcmp(buf, d[i].name) == 0) {
			DPRINTF(("opendir %s %p\n", buf, &d[i]));
			return &d[i];
		}
	DPRINTF(("opendir %s ENOENT\n", buf));
	errno = ENOENT;
	return NULL;
}

static struct dirent *
vfs_readdir(void *v)
{
	static struct dirent dir;
	struct vfs_dir *dd = v;
	if (dd->pos < dd->entries_len) {
		const struct vfs_file *f = &dd->entries[dd->pos++];
		strcpy(dir.d_name, f->name);
		dir.d_namlen = strlen(f->name);
		dir.d_ino = dd->pos;
		dir.d_type = f->type == 'd' ? DT_DIR : DT_REG;
		DPRINTF(("readdir %s %d\n", dir.d_name, dir.d_type));
		dir.d_reclen = _DIRENT_RECLEN(&dir, dir.d_namlen);
		return &dir;
	}
	return NULL;
}

static int
vfs_stat(const char *name , __gl_stat_t *st)
{
	char buf[MAXPATHLEN];
	trim(buf, sizeof(buf), name);
	memset(st, 0, sizeof(*st));

	for (size_t i = 0; i < __arraycount(d); i++)
		if (strcmp(buf, d[i].name) == 0) {
			st->st_mode = S_IFDIR | 0755;
			goto out;
		}

	for (size_t i = 0; i < __arraycount(d); i++) {
		size_t dir_len = strlen(d[i].name);
		if (strncmp(buf, d[i].name, dir_len) != 0)
			continue;
		if (buf[dir_len] != '/')
			continue;
		const char *base = buf + dir_len + 1;

		for (size_t j = 0; j < d[i].entries_len; j++) {
			const struct vfs_file *f = &d[i].entries[j];
			if (strcmp(f->name, base) != 0)
				continue;
			ATF_CHECK(f->type != 'd'); // handled above
			st->st_mode = S_IFREG | 0644;
			goto out;
		}
	}
	DPRINTF(("stat %s ENOENT\n", buf));
	errno = ENOENT;
	return -1;
out:
	DPRINTF(("stat %s %06o\n", buf, st->st_mode));
	return 0;
}

static int
vfs_lstat(const char *name , __gl_stat_t *st)
{
	return vfs_stat(name, st);
}

static void
vfs_closedir(void *v)
{
	struct vfs_dir *dd = v;
	dd->pos = 0;
	DPRINTF(("closedir %p\n", dd));
}

static void
run(const char *p, int flags, /* const char *res */ ...)
{
	glob_t gl;
	size_t i;
	int e;

	DPRINTF(("pattern %s\n", p));
	memset(&gl, 0, sizeof(gl));
	gl.gl_opendir = vfs_opendir;
	gl.gl_readdir = vfs_readdir;
	gl.gl_closedir = vfs_closedir;
	gl.gl_stat = vfs_stat;
	gl.gl_lstat = vfs_lstat;

	switch ((e = glob(p, GLOB_ALTDIRFUNC | flags, NULL, &gl))) {
	case 0:
		break;
	case GLOB_NOSPACE:
		fprintf(stderr, "Malloc call failed.\n");
		goto bad;
	case GLOB_ABORTED:
		fprintf(stderr, "Unignored error.\n");
		goto bad;
	case GLOB_NOMATCH:
		fprintf(stderr, "No match, and GLOB_NOCHECK was not set.\n");
		goto bad;
	case GLOB_NOSYS:
		fprintf(stderr, "Implementation does not support function.\n");
		goto bad;
	default:
		fprintf(stderr, "Unknown error %d.\n", e);
		goto bad;
	}

	for (i = 0; i < gl.gl_pathc; i++)
		DPRINTF(("glob result %zu: %s\n", i, gl.gl_pathv[i]));

	va_list res;
	va_start(res, flags);
	i = 0;
	const char *name;
	while ((name = va_arg(res, const char *)) != NULL && i < gl.gl_pathc) {
		ATF_CHECK_STREQ(gl.gl_pathv[i], name);
		i++;
	}
	va_end(res);
	ATF_CHECK_EQ_MSG(i, gl.gl_pathc,
	    "expected %zu results, got %zu", i, gl.gl_pathc);
	ATF_CHECK_EQ_MSG(name, NULL,
	    "\"%s\" should have been found, but wasn't", name);

	globfree(&gl);
	return;
bad:
	ATF_REQUIRE_MSG(e == 0, "No match for `%s'", p);
}

#define run(p, flags, ...) (run)(p, flags, __VA_ARGS__, (const char *) 0)

ATF_TC(glob_range);
ATF_TC_HEAD(glob_range, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test glob(3) range");
}

ATF_TC_BODY(glob_range, tc)
{
	run("a/b/[x-z]", 0,
	    "a/b/x", "a/b/y", "a/b/z");
}

ATF_TC(glob_range_not);
ATF_TC_HEAD(glob_range_not, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test glob(3) ! range");
}

ATF_TC_BODY(glob_range_not, tc)
{
	run("a/b/[!x-z]", 0,
	    "a/b/w");
}

ATF_TC(glob_star);
ATF_TC_HEAD(glob_star, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test glob(3) ** with GLOB_STAR");
}

ATF_TC_BODY(glob_star, tc)
{
	run("a/**", GLOB_STAR,
	    "a/1", "a/3", "a/4", "a/b", "a/b/w", "a/b/x", "a/b/y", "a/b/z");
}

ATF_TC(glob_star_not);
ATF_TC_HEAD(glob_star_not, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test glob(3) ** without GLOB_STAR");
}

ATF_TC_BODY(glob_star_not, tc)
{
	run("a/**", 0,
	    "a/1", "a/3", "a/4", "a/b");
}

ATF_TC(glob_star_star);
ATF_TC_HEAD(glob_star_star, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test glob(3) with star-star");
}

ATF_TC_BODY(glob_star_star, tc)
{
	run("**", GLOB_STAR,
	    "a",
	    "a/1", "a/3", "a/4", "a/b",
	    "a/b/w", "a/b/x", "a/b/y", "a/b/z");
}

ATF_TC(glob_hidden);
ATF_TC_HEAD(glob_hidden, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test glob(3) with hidden directory");
}

ATF_TC_BODY(glob_hidden, tc)
{
	run(".**", GLOB_STAR,
	    ".hidden-dir",
	    ".hidden-dir/visible-file");
}

#if 0
ATF_TC(glob_nocheck);
ATF_TC_HEAD(glob_nocheck, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test glob(3) pattern with backslash and GLOB_NOCHECK");
}


ATF_TC_BODY(glob_nocheck, tc)
{
	static const char pattern[] = { 'f', 'o', 'o', '\\', ';', 'b', 'a',
	    'r', '\0' };
	static const char *glob_nocheck[] = {
	    pattern
	};
	run(pattern, GLOB_NOCHECK, glob_nocheck, __arraycount(glob_nocheck));
}
#endif

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, glob_star);
	ATF_TP_ADD_TC(tp, glob_star_not);
	ATF_TP_ADD_TC(tp, glob_range);
	ATF_TP_ADD_TC(tp, glob_range_not);
	ATF_TP_ADD_TC(tp, glob_star_star);
	ATF_TP_ADD_TC(tp, glob_hidden);
/*
 * Remove this test for now - the GLOB_NOCHECK return value has been
 * re-defined to return a modified pattern in revision 1.33 of glob.c
 *
 *	ATF_TP_ADD_TC(tp, glob_nocheck);
 */

	return atf_no_error();
}
