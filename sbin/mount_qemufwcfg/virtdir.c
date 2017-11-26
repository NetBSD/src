/* $NetBSD: virtdir.c,v 1.2 2017/11/26 03:06:24 christos Exp $ */

/*
 * Copyright © 2007 Alistair Crooks.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <util.h>

#include "virtdir.h"

 /* utility comparison routine for sorting and searching */
static int
compare(const void *vp1, const void *vp2)
{
	const virt_dirent_t *tp1 = (const virt_dirent_t *) vp1;
	const virt_dirent_t *tp2 = (const virt_dirent_t *) vp2;

	return strcmp(tp1->name, tp2->name);
}

/* ensure intermediate directories exist */
static void
mkdirs(virtdir_t *tp, const char *path, size_t size)
{
	virt_dirent_t	*ep;
	char		 name[MAXPATHLEN];
	char		*slash;

	(void) strlcpy(name, path, sizeof(name));
	for (slash = name + 1 ; (slash = strchr(slash + 1, '/')) != NULL ; ) {
		*slash = 0x0;
		if ((ep = virtdir_find(tp, name, strlen(name))) == NULL) {
			virtdir_add(tp, name, strlen(name), 'd', NULL, 0, 0);
		}
		*slash = '/';
	}
}

/* get rid of multiple slashes in input */
static size_t
normalise(const char *name, size_t namelen, char *path, size_t pathsize)
{
	const char	*np;
	char		*pp;
	int		 done;

	for (pp = path, np = name, done = 0 ; !done &&
	    (size_t)(pp - path) < pathsize - 1 &&
	    (size_t)(np - name) <= namelen; ) {
		switch (*np) {
		case '/':
			if (pp == path || *(pp - 1) != '/') {
				*pp++ = *np;
			}
			np += 1;
			break;
		case '\0':
			done = 1;
			break;
		default:
			*pp++ = *np++;
			break;
		}
	}
	/* XXX - trailing slash? */
	*pp = '\0';
	return (size_t)(pp - path);
}

/* initialise the tree */
int
virtdir_init(virtdir_t *tp, const char *rootdir, struct stat *d, struct stat *f, struct stat *l)
{
	tp->dir = *d;
	tp->dir.st_mode = S_IFDIR | 0755;
	tp->dir.st_nlink = 2;
	tp->file = *f;
	tp->file.st_mode = S_IFREG | 0644;
	tp->file.st_nlink = 1;
	tp->lnk = *l;
	tp->lnk.st_mode = S_IFLNK | 0644;
	tp->lnk.st_nlink = 1;
	if (rootdir != NULL) {
		tp->rootdir = estrdup(rootdir);
	}
	return 1;
}

/* add an entry to the tree */
int
virtdir_add(virtdir_t *tp, const char *name, size_t size, uint8_t type, const char *tgt, size_t tgtlen, uint16_t select)
{
	char		path[MAXPATHLEN];
	size_t		pathlen;

	pathlen = normalise(name, size, path, sizeof(path));
	if (virtdir_find(tp, path, pathlen) != NULL) {
		/* attempt to add a duplicate directory entry */
		return 0;
	}
	if (tp->c == tp->size || tp->size == 0) {
		tp->size += 10,
		tp->v = erealloc(tp->v, tp->size * sizeof(*tp->v));
	}
	tp->v[tp->c].namelen = pathlen;
	if ((tp->v[tp->c].name = estrndup(path, pathlen)) == NULL) {
		return 0;
	}
	tp->v[tp->c].d_name = strrchr(tp->v[tp->c].name, '/') + 1;
	tp->v[tp->c].type = type;
	tp->v[tp->c].ino = (ino_t) random() & 0xfffff;
	tp->v[tp->c].tgtlen = tgtlen;
	if (tgt != NULL) {
		tp->v[tp->c].tgt = estrndup(tgt, tgtlen);
	}
	tp->v[tp->c].select = select;
	tp->c += 1;
	qsort(tp->v, tp->c, sizeof(tp->v[0]), compare);
	mkdirs(tp, path, pathlen);
	return 1;
}

/* find an entry in the tree */
virt_dirent_t *
virtdir_find(virtdir_t *tp, const char *name, size_t namelen)
{
	virt_dirent_t	e;
	char		path[MAXPATHLEN];

	(void) memset(&e, 0x0, sizeof(e));
	e.namelen = normalise(name, namelen, path, sizeof(path));
	e.name = path;
	return bsearch(&e, tp->v, tp->c, sizeof(tp->v[0]), compare);
}

/* return the virtual offset in the tree */
off_t
virtdir_offset(virtdir_t *tp, virt_dirent_t *dp)
{
	return dp - tp->v;
}

/* analogous to opendir(3) - open a directory, save information, and
* return a pointer to the dynamically allocated structure */
VIRTDIR *
openvirtdir(virtdir_t *tp, const char *d)
{
	VIRTDIR	*dirp;

	dirp = emalloc(sizeof(*dirp));
	dirp->dirname = estrdup(d);
	dirp->dirnamelen = strlen(d);
	dirp->tp = tp;
	dirp->i = 0;
	return dirp;
}

/* analogous to readdir(3) - read the next entry in the directory that
* was opened, and return a pointer to it */
virt_dirent_t *
readvirtdir(VIRTDIR *dirp)
{
	char	*from;

	for ( ; dirp->i < dirp->tp->c ; dirp->i++) {
		from = (strcmp(dirp->dirname, "/") == 0) ?
		    &dirp->tp->v[dirp->i].name[1] :
		    &dirp->tp->v[dirp->i].name[dirp->dirnamelen + 1];
		if (strncmp(dirp->tp->v[dirp->i].name, dirp->dirname,
		    dirp->dirnamelen) == 0 &&
		    *from != '\0' && strchr(from, '/') == NULL) {
			return &dirp->tp->v[dirp->i++];
		}
	}
	return NULL;
}

/* free the storage associated with the virtual directory structure */
void
closevirtdir(VIRTDIR *dirp)
{
	free(dirp->dirname);
	free(dirp);
}
