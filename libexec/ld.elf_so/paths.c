/*	$NetBSD: paths.c,v 1.9 1999/12/20 02:43:58 christos Exp $	 */

/*
 * Copyright 1996 Matt Thomas <matt@3am-software.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/gmon.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <sys/mount.h>
#include <sys/mbuf.h>
#include <sys/resource.h>
#include <vm/vm_param.h>
#include <machine/cpu.h>

#include "debug.h"
#include "rtld.h"

static Search_Path *_rtld_find_path __P((Search_Path *, const char *, size_t));
static Search_Path **_rtld_append_path __P((Search_Path **, Search_Path **,
    const char *, const char *, bool));
static void _rtld_process_mapping __P((Library_Xform **, char *, char *, bool));

static Search_Path *
_rtld_find_path(path, pathstr, pathlen)
	Search_Path *path;
	const char *pathstr;
	size_t pathlen;
{
	for (; path != NULL; path = path->sp_next) {
		if (pathlen == path->sp_pathlen &&
		    memcmp(path->sp_path, pathstr, pathlen) == 0)
			return path;
	}
	return NULL;
}

static Search_Path **
_rtld_append_path(head_p, path_p, bp, ep, dodebug)
	Search_Path **head_p, **path_p;
	const char *bp;
	const char *ep;
	bool dodebug;
{
	char *cp;
	Search_Path *path;

	if (bp == NULL || bp == ep || *bp == '\0')
		return path_p;

	if (_rtld_find_path(*head_p, bp, ep - bp) != NULL)
		return path_p;

	path = CNEW(Search_Path);
	path->sp_pathlen = ep - bp;
	cp = xmalloc(path->sp_pathlen + 1);
	strncpy(cp, bp, path->sp_pathlen);
	cp[path->sp_pathlen] = '\0';
	path->sp_path = cp;
	path->sp_next = (*path_p);
	(*path_p) = path;
	path_p = &path->sp_next;

	if (dodebug)
		dbg((" added path \"%s\"", path->sp_path));
	return path_p;
}

void
_rtld_add_paths(path_p, pathstr, dodebug)
	Search_Path **path_p;
	const char *pathstr;
	bool dodebug;
{
	Search_Path **head_p = path_p;

	if (pathstr == NULL)
		return;

	if (pathstr[0] == ':') {
		/*
		 * Leading colon means append to current path
		 */
		while ((*path_p) != NULL)
			path_p = &(*path_p)->sp_next;
		pathstr++;
	}

	for (;;) {
		const char *bp = pathstr;
		const char *ep = strchr(bp, ':');
		if (ep == NULL)
			ep = &pathstr[strlen(pathstr)];

		path_p = _rtld_append_path(head_p, path_p, bp, ep, dodebug);

		if (ep[0] == '\0')
			break;
		pathstr = ep + 1;
	}
}


struct sysctldesc {
	const char *name;
	int type;
};

struct list {
	const struct sysctldesc *ctl;
	int numentries;
};

static struct sysctldesc ctl_machdep[] = CTL_MACHDEP_NAMES;
static struct sysctldesc ctl_toplvl[] = CTL_NAMES;

struct list toplevel[] = {
	{ 0, 0 },
	{ ctl_toplvl, CTL_MAXID },
	{ 0, -1 },
};

struct list secondlevel[] = {
	{ 0, 0 },			/* CTL_UNSPEC */
	{ 0, KERN_MAXID },		/* CTL_KERN */
	{ 0, VM_MAXID },		/* CTL_VM */
	{ 0, VFS_MAXID },		/* CTL_VFS */
	{ 0, NET_MAXID },		/* CTL_NET */
	{ 0, CTL_DEBUG_MAXID },		/* CTL_DEBUG */
	{ 0, HW_MAXID },		/* CTL_HW */
#ifdef CTL_MACHDEP_NAMES
	{ ctl_machdep, CPU_MAXID },	/* CTL_MACHDEP */
#else
	{ 0, 0 },			/* CTL_MACHDEP */
#endif
	{ 0, USER_MAXID },		/* CTL_USER_NAMES */
	{ 0, DDBCTL_MAXID },		/* CTL_DDB_NAMES */
	{ 0, 2 },			/* dummy name */
	{ 0, -1 },
};

struct list *lists[] = {
	toplevel,
	secondlevel,
	0
};

#define CTL_MACHDEP_SIZE (sizeof(ctl_machdep) / sizeof(ctl_machdep[0]))

/*
 * Process library mappings of the form:
 *	<library_name>	<machdep_variable> <value,...:library_name,...> ... 
 */
