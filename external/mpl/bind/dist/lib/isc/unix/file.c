/*	$NetBSD: file.c,v 1.7 2023/01/25 21:43:31 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*
 * Portions Copyright (c) 1987, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*! \file */

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>   /* Required for utimes on some platforms. */
#include <unistd.h> /* Required for mkstemp on NetBSD. */

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif /* ifdef HAVE_SYS_MMAN_H */

#include <isc/dir.h>
#include <isc/file.h>
#include <isc/log.h>
#include <isc/md.h>
#include <isc/mem.h>
#include <isc/platform.h>
#include <isc/print.h>
#include <isc/random.h>
#include <isc/string.h>
#include <isc/time.h>
#include <isc/util.h>

#include "errno2result.h"

/*
 * XXXDCL As the API for accessing file statistics undoubtedly gets expanded,
 * it might be good to provide a mechanism that allows for the results
 * of a previous stat() to be used again without having to do another stat,
 * such as perl's mechanism of using "_" in place of a file name to indicate
 * that the results of the last stat should be used.  But then you get into
 * annoying MP issues.   BTW, Win32 has stat().
 */
static isc_result_t
file_stats(const char *file, struct stat *stats) {
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(file != NULL);
	REQUIRE(stats != NULL);

	if (stat(file, stats) != 0) {
		result = isc__errno2result(errno);
	}

	return (result);
}

static isc_result_t
fd_stats(int fd, struct stat *stats) {
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(stats != NULL);

	if (fstat(fd, stats) != 0) {
		result = isc__errno2result(errno);
	}

	return (result);
}

isc_result_t
isc_file_getsizefd(int fd, off_t *size) {
	isc_result_t result;
	struct stat stats;

	REQUIRE(size != NULL);

	result = fd_stats(fd, &stats);

	if (result == ISC_R_SUCCESS) {
		*size = stats.st_size;
	}

	return (result);
}

isc_result_t
isc_file_mode(const char *file, mode_t *modep) {
	isc_result_t result;
	struct stat stats;

	REQUIRE(modep != NULL);

	result = file_stats(file, &stats);
	if (result == ISC_R_SUCCESS) {
		*modep = (stats.st_mode & 07777);
	}

	return (result);
}

isc_result_t
isc_file_getmodtime(const char *file, isc_time_t *modtime) {
	isc_result_t result;
	struct stat stats;

	REQUIRE(file != NULL);
	REQUIRE(modtime != NULL);

	result = file_stats(file, &stats);

	if (result == ISC_R_SUCCESS) {
#if defined(HAVE_STAT_NSEC)
		isc_time_set(modtime, stats.st_mtime, stats.st_mtim.tv_nsec);
#else  /* if defined(HAVE_STAT_NSEC) */
		isc_time_set(modtime, stats.st_mtime, 0);
#endif /* if defined(HAVE_STAT_NSEC) */
	}

	return (result);
}

isc_result_t
isc_file_getsize(const char *file, off_t *size) {
	isc_result_t result;
	struct stat stats;

	REQUIRE(file != NULL);
	REQUIRE(size != NULL);

	result = file_stats(file, &stats);

	if (result == ISC_R_SUCCESS) {
		*size = stats.st_size;
	}

	return (result);
}

isc_result_t
isc_file_settime(const char *file, isc_time_t *when) {
	struct timeval times[2];

	REQUIRE(file != NULL && when != NULL);

	/*
	 * tv_sec is at least a 32 bit quantity on all platforms we're
	 * dealing with, but it is signed on most (all?) of them,
	 * so we need to make sure the high bit isn't set.  This unfortunately
	 * loses when either:
	 *   * tv_sec becomes a signed 64 bit integer but long is 32 bits
	 *	and isc_time_seconds > LONG_MAX, or
	 *   * isc_time_seconds is changed to be > 32 bits but long is 32 bits
	 *      and isc_time_seconds has at least 33 significant bits.
	 */
	times[0].tv_sec = times[1].tv_sec = (long)isc_time_seconds(when);

	/*
	 * Here is the real check for the high bit being set.
	 */
	if ((times[0].tv_sec &
	     (1ULL << (sizeof(times[0].tv_sec) * CHAR_BIT - 1))) != 0)
	{
		return (ISC_R_RANGE);
	}

	/*
	 * isc_time_nanoseconds guarantees a value that divided by 1000 will
	 * fit into the minimum possible size tv_usec field.
	 */
	times[0].tv_usec = times[1].tv_usec =
		(int32_t)(isc_time_nanoseconds(when) / 1000);

	if (utimes(file, times) < 0) {
		return (isc__errno2result(errno));
	}

	return (ISC_R_SUCCESS);
}

