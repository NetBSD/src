/*	$NetBSD: compat_statfs.c,v 1.9 2019/10/04 01:28:03 christos Exp $	*/

/*-
 * Copyright (c) 2004, 2019 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: compat_statfs.c,v 1.9 2019/10/04 01:28:03 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#define __LIBC12_SOURCE__

#include "namespace.h"
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <compat/sys/mount.h>
#include <compat/include/fstypes.h>
#include <compat/sys/statvfs.h>
#include <string.h>
#include <stdlib.h>

__warn_references(statfs,
    "warning: reference to obsolete statfs(); use statvfs()")

__warn_references(fstatfs,
    "warning: reference to obsolete fstatfs(); use fstatvfs()")

__warn_references(fhstatfs,
    "warning: reference to obsolete fhstatfs(); use fhstatvfs()")

__warn_references(getfsstat,
    "warning: reference to obsolete getfsstat(); use getvfsstat()")

__strong_alias(statfs, __compat_statfs)
__strong_alias(fstatfs, __compat_fstatfs)
__strong_alias(fhstatfs, __compat_fhstatfs)
__strong_alias(getfsstat, __compat_getfsstat)

int
__compat_statfs(const char *file, struct statfs12 *ost)
{
	struct statvfs nst;
	int ret;

	if ((ret = __statvfs90(file, &nst)) == -1)
		return ret;
	statvfs_to_statfs12(&nst, ost);
	return ret;
}

int
__compat_fstatfs(int f, struct statfs12 *ost)
{
	struct statvfs nst;
	int ret;

	if ((ret = __fstatvfs90(f, &nst)) == -1)
		return ret;
	statvfs_to_statfs12(&nst, ost);
	return ret;
}

int
__compat_fhstatfs(const struct compat_30_fhandle *fh, struct statfs12 *ost)
{
	struct statvfs nst;
	int ret;

	if ((ret = __fhstatvfs190(fh, FHANDLE30_SIZE, &nst, ST_WAIT)) == -1)
		return ret;
	statvfs_to_statfs12(&nst, ost);
	return ret;
}

int
__compat_getfsstat(struct statfs12 *ost, long size, int flags)
{
	struct statvfs *nst;
	int ret, i;
	size_t bsize = (size_t)(size / sizeof(*ost)) * sizeof(*nst);

	if (ost != NULL) {
		if ((nst = malloc(bsize)) == NULL)
			return -1;
	} else
		nst = NULL;

	if ((ret = __getvfsstat90(nst, bsize, flags)) == -1)
		goto done;
	if (nst)
		for (i = 0; i < ret; i++)
			statvfs_to_statfs12(&nst[i], &ost[i]);
done:
	if (nst)
		free(nst);
	return ret;
}
