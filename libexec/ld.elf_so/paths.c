/*	$NetBSD: paths.c,v 1.27 2003/07/24 10:12:26 skrll Exp $	 */

/*
 * Copyright 1996 Matt Thomas <matt@3am-software.com>
 * Copyright 2002 Charles M. Hannum <root@ihack.net>
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
#include <sys/socket.h>
#include <sys/mount.h>
#include <sys/mbuf.h>
#include <sys/resource.h>
#include <machine/cpu.h>

#include "debug.h"
#include "rtld.h"

static Search_Path *_rtld_find_path(Search_Path *, const char *, size_t);
static Search_Path **_rtld_append_path(Search_Path **, Search_Path **,
    const char *, size_t);
static void _rtld_process_mapping(Library_Xform **, char *, size_t);

static Search_Path *
_rtld_find_path(Search_Path *path, const char *pathstr, size_t pathlen)
{

	for (; path != NULL; path = path->sp_next) {
		if (pathlen == path->sp_pathlen &&
		    memcmp(path->sp_path, pathstr, pathlen) == 0)
			return path;
	}
	return NULL;
}

static Search_Path **
_rtld_append_path(Search_Path **head_p, Search_Path **path_p,
    const char *bp, size_t len)
{
	char *cp;
	Search_Path *path;

	if (_rtld_find_path(*head_p, bp, len) != NULL)
		return path_p;

	path = NEW(Search_Path);
	path->sp_pathlen = len;
	cp = xmalloc(len + 1);
	memcpy(cp, bp, len);
	cp[len] = '\0';
	path->sp_path = cp;
	path->sp_next = (*path_p);
	(*path_p) = path;
	path_p = &path->sp_next;

	dbg((" added path \"%s\"", path->sp_path));
	return path_p;
}

void
_rtld_add_paths(Search_Path **path_p, const char *pathstr)
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

		path_p = _rtld_append_path(head_p, path_p, bp, ep - bp);

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

#ifdef CTL_MACHDEP_NAMES
static const struct sysctldesc ctl_machdep[] = CTL_MACHDEP_NAMES;
#endif
static const struct sysctldesc ctl_toplvl[] = CTL_NAMES;

const struct list toplevel[] = {
	{ 0, 0 },
	{ ctl_toplvl, CTL_MAXID },
	{ 0, -1 },
};

const struct list secondlevel[] = {
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

const struct list *lists[] = {
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
_rtld_process_mapping(Library_Xform **lib_p, char *bp, size_t len)
{
	static const char WS[] = " \t\n";
	Library_Xform *hwptr = NULL;
	char *ptr, *key, *lib, *l;
	int i, j, k;
	
	dbg((" processing mapping \"%s\"", bp));

	if ((ptr = strsep(&bp, WS)) == NULL)
		return;

	dbg((" library \"%s\"", ptr));

	hwptr = xmalloc(sizeof(*hwptr));
	memset(hwptr, 0, sizeof(*hwptr));
	hwptr->name = xstrdup(ptr);

	while ((ptr = strsep(&bp, WS)) != NULL)
		if (*ptr != '\0')
			break;
	if (ptr == NULL) {
		xwarnx("missing sysctl variable name");
		goto cleanup;
	}

	dbg((" sysctl \"%s\"", ptr));

	for (i = 0; (l = strsep(&ptr, ".")) != NULL; i++) {

		if (lists[i] == NULL || i >= RTLD_MAX_CTL) {
			xwarnx("sysctl nesting too deep");
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
				xwarnx("unknown sysctl variable name `%s'", l);
				goto cleanup;
			}

			hwptr->ctl[hwptr->ctlmax] = k;
			hwptr->ctltype[hwptr->ctlmax++] =
			    lists[i][j].ctl[k].type;
		}
	}

	for (i = 0; i < hwptr->ctlmax; i++)
		dbg((" sysctl %d, %d", hwptr->ctl[i], hwptr->ctltype[i]));

	for (i = 0; (ptr = strsep(&bp, WS)) != NULL; i++) {
		if (*ptr == '\0') {
			/* back up index and continue */
			i--;
			continue;
		}
		if (i == RTLD_MAX_ENTRY) {
no_more:
			xwarnx("maximum library entries exceeded `%s'",
			    hwptr->name);
			goto cleanup;
		}
		if ((key = strsep(&ptr, ":")) == NULL) {
			xwarnx("missing sysctl variable value for `%s'",
			    hwptr->name);
			goto cleanup;
		}
		if ((lib = strsep(&ptr, ":")) == NULL) {
			xwarnx("missing sysctl library list for `%s'",
			    hwptr->name);
			goto cleanup;
		}
		for (j = 0; (l = strsep(&lib, ",")) != NULL; j++) {
			if (j == RTLD_MAX_LIBRARY) {
				xwarnx("maximum library entries exceeded `%s'",
				    hwptr->name);
				goto cleanup;
			}
			dbg((" library \"%s\"", l));
			hwptr->entry[i].library[j] = xstrdup(l);
		}
		if (j == 0) {
			xwarnx("No library map entries for `%s/%s'",
				hwptr->name, ptr);
			goto cleanup;
		}
		j = i;
		for (; (l = strsep(&key, ",")) != NULL; i++) {
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
		xwarnx("No library entries for `%s'", hwptr->name);
		goto cleanup;
	}

	hwptr->next = *lib_p;
	*lib_p = hwptr;

	return;

cleanup:
	if (hwptr->name)
		free(hwptr->name);
	free(hwptr);
}

void
_rtld_process_hints(Search_Path **path_p, Library_Xform **lib_p, const char *fname)
{
	int fd;
	char *buf, *b, *ebuf;
	struct stat st;
	size_t sz, len;
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

	for (b = buf, ebuf = buf + sz; b < ebuf; ) {
		b += strspn(b, " \t\n");
		if (*b == '\0')
			break;

		len = strcspn(b, "\n#");
		if (len == 0) {
			b += strcspn(b, "\n");			
			continue;
		}

		doing_path = *b == '/';
		if (doing_path) {
			size_t tmp = len - 1;
			while (b[tmp] == '#' || b[tmp] == ' ' || b[tmp] == '\t')
				tmp--;
			path_p = _rtld_append_path(head_p, path_p, b, tmp + 1);
		} else {
			char tmp = b[len];
			b[len] = '\0';
			_rtld_process_mapping(lib_p, b, len);
			b[len] = tmp;
		}
			
		b += len;
	}

	(void)munmap(buf, sz);
}
