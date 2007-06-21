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

#include "virtdir.h"
#include "defs.h"

#ifndef PREFIX
#define PREFIX		""
#endif

#ifndef DEF_CONF_FILE
#define DEF_CONF_FILE	"/etc/icfs.conf"
#endif

DEFINE_ARRAY(strv_t, char *);

static struct stat	 vfs;		/* stat info of directory */
static virtdir_t	 tree;		/* virtual directory tree */
static int		 verbose; 	/* how chatty are we? */





/********************************************************************/

/* convert a string to lower case */
static char *
strtolower(const char *path, char *name, size_t size)
{
	const char	*cp;
	char		*np;

	for (cp = path, np = name ; (int)(np - name) < size ; np++, cp++) {
		*np = tolower((unsigned)*cp);
	}
	return name;
}

/* add a name and its lower case entry */
static void
add_entry(virtdir_t *tp, const char *name, uint8_t type)
{
	char	 icname[MAXPATHLEN];
	char	*root;
	int	 len;

	root = virtdir_rootdir(&tree);
	len = strlen(root);
	strtolower(&name[len], icname, sizeof(icname));
	virtdir_add(tp, &name[len], strlen(name) - len, type, icname,
				strlen(icname));
}

/* file system operations start here */

/* perform the stat operation */
static int 
icfs_getattr(const char *path, struct stat *st)
{
	virt_dirent_t	*ep;
	char		 name[MAXPATHLEN];

	(void) memset(st, 0x0, sizeof(*st));
	if (strcmp(path, "/") == 0) {
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2;
		return 0;
	}
	if ((ep = virtdir_find_tgt(&tree, path, strlen(path))) == NULL) {
		return -ENOENT;
	}
	(void) snprintf(name, sizeof(name), "%s/%s", virtdir_rootdir(&tree), ep->name);
	if (stat(name, st) < 0) {
		return -errno;
	}
	return 0;
}

/* readdir operation */
static int 
icfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
	      off_t offset, struct fuse_file_info *fi)
{
	virt_dirent_t	*ep;
	VIRTDIR		*dirp;
	char		 name[MAXPATHLEN];

	(void) fi;

	if ((ep = virtdir_find_tgt(&tree, path, strlen(path))) == NULL) {
		return -ENOENT;
	}
	(void) snprintf(name, sizeof(name), "%s/%s", virtdir_rootdir(&tree), ep->name);
	if ((dirp = openvirtdir(&tree, ep->name)) == NULL) {
		return -ENOENT;
	}
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	while ((ep = readvirtdir(dirp)) != NULL) {
		strtolower(ep->d_name, name, sizeof(name));
		(void) filler(buf, name, NULL, 0);
	}
	(void) closevirtdir(dirp);
	return 0;
}

/* open the file in the file system */
static int 
icfs_open(const char *path, struct fuse_file_info *fi)
{
	return 0;
}

/* read the file's contents in the file system */
static int 
icfs_read(const char *path, char *buf, size_t size, off_t offset,
	   struct fuse_file_info * fi)
{
	virt_dirent_t	*ep;
	char		 name[MAXPATHLEN];
	int		 fd;
	int		 cc;

	(void) fi;

	if ((ep = virtdir_find_tgt(&tree, path, strlen(path))) == NULL) {
		return -ENOENT;
	}
	(void) snprintf(name, sizeof(name), "%s/%s", virtdir_rootdir(&tree), ep->name);
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
icfs_write(const char *path, const char *buf, size_t size, off_t offset,
	   struct fuse_file_info * fi)
{
	virt_dirent_t	*ep;
	char		 name[MAXPATHLEN];
	int		 fd;
	int		 cc;

	(void) fi;

