/*	$NetBSD: linux_statfs.h,v 1.5 2010/11/02 18:14:05 chs Exp $	*/

/*-
 * Copyright (c) 1995, 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz; by Jason R. Thorpe
 * of the Numerical Aerospace Simulation Facility, NASA Ames Research Center.
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

#ifndef _LINUX_STATFS_H
#define _LINUX_STATFS_H

static void __unused bsd_to_linux_statfs(const struct statvfs *,
    struct linux_statfs *);
static void __unused bsd_to_linux_statfs64(const struct statvfs *,
    struct linux_statfs64 *);

/*
 * Convert NetBSD statvfs structure to Linux statfs structure.
 * Linux doesn't have f_flag, and we can't set f_frsize due
 * to glibc statvfs() bug (see below).
 */
static void __unused
bsd_to_linux_statfs(const struct statvfs *bsp, struct linux_statfs *lsp)
{
	int i;

	for (i = 0; i < linux_fstypes_cnt; i++) {
		if (strcmp(bsp->f_fstypename, linux_fstypes[i].bsd) == 0) {
			lsp->l_ftype = linux_fstypes[i].linux;
			break;
		}
	}

	if (i == linux_fstypes_cnt) {
		lsp->l_ftype = LINUX_DEFAULT_SUPER_MAGIC;
	}

	/*
	 * The sizes are expressed in number of blocks. The block
	 * size used for the size is f_frsize for POSIX-compliant
	 * statvfs. Linux statfs uses f_bsize as the block size
	 * (f_frsize used to not be available in Linux struct statfs).
	 * However, glibc 2.3.3 statvfs() wrapper fails to adjust the block
	 * counts for different f_frsize if f_frsize is provided by the kernel.
	 * POSIX conforming apps thus get wrong size if f_frsize
	 * is different to f_bsize. Thus, we just pretend we don't
	 * support f_frsize.
	 */

	lsp->l_fbsize = bsp->f_frsize;
	lsp->l_ffrsize = 0;			/* compat */
	lsp->l_fblocks = bsp->f_blocks;
	lsp->l_fbfree = bsp->f_bfree;
	lsp->l_fbavail = bsp->f_bavail;
	lsp->l_ffiles = bsp->f_files;
	lsp->l_fffree = bsp->f_ffree;
	/* Linux sets the fsid to 0..., we don't */
	lsp->l_ffsid.val[0] = bsp->f_fsidx.__fsid_val[0];
	lsp->l_ffsid.val[1] = bsp->f_fsidx.__fsid_val[1];
	lsp->l_fnamelen = bsp->f_namemax;
	(void)memset(lsp->l_fspare, 0, sizeof(lsp->l_fspare));
}

/*
 * Convert NetBSD statvfs structure to Linux statfs64 structure.
 * See comments in bsd_to_linux_statfs() for further background.
 * We can safely pass correct bsize and frsize here, since Linux glibc
 * statvfs() doesn't use statfs64().
 */
static void __unused
bsd_to_linux_statfs64(const struct statvfs *bsp, struct linux_statfs64 *lsp)
{
	int i, div;

	for (i = 0; i < linux_fstypes_cnt; i++) {
		if (strcmp(bsp->f_fstypename, linux_fstypes[i].bsd) == 0) {
			lsp->l_ftype = linux_fstypes[i].linux;
			break;
		}
	}

	if (i == linux_fstypes_cnt) {
		lsp->l_ftype = LINUX_DEFAULT_SUPER_MAGIC;
	}

	div = bsp->f_frsize ? (bsp->f_bsize / bsp->f_frsize) : 1;
	if (div == 0)
		div = 1;
	lsp->l_fbsize = bsp->f_bsize;
	lsp->l_ffrsize = bsp->f_frsize;
	lsp->l_fblocks = bsp->f_blocks / div;
	lsp->l_fbfree = bsp->f_bfree / div;
	lsp->l_fbavail = bsp->f_bavail / div;
	lsp->l_ffiles = bsp->f_files;
	lsp->l_fffree = bsp->f_ffree / div;
	/* Linux sets the fsid to 0..., we don't */
	lsp->l_ffsid.val[0] = bsp->f_fsidx.__fsid_val[0];
	lsp->l_ffsid.val[1] = bsp->f_fsidx.__fsid_val[1];
	lsp->l_fnamelen = bsp->f_namemax;
	(void)memset(lsp->l_fspare, 0, sizeof(lsp->l_fspare));
}

#endif /* !_LINUX_STATFS_H */
