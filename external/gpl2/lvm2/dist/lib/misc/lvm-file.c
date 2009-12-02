/*	$NetBSD: lvm-file.c,v 1.1.1.3 2009/12/02 00:26:44 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "lib.h"
#include "lvm-file.h"
#include "lvm-string.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <dirent.h>

/*
 * Creates a temporary filename, and opens a descriptor to the
 * file.  Both the filename and descriptor are needed so we can
 * rename the file after successfully writing it.  Grab
 * NFS-supported exclusive fcntl discretionary lock.
 */
int create_temp_name(const char *dir, char *buffer, size_t len, int *fd,
		     unsigned *seed)
{
	int i, num;
	pid_t pid;
	char hostname[255];
	struct flock lock = {
		.l_type = F_WRLCK,
		.l_whence = 0,
		.l_start = 0,
		.l_len = 0
	};

	num = rand_r(seed);
	pid = getpid();
	if (gethostname(hostname, sizeof(hostname)) < 0) {
		log_sys_error("gethostname", "");
		strcpy(hostname, "nohostname");
	}

	for (i = 0; i < 20; i++, num++) {

		if (dm_snprintf(buffer, len, "%s/.lvm_%s_%d_%d",
				 dir, hostname, pid, num) == -1) {
			log_error("Not enough space to build temporary file "
				  "string.");
			return 0;
		}

		*fd = open(buffer, O_CREAT | O_EXCL | O_WRONLY | O_APPEND,
			   S_IRUSR | S_IRGRP | S_IROTH |
			   S_IWUSR | S_IWGRP | S_IWOTH);
		if (*fd < 0)
			continue;

		if (!fcntl(*fd, F_SETLK, &lock))
			return 1;

		if (close(*fd))
			log_sys_error("close", buffer);
	}

	return 0;
}

/*
 * NFS-safe rename of a temporary file to a common name, designed
 * to avoid race conditions and not overwrite the destination if
 * it exists.
 *
 * Try to create the new filename as a hard link to the original.
 * Check the link count of the original file to see if it worked.
 * (Assumes nothing else touches our temporary file!)  If it
 * worked, unlink the old filename.
 */
int lvm_rename(const char *old, const char *new)
{
	struct stat buf;

	if (link(old, new)) {
		log_error("%s: rename to %s failed: %s", old, new,
			  strerror(errno));
		return 0;
	}

	if (stat(old, &buf)) {
		log_sys_error("stat", old);
		return 0;
	}

	if (buf.st_nlink != 2) {
		log_error("%s: rename to %s failed", old, new);
		return 0;
	}

	if (unlink(old)) {
		log_sys_error("unlink", old);
		return 0;
	}

	return 1;
}

int path_exists(const char *path)
{
	struct stat info;

	if (!*path)
		return 0;

	if (stat(path, &info) < 0)
		return 0;

	return 1;
}

int dir_exists(const char *path)
{
	struct stat info;

	if (!*path)
		return 0;

	if (stat(path, &info) < 0)
		return 0;

	if (!S_ISDIR(info.st_mode))
		return 0;

	return 1;
}

int is_empty_dir(const char *dir)
{
	struct dirent *dirent;
	DIR *d;

	if (!(d = opendir(dir))) {
		log_sys_error("opendir", dir);
		return 0;
	}

	while ((dirent = readdir(d)))
		if (strcmp(dirent->d_name, ".") && strcmp(dirent->d_name, ".."))
			break;

	if (closedir(d)) {
		log_sys_error("closedir", dir);
	}

	return dirent ? 0 : 1;
}

void sync_dir(const char *file)
{
	int fd;
	char *dir, *c;

	if (!(dir = dm_strdup(file))) {
		log_error("sync_dir failed in strdup");
		return;
	}

	if (!dir_exists(dir)) {
		c = dir + strlen(dir);
		while (*c != '/' && c > dir)
			c--;

		if (c == dir)
			*c++ = '.';

		*c = '\0';
	}

	if ((fd = open(dir, O_RDONLY)) == -1) {
		log_sys_error("open", dir);
		goto out;
	}

	if (fsync(fd) && (errno != EROFS) && (errno != EINVAL))
		log_sys_error("fsync", dir);

	if (close(fd))
		log_sys_error("close", dir);

      out:
	dm_free(dir);
}

/*
 * Attempt to obtain fcntl lock on a file, if necessary creating file first
 * or waiting.
 * Returns file descriptor on success, else -1.
 * mode is F_WRLCK or F_RDLCK
 */
int fcntl_lock_file(const char *file, short lock_type, int warn_if_read_only)
{
	int lockfd;
	char *dir;
	char *c;
	struct flock lock = {
		.l_type = lock_type,
		.l_whence = 0,
		.l_start = 0,
		.l_len = 0
	};

	if (!(dir = dm_strdup(file))) {
		log_error("fcntl_lock_file failed in strdup.");
		return -1;
	}

	if ((c = strrchr(dir, '/')))
		*c = '\0';

	if (!dm_create_dir(dir)) {
		dm_free(dir);
		return -1;
	}

	dm_free(dir);

	log_very_verbose("Locking %s (%s, %hd)", file,
			 (lock_type == F_WRLCK) ? "F_WRLCK" : "F_RDLCK",
			 lock_type);
	if ((lockfd = open(file, O_RDWR | O_CREAT, 0777)) < 0) {
		/* EACCES has been reported on NFS */
		if (warn_if_read_only || (errno != EROFS && errno != EACCES))
			log_sys_error("open", file);
		else
			stack;

		return -1;
	}

	if (fcntl(lockfd, F_SETLKW, &lock)) {
		log_sys_error("fcntl", file);
		close(lockfd);
		return -1;
	}

	return lockfd;
}

void fcntl_unlock_file(int lockfd)
{
	struct flock lock = {
		.l_type = F_UNLCK,
		.l_whence = 0,
		.l_start = 0,
		.l_len = 0
	};

	log_very_verbose("Unlocking fd %d", lockfd);

	if (fcntl(lockfd, F_SETLK, &lock) == -1)
		log_error("fcntl unlock failed on fd %d: %s", lockfd,
			  strerror(errno));

	if (close(lockfd))
		log_error("lock file close failed on fd %d: %s", lockfd,
			  strerror(errno));
}

int lvm_fclose(FILE *fp, const char *filename)
{
	if (!dm_fclose(fp))
		return 0;
	if (errno == 0)
		log_error("%s: write error", filename);
	else
		log_sys_error("write error", filename);
	return EOF;
}
