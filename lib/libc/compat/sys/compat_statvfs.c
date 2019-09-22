/*	$NetBSD: compat_statvfs.c,v 1.1 2019/09/22 22:59:38 christos Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas
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
__RCSID("$NetBSD: compat_statvfs.c,v 1.1 2019/09/22 22:59:38 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#define __LIBC12_SOURCE__

#include <stdlib.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <compat/include/fstypes.h>
#include <compat/sys/statvfs.h>

__warn_references(statvfs,
    "warning: reference to compatibility statvfs(); include <sys/statvfs.h> to generate correct reference")
__warn_references(statvfs1,
    "warning: reference to compatibility statvfs1(); include <sys/statvfs.h> to generate correct reference")

__warn_references(fstatvfs,
    "warning: reference to compatibility fstatvfs(); include <sys/statvfs.h> to generate correct reference")
__warn_references(fstatvfs1,
    "warning: reference to compatibility fstatvfs1(); include <sys/statvfs.h> to generate correct reference")

__warn_references(getvfsstat,
    "warning: reference to compatibility getvfsstat(); include <sys/statvfs.h> to generate correct reference")

__strong_alias(statvfs, __compat_statvfs)
__strong_alias(statvfs1, __compat_statvfs1)
__strong_alias(fstatvfs, __compat_fstatvfs)
__strong_alias(fstatvfs1, __compat_fstatvfs1)
__strong_alias(getvfsstat, __compat_getvfsstat)

int
__compat_statvfs(const char *path, struct statvfs90 *buf)
{
	struct statvfs sb;
	int error = __statvfs190(path, &sb, 0);
	if (error != -1)
		statvfs_to_statvfs90(&sb, buf);
	return error;
}

int
__compat_statvfs1(const char *path, struct statvfs90 *buf, int flags)
{
	struct statvfs sb;
	int error = __statvfs190(path, &sb, flags);
	if (error != -1)
		statvfs_to_statvfs90(&sb, buf);
	return error;
}

int
__compat_fstatvfs(int fd, struct statvfs90 *buf)
{
	struct statvfs sb;
	int error = __fstatvfs190(fd, &sb, 0);
	if (error != -1)
		statvfs_to_statvfs90(&sb, buf);
	return error;
}

int
__compat_fstatvfs1(int fd, struct statvfs90 *buf, int flags)
{
	struct statvfs sb;
	int error = __fstatvfs190(fd, &sb, flags);
	if (error != -1)
		statvfs_to_statvfs90(&sb, buf);
	return error;
}

int
__compat_getvfsstat(struct statvfs90 *buf, size_t size, int flags)
{
	size_t count = size / sizeof(*buf);
	struct statvfs *sb = calloc(count, sizeof(*sb));

	if (sb == NULL)
		return -1;

	int error = __getvfsstat90(sb, count * sizeof(*sb), flags);
	if (error != -1) {
		for (size_t i = 0; i < count; i++)
			statvfs_to_statvfs90(sb + i, buf + i);
	}
	free(sb);
	return error;
}
