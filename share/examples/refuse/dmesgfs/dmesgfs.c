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
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "virtdir.h"
#include "defs.h"

static int		 verbose; /* how chatty are we? */

static virtdir_t	 pci;

/* a device node describes the device that is printed in the dmesg */
typedef struct devicenode_t {
	char	*dev;		/* device name */
	int	 devlen;	/* length of name */
	char	*parent;	/* its parent device name */
	int	 parentlen;	/* length of parent name */
	char	*descr;		/* description of the device itself */
	int	 descrlen;	/* length of description */
	int	 p;		/* device node subscript of parent */
	int	 dir;		/* nonzero if this is a directory */
} devicenode_t;

DEFINE_ARRAY(devices_t, devicenode_t);

static devices_t	devices;


/* perform the stat operation */
/* if this is the root, then just synthesise the data */
static int 
dmesgfs_getattr(const char *path, struct stat *st)
{
	virt_dirent_t	*ep;

	if (strcmp(path, "/") == 0) {
		(void) memset(st, 0x0, sizeof(*st));
		st->st_mode = (S_IFDIR | 0755);
		st->st_nlink = 2;
		return 0;
	}
	if ((ep = virtdir_find(&pci, path, strlen(path))) == NULL) {
		return -ENOENT;
	}
	switch(ep->type) {
	case 'f':
		(void) memcpy(st, &pci.file, sizeof(*st));
		st->st_size = ep->tgtlen;
		st->st_mode = S_IFREG | 0644;
		break;
	case 'd':
		(void) memcpy(st, &pci.dir, sizeof(*st));
		break;
	case 'l':
		(void) memcpy(st, &pci.lnk, sizeof(*st));
		st->st_size = ep->tgtlen;
		st->st_mode = S_IFLNK | 0755;
		break;
	}
	st->st_ino = virtdir_offset(&pci, ep) + 10;
	return 0;
}

/* readdir operation */
static int 
dmesgfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
	      off_t offset, struct fuse_file_info * fi)
{
	static VIRTDIR	*dirp;
	virt_dirent_t	*dp;

	if (offset == 0) {
		if ((dirp = openvirtdir(&pci, path)) == NULL) {
			return 0;
		}
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
	}
	while ((dp = readvirtdir(dirp)) != NULL) {
		if (filler(buf, dp->d_name, NULL, 0) != 0) {
			return 0;
		}
	}
	closevirtdir(dirp);
	dirp = NULL;
	return 0;
}

/* open the file in the file system */
static int 
dmesgfs_open(const char *path, struct fuse_file_info * fi)
{
	return 0;
}

/* read the file's contents in the file system */
static int 
dmesgfs_read(const char *path, char *buf, size_t size, off_t offset,
	   struct fuse_file_info * fi)
{
	virt_dirent_t	*ep;

	if ((ep = virtdir_find(&pci, path, strlen(path))) == NULL) {
		return -ENOENT;
	}
	if (ep->tgt == NULL) {
		return -ENOENT;
	}
	(void) memcpy(buf, &ep->tgt[offset], size);
	return ep->tgtlen;
}

/* fill in the statvfs struct */
static int
dmesgfs_statfs(const char *path, struct statvfs *st)
{
	(void) memset(st, 0x0, sizeof(*st));
	return 0;
}

/* read the symbolic link */
static int
dmesgfs_readlink(const char *path, char *buf, size_t size)
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
static struct fuse_operations dmesgfs_oper = {
	.getattr = dmesgfs_getattr,
	.readlink = dmesgfs_readlink,
	.readdir = dmesgfs_readdir,
	.open = dmesgfs_open,
	.read = dmesgfs_read,
	.statfs = dmesgfs_statfs
};

/* save `n' chars of `s' in malloc'd memory */
static char *
strnsave(const char *s, int n)
{
	char	*cp;

	if (n < 0) {
		n = strlen(s);
	}
	NEWARRAY(char, cp, n + 1, "strnsave", exit(EXIT_FAILURE));
	(void) memcpy(cp, s, n);
	cp[n] = 0x0;
	return cp;
}

/* find a device in the device node tree */
static int
finddev(const char *s, int len, int updir)
{
	int	i;

	for (i = 0 ; i < devices.c ; i++) {
		if (strncmp(devices.v[i].dev, s, len) == 0 &&
		    devices.v[i].devlen == len) {
			if (updir) {
				devices.v[i].dir = 1;
			}
			return i;
		}
	}
	return -1;
}

