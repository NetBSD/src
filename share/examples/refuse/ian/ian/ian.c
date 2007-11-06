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

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "fetch.h"

#include "virtdir.h"
#include "defs.h"

#ifndef DEFAULT_CACHE_DIR
#define DEFAULT_CACHE_DIR	"/tmp"
#endif

static char		*cachedir;	/* location for cached files */

static int		 verbose; /* how chatty are we? */

static virtdir_t	 ian;

/* retrieve a file from the remote location, and cache it in cachedir */
static int
getfile(const char *url)
{
	struct url_stat	 urlstat;
	struct url	*newurlp;
	int64_t		 cc;
	char		 buf[BUFSIZ * 8];
	char		 tmpname[BUFSIZ];
	char		 newurl[BUFSIZ];
	char		*cp;
	FILE		*out;
	FILE		*fp;
	int		 got;
	int		 ret;
	int		 fd;

	if ((cp = strchr(url, ':')) == NULL) {
		warn("malformed URL `%s'", url + 1);
		return 0;
	}
	(void) snprintf(newurl, sizeof(newurl), "%.*s/%s", 
		(int)(cp - url), url + 1, cp + 1);
	if ((newurlp = fetchParseURL(newurl)) == NULL) {
		/* add as virtdir */
		virtdir_add(&ian, newurl, strlen(newurl), 'd', "", strlen(""));
		return 1;
	}
	if (newurlp->host[0] == 0x0 || strcmp(newurlp->doc, "/") == 0) {
		virtdir_add(&ian, newurl, strlen(newurl), 'd', "", strlen(""));
		return 1;
	}
	if ((fp = fetchXGetURL(newurl, &urlstat, "p")) == NULL) {
		if (verbose) {
			warn("Error [%d] \"%s\" \"%s\"\n", fetchLastErrCode, newurl, fetchLastErrString);
		}
		/* XXX - before adding as a directory, we should check it does actually exist */
		virtdir_add(&ian, newurl, strlen(newurl), 'd', "", strlen(""));
		return 1;
	}
	if (verbose) {
		printf("::::::::::\n%s\nSize: %lld\nmtime: %s::::::::::\n",
			newurl,
			(long long) urlstat.size,
			ctime(&urlstat.mtime));
	}
	(void) snprintf(tmpname, sizeof(tmpname), "%s/fetch.XXXXXX", cachedir);
	fd = mkstemp(tmpname);
	if ((out = fdopen(fd, "w")) == NULL) {
		warnx("can't create output file `%s'", tmpname);
		return 0;
	}
	for (ret = 1, cc = 0 ; (got = fread(buf, 1, sizeof(buf), fp)) > 0 ; cc += got) {
		if (fwrite(buf, 1, got, out) != got) {
			warnx("short write");
			ret = 0;
			break;
		}
	}
	(void) fclose(out);
	(void) fclose(fp);
	if (ret) {
		virtdir_add(&ian, newurl, strlen(newurl), 'f', tmpname, strlen(tmpname));
	}
	return ret;
}


/* perform the stat operation */
/* if this is the root, then just synthesise the data */
/* otherwise, retrieve the data, and be sure to fill in the size */
static int 
ian_getattr(const char *path, struct stat *st)
{
	virt_dirent_t	*ep;

	if (strcmp(path, "/") == 0) {
		(void) memset(st, 0x0, sizeof(*st));
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2;
		return 0;
	}
	if ((ep = virtdir_find(&ian, path + 1, strlen(path) - 1)) == NULL) {
		if (!getfile(path)) {
			return -ENOENT;
		}
		ep = virtdir_find(&ian, path + 1, strlen(path) - 1);
	}
	switch(ep->type) {
	case 'f':
		(void) memcpy(st, &ian.file, sizeof(*st));
		if (stat(ep->tgt, st) < 0) {
			warn("`%s' has gone", ep->tgt);
		}
		break;
	case 'd':
		(void) memcpy(st, &ian.dir, sizeof(*st));
		break;
	case 'l':
		(void) memcpy(st, &ian.lnk, sizeof(*st));
		st->st_size = ep->tgtlen;
		break;
	}
	return 0;
}

/* readdir operation */
static int 
ian_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
	      off_t offset, struct fuse_file_info * fi)
{
	virt_dirent_t	*dp;
	VIRTDIR		*dirp;

	if ((dirp = openvirtdir(&ian, path)) == NULL) {
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
ian_open(const char *path, struct fuse_file_info * fi)
{
	return 0;
}

/* read the file's contents in the file system */
static int 
ian_read(const char *path, char *buf, size_t size, off_t offset,
	   struct fuse_file_info * fi)
{
	virt_dirent_t	*ep;
	int		 cc;
	int		 fd;

	if ((ep = virtdir_find(&ian, path + 1, strlen(path) - 1)) == NULL) {
		return -ENOENT;
	}
	if ((fd = open(ep->tgt, O_RDONLY, 0666)) < 0) {
		return -EACCES;
	}
	if (lseek(fd, offset, SEEK_SET) < 0) {
		(void) close(fd);
		return -EBADF;
	}
	if ((cc = read(fd, buf, size)) < 0) {
		(void) close(fd);
		return -EBADF;
	}
	(void) close(fd);
	return cc;
}

/* fill in the statvfs struct */
static int
ian_statfs(const char *path, struct statvfs *st)
{
	(void) memset(st, 0x0, sizeof(*st));
	return 0;
}

/* read the symbolic link */
static int
ian_readlink(const char *path, char *buf, size_t size)
{
	virt_dirent_t	*ep;

	if ((ep = virtdir_find(&ian, path, strlen(path))) == NULL) {
		return -ENOENT;
	}
	if (ep->tgt == NULL) {
		return -ENOENT;
	}
	(void) strlcpy(buf, ep->tgt, size);
	return 0;
}

/* operations struct */
static struct fuse_operations ian_oper = {
	.getattr = ian_getattr,
	.readlink = ian_readlink,
	.readdir = ian_readdir,
	.open = ian_open,
	.read = ian_read,
	.statfs = ian_statfs
};

int
main(int argc, char **argv)
{
	int	i;

	cachedir = strdup(DEFAULT_CACHE_DIR);
	while ((i = getopt(argc, argv, "v")) != -1) {
		switch(i) {
		case 'd':
			(void) free(cachedir);
			cachedir = strdup(optarg);
			break;
		case 'v':
			verbose += 1;
			break;
		default:
			warn("unrecognised option `%c'", i);
			exit(EXIT_FAILURE);
		}
	}
	(void) daemon(1, 1);
	virtdir_add(&ian, "ftp:", strlen("ftp:"), 'd', "", 0);
	virtdir_add(&ian, "http:", strlen("http:"), 'd', "", 0);
	virtdir_add(&ian, "https:", strlen("https:"), 'd', "", 0);
	return fuse_main(argc, argv, &ian_oper, NULL);
}
