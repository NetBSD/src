/*	$NetBSD: darwin_mount.h,v 1.1 2003/09/02 21:31:03 manu Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
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

#ifndef	_DARWIN_MOUNT_H_
#define	_DARWIN_MOUNT_H_

#define DARWIN_MNT_RDONLY	0x00000001
#define DARWIN_MNT_SYNCHRONOUS	0x00000002
#define DARWIN_MNT_NOEXEC	0x00000004
#define DARWIN_MNT_NOSUID	0x00000008
#define DARWIN_MNT_NODEV	0x00000010
#define DARWIN_MNT_UNION	0x00000020
#define DARWIN_MNT_ASYNC	0x00000040
#define DARWIN_MNT_EXRDONLY	0x00000080
#define DARWIN_MNT_EXPORTED	0x00000100
#define DARWIN_MNT_DEFEXPORTED	0x00000200
#define DARWIN_MNT_EXPORTANON	0x00000400
#define DARWIN_MNT_EXKERB	0x00000800
#define DARWIN_MNT_LOCAL	0x00001000
#define DARWIN_MNT_QUOTA	0x00002000
#define DARWIN_MNT_ROOTFS	0x00004000
#define DARWIN_MNT_DOVOLFS	0x00008000
#define DARWIN_MNT_UPDATE	0x00010000
#define DARWIN_MNT_DELEXPORT	0x00020000
#define DARWIN_MNT_RELOAD	0x00040000
#define DARWIN_MNT_FORCE	0x00080000
#define DARWIN_MNT_DONTBROWSE	0x00100000
#define DARWIN_MNT_UNKNOWNPERMISSIONS	0x00200000
#define DARWIN_MNT_AUTOMOUNTED	0x00400000
#define DARWIN_MNT_JOURNALED	0x00800000
#define DARWIN_MNTK_UNMOUNT	0x01000000
#define DARWIN_MNTK_MWAIT	0x02000000
#define DARWIN_MNTK_WANTRDWR	0x04000000
#define DARWIN_MNT_REVEND	0x08000000
#define DARWIN_MNT_FIXEDSCRIPTENCODING	0x10000000

#define DARWIN_MFSNAMELEN	15
#define DARWIN_MNAMELEN		90 

struct darwin_statfs {
	short	f_otype;
	short	f_oflags;
	long	f_bsize;
	long	f_iosize;
	long	f_blocks;
	long	f_bfree;
	long	f_bavail;
	long	f_files;
	long	f_ffree;
	fsid_t	f_fsid;
	uid_t	f_owner;
	short	f_reserved1;
	short	f_type;
	long	f_flags;
	long	f_reserved2[2];
	char	f_fstypename[DARWIN_MFSNAMELEN];
	char	f_mntonname[DARWIN_MNAMELEN];
	char	f_mntfromname[DARWIN_MNAMELEN];
	char	f_reserved3;
	long	f_reserved4[4];
};

#endif /* _DARWIN_MOUNT_H_ */
