/*	$NetBSD: pkg_io.c,v 1.1.1.2 2009/02/02 20:44:07 joerg Exp $	*/
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

__RCSID("$NetBSD: pkg_io.c,v 1.1.1.2 2009/02/02 20:44:07 joerg Exp $");

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
	struct url *url;
	fetchIO *fetch;
	char buffer[32768];
};

static int
fetch_archive_open(struct archive *a, void *client_data)
{
	struct fetch_archive *f = client_data;

	f->fetch = fetchGet(f->url, fetch_flags);
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

static struct archive *
open_archive_by_url(struct url *url, void **cookie)
{
	struct fetch_archive *f;
	struct archive *a;

	f = xmalloc(sizeof(*f));
	f->url = url;

	a = archive_read_new();
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	if (archive_read_open(a, f, fetch_archive_open, fetch_archive_read,
	    fetch_archive_close)) {
		archive_read_close(a);
		free(f);
		return NULL;
	}

	*cookie = f;
	return a;
}

struct archive *
open_archive(const char *url, void **cookie)
{
	struct url *u;
	struct archive *a;

	if (!IS_URL(url)) {
		a = archive_read_new();
		archive_read_support_compression_all(a);
		archive_read_support_format_all(a);
		if (archive_read_open_filename(a, url, 1024)) {
			archive_read_close(a);
			return NULL;
		}
		*cookie = NULL;
		return a;
	}

	if ((u = fetchParseURL(url)) == NULL)
		return NULL;

	a = open_archive_by_url(u, cookie);

	fetchFreeURL(u);
	return a;
}

void
close_archive(void *cookie)
{
	free(cookie);
}

static int
strip_suffix(char *filename)
{
	size_t len;

	len = strlen(filename);
	if (len <= 4)
		return 0;
	if (strcmp(filename + len - 4, ".tgz") == 0 ||
	    strcmp(filename + len - 4, ".tbz") == 0) {
		filename[len - 4] = '\0';
		return 1;
	} else
		return 0;
}

static int
find_best_package(struct url *url, const char *pattern, struct url **best_url)
{
	char *cur_match, *url_pattern, *best_match = NULL;
	struct url_list ue;
	size_t i;

	if (*best_url) {
		if ((best_match = fetchUnquoteFilename(*best_url)) == NULL)
			return -1;
	} else
		best_match = NULL; 

	if (best_match && strip_suffix(best_match) == 0) {
		free(best_match);
		return -1;
	}

	for (i = 0; pattern[i] != '\0'; ++i) {
		if (!isalnum((unsigned char)(pattern[i])) &&
		    (pattern[i]) != '-')
			break;
	}
	url_pattern = xasprintf("%*.*s*", (int)i, (int)i, pattern);

	fetchInitURLList(&ue);
	if (fetchList(&ue, url, url_pattern, fetch_flags)) {
		char *base_url;
		base_url = fetchStringifyURL(url);
		warnx("Can't process %s%s: %s", base_url, url_pattern,
		    fetchLastErrString);
		free(base_url);
		free(url_pattern);
		fetchFreeURLList(&ue);
		return -1;
	}
	free(url_pattern);

	for (i = 0; i < ue.length; ++i) {
		cur_match = fetchUnquoteFilename(ue.urls + i);

		if (cur_match == NULL) {
			free(best_match);
			fetchFreeURLList(&ue);
			return -1;
		}
		if (strip_suffix(cur_match) == 0) {
			free(cur_match);
			continue;	
		}
		if (pkg_order(pattern, cur_match, best_match) == 1) {
			if (*best_url)
				fetchFreeURL(*best_url);
			*best_url = fetchCopyURL(ue.urls + i);
			free(best_match);
			best_match = cur_match;
			cur_match = NULL;
			if (*best_url == NULL) {
				free(best_match);
				return -1;
			}
		}
		free(cur_match);
	}
	free(best_match);
	fetchFreeURLList(&ue);
	return 0;
}

struct archive *
find_archive(const char *fname, void **cookie)
{
	struct archive *a;
	struct path *path;
	const char *cur_path;
	struct url *url, *best_match;
	char tmp[MaxPathSize];

	best_match = NULL;

	a = open_archive(fname, cookie);
	if (a != NULL)
		return a;

	if (strchr(fname, '/') != NULL) {
		const char *last_slash;

		last_slash = strrchr(fname, '/');
		snprintf(tmp, sizeof(tmp), "%s%.*s",
		    IS_URL(fname) ? "" : "file://",
		    (int)(last_slash - fname + 1), fname);
		url = fetchParseURL(tmp);
		if (url == NULL)
			return NULL;
		fname = last_slash + 1; /* XXX fetchUnquoteFilename */
		find_best_package(url, fname, &best_match);
		fetchFreeURL(url);
	} else {
		TAILQ_FOREACH(path, &PkgPath, pl_entry) {
			cur_path = path->pl_path;
			if (!IS_URL(cur_path)) {
				snprintf(tmp, sizeof(tmp), "file://%s", cur_path);
				cur_path = tmp;
			}
			url = fetchParseURL(cur_path);
			if (url == NULL)
				continue;
			find_best_package(url, fname, &best_match);
			/* XXX Check return value and complain */
			fetchFreeURL(url);
		}
	}

	if (best_match == NULL)
		return NULL;
	a = open_archive_by_url(best_match, cookie);
	fetchFreeURL(best_match);
	return a;
}
