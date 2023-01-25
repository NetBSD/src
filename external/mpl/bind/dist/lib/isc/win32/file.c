/*	$NetBSD: file.c,v 1.9 2023/01/25 21:43:32 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#undef rename
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <io.h>
#include <limits.h>
#include <process.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/utime.h>

#include <isc/file.h>
#include <isc/md.h>
#include <isc/mem.h>
#include <isc/platform.h>
#include <isc/print.h>
#include <isc/random.h>
#include <isc/result.h>
#include <isc/stat.h>
#include <isc/string.h>
#include <isc/time.h>
#include <isc/util.h>

#include "errno2result.h"

static const char alphnum[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuv"
			      "wxyz0123456789";

/*
 * Emulate UNIX mkstemp, which returns an open FD to the new file
 *
 */
static int
gettemp(char *path, bool binary, int *doopen) {
	char *start, *trv;
	struct stat sbuf;
	int flags = O_CREAT | O_EXCL | O_RDWR;

	if (binary) {
		flags |= _O_BINARY;
	}

	trv = strrchr(path, 'X');
	trv++;
	/* extra X's get set to 0's */
	while (*--trv == 'X') {
		uint32_t which = isc_random_uniform(sizeof(alphnum) - 1);
		*trv = alphnum[which];
	}
	/*
	 * check the target directory; if you have six X's and it
	 * doesn't exist this runs for a *very* long time.
	 */
	for (start = trv + 1;; --trv) {
		if (trv <= path) {
			break;
		}
		if (*trv == '\\') {
			*trv = '\0';
			if (stat(path, &sbuf)) {
				return (0);
			}
			if (!S_ISDIR(sbuf.st_mode)) {
				errno = ENOTDIR;
				return (0);
			}
			*trv = '\\';
			break;
		}
	}

	for (;;) {
		if (doopen) {
			if ((*doopen = open(path, flags,
					    _S_IREAD | _S_IWRITE)) >= 0)
			{
				return (1);
			}
			if (errno != EEXIST) {
				return (0);
			}
		} else if (stat(path, &sbuf)) {
			return (errno == ENOENT ? 1 : 0);
		}

		/* tricky little algorithm for backward compatibility */
		for (trv = start;;) {
			if (!*trv) {
				return (0);
			}
			if (*trv == 'z') {
				*trv++ = 'a';
			} else {
				if (isdigit((unsigned char)*trv)) {
					*trv = 'a';
				} else {
					++*trv;
				}
				break;
			}
		}
	}
	/*NOTREACHED*/
}

static int
mkstemp(char *path, bool binary) {
	int fd;

	return (gettemp(path, binary, &fd) ? fd : -1);
}

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

/*
 * isc_file_safemovefile is needed to be defined here to ensure that
 * any file with the new name is renamed to a backup name and then the
 * rename is done. If all goes well then the backup can be deleted,
 * otherwise it gets renamed back.
 */

int
isc_file_safemovefile(const char *oldname, const char *newname) {
	BOOL filestatus;
	char buf[512];
	struct stat sbuf;
	BOOL exists = FALSE;
	int tmpfd;

	/*
	 * Make sure we have something to do
	 */
	if (stat(oldname, &sbuf) != 0) {
		errno = ENOENT;
		return (-1);
	}

	/*
	 * Rename to a backup the new file if it still exists
	 */
	if (stat(newname, &sbuf) == 0) {
		exists = TRUE;
		strlcpy(buf, newname, sizeof(buf));
		strlcat(buf, ".XXXXX", sizeof(buf));
		tmpfd = mkstemp(buf, true);
		if (tmpfd > 0) {
			_close(tmpfd);
		}
		(void)DeleteFile(buf);
		_chmod(newname, _S_IREAD | _S_IWRITE);

		filestatus = MoveFile(newname, buf);
	}
	/* Now rename the file to the new name
	 */
	_chmod(oldname, _S_IREAD | _S_IWRITE);

	filestatus = MoveFile(oldname, newname);
	if (filestatus == 0) {
		/*
		 * Try to rename the backup back to the original name
		 * if the backup got created
		 */
		if (exists == TRUE) {
			filestatus = MoveFile(buf, newname);
			if (filestatus == 0) {
				errno = EACCES;
			}
		}
		return (-1);
	}

	/*
	 * Delete the backup file if it got created
	 */
	if (exists == TRUE) {
		(void)DeleteFile(buf);
	}
	return (0);
}

