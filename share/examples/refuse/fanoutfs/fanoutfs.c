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

#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "defs.h"

#ifndef PREFIX
#define PREFIX		""
#endif

#ifndef DEF_CONF_FILE
#define DEF_CONF_FILE	"/etc/fanoutfs.conf"
#endif

DEFINE_ARRAY(strv_t, char *);

static struct stat	 vfs;		/* stat info of directory */
static strv_t		 dirs;		/* the directories, in order */
static char		*conffile;	/* configuration file name */
static int		 verbose; 	/* how chatty are we? */





/********************************************************************/

static int
readconf(const char *f)
{
	char	 buf[BUFSIZ];
	char	*cp;
	FILE	*fp;
	int	 line;

	if ((fp = fopen((f) ? f : PREFIX DEF_CONF_FILE, "r")) == NULL) {
		warn("can't read configuration file `%s'\n", f);
		return 0;
	}
	for (line = 1 ;  fgets(buf, sizeof(buf), fp) != NULL ; line += 1) {
		buf[strlen(buf) - 1] = 0x0;
		for (cp = buf ; *cp && isspace((unsigned)*cp) ; cp++) {
		}
		if (*cp == '\n' || *cp == 0x0 || *cp == '#') {
			continue;
		}
		ALLOC(char *, dirs.v, dirs.size, dirs.c, 10, 10,
			"readconf", exit(EXIT_FAILURE));
		dirs.v[dirs.c++] = strdup(cp);
	}
	(void) fclose(fp);
	return 1;
}

/* yes, this does too much work */
static void
sighup(int n)
{
	int	i;

	printf("Reading configuration file `%s'\n", conffile);
	for (i = 0 ; i < dirs.c ; i++) {
		FREE(dirs.v[i]);
	}
	dirs.c = 0;
	readconf(conffile);
}

/* find the correct entry in the list of directories */
static int
findentry(const char *path, char *name, size_t namesize, struct stat *sp)
{
	struct stat	st;
	int		i;

	if (sp == NULL) {
		sp = &st;
	}
	for (i = 0 ; i < dirs.c ; i++) {
		(void) snprintf(name, namesize, "%s%s", dirs.v[i], path);
		if (stat(name, sp) == 0) {
			return 1;
		}
	}
	return 0;
}

/* return 1 if the string `s' is present in the array */
static int
present(char *s, strv_t *sp)
{
	int	i;

	for (i = 0 ; i < sp->c && strcmp(s, sp->v[i]) != 0 ; i++) {
	}
	return (i < sp->c);
}

/* make sure the directory hierarchy exists */
static int
mkdirs(char *path)
{
	char	 name[MAXPATHLEN];
	char	*slash;

	(void) snprintf(name, sizeof(name), "%s%s", dirs.v[0], path);
	slash = &name[strlen(path) + 1];
	while ((slash = strchr(slash, '/')) != NULL) {
		*slash = 0x0;
printf("mkdirs: dir `%s'\n", name);
		if (mkdir(name, 0777) < 0) {
			return 0;
		}
		*slash = '/';
	}
	return 1;
}

/* file system operations start here */

/* perform the stat operation */
static int 
fanoutfs_getattr(const char *path, struct stat *st)
{
	char	name[MAXPATHLEN];

	(void) memset(st, 0x0, sizeof(*st));
	if (strcmp(path, "/") == 0) {
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2;
		return 0;
	}
	if (!findentry(path, name, sizeof(name), st)) {
		return -ENOENT;
	}
	return 0;
}

/* readdir operation */
static int 
fanoutfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
	      off_t offset, struct fuse_file_info *fi)
{
	struct dirent	*dp;
	strv_t		 names;
	char		 name[MAXPATHLEN];
	DIR		*dirp;
	int		 i;

	(void) fi;

	(void) memset(&names, 0x0, sizeof(names));
	for (i = 0 ; i < dirs.c ; i++) {
		(void) snprintf(name, sizeof(name), "%s%s", dirs.v[i], path);
		if ((dirp = opendir(name)) == NULL) {
			continue;
		}
		while ((dp = readdir(dirp)) != NULL) {
			if (!present(dp->d_name, &names)) {
				ALLOC(char *, names.v, names.size, names.c,
					10, 10, "readdir", exit(EXIT_FAILURE));
				names.v[names.c++] = strdup(dp->d_name);
			}
		}
		(void) closedir(dirp);
	}
	for (i = 0 ; i < names.c ; i++) {
		(void) filler(buf, names.v[i], NULL, 0);
		FREE(names.v[i]);
	}
	if (i > 0) {
		FREE(names.v);
	}
	return 0;
}

