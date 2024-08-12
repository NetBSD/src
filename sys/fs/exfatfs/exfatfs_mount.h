/* $NetBSD: exfatfs_mount.h,v 1.1.2.3 2024/08/12 22:32:11 perseant Exp $ */

/*-
 * Copyright (c) 2022, 2024 The NetBSD Foundation, Inc.
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

#ifndef EXFATFS_MOUNT_H_
#define EXFATFS_MOUNT_H_

#include <sys/types.h>

/*
 *  Arguments to mount EXFAT filesystems.
 */
struct exfatfs_args {
	char	*fspec;		/* blocks special holding the fs to mount */
	uid_t	uid;		/* uid that owns files */
	gid_t	gid;		/* gid that owns files */
	mode_t	mask;		/* mask to be applied for file perms */
	int	flags;		/* see below */
	int	version;	/* version of the struct */
#define EXFATFSMNT_VERSION      1
	mode_t	dirmask;	/* mask to be applied for directory perms */
	int	gmtoff;		/* offset from UTC in seconds */
};

#ifdef _KERNEL
#define	MPTOXMP(mp)	((struct exfatfs_mount *)(mp)->mnt_data)
#define	XMPTOMP(xmp)	((xmp)->xm_mp)

struct exfatfs_mount {
	struct mount *xm_mp;
	u_int32_t xm_flags;
#define EXFATFS_MNTOPT		0x0
#define	EXFATFSMNT_RONLY	0x80000000	/* mounted read-only	*/
#define	EXFATFSMNT_SYNC		0x40000000	/* mounted synchronous	*/
	struct exfatfs *xm_fs;
};

#define EXFATFSMNT_MNTOPT 0x0
#endif /* _KERNEL */

#endif /* EXFATFS_MOUNT_H_ */