#undef TEMPLATE
#define TEMPLATE "tmp-XXXXXXXXXX" /*%< 14 characters. */

isc_result_t
isc_file_mktemplate(const char *path, char *buf, size_t buflen) {
	return (isc_file_template(path, TEMPLATE, buf, buflen));
}

isc_result_t
isc_file_template(const char *path, const char *templet, char *buf,
		  size_t buflen) {
	const char *s;

	REQUIRE(templet != NULL);
	REQUIRE(buf != NULL);

	if (path == NULL) {
		path = "";
	}

	s = strrchr(templet, '/');
	if (s != NULL) {
		templet = s + 1;
	}

	s = strrchr(path, '/');

	if (s != NULL) {
		size_t prefixlen = s - path + 1;
		if ((prefixlen + strlen(templet) + 1) > buflen) {
			return (ISC_R_NOSPACE);
		}

		/* Copy 'prefixlen' bytes and NUL terminate. */
		strlcpy(buf, path, ISC_MIN(prefixlen + 1, buflen));
		strlcat(buf, templet, buflen);
	} else {
		if ((strlen(templet) + 1) > buflen) {
			return (ISC_R_NOSPACE);
		}

		strlcpy(buf, templet, buflen);
	}

	return (ISC_R_SUCCESS);
}

static const char alphnum[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuv"
			      "wxyz0123456789";

isc_result_t
isc_file_renameunique(const char *file, char *templet) {
	char *x;
	char *cp;

	REQUIRE(file != NULL);
	REQUIRE(templet != NULL);

	cp = templet;
	while (*cp != '\0') {
		cp++;
	}
	if (cp == templet) {
		return (ISC_R_FAILURE);
	}

	x = cp--;
	while (cp >= templet && *cp == 'X') {
		*cp = alphnum[isc_random_uniform(sizeof(alphnum) - 1)];
		x = cp--;
	}
	while (link(file, templet) == -1) {
		if (errno != EEXIST) {
			return (isc__errno2result(errno));
		}
		for (cp = x;;) {
			const char *t;
			if (*cp == '\0') {
				return (ISC_R_FAILURE);
			}
			t = strchr(alphnum, *cp);
			if (t == NULL || *++t == '\0') {
				*cp++ = alphnum[0];
			} else {
				*cp = *t;
				break;
			}
		}
	}
	if (unlink(file) < 0) {
		if (errno != ENOENT) {
			return (isc__errno2result(errno));
		}
	}
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_file_openunique(char *templet, FILE **fp) {
	int mode = S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
	return (isc_file_openuniquemode(templet, mode, fp));
}

isc_result_t
isc_file_openuniqueprivate(char *templet, FILE **fp) {
	int mode = S_IWUSR | S_IRUSR;
	return (isc_file_openuniquemode(templet, mode, fp));
}

isc_result_t
isc_file_openuniquemode(char *templet, int mode, FILE **fp) {
	int fd;
	FILE *f;
	isc_result_t result = ISC_R_SUCCESS;
	char *x;
	char *cp;

	REQUIRE(templet != NULL);
	REQUIRE(fp != NULL && *fp == NULL);

	cp = templet;
	while (*cp != '\0') {
		cp++;
	}
	if (cp == templet) {
		return (ISC_R_FAILURE);
	}

	x = cp--;
	while (cp >= templet && *cp == 'X') {
		*cp = alphnum[isc_random_uniform(sizeof(alphnum) - 1)];
		x = cp--;
	}

	while ((fd = open(templet, O_RDWR | O_CREAT | O_EXCL, mode)) == -1) {
		if (errno != EEXIST) {
			return (isc__errno2result(errno));
		}
		for (cp = x;;) {
			char *t;
			if (*cp == '\0') {
				return (ISC_R_FAILURE);
			}
			t = strchr(alphnum, *cp);
			if (t == NULL || *++t == '\0') {
				*cp++ = alphnum[0];
			} else {
				*cp = *t;
				break;
			}
		}
	}
	f = fdopen(fd, "w+");
	if (f == NULL) {
		result = isc__errno2result(errno);
		if (remove(templet) < 0) {
			isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
				      ISC_LOGMODULE_FILE, ISC_LOG_ERROR,
				      "remove '%s': failed", templet);
		}
		(void)close(fd);
	} else {
		*fp = f;
	}

	return (result);
}

isc_result_t
isc_file_bopenunique(char *templet, FILE **fp) {
	int mode = S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
	return (isc_file_openuniquemode(templet, mode, fp));
}

isc_result_t
isc_file_bopenuniqueprivate(char *templet, FILE **fp) {
	int mode = S_IWUSR | S_IRUSR;
	return (isc_file_openuniquemode(templet, mode, fp));
}

isc_result_t
isc_file_bopenuniquemode(char *templet, int mode, FILE **fp) {
	return (isc_file_openuniquemode(templet, mode, fp));
}

isc_result_t
isc_file_remove(const char *filename) {
	int r;

	REQUIRE(filename != NULL);

	r = unlink(filename);
	if (r == 0) {
		return (ISC_R_SUCCESS);
	} else {
		return (isc__errno2result(errno));
	}
}

isc_result_t
isc_file_rename(const char *oldname, const char *newname) {
	int r;

	REQUIRE(oldname != NULL);
	REQUIRE(newname != NULL);

	r = rename(oldname, newname);
	if (r == 0) {
		return (ISC_R_SUCCESS);
	} else {
		return (isc__errno2result(errno));
	}
}

bool
isc_file_exists(const char *pathname) {
	struct stat stats;

	REQUIRE(pathname != NULL);

	return (file_stats(pathname, &stats) == ISC_R_SUCCESS);
}

isc_result_t
isc_file_isplainfile(const char *filename) {
	/*
	 * This function returns success if filename is a plain file.
	 */
	struct stat filestat;
	memset(&filestat, 0, sizeof(struct stat));

	if ((stat(filename, &filestat)) == -1) {
		return (isc__errno2result(errno));
	}

	if (!S_ISREG(filestat.st_mode)) {
		return (ISC_R_INVALIDFILE);
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_file_isplainfilefd(int fd) {
	/*
	 * This function returns success if filename is a plain file.
	 */
	struct stat filestat;
	memset(&filestat, 0, sizeof(struct stat));

	if ((fstat(fd, &filestat)) == -1) {
		return (isc__errno2result(errno));
	}

	if (!S_ISREG(filestat.st_mode)) {
		return (ISC_R_INVALIDFILE);
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_file_isdirectory(const char *filename) {
	/*
	 * This function returns success if filename exists and is a
	 * directory.
	 */
	struct stat filestat;
	memset(&filestat, 0, sizeof(struct stat));

	if ((stat(filename, &filestat)) == -1) {
		return (isc__errno2result(errno));
	}

	if (!S_ISDIR(filestat.st_mode)) {
		return (ISC_R_INVALIDFILE);
	}

	return (ISC_R_SUCCESS);
}

bool
isc_file_isabsolute(const char *filename) {
	REQUIRE(filename != NULL);
	return (filename[0] == '/');
}

bool
isc_file_iscurrentdir(const char *filename) {
	REQUIRE(filename != NULL);
	return (filename[0] == '.' && filename[1] == '\0');
}

bool
isc_file_ischdiridempotent(const char *filename) {
	REQUIRE(filename != NULL);
	if (isc_file_isabsolute(filename)) {
		return (true);
	}
	if (isc_file_iscurrentdir(filename)) {
		return (true);
	}
	return (false);
}

const char *
isc_file_basename(const char *filename) {
	const char *s;

	REQUIRE(filename != NULL);

	s = strrchr(filename, '/');
	if (s == NULL) {
		return (filename);
	}

	return (s + 1);
}

isc_result_t
isc_file_progname(const char *filename, char *buf, size_t buflen) {
	const char *base;
	size_t len;

	REQUIRE(filename != NULL);
	REQUIRE(buf != NULL);

	base = isc_file_basename(filename);
	len = strlen(base) + 1;

	if (len > buflen) {
		return (ISC_R_NOSPACE);
	}
	memmove(buf, base, len);

	return (ISC_R_SUCCESS);
}

/*
 * Put the absolute name of the current directory into 'dirname', which is
 * a buffer of at least 'length' characters.  End the string with the
 * appropriate path separator, such that the final product could be
 * concatenated with a relative pathname to make a valid pathname string.
 */
static isc_result_t
dir_current(char *dirname, size_t length) {
	char *cwd;
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(dirname != NULL);
	REQUIRE(length > 0U);

	cwd = getcwd(dirname, length);

	if (cwd == NULL) {
		if (errno == ERANGE) {
			result = ISC_R_NOSPACE;
		} else {
			result = isc__errno2result(errno);
		}
	} else {
		if (strlen(dirname) + 1 == length) {
			result = ISC_R_NOSPACE;
		} else if (dirname[1] != '\0') {
			strlcat(dirname, "/", length);
		}
	}

	return (result);
}

isc_result_t
isc_file_absolutepath(const char *filename, char *path, size_t pathlen) {
	isc_result_t result;
	result = dir_current(path, pathlen);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	if (strlen(path) + strlen(filename) + 1 > pathlen) {
		return (ISC_R_NOSPACE);
	}
	strlcat(path, filename, pathlen);
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_file_truncate(const char *filename, isc_offset_t size) {
	isc_result_t result = ISC_R_SUCCESS;

	if (truncate(filename, size) < 0) {
		result = isc__errno2result(errno);
	}
	return (result);
}

isc_result_t
isc_file_safecreate(const char *filename, FILE **fp) {
	isc_result_t result;
	int flags;
	struct stat sb;
	FILE *f;
	int fd;

	REQUIRE(filename != NULL);
	REQUIRE(fp != NULL && *fp == NULL);

	result = file_stats(filename, &sb);
	if (result == ISC_R_SUCCESS) {
		if ((sb.st_mode & S_IFREG) == 0) {
			return (ISC_R_INVALIDFILE);
		}
		flags = O_WRONLY | O_TRUNC;
	} else if (result == ISC_R_FILENOTFOUND) {
		flags = O_WRONLY | O_CREAT | O_EXCL;
	} else {
		return (result);
	}

	fd = open(filename, flags, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		return (isc__errno2result(errno));
	}

	f = fdopen(fd, "w");
	if (f == NULL) {
		result = isc__errno2result(errno);
		close(fd);
		return (result);
	}

	*fp = f;
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_file_splitpath(isc_mem_t *mctx, const char *path, char **dirname,
		   char const **bname) {
	char *dir;
	const char *file, *slash;

	if (path == NULL) {
		return (ISC_R_INVALIDFILE);
	}

	slash = strrchr(path, '/');

	if (slash == path) {
		file = ++slash;
		dir = isc_mem_strdup(mctx, "/");
	} else if (slash != NULL) {
		file = ++slash;
		dir = isc_mem_allocate(mctx, slash - path);
		strlcpy(dir, path, slash - path);
	} else {
		file = path;
		dir = isc_mem_strdup(mctx, ".");
	}

	if (dir == NULL) {
		return (ISC_R_NOMEMORY);
	}

	if (*file == '\0') {
		isc_mem_free(mctx, dir);
		return (ISC_R_INVALIDFILE);
	}

	*dirname = dir;
	*bname = file;

	return (ISC_R_SUCCESS);
}

void *
isc_file_mmap(void *addr, size_t len, int prot, int flags, int fd,
	      off_t offset) {
#ifdef HAVE_MMAP
	return (mmap(addr, len, prot, flags, fd, offset));
#else  /* ifdef HAVE_MMAP */
	void *buf;
	ssize_t ret;
	off_t end;

	UNUSED(addr);
	UNUSED(prot);
	UNUSED(flags);

	end = lseek(fd, 0, SEEK_END);
	lseek(fd, offset, SEEK_SET);
	if (end - offset < (off_t)len) {
		len = end - offset;
	}

	buf = malloc(len);
	if (buf == NULL) {
		return (NULL);
	}

	ret = read(fd, buf, len);
	if (ret != (ssize_t)len) {
		free(buf);
		buf = NULL;
	}

	return (buf);
#endif /* ifdef HAVE_MMAP */
}

int
isc_file_munmap(void *addr, size_t len) {
#ifdef HAVE_MMAP
	return (munmap(addr, len));
#else  /* ifdef HAVE_MMAP */
	UNUSED(len);

	free(addr);
	return (0);
#endif /* ifdef HAVE_MMAP */
}

#define DISALLOW "\\/ABCDEFGHIJKLMNOPQRSTUVWXYZ"

static isc_result_t
digest2hex(unsigned char *digest, unsigned int digestlen, char *hash,
	   size_t hashlen) {
	unsigned int i;
	int ret;
	for (i = 0; i < digestlen; i++) {
		size_t left = hashlen - i * 2;
		ret = snprintf(hash + i * 2, left, "%02x", digest[i]);
		if (ret < 0 || (size_t)ret >= left) {
			return (ISC_R_NOSPACE);
		}
	}
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_file_sanitize(const char *dir, const char *base, const char *ext,
		  char *path, size_t length) {
	char buf[PATH_MAX];
	unsigned char digest[ISC_MAX_MD_SIZE];
	unsigned int digestlen;
	char hash[ISC_MAX_MD_SIZE * 2 + 1];
	size_t l = 0;
	isc_result_t err;

	REQUIRE(base != NULL);
	REQUIRE(path != NULL);

	l = strlen(base) + 1;

	/*
	 * allow room for a full sha256 hash (64 chars
	 * plus null terminator)
	 */
	if (l < 65U) {
		l = 65;
	}

	if (dir != NULL) {
		l += strlen(dir) + 1;
	}
	if (ext != NULL) {
		l += strlen(ext) + 1;
	}

	if (l > length || l > (unsigned)PATH_MAX) {
		return (ISC_R_NOSPACE);
	}

	/* Check whether the full-length SHA256 hash filename exists */
	err = isc_md(ISC_MD_SHA256, (const unsigned char *)base, strlen(base),
		     digest, &digestlen);
	if (err != ISC_R_SUCCESS) {
		return (err);
	}

	err = digest2hex(digest, digestlen, hash, sizeof(hash));
	if (err != ISC_R_SUCCESS) {
		return (err);
	}

	snprintf(buf, sizeof(buf), "%s%s%s%s%s", dir != NULL ? dir : "",
		 dir != NULL ? "/" : "", hash, ext != NULL ? "." : "",
		 ext != NULL ? ext : "");
	if (isc_file_exists(buf)) {
		strlcpy(path, buf, length);
		return (ISC_R_SUCCESS);
	}

	/* Check for a truncated SHA256 hash filename */
	hash[16] = '\0';
	snprintf(buf, sizeof(buf), "%s%s%s%s%s", dir != NULL ? dir : "",
		 dir != NULL ? "/" : "", hash, ext != NULL ? "." : "",
		 ext != NULL ? ext : "");
	if (isc_file_exists(buf)) {
		strlcpy(path, buf, length);
		return (ISC_R_SUCCESS);
	}

	/*
	 * If neither hash filename already exists, then we'll use
	 * the original base name if it has no disallowed characters,
	 * or the truncated hash name if it does.
	 */
	if (strpbrk(base, DISALLOW) != NULL) {
		strlcpy(path, buf, length);
		return (ISC_R_SUCCESS);
	}

	snprintf(buf, sizeof(buf), "%s%s%s%s%s", dir != NULL ? dir : "",
		 dir != NULL ? "/" : "", base, ext != NULL ? "." : "",
		 ext != NULL ? ext : "");
	strlcpy(path, buf, length);
	return (ISC_R_SUCCESS);
}

bool
isc_file_isdirwritable(const char *path) {
	return (access(path, W_OK | X_OK) == 0);
}