	if ((ep = virtdir_find_tgt(&tree, path, strlen(path))) == NULL) {
		return -ENOENT;
	}
	(void) snprintf(name, sizeof(name), "%s/%s", virtdir_rootdir(&tree), ep->name);
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
icfs_statfs(const char *path, struct statvfs *st)
{
	(void) memset(st, 0x0, sizeof(*st));
	st->f_bsize = st->f_frsize = st->f_iosize = 512;
	st->f_owner = vfs.st_uid;
	st->f_files = 1;
	return 0;
}

/* "remove" a file */
static int
icfs_unlink(const char *path)
{
	virt_dirent_t	*ep;
	char		 name[MAXPATHLEN];

	if ((ep = virtdir_find_tgt(&tree, path, strlen(path))) == NULL) {
		return -ENOENT;
	}
	(void) snprintf(name, sizeof(name), "%s/%s", virtdir_rootdir(&tree), ep->name);
	if (unlink(name) < 0) {
		return -errno;
	}
	/* XXX - delete entry */
	return 0;
}

/* check the access on a file */
static int
icfs_access(const char *path, int acc)
{
	virt_dirent_t	*ep;
	char		 name[MAXPATHLEN];

	if ((ep = virtdir_find_tgt(&tree, path, strlen(path))) == NULL) {
		return -ENOENT;
	}
	(void) snprintf(name, sizeof(name), "%s/%s", virtdir_rootdir(&tree), ep->name);
	if (access(name, acc) < 0) {
		return -errno;
	}
	return 0;
}

/* change the mode of a file */
static int
icfs_chmod(const char *path, mode_t mode)
{
	virt_dirent_t	*ep;
	char		 name[MAXPATHLEN];

	if ((ep = virtdir_find_tgt(&tree, path, strlen(path))) == NULL) {
		return -ENOENT;
	}
	(void) snprintf(name, sizeof(name), "%s/%s", virtdir_rootdir(&tree), ep->name);
	if (chmod(name, mode) < 0) {
		return -errno;
	}
	return 0;
}

/* change the owner and group of a file */
static int
icfs_chown(const char *path, uid_t uid, gid_t gid)
{
	virt_dirent_t	*ep;
	char		 name[MAXPATHLEN];

	if ((ep = virtdir_find_tgt(&tree, path, strlen(path))) == NULL) {
		return -ENOENT;
	}
	(void) snprintf(name, sizeof(name), "%s/%s", virtdir_rootdir(&tree), ep->name);
	if (lchown(name, uid, gid) < 0) {
		return -errno;
	}
	return 0;
}

/* "rename" a file */
static int
icfs_rename(const char *from, const char *to)
{
#if 0
	char	fromname[MAXPATHLEN];
	char	toname[MAXPATHLEN];

	virt_dirent_t	*ep;

	if ((ep = virtdir_find_tgt(&tree, from, strlen(from))) == NULL) {
		return -ENOENT;
	}
	(void) snprintf(toname, sizeof(toname), "%s%s", dirs.v[0], to);
	if (!mkdirs(toname)) {
		return -ENOENT;
	}
	if (rename(fromname, toname) < 0) {
		return -EPERM;
	}
#endif
	return 0;
}

/* create a file */
static int
icfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	virt_dirent_t	*ep;
	char		*slash;
	char		 name[MAXPATHLEN];
	int		 fd;

