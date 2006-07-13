/*	$NetBSD: compat_getfh.c,v 1.1 2006/07/13 13:07:31 martin Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Martin Husemann <martin@NetBSD.org>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__RCSID("$NetBSD: compat_getfh.c,v 1.1 2006/07/13 13:07:31 martin Exp $");
#endif /* LIBC_SCCS and not lint */

#define __LIBC12_SOURCE__

#include <sys/types.h>
#include <sys/mount.h>

__warn_references(getfh,
    "warning: reference to compatibility getfh(); include <sys/mount.h> to generate correct reference")

/*
 * Old fhandle_t
 */

/* Old, fixed size filehandle structures (used upto (including) 3.x) */
struct compat_30_fid{
	unsigned short	fid_len;
	unsigned short	fid_reserved;
	char		fid_data[16];
};
struct compat_30_fhandle {
	fsid_t	fh_fsid;
	struct compat_30_fid fh_fid;
};

int     __getfh30(const char *, fhandle_t*, size_t *);
int	getfh(const char *path, struct compat_30_fhandle *fhp);

/*
 * Convert old getfh() call to new calling convention
 */
int
getfh(const char *path, struct compat_30_fhandle *fhp)
{
	size_t fh_size = sizeof(*fhp);

	return __getfh30(path, (fhandle_t *)fhp, &fh_size);
}