static void
_rtld_process_mapping(lib_p, bp, ep, dodebug)
	Library_Xform **lib_p;
	char *bp, *ep;
	bool dodebug;
{
	static const char WS[] = " \t\n";
	Library_Xform *hwptr = NULL;
	char *ptr, *key, *lib, *l;
	int i, j, k;
	
	if (bp == NULL || bp == ep || *bp == '\0')
		return;

	if (dodebug)
		dbg((" processing mapping \"%s\"", bp));

	if ((ptr = strsep(&bp, WS)) == NULL)
		return;

	if (dodebug)
		dbg((" library \"%s\"", ptr));

	hwptr = xmalloc(sizeof(*hwptr));
	memset(hwptr, 0, sizeof(*hwptr));
	hwptr->name = xstrdup(ptr);

	if ((ptr = strsep(&bp, WS)) == NULL) {
		warnx("missing sysctl variable name");
		goto cleanup;
	}

	if (dodebug)
		dbg((" sysctl \"%s\"", ptr));

	for (i = 0; (l = strsep(&ptr, ".")) != NULL; i++) {

		if (lists[i] == NULL || i >= RTLD_MAX_CTL) {
			warnx("sysctl nesting too deep");
			goto cleanup;
		}

		for (j = 1; lists[i][j].numentries != -1; j++) {

			if (lists[i][j].ctl == NULL)
				continue;

			for (k = 1; k < lists[i][j].numentries; k++) {
				if (strcmp(lists[i][j].ctl[k].name, l) == 0)
					break;
			}

			if (lists[i][j].numentries == -1) {
				warnx("unknown sysctl variable name `%s'", l);
				goto cleanup;
			}

			hwptr->ctl[hwptr->ctlmax] = k;
			hwptr->ctltype[hwptr->ctlmax++] =
			    lists[i][j].ctl[k].type;
		}
	}

	if (dodebug)
		for (i = 0; i < hwptr->ctlmax; i++)
			dbg((" sysctl %d, %d", hwptr->ctl[i],
			    hwptr->ctltype[i]));

	for (i = 0; (ptr = strsep(&bp, WS)) != NULL; i++) {
		if (i == RTLD_MAX_ENTRY) {
no_more:
			warnx("maximum library entries exceeded `%s'",
			    hwptr->name);
			goto cleanup;
		}
		if ((key = strsep(&ptr, ":")) == NULL) {
			warnx("missing sysctl variable value for `%s'",
			    hwptr->name);
			goto cleanup;
		}
		if ((lib = strsep(&ptr, ":")) == NULL) {
			warnx("missing sysctl library list for `%s'",
			    hwptr->name);
			goto cleanup;
		}
		for (j = 0; (l = strsep(&lib, ",")) != NULL; j++) {
			if (j == RTLD_MAX_LIBRARY) {
				warnx("maximum library entries exceeded `%s'",
				    hwptr->name);
				goto cleanup;
			}
			if (dodebug)
				dbg((" library \"%s\"", l));
			hwptr->entry[i].library[j] = xstrdup(l);
		}
		if (j == 0) {
			warnx("No library map entries for `%s/%s'",
				hwptr->name, ptr);
			goto cleanup;
		}
		j = i;
		for (; (l = strsep(&key, ",")) != NULL; i++) {
			if (dodebug)
				dbg((" key \"%s\"", l));
			if (i == RTLD_MAX_ENTRY)
				goto no_more;
			if (i != j)
				(void)memcpy(hwptr->entry[i].library, 
				    hwptr->entry[j].library,
				    sizeof(hwptr->entry[j].library));
			hwptr->entry[i].value = xstrdup(l);
		}

		if (j != i)
			i--;
	}

	if (i == 0) {
		warnx("No library entries for `%s'", hwptr->name);
		goto cleanup;
	}


	hwptr->next = NULL;
	if (*lib_p != NULL)
		(*lib_p)->next = hwptr;
	*lib_p = hwptr;

	return;

cleanup:
	if (hwptr->name)
		free(hwptr->name);
	free(hwptr);
}

void
_rtld_process_hints(path_p, lib_p, fname, dodebug)
	Search_Path **path_p;
	Library_Xform **lib_p;
	const char *fname;
	bool dodebug;
{
	int fd;
	char *p, *buf, *b, *ebuf;
	struct stat st;
	size_t sz;
	Search_Path **head_p = path_p;
	int doing_path = 0;

	if ((fd = open(fname, O_RDONLY)) == -1) {
		/* Don't complain */
		return;
	}

	if (fstat(fd, &st) == -1) {
		/* Complain */
		xwarn("fstat: %s", fname);
		return;
	}

	sz = (size_t) st.st_size;

	buf = mmap(0, sz, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FILE, fd, 0);
	if (buf == MAP_FAILED) {
		xwarn("fstat: %s", fname);
		(void)close(fd);
		return;
	}
	(void)close(fd);

	while ((*path_p) != NULL)
		path_p = &(*path_p)->sp_next;

	for (b = NULL, p = buf, ebuf = buf + sz; p < ebuf; p++) {

		if ((p == buf || p[-1] == '\0') && b == NULL)
			b = p;

		switch (*p) {
		case '/':
			if (b == p)
				doing_path = 1;
			break;

		case ' ': case '\t':
			if (b == p)
				b++;
			break;

		case '\n':
			*p = '\0';
			if (doing_path)
				path_p = _rtld_append_path(head_p, path_p, b, p,
				    dodebug);
			else
				_rtld_process_mapping(lib_p, b, p, dodebug);
			b = NULL;
			break;

		case '#':
			if (b != p) {
				char *sp;
				for  (sp = p - 1; *sp == ' ' ||
				    *sp == '\t'; --sp)
					continue;
				*++sp = '\0';
				if (doing_path)
					path_p = _rtld_append_path(head_p,
					    path_p, b, sp, dodebug);
				else
					_rtld_process_mapping(lib_p, b, sp,
					    dodebug);
				*sp = ' ';
			}
			b = NULL;
			break;

		default:
			if (b == p)
				doing_path = 0;
			break;
		}
	}

	if (doing_path)
		path_p = _rtld_append_path(head_p, path_p, b, ebuf, dodebug);
	else
		_rtld_process_mapping(lib_p, b, ebuf, dodebug);

	(void)munmap(buf, sz);
}
