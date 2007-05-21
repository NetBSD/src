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

static int		 verbose; /* how chatty are we? */

static virtdir_t	 pci;


/* perform the stat operation */
/* if this is the root, then just synthesise the data */
/* otherwise, retrieve the data, and be sure to fill in the size */
static int 
pcifs_getattr(const char *path, struct stat *st)
{
	virt_dirent_t	*ep;

	if (strcmp(path, "/") == 0) {
		(void) memset(st, 0x0, sizeof(*st));
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2;
		return 0;
	}
	if ((ep = virtdir_find(&pci, path, strlen(path))) == NULL) {
		return -ENOENT;
	}
	switch(ep->type) {
	case 'f':
		break;
	case 'd':
		(void) memcpy(st, &pci.dir, sizeof(*st));
		break;
	case 'l':
		(void) memcpy(st, &pci.lnk, sizeof(*st));
		st->st_size = ep->tgtlen;
		st->st_mode = S_IFLNK | 0644;
		break;
	}
	return 0;
}

/* readdir operation */
static int 
pcifs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
	      off_t offset, struct fuse_file_info * fi)
{
	virt_dirent_t	*dp;
	VIRTDIR		*dirp;

	if ((dirp = openvirtdir(&pci, path)) == NULL) {
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
pcifs_open(const char *path, struct fuse_file_info * fi)
{
	return 0;
}

/* read the file's contents in the file system */
static int 
pcifs_read(const char *path, char *buf, size_t size, off_t offset,
	   struct fuse_file_info * fi)
{
	return 0;
}

/* fill in the statvfs struct */
static int
pcifs_statfs(const char *path, struct statvfs *st)
{
	(void) memset(st, 0x0, sizeof(*st));
	return 0;
}

/* read the symbolic link */
static int
pcifs_readlink(const char *path, char *buf, size_t size)
{
	virt_dirent_t	*ep;

	if ((ep = virtdir_find(&pci, path, strlen(path))) == NULL) {
		return -ENOENT;
	}
	if (ep->tgt == NULL) {
		return -ENOENT;
	}
	(void) strlcpy(buf, ep->tgt, size);
	return 0;
}

/* operations struct */
static struct fuse_operations pcifs_oper = {
	.getattr = pcifs_getattr,
	.readlink = pcifs_readlink,
	.readdir = pcifs_readdir,
	.open = pcifs_open,
	.read = pcifs_read,
	.statfs = pcifs_statfs
};

/* build up a fuse_tree from the information in the database */
static int
build_tree(virtdir_t *tp, int bus)
{
	struct stat	 dir;
	struct stat	 f;
	FILE		*pp;
	char		 name[MAXPATHLEN];
	char		 buf[BUFSIZ];
	char		*cp;
	int		 cc;

	(void) stat(".", &dir);
	(void) memcpy(&f, &dir, sizeof(f));
	(void) snprintf(buf, sizeof(buf), "pcictl pci%d list", bus);
	f.st_mode = S_IFREG | 0644;
	if ((pp = popen(buf, "r")) == NULL) {
		return 0;
	}
	while (fgets(buf, sizeof(buf), pp) != NULL) {
		buf[strlen(buf) - 1] = 0x0;
		if ((cp = strchr(buf, ' ')) == NULL) {
			continue;
		}
		cc = snprintf(name, sizeof(name), "/%.*s", (int)(cp - buf), buf);
		virtdir_add(tp, name, cc, 'l', cp + 1, strlen(cp + 1));
		if (verbose) {
			printf("pcifs: adding symbolic link `%s' -> `%s'\n", name, cp + 1);
		}
	}
	(void) pclose(pp);
	return 1;
}

int 
main(int argc, char **argv)
{
	int	 bus;
	int	 i;

	bus = 0;
	while ((i = getopt(argc, argv, "b:v")) != -1) {
		switch(i) {
		case 'b':
			bus = atoi(optarg);
			break;
		case 'v':
			verbose += 1;
			break;
		}
	}
	if (!build_tree(&pci, bus)) {
		exit(EXIT_FAILURE);
	}
	return fuse_main(argc, argv, &pcifs_oper, NULL);
}
