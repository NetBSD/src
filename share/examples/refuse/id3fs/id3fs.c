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

#include <db.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "virtdir.h"
#include "defs.h"

static char		*prefix; /* where the music is */
static int		 verbose; /* how chatty are we? */

static BTREEINFO	 bt;

static virtdir_t	 id3;


/* perform the stat operation */
/* if this is the root, then just synthesise the data */
/* otherwise, retrieve the data, and be sure to fill in the size */
static int 
id3fs_getattr(const char *path, struct stat *st)
{
	virt_dirent_t	*ep;

	if (strcmp(path, "/") == 0) {
		(void) memset(st, 0x0, sizeof(*st));
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2;
		return 0;
	}
	if ((ep = virtdir_find(&id3, path, strlen(path))) == NULL) {
		return -ENOENT;
	}
	switch(ep->type) {
	case 'f':
		(void) memcpy(st, &id3.file, sizeof(*st));
		break;
	case 'd':
		(void) memcpy(st, &id3.dir, sizeof(*st));
		break;
	case 'l':
		(void) memcpy(st, &id3.lnk, sizeof(*st));
		st->st_size = ep->tgtlen;
		break;
	}
	return 0;
}

/* readdir operation */
static int 
id3fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
	      off_t offset, struct fuse_file_info * fi)
{
	virt_dirent_t	*dp;
	VIRTDIR		*dirp;

	if ((dirp = openvirtdir(&id3, path)) == NULL) {
		return 0;
	}
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	while ((dp = readvirtdir(dirp)) != NULL) {
		filler(buf, dp->d_name, NULL, 0);
	}
	closevirtdir(dirp);
	return 0;
}

/* open the file in the file system */
static int 
id3fs_open(const char *path, struct fuse_file_info * fi)
{
	return 0;
}

/* read the file's contents in the file system */
static int 
id3fs_read(const char *path, char *buf, size_t size, off_t offset,
	   struct fuse_file_info * fi)
{
	return 0;
}

/* fill in the statvfs struct */
static int
id3fs_statfs(const char *path, struct statvfs *st)
{
	(void) memset(st, 0x0, sizeof(*st));
	return 0;
}

/* read the symbolic link */
static int
id3fs_readlink(const char *path, char *buf, size_t size)
{
	virt_dirent_t	*ep;

	if ((ep = virtdir_find(&id3, path, strlen(path))) == NULL) {
		return -ENOENT;
	}
	if (ep->tgt == NULL) {
		return -ENOENT;
	}
	(void) strlcpy(buf, ep->tgt, size);
	return 0;
}

/* operations struct */
static struct fuse_operations id3fs_oper = {
	.getattr = id3fs_getattr,
	.readlink = id3fs_readlink,
	.readdir = id3fs_readdir,
	.open = id3fs_open,
	.read = id3fs_read,
	.statfs = id3fs_statfs
};

/* build up a fuse_tree from the information in the database */
static void
build_id3_tree(DB *db, virtdir_t *tp)
{
	struct stat	 dir;
	struct stat	 f;
	const char	*d;
	char		 name[MAXPATHLEN];
	char		*slash;
	char		*key;
	char		*val;
	int		 flag;
	DBT		 k;
	DBT		 v;

	(void) stat(".", &dir);
	(void) memcpy(&f, &dir, sizeof(f));
	f.st_mode = S_IFREG | 0644;
	(void) memset(&k, 0x0, sizeof(k));
	(void) memset(&v, 0x0, sizeof(v));
	for (flag = R_FIRST ; (*db->seq)(db, &k, &v, flag) == 0 ; flag = R_NEXT) {
		key = (char *) k.data;
		switch (*key) {
		case 'a':
			d = "/artists";
			break;
		case 'g':
			d = "/genre";
			break;
		case 'y':
			d = "/year";
			break;
		default:
			continue;
		}
		val = (char *) v.data;
		if ((slash = strrchr(key, '/')) == NULL) {
			slash = key;
		}
		slash += 1;
		/* add the symbolic link in that directory */
		(void) snprintf(name, sizeof(name), "%s/%s/%s", d, val, slash);
		if (!virtdir_find(tp, name, strlen(name))) {
			virtdir_add(tp, name, strlen(name), 'l', key + 1);
		}
	}
}

int 
main(int argc, char **argv)
{
	char	 name[MAXPATHLEN];
	int	 i;
	DB	*db;

	while ((i = getopt(argc, argv, "p:v")) != -1) {
		switch(i) {
		case 'p':
			prefix = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
		}
	}
	bt.lorder = 4321;
	(void) snprintf(name, sizeof(name), "%s/%s", (prefix) ? prefix : "/usr/music", ".id3.db");
	if ((db = dbopen(name, O_RDONLY, 0666, DB_BTREE, &bt)) == NULL) {
		warn("null id3 database");
	}
	build_id3_tree(db, &id3);
	return fuse_main(argc, argv, &id3fs_oper);
}
