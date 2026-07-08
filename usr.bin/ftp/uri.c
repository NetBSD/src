/*	$NetBSD: uri.c,v 1.3 2026/07/08 19:27:04 mlelstv Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ssl.h"
#include "ftp_var.h"
#include "uri.h"

typedef struct {
	const char *buf;
	size_t scheme;
	size_t authority;
	size_t path;
	size_t query;
	size_t fragment;
} uriparts_t;

#define SCHEME(p) ((p)->buf)
#define AUTHORITY(p) (SCHEME(p) + (p)->scheme)
#define PATH(p) (AUTHORITY(p) + (p)->authority)
#define QUERY(p) (PATH(p) + (p)->path)
#define FRAGMENT(p) (QUERY(p) + (p)->query)

static void
split_uri(const char *uri, uriparts_t *parts)
{
	char *colon, *slash, *query, *sharp;

	parts->buf = uri;
	parts->scheme = 0;
	parts->authority = 0;
	parts->path = 0;
	parts->query = 0;
	parts->fragment = 0;

	colon = strchr(uri, ':');
	slash = strchr(uri, '/');
	if (colon && (slash == NULL || (colon - uri) < (slash - uri))) {
		parts->scheme = colon + 1 - uri;
		uri = colon + 1;
	}       

	if (uri[0] == '/' && uri[1] == '/') {
		slash = strchr(uri + 2, '/');
		if (slash == NULL) {
			parts->authority = strlen(uri); 
			return;
		}
		parts->authority = slash - uri;
		uri = slash;
	}       

	query = strchr(uri, '?');
	sharp = strchr(uri, '#');
	if (query && (sharp == NULL || (query - uri) < (sharp - uri))) {
		parts->path = query - uri;
		uri = query;
		if (sharp != NULL) {
			parts->query = sharp - query;
			parts->fragment = strlen(sharp);
			return;
		}
		parts->query = strlen(query);
		return;
	}

	if (sharp != NULL) {
		parts->path = sharp - uri;
		parts->fragment = strlen(sharp);
		return;
	}

	parts->path = strlen(uri);
}

static char *
build_uri(uriparts_t *s, uriparts_t *a, uriparts_t *p,
    uriparts_t *q, uriparts_t *f)
{
	size_t len;
	char *buf, *out;

	len = s->scheme + a->authority + p->path + q->query + f->fragment;
	buf = ftp_malloc(len + 1);

	out = buf;
	memcpy(out, SCHEME(s), s->scheme);
	out += s->scheme;
	memcpy(out, AUTHORITY(a), a->authority);
	out += a->authority;
	memcpy(out, PATH(p), p->path);
	out += p->path;
	memcpy(out, QUERY(q), q->query);
	out += q->query;
	memcpy(out, FRAGMENT(f), f->fragment);
	out += f->fragment;
	*out = '\0';

	return buf;
}

static char *
remove_dot_segments(const char *path, size_t len)
{
	char *p;
	char *pbuf, *buf, *out, *seg;
	size_t n;

	p = pbuf = ftp_malloc(len + 1);
	strcpy(p, path);

	buf = ftp_malloc(len + 1);
	out = buf;
	*out = '\0';

	while (len > 0) {
		/* A */
		if (p[0] == '.') {
			if (len > 1 && p[1] == '/') {
				p += 2;
				len -= 2;
				continue;
			} else if (len > 2 && p[1] == '.' && p[2] == '/') {
				p += 3;
				len -= 3;
				continue;
			}
		}

		/* B */
		if (p[0] == '/') {
			if (len > 2 && p[1] == '.' && p[2] == '/') {
				p += 2;
				len -= 2;
				continue;
			} else if (len == 2 && p[1] == '.') {
				p += 1;
				len -= 1;
				p[0] = '/';
				continue;
			}
		}

		/* C */
		if (len > 2 && p[0] == '/' && p[1] == '.' && p[2] == '.') {
			if (len > 3 && p[3] == '/') {
				p += 3;
				len -= 3;
				seg = strrchr(buf, '/');
				if (seg != NULL)
					*seg = '\0';
				continue;
			} else if (len == 3) {
				p += 2;
				len -= 2;
				p[0] = '/';
				seg = strrchr(buf, '/');
				if (seg != NULL)
					*seg = '\0';
				continue;
			}
		}

		/* D */
		if (p[0] == '.') {
			if (len == 1) {
				p += 1;
				len -= 1;
				continue;
			} else if (len == 2 && p[1] == '.') {
				p += 2;
				len -= 2;
				continue;
			}
		}

		/* E */
		if (p[0] == '/') {
			seg = strchr(p+1, '/');
		} else {
			seg = strchr(p, '/');
		}
		if (seg != NULL) {
			n = seg - p;
			if (n > len)
				n = len;
		} else
			n = len;

		memcpy(out, p, n);
		out += n;
		*out = '\0';
		p += n;
		len -= n;
	};

	FREEPTR(pbuf);

	return buf;
}

static char *
merge_paths(uriparts_t *base, uriparts_t *ref)
{
	char *buf;
	const char *path, *last;
	size_t len;

	if (base->authority && base->path == 0) {
		len = 1 + ref->path;
		buf = ftp_malloc(len + 1);
		buf[0] = '/';
		memcpy(buf + 1, PATH(ref), ref->path);
	} else {
		path = PATH(base);
		last = strrchr(path, '/');
		if (last == NULL || (size_t)(last - path) > base->path)
			len = 0;
		else
			len = (size_t)(last - path) + 1;
		buf = ftp_malloc(len + ref->path + 1);
		memcpy(buf, path, len);
		memcpy(buf + len, PATH(ref), ref->path);
		len += ref->path;
	}
	buf[len] = '\0';

	return buf;
}

char *
make_absurl(char *refurl, const char *baseurl)
{
	uriparts_t base;
	uriparts_t ref;
	uriparts_t p;
	char *buf, *tmp1, *tmp2;
	
	split_uri(baseurl, &base);
	split_uri(refurl, &ref);

	p.scheme = p.authority = p.path = p.query = p.fragment = 0;

	if (ref.scheme) {
		tmp1 = remove_dot_segments(PATH(&ref), ref.path);
		p.buf = tmp1;
		p.path = strlen(tmp1);
		buf = build_uri(&ref, &ref, &p, &ref, &ref);
		FREEPTR(tmp1);
	} else {
		if (ref.authority) {
			tmp1 = remove_dot_segments(PATH(&ref), ref.path);
			p.buf = tmp1;
			p.path = strlen(tmp1);
			buf = build_uri(&base, &ref, &p, &ref, &ref);
			FREEPTR(tmp1);
		} else {
			if (ref.path == 0) {
				if (ref.query) {
					buf = build_uri(&base, &base, &base, &ref, &ref);
				} else {
					buf = build_uri(&base, &base, &base, &base, &ref);
				}
			} else {
				if (PATH(&ref)[0] == '/') {
					tmp1 = remove_dot_segments(PATH(&ref), ref.path);
					p.buf = tmp1;
					p.path = strlen(tmp1);
					buf = build_uri(&base, &base, &p, &ref, &ref);
					FREEPTR(tmp1);
				} else {
					tmp2 = merge_paths(&base, &ref);
					tmp1 = remove_dot_segments(tmp2, strlen(tmp2));
					p.buf = tmp1;
					p.path = strlen(tmp1);
					FREEPTR(tmp2);

					buf = build_uri(&base, &base, &p, &ref, &ref);
					FREEPTR(tmp1);
				}
			}
		}
	}

	FREEPTR(refurl);

	return buf;
}