/* open the file in the file system */
static int 
fanoutfs_open(const char *path, struct fuse_file_info *fi)
{
	char	name[MAXPATHLEN];

	if (!findentry(path, name, sizeof(name), NULL)) {
		return -ENOENT;
	}
	return 0;
}

/* read the file's contents in the file system */
static int 
fanoutfs_read(const char *path, char *buf, size_t size, off_t offset,
	   struct fuse_file_info * fi)
{
	char	name[MAXPATHLEN];
	int	fd;
	int	cc;

	(void) fi;

	if (!findentry(path, name, sizeof(name), NULL)) {
		return -ENOENT;
	}
	if ((fd = open(name, O_RDONLY, 0666)) < 0) {
		return -ENOENT;
	}
	if (lseek(fd, offset, SEEK_SET) < 0) {
		(void) close(fd);
		return -EBADF;
	}
	if ((cc = read(fd, buf, size)) < 0) {
		(void) close(fd);
		return -errno;
	}
	(void) close(fd);
	return cc;
}

/* write the file's contents in the file system */
static int 
fanoutfs_write(const char *path, const char *buf, size_t size, off_t offset,
	   struct fuse_file_info * fi)
{
	char	name[MAXPATHLEN];
	int	fd;
	int	cc;

	(void) fi;

	if (!findentry(path, name, sizeof(name), NULL)) {
		return -ENOENT;
	}
	if ((fd = open(name, O_WRONLY, 0666)) < 0) {
		return -ENOENT;
	}
	if (lseek(fd, offset, SEEK_SET) < 0) {
		(void) close(fd);
		return -EBADF;
	}
	if ((cc = write(fd, buf, size)) < 0) {
		(void) close(fd);
		return -errno;
	}
	(void) close(fd);
	return cc;
}

/* fill in the statvfs struct */
static int
fanoutfs_statfs(const char *path, struct statvfs *st)
{
	(void) memset(st, 0x0, sizeof(*st));
	st->f_bsize = st->f_frsize = st->f_iosize = 512;
	st->f_owner = vfs.st_uid;
	st->f_files = 1;
	return 0;
}

/* "remove" a file */
static int
fanoutfs_unlink(const char *path)
{
	char	name[MAXPATHLEN];

	if (!findentry(path, name, sizeof(name), NULL)) {
		return -ENOENT;
	}
	if (unlink(name) < 0) {
		return -errno;
	}
	return 0;
}

/* check the access on a file */
static int
fanoutfs_access(const char *path, int acc)
{
	char	name[MAXPATHLEN];

	if (!findentry(path, name, sizeof(name), NULL)) {
		return -ENOENT;
	}
	if (access(name, acc) < 0) {
		return -errno;
	}
	return 0;
}

/* change the mode of a file */
static int
fanoutfs_chmod(const char *path, mode_t mode)
{
	char	name[MAXPATHLEN];

	if (!findentry(path, name, sizeof(name), NULL)) {
		return -ENOENT;
	}
	if (chmod(name, mode) < 0) {
		return -errno;
	}
	return 0;
}

/* change the owner and group of a file */
static int
fanoutfs_chown(const char *path, uid_t uid, gid_t gid)
{
	char	name[MAXPATHLEN];

	if (!findentry(path, name, sizeof(name), NULL)) {
		return -ENOENT;
	}
	if (lchown(name, uid, gid) < 0) {
		return -errno;
	}
	return 0;
}

/* "rename" a file */
static int
fanoutfs_rename(const char *from, const char *to)
{
	char	fromname[MAXPATHLEN];
	char	toname[MAXPATHLEN];

	if (!findentry(from, fromname, sizeof(fromname), NULL)) {
		return -ENOENT;
	}
	(void) snprintf(toname, sizeof(toname), "%s%s", dirs.v[0], to);
	if (!mkdirs(toname)) {
		return -ENOENT;
	}
	if (rename(fromname, toname) < 0) {
		return -EPERM;
	}
	return 0;
}

