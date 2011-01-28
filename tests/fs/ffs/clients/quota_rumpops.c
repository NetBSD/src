/*	$NetBSD: quota_rumpops.c,v 1.1.2.1 2011/01/28 18:38:08 bouyer Exp $	*/

/*
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: quota_rumpops.c,v 1.1.2.1 2011/01/28 18:38:08 bouyer Exp $");
#endif /* !lint */

#include <stdio.h>
#include <err.h>
#include <sys/types.h>
#include <sys/quota.h>
#include <sys/statvfs.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>
#include <rump/rumpclient.h>

#ifdef DEBUGJACK
#define DPRINTF(x) mydprintf x
static void
mydprintf(const char *fmt, ...)
{
	va_list ap;

	if (ISDUP2D(STDERR_FILENO))
		return;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

#else
#define DPRINTF(x)
#endif

static void __attribute__((constructor))
rcinit(void)
{
	DPRINTF("rcinit\n");
	if (rumpclient_init() == -1)
		err(1, "rump client init");
}

int __quotactl50(const char *, struct plistref *);
int
__quotactl50(const char * mnt, struct plistref *p)
{
	int error;
	error = rump_sys_quotactl(mnt, p);
	DPRINTF(("quotactl <- %d\n", error));
	return error;
}

int
getvfsstat(struct statvfs *buf, size_t bufsize, int flags)
{
	int error;

	error = rump_sys_getvfsstat(buf, bufsize, flags);
	DPRINTF(("getvfsstat <- %d\n", error));
	return error;
}
