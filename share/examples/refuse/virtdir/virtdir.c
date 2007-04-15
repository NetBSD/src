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
#include <sys/stat.h>

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "virtdir.h"
#include "defs.h"

 /* utility comparison routine for sorting and searching */
static int
compare(const void *vp1, const void *vp2)
{
	const virt_dirent_t	*tp1 = (const virt_dirent_t *) vp1;
	const virt_dirent_t	*tp2 = (const virt_dirent_t *) vp2;

	return strcmp(tp1->name, tp2->name);
}

/* save `n' chars of `s' in allocated storage */
static char *
strnsave(const char *s, int n)
{
	char	*cp;

	if (n < 0) {
		n = strlen(s);
	}
	NEWARRAY(char, cp, n + 1, "strnsave", return NULL);
	(void) memcpy(cp, s, n);
	cp[n] = 0x0;
	return cp;
}

/* initialise the tree */
int
virtdir_init(virtdir_t *tp, struct stat *d, struct stat *f, struct stat *l)
{
	(void) memcpy(&tp->dir, d, sizeof(tp->dir));
	tp->dir.st_mode = S_IFDIR | 0755;
	tp->dir.st_nlink = 2;
	(void) memcpy(&tp->file, f, sizeof(tp->file));
	tp->file.st_mode = S_IFREG | 0644;
	tp->file.st_nlink = 1;
	(void) memcpy(&tp->lnk, l, sizeof(tp->lnk));
	tp->lnk.st_mode = S_IFLNK | 0644;
	tp->lnk.st_nlink = 1;
	return 1;
}

/* add an entry to the tree */
int
virtdir_add(virtdir_t *tp, const char *name, size_t size, uint8_t type, char *tgt)
{
	struct stat	 st;

	if (tp->v == NULL) {
		(void) stat(".", &st);
		virtdir_init(tp, &st, &st, &st);
	}
	ALLOC(virt_dirent_t, tp->v, tp->size, tp->c, 10, 10, "virtdir_add",
			return 0);
	tp->v[tp->c].namelen = size;
	if ((tp->v[tp->c].name = strnsave(name, size)) == NULL) {
		return 0;
	}
	tp->v[tp->c].d_name = strrchr(tp->v[tp->c].name, '/') + 1;
	tp->v[tp->c].type = type;
	if (tgt != NULL) {
		tp->v[tp->c].tgt = strdup(tgt);
		tp->v[tp->c].tgtlen = strlen(tgt);
	}
	tp->c += 1;
	qsort(tp->v, tp->c, sizeof(tp->v[0]), compare);
	return 1;
}

/* add an entry to the tree */
int
virtdir_del(virtdir_t *tp, const char *name, size_t size)
{
	virt_dirent_t	*ep;
	int			 i;

	if ((ep = virtdir_find(tp, name, size)) == NULL) {
		return 0;
	}
	i = (int)(ep - tp->v) / sizeof(tp->v[0]);
	for (tp->c -= 1 ; i < tp->c ; i++) {
		tp->v[i] = tp->v[i + 1];
	}
	return 1;
}

/* find an entry in the tree */
virt_dirent_t *
virtdir_find(virtdir_t *tp, const char *name, size_t namelen)
{
	virt_dirent_t	e;

	(void) memset(&e, 0x0, sizeof(e));
	e.name = __UNCONST(name);
	e.namelen = namelen;
	return bsearch(&e, tp->v, tp->c, sizeof(tp->v[0]), compare);
}

/* analogous to opendir(3) - open a directory, save information, and
* return a pointer to the dynamically allocated structure */
VIRTDIR *
openvirtdir(virtdir_t *tp, const char *d)
{
	VIRTDIR	*dirp;

	NEW(VIRTDIR, dirp, "openvirtdir", exit(EXIT_FAILURE));
	dirp->dirname = strdup(d);
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
		    strchr(from, '/') == NULL) {
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
	FREE(dirp);
}

/* find a target in the tree -- not quick! */
virt_dirent_t *
virtdir_find_tgt(virtdir_t *tp, const char *tgt, size_t tgtlen)
{
	/* we don't need no stinking binary searches */
	int	i;

	for (i = 0 ; i < tp->c ; i++) {
		if (tp->v[i].tgt && strcmp(tp->v[i].tgt, tgt) == 0) {
			return &tp->v[i];
		}
	}
	return NULL;
}

/* kill all of the space allocated to the tree */
void
virtdir_drop(virtdir_t *tp)
{
	int	i;

	for (i = 0 ; i < tp->c ; i++) {
		FREE(tp->v[i].name);
		if (tp->v[i].tgt) {
			FREE(tp->v[i].tgt);
		}
	}
	FREE(tp->v);
}

#ifdef VIRTDIR_DEBUG
static void
ptree(virtdir_t * tp)
{
	int	i;

	for (i = 0 ; i < tp->c ; i++) {
		printf("%s, tgt %s\n", tp->v[i].name, tp->v[i].tgt);
	}
}
#endif

#ifdef VIRTDIR_DEBUG
int
main(int argc, char **argv)
{
	virt_dirent_t	*tp;
	virtdir_t		 t;
	struct stat		 st;

	(void) memset(&t, 0x0, sizeof(t));
	stat(".", &st);
	virtdir_add(&t, ".", 1, 'd', NULL);
	stat("..", &st);
	virtdir_add(&t, "..", 2, 'd', NULL);
	st.st_mode = S_IFREG | 0644;
	virtdir_add(&t, "file1", 5, 'f', NULL);
	ptree(&t);
	virtdir_add(&t, "file2", 5, 'f', NULL);
	virtdir_add(&t, "file0", 5, 'f', NULL);
	virtdir_add(&t, "abcde", 5, 'f', NULL);
	virtdir_add(&t, "bcdef", 5, 'f', NULL);
	virtdir_add(&t, "a", 1, 'f', NULL);
	ptree(&t);
	if ((tp = virtdir_find(&t, "a", 1)) == NULL) {
		printf("borked2\n");
	} else {
		printf("a found\n");
	}
	virtdir_drop(&t);
	exit(EXIT_SUCCESS);
}
#endif