/* add a device to the device node tree */
static void
add_dev(const char *dev, int len, const char *parent, int parentlen, const char *descr, int descrlen)
{
	int	d;

	if ((d = finddev(dev, len, 0)) < 0) {
		ALLOC(devicenode_t, devices.v, devices.size, devices.c, 10, 10, "add_dev", exit(EXIT_FAILURE));
		devices.v[devices.c].dev = strnsave(dev, len);
		devices.v[devices.c].devlen = len;
		devices.v[devices.c].parent = strnsave(parent, parentlen);
		devices.v[devices.c].parentlen = parentlen;
		devices.v[devices.c].descr = strnsave(descr, descrlen);
		devices.v[devices.c].descrlen = descrlen;
		devices.c += 1;
	}
}

/* print the parent device pathname */
static int
pparent(int d, char *buf, size_t size)
{
	int	cc;

	if (d != 0) {
		cc = pparent(devices.v[d].p, buf, size);
		size -= cc;
	}
	(void) strlcat(buf, "/", size);
	(void) strlcat(buf, devices.v[d].dev, size - 1);
	return devices.v[d].devlen + 1;
}

#define NEXUS_DESCRIPTION	"Nexus - the root of everything"

/* build up a fuse_tree from the information in the database */
static int
build_tree(virtdir_t *tp, const char *nexus, char type)
{
	struct stat	 dir;
	struct stat	 f;
	regmatch_t	 matchv[10];
	regex_t		 r;
	FILE		*pp;
	char		 buf[BUFSIZ];
	int		 i;
	int		 p;

	(void) stat(".", &dir);
	(void) memcpy(&f, &dir, sizeof(f));
	f.st_mode = S_IFREG | 0644;
	if (regcomp(&r, "^([a-z0-9]+) at ([a-z0-9]+)(.*)", REG_EXTENDED) != 0) {
		warn("can't compile regular expression\n");
		return 0;
	}
	if ((pp = popen("dmesg", "r")) == NULL) {
		return 0;
	}
	add_dev(nexus, strlen(nexus), nexus, strlen(nexus), NEXUS_DESCRIPTION,
		strlen(NEXUS_DESCRIPTION));
	while (fgets(buf, sizeof(buf), pp) != NULL) {
		if (type == 'l') {
			buf[strlen(buf) - 1] = 0x0;
		}
		if (regexec(&r, buf, 10, matchv, 0) == 0) {
			add_dev(&buf[matchv[1].rm_so],
				matchv[1].rm_eo - matchv[1].rm_so,
				&buf[matchv[2].rm_so],
				matchv[2].rm_eo - matchv[2].rm_so,
				buf,
				matchv[0].rm_eo - matchv[0].rm_so);
		}
	}
	printf("%d devices\n", devices.c);
	(void) pclose(pp);
	for (i = 0 ; i < devices.c ; i++) {
		if ((p = finddev(devices.v[i].parent, devices.v[i].parentlen, 1)) < 0) {
			warn("No parent device for %s\n", devices.v[i].dev);
		}
		devices.v[i].p = p;
	}
	for (i = 0 ; i < devices.c ; i++) {
		pparent(i, buf, sizeof(buf));
		virtdir_add(tp, buf, strlen(buf),
			(devices.v[i].dir) ? 'd' : type,
			devices.v[i].descr, devices.v[i].descrlen);
		if (devices.v[i].dir) {
			(void) strlcat(buf, "/", sizeof(buf));
			(void) strlcat(buf, "Description", sizeof(buf));
			virtdir_add(tp, buf, strlen(buf), type,
				devices.v[i].descr, devices.v[i].descrlen);
		}
		if (verbose) {
			printf("dmesgfs: adding %s `%s' -> `%s'\n",
				(type == 'l') ? "symbolic link" : "file",
				buf, devices.v[i].descr);
		}
		buf[0] = 0x0;
	}
	return 1;
}

int 
main(int argc, char **argv)
{
	char	*nexus;
	char	 type;
	int	 i;

	nexus = NULL;
	type = 'l';
	while ((i = getopt(argc, argv, "fln:v")) != -1) {
		switch(i) {
		case 'f':
			type = 'f';
			break;
		case 'l':
			type = 'l';
			break;
		case 'n':
			nexus = optarg;
			break;
		case 'v':
			verbose += 1;
			break;
		}
	}
	if (!build_tree(&pci, (nexus) ? nexus : "mainbus0", type)) {
		exit(EXIT_FAILURE);
	}
	return fuse_main(argc, argv, &dmesgfs_oper, NULL);
}
