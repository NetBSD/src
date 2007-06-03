/*	$NetBSD: os.c,v 1.1.1.3.6.1 2007/06/03 17:20:27 wrstuden Exp $	*/

/*
 * Copyright (C) 2004  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* Id: os.c,v 1.4 2004/03/05 04:58:32 marka Exp */

#include <config.h>

#include <rndc/os.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <io.h>
#include <sys/stat.h>

int
set_user(FILE *fd, const char *user) {
	return (0);
}

/*
 * Note that the error code EOPNOTSUPP does not exist
 * on win32 so we are forced to fall back to using
 * ENOENT for now. WSAEOPNOTSUPP does exist but it
 * should only be used for sockets.
 */

FILE *
safe_create(const char *filename) {
	int fd;
	FILE *f;
        struct stat sb;

        if (stat(filename, &sb) == -1) {
                if (errno != ENOENT)
			return (NULL);
        } else if ((sb.st_mode & S_IFREG) == 0) {
		errno = ENOENT;
		return (NULL);
	}

	fd = open(filename, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
	if (fd == -1)
		return (NULL);
	f = fdopen(fd, "w");
	if (f == NULL)
		close(fd);
	return (f);
}
