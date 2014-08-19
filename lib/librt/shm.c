/*	$NetBSD: shm.c,v 1.1.8.2 2014/08/20 00:02:20 tls Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mindaugas Rasiukevicius.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Interface for POSIX shared memory objects.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: shm.c,v 1.1.8.2 2014/08/20 00:02:20 tls Exp $");

#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

/*
 * Shared memory objects are supported using tmpfs.
 */
#define	SHMFS_DIR_PATH		"/var/shm"
#define	SHMFS_DIR_MODE		(S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO)
#define	SHMFS_OBJ_PREFIX	".shmobj_"

#define	MOUNT_SHMFS		MOUNT_TMPFS

static const char *		_shmfs_path = NULL;

static bool
_shm_check_fs(void)
{
	const char *shmfs = SHMFS_DIR_PATH;
	struct statvfs sv;
	struct stat st;

	if (statvfs1(shmfs, &sv, ST_NOWAIT) == -1) {
		return false;
	}
	if (strncmp(sv.f_fstypename, MOUNT_SHMFS, sizeof(sv.f_fstypename))) {
		return false;
	}

	if (lstat(shmfs, &st) == -1) {
		return false;
	}
	if ((st.st_mode & SHMFS_DIR_MODE) != SHMFS_DIR_MODE) {
		return false;
	}

	_shmfs_path = shmfs;
	return true;
}

static bool
_shm_get_path(char *buf, size_t len, const char *name)
{
	int ret;

	if (__predict_false(!_shmfs_path) && !_shm_check_fs()) {
		errno = ENOTSUP;
		return false;
	}

	/*
	 * As per POSIX: the name should begin with a slash character.
	 * We may disallow other slashes (implementation-defined behaviour).
	 */
	if (*name++ != '/' || strchr(name, '/') != NULL) {
		errno = EINVAL;
		return false;
	}

	ret = snprintf(buf, len, "%s/%s%s",
	    _shmfs_path, SHMFS_OBJ_PREFIX, name);

	if ((size_t)ret >= PATH_MAX) {
		errno = ENAMETOOLONG;
		return false;
	}
	return ret != -1;
}

int
shm_open(const char *name, int oflag, mode_t mode)
{
	char path[PATH_MAX + 1];

	if (!_shm_get_path(path, sizeof(path), name)) {
		return -1;
	}
	return open(path, oflag | O_CLOEXEC | O_NOFOLLOW, mode);
}

int
shm_unlink(const char *name)
{
	char path[PATH_MAX + 1];

	if (!_shm_get_path(path, sizeof(path), name)) {
		return -1;
	}
	return unlink(path);
}
