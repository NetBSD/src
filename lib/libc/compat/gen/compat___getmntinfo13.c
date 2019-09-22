/*	$NetBSD: compat___getmntinfo13.c,v 1.1 2019/09/22 22:59:38 christos Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
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
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: compat___getmntinfo13.c,v 1.1 2019/09/22 22:59:38 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#define __LIBC12_SOURCE__

#include "namespace.h"

#include <sys/types.h>
#include <sys/param.h>

#include <assert.h>
#include <compat/sys/statvfs.h>
#include <string.h>
#include <stdlib.h>

__warn_references(__getmntinfo13,
    "warning: reference to obsolete __getmntinfo13(); use statvfs()")

__strong_alias(__getmntinfo13, __compat___getmntinfo13)

/*
 * Return information about mounted filesystems.
 */
int
__compat___getmntinfo13(struct statvfs90 **mntbufp, int flags)
{
	static struct statvfs90 *mntbuf;
	static int mntsize;
	static size_t bufsize;

	_DIAGASSERT(mntbufp != NULL);

	if (mntsize <= 0 &&
	    (mntsize = __compat_getvfsstat(NULL, 0L, MNT_NOWAIT)) == -1)
		return (0);
	if (bufsize > 0 &&
	    (mntsize = __compat_getvfsstat(mntbuf, bufsize, flags)) == -1)
		return (0);
	while (bufsize <= mntsize * sizeof(struct statvfs90)) {
		if (mntbuf)
			free(mntbuf);
		bufsize = (mntsize + 1) * sizeof(struct statvfs90);
		if ((mntbuf = malloc(bufsize)) == NULL)
			return (0);
		if ((mntsize = __compat_getvfsstat(mntbuf, bufsize,
		    flags)) == -1)
			return (0);
	}
	*mntbufp = mntbuf;
	return (mntsize);
}
