/* $NetBSD: legacy.h,v 1.1 2022/01/22 07:57:30 pho Exp $ */

/*
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#if !defined(_FUSE_LEGACY_H_)
#define _FUSE_LEGACY_H_

#include <sys/fstypes.h>

/* Legacy data types and functions that had once existed but have been
 * removed from the FUSE API. */

#if !defined(FUSE_H_)
#  error Do not include this header directly. Include <fuse.h> instead.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* statfs structure used by FUSE < 1.9. On 2.1 it's been replaced with
 * "struct statfs" and later replaced again with struct statvfs on
 * 2.5. */
struct fuse_statfs {
	long block_size;
	long blocks;
	long blocks_free;
	long files;
	long files_free;
	long namelen;
};

/* Linux-specific struct statfs; used by FUSE >= 2.1 && < 2.5. */
struct statfs {
	long		f_type;
	long		f_bsize;
	fsblkcnt_t	f_blocks;
	fsblkcnt_t	f_bfree;
	fsblkcnt_t	f_bavail;
	fsfilcnt_t	f_files;
	fsfilcnt_t	f_ffree;
	fsid_t		f_fsid;
	long		f_namelen;
	long		f_frsize;
	long		f_flags;
};

/* Handle for a getdir() operation. Removed as of FUSE 3.0. */
typedef void *fuse_dirh_t;

/* Enable debugging output. Removed on FUSE 3.0. */
#define FUSE_DEBUG	(1 << 1)

/* Invalidate cached data of a file. Added on FUSE 1.9 and removed on
 * FUSE 3.0. Not to be confused with fuse_invalidate_path() appeared
 * on FUSE 3.2. */
int fuse_invalidate(struct fuse *f, const char *path);

/* Check whether a mount option should be passed to the kernel or the
 * library. Added on FUSE 1.9 and removed on FUSE 3.0. */
int fuse_is_lib_option(const char *opt);

#ifdef __cplusplus
}
#endif

#endif
