/*	$NetBSD: getnfsquota.c,v 1.5 2012/01/09 15:31:11 dholland Exp $	*/

/*
 * Copyright (c) 1980, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Robert Elz at The University of Melbourne.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1980, 1990, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)quota.c	8.4 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: getnfsquota.c,v 1.5 2012/01/09 15:31:11 dholland Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/mount.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <quota.h>
#include <quota/quota.h>

#include "quotapvt.h"

int
getnfsquota(const char *mp, struct quotaval *qv,
    uint32_t id, const char *class)
{
	struct statvfs *mounts;
	size_t size;
	int nummounts, i, ret;
	int serrno;

	/*
	 * For some reason getnfsquota was defined so that the mount
	 * information passed in is the f_mntfromname (containing the
	 * remote host and path) from statvfs. The only way to convert
	 * this back to something generally useful is to search the
	 * available mounts for something that matches. Sigh.
	 *
	 * Note that we can't use getmntinfo(3) as the caller probably
	 * is and another use would potentially interfere.
	 */

	nummounts = getvfsstat(NULL, (size_t)0, MNT_NOWAIT);
	if (nummounts < 0) {
		return -1;
	}
	if (nummounts == 0) {
		errno = ENOENT;
		return -1;
	}
	size = nummounts * sizeof(mounts[0]);
	mounts = malloc(size);
	if (mounts == NULL) {
		return -1;
	}
	nummounts = getvfsstat(mounts, size, MNT_NOWAIT);
	if (nummounts < 0) {
		serrno = errno;
		free(mounts);
		errno = serrno;
		return -1;
	}

	/*
	 * Note: if the size goes up, someone added a new mount; it
	 * can't be the one we're looking for. Assume it will end up
	 * at the end of the list so we don't need to refetch the
	 * info, and reset nummounts to avoid chugging off the end
	 * of the array.
	 */
	if (nummounts * sizeof(mounts[0]) > size) {
		nummounts = size / sizeof(mounts[0]);
	}

	for (i=0; i<nummounts; i++) {
		if (!strcmp(mounts[i].f_mntfromname, mp)) {
			ret = __quota_getquota(mounts[i].f_mntonname,
					       qv, id, class);
			serrno = errno;
			free(mounts);
			errno = serrno;
			return ret;
		}
	}
	free(mounts);
	errno = ENOENT;
	return -1;
}
