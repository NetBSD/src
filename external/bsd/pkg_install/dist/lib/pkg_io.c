/*	$NetBSD: pkg_io.c,v 1.1.1.1 2008/09/30 19:00:27 joerg Exp $	*/
/*-
 * Copyright (c) 2008 Joerg Sonnenberger <joerg@NetBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <nbcompat.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif

__RCSID("$NetBSD: pkg_io.c,v 1.1.1.1 2008/09/30 19:00:27 joerg Exp $");

#include <archive.h>
#include <archive_entry.h>
#if HAVE_ERR_H
#include <err.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#include <fetch.h>
#include <stdlib.h>

#include "lib.h"

struct fetch_archive {
	const char *url;
	fetchIO *fetch;
	char buffer[32768];
};

static int
fetch_archive_open(struct archive *a, void *client_data)
{
	struct fetch_archive *f = client_data;

	f->fetch = fetchGetURL(f->url, "");
	if (f->fetch == NULL)
		return ENOENT;
	return 0;
}

static ssize_t
fetch_archive_read(struct archive *a, void *client_data,
    const void **buffer)
{
	struct fetch_archive *f = client_data;
	
	*buffer = f->buffer;
	return fetchIO_read(f->fetch, f->buffer, sizeof(f->buffer));	
}

static int
fetch_archive_close(struct archive *a, void *client_data)
{
	struct fetch_archive *f = client_data;

	if (f->fetch != NULL)
		fetchIO_close(f->fetch);
	return 0;
}

struct archive *
open_remote_archive(const char *url, void **cookie)
{
	struct fetch_archive *f;
	struct archive *archive;

	f = malloc(sizeof(*f));
	if (f == NULL)
		err(2, "cannot allocate memory for remote archive");
	f->url = url;

	archive = archive_read_new();
	archive_read_support_compression_all(archive);
	archive_read_support_format_all(archive);
	if (archive_read_open(archive, f, fetch_archive_open, fetch_archive_read,
	    fetch_archive_close))
		errx(2, "cannot open archive: %s", archive_error_string(archive));

	*cookie = f;

	return archive;
}

void
close_remote_archive(void *cookie)
{
	free(cookie);
}

struct archive *
open_local_archive(const char *path, void **cookie)
{
	struct archive *archive;

	archive = archive_read_new();
	archive_read_support_compression_all(archive);
	archive_read_support_format_all(archive);
	if (archive_read_open_filename(archive, path, 1024))
		errx(2, "cannot open archive: %s",
		    archive_error_string(archive));
	*cookie = NULL;

	return archive;
}

void
close_local_archive(void *cookie)
{
}