	if ((slash = strrchr(path, '/')) == NULL) {
		return -ENOENT;
	}
	if ((ep = virtdir_find_tgt(&tree, path, (int)(slash - path) - 1)) == NULL) {
		return -ENOENT;
	}
	(void) snprintf(name, sizeof(name), "%s/%s%s", virtdir_rootdir(&tree), ep->name, slash);
	if ((fd = open(name, O_RDWR | O_CREAT | O_EXCL, 0666)) < 0) {
		return -EPERM;
	}
	(void) close(fd);
	add_entry(&tree, name, 'f');
	return 0;
}

/* create a special node */
static int
icfs_mknod(const char *path, mode_t mode, dev_t d)
{
	virt_dirent_t	*ep;
	char		*slash;
	char		 name[MAXPATHLEN];

	if ((slash = strrchr(path, '/')) == NULL) {
		return -ENOENT;
	}
	if ((ep = virtdir_find_tgt(&tree, path, (int)(slash - path) - 1)) == NULL) {
		return -ENOENT;
	}
	(void) snprintf(name, sizeof(name), "%s/%s/%s", virtdir_rootdir(&tree), ep->name, slash + 1);
	if (mknod(name, mode, d) < 0) {
		return -EPERM;
	}
	add_entry(&tree, name, ((mode & S_IFMT) == S_IFCHR) ? 'c' : 'b');
	return 0;
}

/* create a directory */
static int
icfs_mkdir(const char *path, mode_t mode)
{
	virt_dirent_t	*ep;
	char		*slash;
	char		 name[MAXPATHLEN];

	if ((slash = strrchr(path, '/')) == NULL) {
		return -ENOENT;
	}
	if ((ep = virtdir_find_tgt(&tree, path, (int)(slash - path) - 1)) == NULL) {
		return -EEXIST;
	}
	(void) snprintf(name, sizeof(name), "%s/%s/%s", virtdir_rootdir(&tree), ep->name, slash + 1);
	if (mkdir(name, mode) < 0) {
		return -EPERM;
	}
	add_entry(&tree, name, 'd');
	return 0;
}

/* create a symbolic link */
static int
icfs_symlink(const char *path, const char *tgt)
{
	virt_dirent_t	*ep;
	char		*slash;
	char		 name[MAXPATHLEN];

	if ((slash = strrchr(path, '/')) == NULL) {
		return -ENOENT;
	}
	if ((ep = virtdir_find_tgt(&tree, path, (int)(slash - path) - 1)) == NULL) {
		return -EEXIST;
	}
	(void) snprintf(name, sizeof(name), "%s/%s/%s", virtdir_rootdir(&tree), ep->name, slash + 1);
	if (symlink(name, tgt) < 0) {
		return -EPERM;
	}
	add_entry(&tree, name, 'l');
	return 0;
}

/* create a link */
static int
icfs_link(const char *path, const char *tgt)
{
	virt_dirent_t	*ep;
	char		*slash;
	char		 name[MAXPATHLEN];

	if ((slash = strrchr(path, '/')) == NULL) {
		return -ENOENT;
	}
	if ((ep = virtdir_find_tgt(&tree, path, (int)(slash - path) - 1)) == NULL) {
		return -EEXIST;
	}
	(void) snprintf(name, sizeof(name), "%s/%s/%s", virtdir_rootdir(&tree), ep->name, slash + 1);
	if (link(name, tgt) < 0) {
		return -errno;
	}
	add_entry(&tree, name, 'f');	/* XXX */
	return 0;
}

/* read the contents of a symbolic link */
static int
icfs_readlink(const char *path, char *buf, size_t size)
{
	virt_dirent_t	*ep;
	char		 name[MAXPATHLEN];

	if ((ep = virtdir_find_tgt(&tree, path, strlen(path))) == NULL) {
		return -ENOENT;
	}
	(void) snprintf(name, sizeof(name), "%s/%s", virtdir_rootdir(&tree), ep->name);
	if (readlink(name, buf, size) < 0) {
		return -errno;
	}
	return 0;
}

/* remove a directory */
static int
icfs_rmdir(const char *path)
{
	virt_dirent_t	*ep;
	char		 name[MAXPATHLEN];

	if ((ep = virtdir_find_tgt(&tree, path, strlen(path))) == NULL) {
		return -ENOENT;
	}
	(void) snprintf(name, sizeof(name), "%s/%s", virtdir_rootdir(&tree), ep->name);
	if (rmdir(name) < 0) {
		return -errno;
	}
	/* XXX - delete entry */
	return 0;
}

/* truncate a file */
static int
icfs_truncate(const char *path, off_t size)
{
	virt_dirent_t	*ep;
	char		 name[MAXPATHLEN];

	if ((ep = virtdir_find_tgt(&tree, path, strlen(path))) == NULL) {
		return -ENOENT;
	}
	(void) snprintf(name, sizeof(name), "%s/%s", virtdir_rootdir(&tree), ep->name);
	if (truncate(name, size) < 0) {
		return -errno;
	}
	return 0;
}

/* set utimes on a file */
static int
icfs_utime(const char *path, struct utimbuf *t)
{
	virt_dirent_t	*ep;
	char		 name[MAXPATHLEN];

	if ((ep = virtdir_find_tgt(&tree, path, strlen(path))) == NULL) {
		return -ENOENT;
	}
	(void) snprintf(name, sizeof(name), "%s/%s", virtdir_rootdir(&tree), ep->name);
	if (utime(name, t) < 0) {
		return -errno;
	}
	return 0;
}

/* operations struct */
static struct fuse_operations icfs_oper = {
	.getattr = icfs_getattr,
	.readlink = icfs_readlink,
	.mknod = icfs_mknod,
	.mkdir = icfs_mkdir,
	.unlink = icfs_unlink,
	.rmdir = icfs_rmdir,
	.symlink = icfs_symlink,
	.rename = icfs_rename,
	.link = icfs_link,
	.chmod = icfs_chmod,
	.chown = icfs_chown,
	.truncate = icfs_truncate,
	.utime = icfs_utime,
	.open = icfs_open,
	.read = icfs_read,
	.write = icfs_write,
	.statfs = icfs_statfs,
	.readdir = icfs_readdir,
	.access = icfs_access,
	.create = icfs_create
};

/* build up a virtdir from the information in the file system */
static int
dodir(virtdir_t *tp, char *rootdir, const char *subdir)
{
	struct dirent	*dp;
	struct stat	 st;
	struct stat	 dir;
	struct stat	 f;
	struct stat	 l;
	char		 icname[MAXPATHLEN];
	char		 name[MAXPATHLEN];
	char		 type;
	DIR		*dirp;
	int		 len;

	if (tp->v == NULL) {
		(void) stat(".", &dir);
		(void) memcpy(&f, &dir, sizeof(f));
		f.st_mode = S_IFREG | 0644;
		(void) memcpy(&l, &f, sizeof(l));
		l.st_mode = S_IFLNK | 0755;
		virtdir_init(tp, rootdir, &dir, &f, &l);
		virtdir_add(tp, "/", 1, 'd', "/", 1);
	}
	(void) snprintf(name, sizeof(name), "%s/%s", rootdir, subdir);
	if ((dirp = opendir(name)) == NULL) {
		warn("dodir: can't opendir `%s'", name);
		return 0;
	}
	len = strlen(tp->rootdir);
	while ((dp = readdir(dirp)) != NULL) {
		if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
			continue;
		}
		(void) snprintf(name, sizeof(name), "%s%s%s%s%s", rootdir, (subdir[0] == 0x0) ? "" : "/", subdir,  "/", dp->d_name);
		if (stat(name, &st) < 0) {
			warnx("can't stat `%s'", name);
			continue;
		}
		switch (st.st_mode & S_IFMT) {
		case S_IFDIR:
			type = 'd';
			break;
		case S_IFREG:
			type = 'f';
			break;
		case S_IFLNK:
			type = 'l';
			break;
		case S_IFBLK:
			type = 'b';
			break;
		case S_IFCHR:
			type = 'c';
			break;
		default:
			type = '?';
			break;
		}
		if (!virtdir_find(tp, &name[len], strlen(name) - len)) {
			strtolower(&name[len], icname, sizeof(icname));
			virtdir_add(tp, &name[len], strlen(name) - len, type, icname,
				strlen(icname));
		}
		if (type == 'd') {
			dodir(tp, rootdir, &name[len + 1]);
		}
	}
	(void) closedir(dirp);
	return 1;
}

int 
main(int argc, char **argv)
{
	int	 i;

	while ((i = getopt(argc, argv, "f:v")) != -1) {
		switch(i) {
		case 'v':
			verbose = 1;
			break;
		}
	}
#if 0
	(void) daemon(1, 1);
#endif
	dodir(&tree, argv[optind], "");
	return fuse_main(argc, argv, &icfs_oper, NULL);
}
