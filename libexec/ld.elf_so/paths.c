/*	$NetBSD: paths.c,v 1.3 1999/02/24 18:31:00 christos Exp $	*/

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
#include <sys/mman.h>
#include <dirent.h>

#include "debug.h"
#include "rtld.h"

Search_Path *_rtld_find_path(Search_Path *, const char *, size_t);

Search_Path *
_rtld_find_path(
    Search_Path *path,
    const char *pathstr,
    size_t pathlen)
{
    for (; path != NULL; path = path->sp_next) {
	if (pathlen == path->sp_pathlen
		&& memcmp(path->sp_path, pathstr, pathlen) == 0)
	    return path;
    }
    return NULL;
}

void
_rtld_add_paths(
    Search_Path **path_p,
    const char *pathstr,
    bool dodebug)
{
    Search_Path *path, **head_p = path_p;

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

	if (bp < ep && (path = _rtld_find_path(*head_p, bp, ep - bp)) == NULL) {
	    char *cp;
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
		dbg(" added path \"%s\"", path->sp_path);
	}

	if (ep[0] == '\0')
	    break;
	pathstr = ep + 1;
    }
}


