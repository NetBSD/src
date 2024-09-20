/*	$NetBSD: nfsv3.h,v 1.1.4.2 2024/09/20 11:31:31 martin Exp $	*/

/*
 * Copyright (c) 2023 Michael van Elst
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Macklem at The University of Guelph.
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

/*
 * nfs definitions as per the version 3 specs
 */

#define NFS_VER3	3
#define	NFS_V3FHSIZE	64

/* Sizes in bytes of various nfs rpc components */
#define	NFSX_V3FH	64
#define	NFSX_V3FATTR	84

/* Different V3 procedure numbers that we need */
#define NFSV3PROC_LOOKUP          3

/* Structs for common parts of the rpc's */
struct nfsv3_time {
	n_long	nfs_sec;
	n_long	nfs_nsec;
};

struct nfsv3_spec {
        n_long specdata1;
        n_long specdata2;
};

/*
 * File attributes and setable attributes.
 */
struct nfsv3_fattr {
	n_long	fa_type;
	n_long	fa_mode;
	n_long	fa_nlink;
	n_long	fa_uid;
	n_long	fa_gid;
	n_long	fa_size[2]; /* nfsuint64 */
	n_long	fa_used[2]; /* nfsuint64 */
	struct nfsv3_spec fa_rdev;
	n_long	fa_fsid[2]; /* nfsuint64 */
	n_long	fa_fileid[2]; /* nfsuint64 */
	struct nfsv3_time fa_atime;
	struct nfsv3_time fa_mtime;
	struct nfsv3_time fa_ctime;
};