/* create a file */
static int
fanoutfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	struct stat	st;
	char		name[MAXPATHLEN];
	int		fd;

	if (findentry(path, name, sizeof(name), &st)) {
		return -EEXIST;
	}
	if ((fd = open(name, O_RDWR | O_CREAT | O_EXCL, 0666)) < 0) {
		return -EPERM;
	}
	(void) close(fd);
	return 0;
}

/* create a special node */
static int
fanoutfs_mknod(const char *path, mode_t mode, dev_t d)
{
	struct stat	st;
	char		name[MAXPATHLEN];

	if (findentry(path, name, sizeof(name), &st)) {
		return -EEXIST;
	}
	if (mknod(name, mode, d) < 0) {
		return -EPERM;
	}
	return 0;
}

/* create a directory */
static int
fanoutfs_mkdir(const char *path, mode_t mode)
{
	char	name[MAXPATHLEN];

	(void) snprintf(name, sizeof(name), "%s%s", dirs.v[0], path);
	if (!mkdirs(name)) {
		return -ENOENT;
	}
	if (mkdir(name, mode) < 0) {
		return -EPERM;
	}
	return 0;
}

/* create a symbolic link */
static int
fanoutfs_symlink(const char *path, const char *tgt)
{
	char	name[MAXPATHLEN];

	(void) snprintf(name, sizeof(name), "%s%s", dirs.v[0], path);
	if (!mkdirs(name)) {
		return -ENOENT;
	}
	if (symlink(name, tgt) < 0) {
		return -EPERM;
	}
	return 0;
}

/* create a link */
static int
fanoutfs_link(const char *path, const char *tgt)
{
	char	name[MAXPATHLEN];

	(void) snprintf(name, sizeof(name), "%s%s", dirs.v[0], path);
	if (!mkdirs(name)) {
		return -ENOENT;
	}
	if (link(name, tgt) < 0) {
		return -errno;
	}
	return 0;
}

/* read the contents of a symbolic link */
static int
fanoutfs_readlink(const char *path, char *buf, size_t size)
{
	char	name[MAXPATHLEN];

	if (!findentry(path, name, sizeof(name), NULL)) {
		return -ENOENT;
	}
	if (readlink(name, buf, size) < 0) {
		return -errno;
	}
	return 0;
}

/* remove a directory */
static int
fanoutfs_rmdir(const char *path)
{
	char	name[MAXPATHLEN];

	if (!findentry(path, name, sizeof(name), NULL)) {
		return -ENOENT;
	}
	if (rmdir(name) < 0) {
		return -errno;
	}
	return 0;
}

/* truncate a file */
static int
fanoutfs_truncate(const char *path, off_t size)
{
	char	name[MAXPATHLEN];

	if (!findentry(path, name, sizeof(name), NULL)) {
		return -ENOENT;
	}
	if (truncate(name, size) < 0) {
		return -errno;
	}
	return 0;
}

/* set utimes on a file */
static int
fanoutfs_utime(const char *path, struct utimbuf *t)
{
	char	name[MAXPATHLEN];

	if (!findentry(path, name, sizeof(name), NULL)) {
		return -ENOENT;
	}
	if (utime(name, t) < 0) {
		return -errno;
	}
	return 0;
}

/* operations struct */
static struct fuse_operations fanoutfs_oper = {
	.getattr = fanoutfs_getattr,
	.readlink = fanoutfs_readlink,
	.mknod = fanoutfs_mknod,
	.mkdir = fanoutfs_mkdir,
	.unlink = fanoutfs_unlink,
	.rmdir = fanoutfs_rmdir,
	.symlink = fanoutfs_symlink,
	.rename = fanoutfs_rename,
	.link = fanoutfs_link,
	.chmod = fanoutfs_chmod,
	.chown = fanoutfs_chown,
	.truncate = fanoutfs_truncate,
	.utime = fanoutfs_utime,
	.open = fanoutfs_open,
	.read = fanoutfs_read,
	.write = fanoutfs_write,
	.statfs = fanoutfs_statfs,
	.readdir = fanoutfs_readdir,
	.access = fanoutfs_access,
	.create = fanoutfs_create
};

int 
main(int argc, char **argv)
{
	int	 i;

	while ((i = getopt(argc, argv, "f:v")) != -1) {
		switch(i) {
		case 'f':
			conffile = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
		}
	}
	(void) signal(SIGHUP, sighup);
	readconf(conffile);
	(void) daemon(1, 1);
	return fuse_main(argc, argv, &fanoutfs_oper);
}
