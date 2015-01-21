/*	$NetBSD: blacklist.c,v 1.1 2015/01/21 16:16:00 christos Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
#include <sys/cdefs.h>
__RCSID("$NetBSD: blacklist.c,v 1.1 2015/01/21 16:16:00 christos Exp $");

#include <stdio.h>
#include <bl.h>

#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
static const char *
expandm(char *buf, size_t len, const char *fmt)
{
	char *p;
	size_t r;

	if ((p = strstr(fmt, "%m")) == NULL)
		return fmt;

	r = (size_t)(p - fmt);
	if (r >= len)
		return fmt;

	strlcpy(buf, fmt, r + 1);
	strlcat(buf, strerror(errno), len);
	strlcat(buf, fmt + r + 2, len);

	return buf;
}

static void
dlog(int level, const char *fmt, ...)
{
	char buf[BUFSIZ];
	va_list ap;

	fprintf(stderr, "%s: ", getprogname());
	va_start(ap, fmt);
	vfprintf(stderr, expandm(buf, sizeof(buf), fmt), ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

int
blacklist(int action, int rfd, const char *msg)
{
	struct blacklist *bl;
	int rv;
	if ((bl = blacklist_open()) == NULL)
		return -1;
	rv = blacklist_r(bl, action, rfd, msg);
	blacklist_close(bl);
	return rv;
}

int
blacklist_r(struct blacklist *bl, int action, int rfd, const char *msg)
{
	return bl_send(bl, action ? BL_ADD : BL_DELETE, rfd, msg);
}

struct blacklist *
blacklist_open(void) {
	return bl_create(false, NULL, dlog);
}

void
blacklist_close(struct blacklist *bl)
{
	bl_destroy(bl);
}