isc_result_t
isc_file_getmodtime(const char *file, isc_time_t *time) {
	int fh;

	REQUIRE(file != NULL);
	REQUIRE(time != NULL);

	if ((fh = open(file, _O_RDONLY | _O_BINARY)) < 0) {
		return (isc__errno2result(errno));
	}

	if (!GetFileTime((HANDLE)_get_osfhandle(fh), NULL, NULL,
			 &time->absolute))
	{
		close(fh);
		errno = EINVAL;
		return (isc__errno2result(errno));
	}
	close(fh);
	return (ISC_R_SUCCESS);
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
isc_file_settime(const char *file, isc_time_t *time) {
	int fh;

	REQUIRE(file != NULL && time != NULL);

	if ((fh = open(file, _O_RDWR | _O_BINARY)) < 0) {
		return (isc__errno2result(errno));
	}

	/*
	 * Set the date via the filedate system call and return.  Failing
	 * this call implies the new file times are not supported by the
	 * underlying file system.
	 */
	if (!SetFileTime((HANDLE)_get_osfhandle(fh), NULL, &time->absolute,
			 &time->absolute))
	{
		close(fh);
		errno = EINVAL;
		return (isc__errno2result(errno));
	}

	close(fh);
	return (ISC_R_SUCCESS);
}

#undef TEMPLATE
#define TEMPLATE "XXXXXXXXXX.tmp" /* 14 characters. */

isc_result_t
isc_file_mktemplate(const char *path, char *buf, size_t buflen) {
	return (isc_file_template(path, TEMPLATE, buf, buflen));
}

isc_result_t
isc_file_template(const char *path, const char *templet, char *buf,
		  size_t buflen) {
	char *s;

	REQUIRE(templet != NULL);
	REQUIRE(buf != NULL);

	if (path == NULL) {
		path = "";
	}

	s = strrchr(templet, '\\');
	if (s != NULL) {
		templet = s + 1;
	}

	s = strrchr(path, '\\');

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

isc_result_t
isc_file_renameunique(const char *file, char *templet) {
	int fd;
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(file != NULL);
	REQUIRE(templet != NULL);

	fd = mkstemp(templet, true);
	if (fd == -1) {
		result = isc__errno2result(errno);
	} else {
		close(fd);
	}

	if (result == ISC_R_SUCCESS) {
		int res;
		res = isc_file_safemovefile(file, templet);
		if (res != 0) {
			result = isc__errno2result(errno);
			(void)unlink(templet);
		}
	}
	return (result);
}

static isc_result_t
openuniquemode(char *templet, int mode, bool binary, FILE **fp) {
	int fd;
	FILE *f;
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(templet != NULL);
	REQUIRE(fp != NULL && *fp == NULL);

	/*
	 * Win32 does not have mkstemp. Using emulation above.
	 */
	fd = mkstemp(templet, binary);

	if (fd == -1) {
		result = isc__errno2result(errno);
	}
	if (result == ISC_R_SUCCESS) {
#if 1
		UNUSED(mode);
#else  /* if 1 */
		(void)fchmod(fd, mode);
#endif /* if 1 */
		f = fdopen(fd, binary ? "wb+" : "w+");
		if (f == NULL) {
			result = isc__errno2result(errno);
			(void)remove(templet);
			(void)close(fd);
		} else {
			*fp = f;
		}
	}

	return (result);
}

isc_result_t
isc_file_openuniqueprivate(char *templet, FILE **fp) {
	int mode = _S_IREAD | _S_IWRITE;
	return (openuniquemode(templet, mode, false, fp));
}

isc_result_t
isc_file_openunique(char *templet, FILE **fp) {
	int mode = _S_IREAD | _S_IWRITE;
	return (openuniquemode(templet, mode, false, fp));
}

isc_result_t
isc_file_openuniquemode(char *templet, int mode, FILE **fp) {
	return (openuniquemode(templet, mode, false, fp));
}

isc_result_t
isc_file_bopenuniqueprivate(char *templet, FILE **fp) {
	int mode = _S_IREAD | _S_IWRITE;
	return (openuniquemode(templet, mode, true, fp));
}

isc_result_t
isc_file_bopenunique(char *templet, FILE **fp) {
	int mode = _S_IREAD | _S_IWRITE;
	return (openuniquemode(templet, mode, true, fp));
}

isc_result_t
isc_file_bopenuniquemode(char *templet, int mode, FILE **fp) {
	return (openuniquemode(templet, mode, true, fp));
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

	r = isc_file_safemovefile(oldname, newname);
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
	 * This function returns success if filename is a directory.
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
	/*
	 * Look for c:\path\... style, c:/path/... or \\computer\shar\path...
	 * the UNC style file specs
	 */
	if ((filename[0] == '\\') && (filename[1] == '\\')) {
		return (true);
	}
	if (isalpha(filename[0]) && filename[1] == ':' && filename[2] == '\\') {
		return (true);
	}
	if (isalpha(filename[0]) && filename[1] == ':' && filename[2] == '/') {
		return (true);
	}
	return (false);
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
	if (filename[0] == '\\') {
		return (true);
	}
	if (filename[0] == '/') {
		return (true);
	}
	if (isc_file_iscurrentdir(filename)) {
		return (true);
	}
	return (false);
}

const char *
isc_file_basename(const char *filename) {
	char *s;

	REQUIRE(filename != NULL);

	s = strrchr(filename, '\\');
	if (s == NULL) {
		return (filename);
	}
	return (s + 1);
}

isc_result_t
isc_file_progname(const char *filename, char *progname, size_t namelen) {
	const char *s;
	const char *p;
	size_t len;

	REQUIRE(filename != NULL);
	REQUIRE(progname != NULL);

	/*
	 * Strip the path from the name
	 */
	s = isc_file_basename(filename);
	if (s == NULL) {
		return (ISC_R_NOSPACE);
	}

	/*
	 * Strip any and all suffixes
	 */
	p = strchr(s, '.');
	if (p == NULL) {
		if (namelen <= strlen(s)) {
			return (ISC_R_NOSPACE);
		}

		strlcpy(progname, s, namelen);
		return (ISC_R_SUCCESS);
	}

	/*
	 * Copy the result to the buffer
	 */
	len = p - s;
	if (len >= namelen) {
		return (ISC_R_NOSPACE);
	}

	/* Copy up to 'len' bytes and NUL terminate. */
	strlcpy(progname, s, ISC_MIN(len + 1, namelen));
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_file_absolutepath(const char *filename, char *path, size_t pathlen) {
	char *ptrname;
	DWORD retval;

	REQUIRE(filename != NULL);
	REQUIRE(path != NULL);

	retval = GetFullPathName(filename, (DWORD)pathlen, path, &ptrname);

	/* Something went wrong in getting the path */
	if (retval == 0) {
		return (ISC_R_NOTFOUND);
	}
	/* Caller needs to provide a larger buffer to contain the string */
	if (retval >= pathlen) {
		return (ISC_R_NOSPACE);
	}
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_file_truncate(const char *filename, isc_offset_t size) {
	int fh;

	REQUIRE(filename != NULL && size >= 0);

	if ((fh = open(filename, _O_RDWR | _O_BINARY)) < 0) {
		return (isc__errno2result(errno));
	}

	if (_chsize(fh, size) != 0) {
		close(fh);
		return (isc__errno2result(errno));
	}
	close(fh);

	return (ISC_R_SUCCESS);
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
		   char const **basename) {
	char *dir;
	const char *file, *slash;
	char *backslash;

	slash = strrchr(path, '/');

	backslash = strrchr(path, '\\');
	if ((slash != NULL && backslash != NULL && backslash > slash) ||
	    (slash == NULL && backslash != NULL))
	{
		slash = backslash;
	}

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
	*basename = file;

	return (ISC_R_SUCCESS);
}

void *
isc_file_mmap(void *addr, size_t len, int prot, int flags, int fd,
	      off_t offset) {
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

	ret = read(fd, buf, (unsigned int)len);
	if (ret != (ssize_t)len) {
		free(buf);
		buf = NULL;
	}

	return (buf);
}

int
isc_file_munmap(void *addr, size_t len) {
	UNUSED(len);
	free(addr);
	return (0);
}

#define DISALLOW "\\/:ABCDEFGHIJKLMNOPQRSTUVWXYZ"

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
	if (l < 65) {
		l = 65;
	}

	if (dir != NULL) {
		l += strlen(dir) + 1;
	}
	if (ext != NULL) {
		l += strlen(ext) + 1;
	}

	if (l > length || l > PATH_MAX) {
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

/*
 * Based on http://blog.aaronballman.com/2011/08/how-to-check-access-rights/
 */
bool
isc_file_isdirwritable(const char *path) {
	DWORD length = 0;
	HANDLE hToken = NULL;
	PSECURITY_DESCRIPTOR security = NULL;
	bool answer = false;

	if (isc_file_isdirectory(path) != ISC_R_SUCCESS) {
		return (answer);
	}

	/*
	 * Figure out buffer size. GetFileSecurity() should not succeed.
	 */
	if (GetFileSecurity(path,
			    OWNER_SECURITY_INFORMATION |
				    GROUP_SECURITY_INFORMATION |
				    DACL_SECURITY_INFORMATION,
			    NULL, 0, &length))
	{
		return (answer);
	}

	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
		return (answer);
	}

	security = malloc(length);
	if (security == NULL) {
		return (answer);
	}

	/*
	 * GetFileSecurity() should succeed.
	 */
	if (!GetFileSecurity(path,
			     OWNER_SECURITY_INFORMATION |
				     GROUP_SECURITY_INFORMATION |
				     DACL_SECURITY_INFORMATION,
			     security, length, &length))
	{
		return (answer);
	}

	if (OpenProcessToken(GetCurrentProcess(),
			     TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_DUPLICATE |
				     STANDARD_RIGHTS_READ,
			     &hToken))
	{
		HANDLE hImpersonatedToken = NULL;

		if (DuplicateToken(hToken, SecurityImpersonation,
				   &hImpersonatedToken))
		{
			GENERIC_MAPPING mapping;
			PRIVILEGE_SET privileges = { 0 };
			DWORD grantedAccess = 0;
			DWORD privilegesLength = sizeof(privileges);
			BOOL result = FALSE;
			DWORD genericAccessRights = GENERIC_WRITE;

			mapping.GenericRead = FILE_GENERIC_READ;
			mapping.GenericWrite = FILE_GENERIC_WRITE;
			mapping.GenericExecute = FILE_GENERIC_EXECUTE;
			mapping.GenericAll = FILE_ALL_ACCESS;

			MapGenericMask(&genericAccessRights, &mapping);
			if (AccessCheck(security, hImpersonatedToken,
					genericAccessRights, &mapping,
					&privileges, &privilegesLength,
					&grantedAccess, &result))
			{
				answer = result;
			}
			CloseHandle(hImpersonatedToken);
		}
		CloseHandle(hToken);
	}
	free(security);
	return (answer);
}
