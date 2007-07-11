/*	$NetBSD: nfs_subs.c,v 1.184.4.1 2007/07/11 20:12:13 mjf Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *
 *	@(#)nfs_subs.c	8.8 (Berkeley) 5/22/95
 */

/*
 * Copyright 2000 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nfs_subs.c,v 1.184.4.1 2007/07/11 20:12:13 mjf Exp $");

#include "fs_nfs.h"
#include "opt_nfs.h"
#include "opt_nfsserver.h"
#include "opt_iso.h"
#include "opt_inet.h"

/*
 * These functions support the macros and help fiddle mbuf chains for
 * the nfs op functions. They do things like create the rpc header and
 * copy data between mbuf chains and uio lists.
 */
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/malloc.h>
#include <sys/filedesc.h>
#include <sys/time.h>
#include <sys/dirent.h>
#include <sys/once.h>
#include <sys/kauth.h>

#include <uvm/uvm_extern.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfsnode.h>
#include <nfs/nfs.h>
#include <nfs/xdr_subs.h>
#include <nfs/nfsm_subs.h>
#include <nfs/nfsmount.h>
#include <nfs/nfsrtt.h>
#include <nfs/nfs_var.h>

#include <miscfs/specfs/specdev.h>

#include <netinet/in.h>
#ifdef ISO
#include <netiso/iso.h>
#endif

/*
 * Data items converted to xdr at startup, since they are constant
 * This is kinda hokey, but may save a little time doing byte swaps
 */
u_int32_t nfs_xdrneg1;
u_int32_t rpc_call, rpc_vers, rpc_reply, rpc_msgdenied, rpc_autherr,
	rpc_mismatch, rpc_auth_unix, rpc_msgaccepted,
	rpc_auth_kerb;
u_int32_t nfs_prog, nfs_true, nfs_false;

/* And other global data */
const nfstype nfsv2_type[9] =
	{ NFNON, NFREG, NFDIR, NFBLK, NFCHR, NFLNK, NFNON, NFCHR, NFNON };
const nfstype nfsv3_type[9] =
	{ NFNON, NFREG, NFDIR, NFBLK, NFCHR, NFLNK, NFSOCK, NFFIFO, NFNON };
const enum vtype nv2tov_type[8] =
	{ VNON, VREG, VDIR, VBLK, VCHR, VLNK, VNON, VNON };
const enum vtype nv3tov_type[8] =
	{ VNON, VREG, VDIR, VBLK, VCHR, VLNK, VSOCK, VFIFO };
int nfs_ticks;
int nfs_commitsize;

MALLOC_DEFINE(M_NFSDIROFF, "NFS diroff", "NFS directory cookies");

/* NFS client/server stats. */
struct nfsstats nfsstats;

/*
 * Mapping of old NFS Version 2 RPC numbers to generic numbers.
 */
const int nfsv3_procid[NFS_NPROCS] = {
	NFSPROC_NULL,
	NFSPROC_GETATTR,
	NFSPROC_SETATTR,
	NFSPROC_NOOP,
	NFSPROC_LOOKUP,
	NFSPROC_READLINK,
	NFSPROC_READ,
	NFSPROC_NOOP,
	NFSPROC_WRITE,
	NFSPROC_CREATE,
	NFSPROC_REMOVE,
	NFSPROC_RENAME,
	NFSPROC_LINK,
	NFSPROC_SYMLINK,
	NFSPROC_MKDIR,
	NFSPROC_RMDIR,
	NFSPROC_READDIR,
	NFSPROC_FSSTAT,
	NFSPROC_NOOP,
	NFSPROC_NOOP,
	NFSPROC_NOOP,
	NFSPROC_NOOP,
	NFSPROC_NOOP
};

/*
 * and the reverse mapping from generic to Version 2 procedure numbers
 */
const int nfsv2_procid[NFS_NPROCS] = {
	NFSV2PROC_NULL,
	NFSV2PROC_GETATTR,
	NFSV2PROC_SETATTR,
	NFSV2PROC_LOOKUP,
	NFSV2PROC_NOOP,
	NFSV2PROC_READLINK,
	NFSV2PROC_READ,
	NFSV2PROC_WRITE,
	NFSV2PROC_CREATE,
	NFSV2PROC_MKDIR,
	NFSV2PROC_SYMLINK,
	NFSV2PROC_CREATE,
	NFSV2PROC_REMOVE,
	NFSV2PROC_RMDIR,
	NFSV2PROC_RENAME,
	NFSV2PROC_LINK,
	NFSV2PROC_READDIR,
	NFSV2PROC_NOOP,
	NFSV2PROC_STATFS,
	NFSV2PROC_NOOP,
	NFSV2PROC_NOOP,
	NFSV2PROC_NOOP,
	NFSV2PROC_NOOP,
};

/*
 * Maps errno values to nfs error numbers.
 * Use NFSERR_IO as the catch all for ones not specifically defined in
 * RFC 1094.
 */
static const u_char nfsrv_v2errmap[ELAST] = {
  NFSERR_PERM,	NFSERR_NOENT,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,
  NFSERR_NXIO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,
  NFSERR_IO,	NFSERR_IO,	NFSERR_ACCES,	NFSERR_IO,	NFSERR_IO,
  NFSERR_IO,	NFSERR_EXIST,	NFSERR_IO,	NFSERR_NODEV,	NFSERR_NOTDIR,
  NFSERR_ISDIR,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,
  NFSERR_IO,	NFSERR_FBIG,	NFSERR_NOSPC,	NFSERR_IO,	NFSERR_ROFS,
  NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,
  NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,
  NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,
  NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,
  NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,
  NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,
  NFSERR_IO,	NFSERR_IO,	NFSERR_NAMETOL,	NFSERR_IO,	NFSERR_IO,
  NFSERR_NOTEMPTY, NFSERR_IO,	NFSERR_IO,	NFSERR_DQUOT,	NFSERR_STALE,
  NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,
  NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,
  NFSERR_IO,	NFSERR_IO,
};

/*
 * Maps errno values to nfs error numbers.
 * Although it is not obvious whether or not NFS clients really care if
 * a returned error value is in the specified list for the procedure, the
 * safest thing to do is filter them appropriately. For Version 2, the
 * X/Open XNFS document is the only specification that defines error values
 * for each RPC (The RFC simply lists all possible error values for all RPCs),
 * so I have decided to not do this for Version 2.
 * The first entry is the default error return and the rest are the valid
 * errors for that RPC in increasing numeric order.
 */
static const short nfsv3err_null[] = {
	0,
	0,
};

static const short nfsv3err_getattr[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	0,
};

static const short nfsv3err_setattr[] = {
	NFSERR_IO,
	NFSERR_PERM,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_INVAL,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOT_SYNC,
	NFSERR_SERVERFAULT,
	0,
};

static const short nfsv3err_lookup[] = {
	NFSERR_IO,
	NFSERR_NOENT,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_NOTDIR,
	NFSERR_NAMETOL,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	0,
};

static const short nfsv3err_access[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	0,
};

static const short nfsv3err_readlink[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_INVAL,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	0,
};

static const short nfsv3err_read[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_NXIO,
	NFSERR_ACCES,
	NFSERR_INVAL,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	NFSERR_JUKEBOX,
	0,
};

static const short nfsv3err_write[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_INVAL,
	NFSERR_FBIG,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	NFSERR_JUKEBOX,
	0,
};

static const short nfsv3err_create[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_EXIST,
	NFSERR_NOTDIR,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_NAMETOL,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	0,
};

static const short nfsv3err_mkdir[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_EXIST,
	NFSERR_NOTDIR,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_NAMETOL,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	0,
};

static const short nfsv3err_symlink[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_EXIST,
	NFSERR_NOTDIR,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_NAMETOL,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	0,
};

static const short nfsv3err_mknod[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_EXIST,
	NFSERR_NOTDIR,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_NAMETOL,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	NFSERR_BADTYPE,
	0,
};

static const short nfsv3err_remove[] = {
	NFSERR_IO,
	NFSERR_NOENT,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_NOTDIR,
	NFSERR_ROFS,
	NFSERR_NAMETOL,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	0,
};

static const short nfsv3err_rmdir[] = {
	NFSERR_IO,
	NFSERR_NOENT,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_EXIST,
	NFSERR_NOTDIR,
	NFSERR_INVAL,
	NFSERR_ROFS,
	NFSERR_NAMETOL,
	NFSERR_NOTEMPTY,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	0,
};

static const short nfsv3err_rename[] = {
	NFSERR_IO,
	NFSERR_NOENT,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_EXIST,
	NFSERR_XDEV,
	NFSERR_NOTDIR,
	NFSERR_ISDIR,
	NFSERR_INVAL,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_MLINK,
	NFSERR_NAMETOL,
	NFSERR_NOTEMPTY,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	0,
};

static const short nfsv3err_link[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_EXIST,
	NFSERR_XDEV,
	NFSERR_NOTDIR,
	NFSERR_INVAL,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_MLINK,
	NFSERR_NAMETOL,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	0,
};

static const short nfsv3err_readdir[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_NOTDIR,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_BAD_COOKIE,
	NFSERR_TOOSMALL,
	NFSERR_SERVERFAULT,
	0,
};

static const short nfsv3err_readdirplus[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_NOTDIR,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_BAD_COOKIE,
	NFSERR_NOTSUPP,
	NFSERR_TOOSMALL,
	NFSERR_SERVERFAULT,
	0,
};

static const short nfsv3err_fsstat[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	0,
};

static const short nfsv3err_fsinfo[] = {
	NFSERR_STALE,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	0,
};

static const short nfsv3err_pathconf[] = {
	NFSERR_STALE,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	0,
};

static const short nfsv3err_commit[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	0,
};

static const short * const nfsrv_v3errmap[] = {
	nfsv3err_null,
	nfsv3err_getattr,
	nfsv3err_setattr,
	nfsv3err_lookup,
	nfsv3err_access,
	nfsv3err_readlink,
	nfsv3err_read,
	nfsv3err_write,
	nfsv3err_create,
	nfsv3err_mkdir,
	nfsv3err_symlink,
	nfsv3err_mknod,
	nfsv3err_remove,
	nfsv3err_rmdir,
	nfsv3err_rename,
	nfsv3err_link,
	nfsv3err_readdir,
	nfsv3err_readdirplus,
	nfsv3err_fsstat,
	nfsv3err_fsinfo,
	nfsv3err_pathconf,
	nfsv3err_commit,
};

extern struct nfsrtt nfsrtt;
extern struct nfsnodehashhead *nfsnodehashtbl;
extern u_long nfsnodehash;

u_long nfsdirhashmask;

int nfs_webnamei __P((struct nameidata *, struct vnode *, struct proc *));

/*
 * Create the header for an rpc request packet
 * The hsiz is the size of the rest of the nfs request header.
 * (just used to decide if a cluster is a good idea)
 */
struct mbuf *
nfsm_reqh(struct nfsnode *np, u_long procid, int hsiz, char **bposp)
{
	struct mbuf *mb;
	char *bpos;

	mb = m_get(M_WAIT, MT_DATA);
	MCLAIM(mb, &nfs_mowner);
	if (hsiz >= MINCLSIZE)
		m_clget(mb, M_WAIT);
	mb->m_len = 0;
	bpos = mtod(mb, void *);

	/* Finally, return values */
	*bposp = bpos;
	return (mb);
}

/*
 * Build the RPC header and fill in the authorization info.
 * The authorization string argument is only used when the credentials
 * come from outside of the kernel.
 * Returns the head of the mbuf list.
 */
struct mbuf *
nfsm_rpchead(cr, nmflag, procid, auth_type, auth_len, auth_str, verf_len,
	verf_str, mrest, mrest_len, mbp, xidp)
	kauth_cred_t cr;
	int nmflag;
	int procid;
	int auth_type;
	int auth_len;
	char *auth_str;
	int verf_len;
	char *verf_str;
	struct mbuf *mrest;
	int mrest_len;
	struct mbuf **mbp;
	u_int32_t *xidp;
{
	struct mbuf *mb;
	u_int32_t *tl;
	char *bpos;
	int i;
	struct mbuf *mreq;
	int siz, grpsiz, authsiz;

	authsiz = nfsm_rndup(auth_len);
	mb = m_gethdr(M_WAIT, MT_DATA);
	MCLAIM(mb, &nfs_mowner);
	if ((authsiz + 10 * NFSX_UNSIGNED) >= MINCLSIZE) {
		m_clget(mb, M_WAIT);
	} else if ((authsiz + 10 * NFSX_UNSIGNED) < MHLEN) {
		MH_ALIGN(mb, authsiz + 10 * NFSX_UNSIGNED);
	} else {
		MH_ALIGN(mb, 8 * NFSX_UNSIGNED);
	}
	mb->m_len = 0;
	mreq = mb;
	bpos = mtod(mb, void *);

	/*
	 * First the RPC header.
	 */
	nfsm_build(tl, u_int32_t *, 8 * NFSX_UNSIGNED);

	*tl++ = *xidp = nfs_getxid();
	*tl++ = rpc_call;
	*tl++ = rpc_vers;
	*tl++ = txdr_unsigned(NFS_PROG);
	if (nmflag & NFSMNT_NFSV3)
		*tl++ = txdr_unsigned(NFS_VER3);
	else
		*tl++ = txdr_unsigned(NFS_VER2);
	if (nmflag & NFSMNT_NFSV3)
		*tl++ = txdr_unsigned(procid);
	else
		*tl++ = txdr_unsigned(nfsv2_procid[procid]);

	/*
	 * And then the authorization cred.
	 */
	*tl++ = txdr_unsigned(auth_type);
	*tl = txdr_unsigned(authsiz);
	switch (auth_type) {
	case RPCAUTH_UNIX:
		nfsm_build(tl, u_int32_t *, auth_len);
		*tl++ = 0;		/* stamp ?? */
		*tl++ = 0;		/* NULL hostname */
		*tl++ = txdr_unsigned(kauth_cred_geteuid(cr));
		*tl++ = txdr_unsigned(kauth_cred_getegid(cr));
		grpsiz = (auth_len >> 2) - 5;
		*tl++ = txdr_unsigned(grpsiz);
		for (i = 0; i < grpsiz; i++)
			*tl++ = txdr_unsigned(kauth_cred_group(cr, i)); /* XXX elad review */
		break;
	case RPCAUTH_KERB4:
		siz = auth_len;
		while (siz > 0) {
			if (M_TRAILINGSPACE(mb) == 0) {
				struct mbuf *mb2;
				mb2 = m_get(M_WAIT, MT_DATA);
				MCLAIM(mb2, &nfs_mowner);
				if (siz >= MINCLSIZE)
					m_clget(mb2, M_WAIT);
				mb->m_next = mb2;
				mb = mb2;
				mb->m_len = 0;
				bpos = mtod(mb, void *);
			}
			i = min(siz, M_TRAILINGSPACE(mb));
			memcpy(bpos, auth_str, i);
			mb->m_len += i;
			auth_str += i;
			bpos += i;
			siz -= i;
		}
		if ((siz = (nfsm_rndup(auth_len) - auth_len)) > 0) {
			for (i = 0; i < siz; i++)
				*bpos++ = '\0';
			mb->m_len += siz;
		}
		break;
	};

	/*
	 * And the verifier...
	 */
	nfsm_build(tl, u_int32_t *, 2 * NFSX_UNSIGNED);
	if (verf_str) {
		*tl++ = txdr_unsigned(RPCAUTH_KERB4);
		*tl = txdr_unsigned(verf_len);
		siz = verf_len;
		while (siz > 0) {
			if (M_TRAILINGSPACE(mb) == 0) {
				struct mbuf *mb2;
				mb2 = m_get(M_WAIT, MT_DATA);
				MCLAIM(mb2, &nfs_mowner);
				if (siz >= MINCLSIZE)
					m_clget(mb2, M_WAIT);
				mb->m_next = mb2;
				mb = mb2;
				mb->m_len = 0;
				bpos = mtod(mb, void *);
			}
			i = min(siz, M_TRAILINGSPACE(mb));
			memcpy(bpos, verf_str, i);
			mb->m_len += i;
			verf_str += i;
			bpos += i;
			siz -= i;
		}
		if ((siz = (nfsm_rndup(verf_len) - verf_len)) > 0) {
			for (i = 0; i < siz; i++)
				*bpos++ = '\0';
			mb->m_len += siz;
		}
	} else {
		*tl++ = txdr_unsigned(RPCAUTH_NULL);
		*tl = 0;
	}
	mb->m_next = mrest;
	mreq->m_pkthdr.len = authsiz + 10 * NFSX_UNSIGNED + mrest_len;
	mreq->m_pkthdr.rcvif = (struct ifnet *)0;
	*mbp = mb;
	return (mreq);
}

/*
 * copies mbuf chain to the uio scatter/gather list
 */
int
nfsm_mbuftouio(mrep, uiop, siz, dpos)
	struct mbuf **mrep;
	struct uio *uiop;
	int siz;
	char **dpos;
{
	char *mbufcp, *uiocp;
	int xfer, left, len;
	struct mbuf *mp;
	long uiosiz, rem;
	int error = 0;

	mp = *mrep;
	mbufcp = *dpos;
	len = mtod(mp, char *) + mp->m_len - mbufcp;
	rem = nfsm_rndup(siz)-siz;
	while (siz > 0) {
		if (uiop->uio_iovcnt <= 0 || uiop->uio_iov == NULL)
			return (EFBIG);
		left = uiop->uio_iov->iov_len;
		uiocp = uiop->uio_iov->iov_base;
		if (left > siz)
			left = siz;
		uiosiz = left;
		while (left > 0) {
			while (len == 0) {
				mp = mp->m_next;
				if (mp == NULL)
					return (EBADRPC);
				mbufcp = mtod(mp, void *);
				len = mp->m_len;
			}
			xfer = (left > len) ? len : left;
			error = copyout_vmspace(uiop->uio_vmspace, mbufcp,
			    uiocp, xfer);
			if (error) {
				return error;
			}
			left -= xfer;
			len -= xfer;
			mbufcp += xfer;
			uiocp += xfer;
			uiop->uio_offset += xfer;
			uiop->uio_resid -= xfer;
		}
		if (uiop->uio_iov->iov_len <= siz) {
			uiop->uio_iovcnt--;
			uiop->uio_iov++;
		} else {
			uiop->uio_iov->iov_base =
			    (char *)uiop->uio_iov->iov_base + uiosiz;
			uiop->uio_iov->iov_len -= uiosiz;
		}
		siz -= uiosiz;
	}
	*dpos = mbufcp;
	*mrep = mp;
	if (rem > 0) {
		if (len < rem)
			error = nfs_adv(mrep, dpos, rem, len);
		else
			*dpos += rem;
	}
	return (error);
}

/*
 * copies a uio scatter/gather list to an mbuf chain.
 * NOTE: can ony handle iovcnt == 1
 */
int
nfsm_uiotombuf(uiop, mq, siz, bpos)
	struct uio *uiop;
	struct mbuf **mq;
	int siz;
	char **bpos;
{
	char *uiocp;
	struct mbuf *mp, *mp2;
	int xfer, left, mlen;
	int uiosiz, clflg, rem;
	char *cp;
	int error;

#ifdef DIAGNOSTIC
	if (uiop->uio_iovcnt != 1)
		panic("nfsm_uiotombuf: iovcnt != 1");
#endif

	if (siz > MLEN)		/* or should it >= MCLBYTES ?? */
		clflg = 1;
	else
		clflg = 0;
	rem = nfsm_rndup(siz)-siz;
	mp = mp2 = *mq;
	while (siz > 0) {
		left = uiop->uio_iov->iov_len;
		uiocp = uiop->uio_iov->iov_base;
		if (left > siz)
			left = siz;
		uiosiz = left;
		while (left > 0) {
			mlen = M_TRAILINGSPACE(mp);
			if (mlen == 0) {
				mp = m_get(M_WAIT, MT_DATA);
				MCLAIM(mp, &nfs_mowner);
				if (clflg)
					m_clget(mp, M_WAIT);
				mp->m_len = 0;
				mp2->m_next = mp;
				mp2 = mp;
				mlen = M_TRAILINGSPACE(mp);
			}
			xfer = (left > mlen) ? mlen : left;
			cp = mtod(mp, char *) + mp->m_len;
			error = copyin_vmspace(uiop->uio_vmspace, uiocp, cp,
			    xfer);
			if (error) {
				/* XXX */
			}
			mp->m_len += xfer;
			left -= xfer;
			uiocp += xfer;
			uiop->uio_offset += xfer;
			uiop->uio_resid -= xfer;
		}
		uiop->uio_iov->iov_base = (char *)uiop->uio_iov->iov_base +
		    uiosiz;
		uiop->uio_iov->iov_len -= uiosiz;
		siz -= uiosiz;
	}
	if (rem > 0) {
		if (rem > M_TRAILINGSPACE(mp)) {
			mp = m_get(M_WAIT, MT_DATA);
			MCLAIM(mp, &nfs_mowner);
			mp->m_len = 0;
			mp2->m_next = mp;
		}
		cp = mtod(mp, char *) + mp->m_len;
		for (left = 0; left < rem; left++)
			*cp++ = '\0';
		mp->m_len += rem;
		*bpos = cp;
	} else
		*bpos = mtod(mp, char *) + mp->m_len;
	*mq = mp;
	return (0);
}

/*
 * Get at least "siz" bytes of correctly aligned data.
 * When called the mbuf pointers are not necessarily correct,
 * dsosp points to what ought to be in m_data and left contains
 * what ought to be in m_len.
 * This is used by the macros nfsm_dissect and nfsm_dissecton for tough
 * cases. (The macros use the vars. dpos and dpos2)
 */
int
nfsm_disct(mdp, dposp, siz, left, cp2)
	struct mbuf **mdp;
	char **dposp;
	int siz;
	int left;
	char **cp2;
{
	struct mbuf *m1, *m2;
	struct mbuf *havebuf = NULL;
	char *src = *dposp;
	char *dst;
	int len;

#ifdef DEBUG
	if (left < 0)
		panic("nfsm_disct: left < 0");
#endif
	m1 = *mdp;
	/*
	 * Skip through the mbuf chain looking for an mbuf with
	 * some data. If the first mbuf found has enough data
	 * and it is correctly aligned return it.
	 */
	while (left == 0) {
		havebuf = m1;
		*mdp = m1 = m1->m_next;
		if (m1 == NULL)
			return (EBADRPC);
		src = mtod(m1, void *);
		left = m1->m_len;
		/*
		 * If we start a new mbuf and it is big enough
		 * and correctly aligned just return it, don't
		 * do any pull up.
		 */
		if (left >= siz && nfsm_aligned(src)) {
			*cp2 = src;
			*dposp = src + siz;
			return (0);
		}
	}
	if (m1->m_flags & M_EXT) {
		if (havebuf) {
			/* If the first mbuf with data has external data
			 * and there is a previous empty mbuf use it
			 * to move the data into.
			 */
			m2 = m1;
			*mdp = m1 = havebuf;
			if (m1->m_flags & M_EXT) {
				MEXTREMOVE(m1);
			}
		} else {
			/*
			 * If the first mbuf has a external data
			 * and there is no previous empty mbuf
			 * allocate a new mbuf and move the external
			 * data to the new mbuf. Also make the first
			 * mbuf look empty.
			 */
			m2 = m_get(M_WAIT, MT_DATA);
			m2->m_ext = m1->m_ext;
			m2->m_data = src;
			m2->m_len = left;
			MCLADDREFERENCE(m1, m2);
			MEXTREMOVE(m1);
			m2->m_next = m1->m_next;
			m1->m_next = m2;
		}
		m1->m_len = 0;
		if (m1->m_flags & M_PKTHDR)
			dst = m1->m_pktdat;
		else
			dst = m1->m_dat;
		m1->m_data = dst;
	} else {
		/*
		 * If the first mbuf has no external data
		 * move the data to the front of the mbuf.
		 */
		if (m1->m_flags & M_PKTHDR)
			dst = m1->m_pktdat;
		else
			dst = m1->m_dat;
		m1->m_data = dst;
		if (dst != src)
			memmove(dst, src, left);
		dst += left;
		m1->m_len = left;
		m2 = m1->m_next;
	}
	*cp2 = m1->m_data;
	*dposp = mtod(m1, char *) + siz;
	/*
	 * Loop through mbufs pulling data up into first mbuf until
	 * the first mbuf is full or there is no more data to
	 * pullup.
	 */
	while ((len = M_TRAILINGSPACE(m1)) != 0 && m2) {
		if ((len = min(len, m2->m_len)) != 0)
			memcpy(dst, m2->m_data, len);
		m1->m_len += len;
		dst += len;
		m2->m_data += len;
		m2->m_len -= len;
		m2 = m2->m_next;
	}
	if (m1->m_len < siz)
		return (EBADRPC);
	return (0);
}

/*
 * Advance the position in the mbuf chain.
 */
int
nfs_adv(mdp, dposp, offs, left)
	struct mbuf **mdp;
	char **dposp;
	int offs;
	int left;
{
	struct mbuf *m;
	int s;

	m = *mdp;
	s = left;
	while (s < offs) {
		offs -= s;
		m = m->m_next;
		if (m == NULL)
			return (EBADRPC);
		s = m->m_len;
	}
	*mdp = m;
	*dposp = mtod(m, char *) + offs;
	return (0);
}

/*
 * Copy a string into mbufs for the hard cases...
 */
int
nfsm_strtmbuf(mb, bpos, cp, siz)
	struct mbuf **mb;
	char **bpos;
	const char *cp;
	long siz;
{
	struct mbuf *m1 = NULL, *m2;
	long left, xfer, len, tlen;
	u_int32_t *tl;
	int putsize;

	putsize = 1;
	m2 = *mb;
	left = M_TRAILINGSPACE(m2);
	if (left > 0) {
		tl = ((u_int32_t *)(*bpos));
		*tl++ = txdr_unsigned(siz);
		putsize = 0;
		left -= NFSX_UNSIGNED;
		m2->m_len += NFSX_UNSIGNED;
		if (left > 0) {
			memcpy((void *) tl, cp, left);
			siz -= left;
			cp += left;
			m2->m_len += left;
			left = 0;
		}
	}
	/* Loop around adding mbufs */
	while (siz > 0) {
		m1 = m_get(M_WAIT, MT_DATA);
		MCLAIM(m1, &nfs_mowner);
		if (siz > MLEN)
			m_clget(m1, M_WAIT);
		m1->m_len = NFSMSIZ(m1);
		m2->m_next = m1;
		m2 = m1;
		tl = mtod(m1, u_int32_t *);
		tlen = 0;
		if (putsize) {
			*tl++ = txdr_unsigned(siz);
			m1->m_len -= NFSX_UNSIGNED;
			tlen = NFSX_UNSIGNED;
			putsize = 0;
		}
		if (siz < m1->m_len) {
			len = nfsm_rndup(siz);
			xfer = siz;
			if (xfer < len)
				*(tl+(xfer>>2)) = 0;
		} else {
			xfer = len = m1->m_len;
		}
		memcpy((void *) tl, cp, xfer);
		m1->m_len = len+tlen;
		siz -= xfer;
		cp += xfer;
	}
	*mb = m1;
	*bpos = mtod(m1, char *) + m1->m_len;
	return (0);
}

/*
 * Directory caching routines. They work as follows:
 * - a cache is maintained per VDIR nfsnode.
 * - for each offset cookie that is exported to userspace, and can
 *   thus be thrown back at us as an offset to VOP_READDIR, store
 *   information in the cache.
 * - cached are:
 *   - cookie itself
 *   - blocknumber (essentially just a search key in the buffer cache)
 *   - entry number in block.
 *   - offset cookie of block in which this entry is stored
 *   - 32 bit cookie if NFSMNT_XLATECOOKIE is used.
 * - entries are looked up in a hash table
 * - also maintained is an LRU list of entries, used to determine
 *   which ones to delete if the cache grows too large.
 * - if 32 <-> 64 translation mode is requested for a filesystem,
 *   the cache also functions as a translation table
 * - in the translation case, invalidating the cache does not mean
 *   flushing it, but just marking entries as invalid, except for
 *   the <64bit cookie, 32bitcookie> pair which is still valid, to
 *   still be able to use the cache as a translation table.
 * - 32 bit cookies are uniquely created by combining the hash table
 *   entry value, and one generation count per hash table entry,
 *   incremented each time an entry is appended to the chain.
 * - the cache is invalidated each time a direcory is modified
 * - sanity checks are also done; if an entry in a block turns
 *   out not to have a matching cookie, the cache is invalidated
 *   and a new block starting from the wanted offset is fetched from
 *   the server.
 * - directory entries as read from the server are extended to contain
 *   the 64bit and, optionally, the 32bit cookies, for sanity checking
 *   the cache and exporting them to userspace through the cookie
 *   argument to VOP_READDIR.
 */

u_long
nfs_dirhash(off)
	off_t off;
{
	int i;
	char *cp = (char *)&off;
	u_long sum = 0L;

	for (i = 0 ; i < sizeof (off); i++)
		sum += *cp++;

	return sum;
}

#define	_NFSDC_MTX(np)		(&NFSTOV(np)->v_interlock)
#define	NFSDC_LOCK(np)		simple_lock(_NFSDC_MTX(np))
#define	NFSDC_UNLOCK(np)	simple_unlock(_NFSDC_MTX(np))
#define	NFSDC_ASSERT_LOCKED(np) LOCK_ASSERT(simple_lock_held(_NFSDC_MTX(np)))

void
nfs_initdircache(vp)
	struct vnode *vp;
{
	struct nfsnode *np = VTONFS(vp);
	struct nfsdirhashhead *dircache;

	dircache = hashinit(NFS_DIRHASHSIZ, HASH_LIST, M_NFSDIROFF,
	    M_WAITOK, &nfsdirhashmask);

	NFSDC_LOCK(np);
	if (np->n_dircache == NULL) {
		np->n_dircachesize = 0;
		np->n_dircache = dircache;
		dircache = NULL;
		TAILQ_INIT(&np->n_dirchain);
	}
	NFSDC_UNLOCK(np);
	if (dircache)
		hashdone(dircache, M_NFSDIROFF);
}

void
nfs_initdirxlatecookie(vp)
	struct vnode *vp;
{
	struct nfsnode *np = VTONFS(vp);
	unsigned *dirgens;

	KASSERT(VFSTONFS(vp->v_mount)->nm_flag & NFSMNT_XLATECOOKIE);

	dirgens = malloc(NFS_DIRHASHSIZ * sizeof (unsigned), M_NFSDIROFF,
	    M_WAITOK|M_ZERO);
	NFSDC_LOCK(np);
	if (np->n_dirgens == NULL) {
		np->n_dirgens = dirgens;
		dirgens = NULL;
	}
	NFSDC_UNLOCK(np);
	if (dirgens)
		free(dirgens, M_NFSDIROFF);
}

static const struct nfsdircache dzero;

static void nfs_unlinkdircache __P((struct nfsnode *np, struct nfsdircache *));
static void nfs_putdircache_unlocked __P((struct nfsnode *,
    struct nfsdircache *));

static void
nfs_unlinkdircache(np, ndp)
	struct nfsnode *np;
	struct nfsdircache *ndp;
{

	NFSDC_ASSERT_LOCKED(np);
	KASSERT(ndp != &dzero);

	if (LIST_NEXT(ndp, dc_hash) == (void *)-1)
		return;

	TAILQ_REMOVE(&np->n_dirchain, ndp, dc_chain);
	LIST_REMOVE(ndp, dc_hash);
	LIST_NEXT(ndp, dc_hash) = (void *)-1; /* mark as unlinked */

	nfs_putdircache_unlocked(np, ndp);
}

void
nfs_putdircache(np, ndp)
	struct nfsnode *np;
	struct nfsdircache *ndp;
{
	int ref;

	if (ndp == &dzero)
		return;

	KASSERT(ndp->dc_refcnt > 0);
	NFSDC_LOCK(np);
	ref = --ndp->dc_refcnt;
	NFSDC_UNLOCK(np);

	if (ref == 0)
		free(ndp, M_NFSDIROFF);
}

static void
nfs_putdircache_unlocked(struct nfsnode *np, struct nfsdircache *ndp)
{
	int ref;

	NFSDC_ASSERT_LOCKED(np);

	if (ndp == &dzero)
		return;

	KASSERT(ndp->dc_refcnt > 0);
	ref = --ndp->dc_refcnt;
	if (ref == 0)
		free(ndp, M_NFSDIROFF);
}

struct nfsdircache *
nfs_searchdircache(vp, off, do32, hashent)
	struct vnode *vp;
	off_t off;
	int do32;
	int *hashent;
{
	struct nfsdirhashhead *ndhp;
	struct nfsdircache *ndp = NULL;
	struct nfsnode *np = VTONFS(vp);
	unsigned ent;

	/*
	 * Zero is always a valid cookie.
	 */
	if (off == 0)
		/* XXXUNCONST */
		return (struct nfsdircache *)__UNCONST(&dzero);

	if (!np->n_dircache)
		return NULL;

	/*
	 * We use a 32bit cookie as search key, directly reconstruct
	 * the hashentry. Else use the hashfunction.
	 */
	if (do32) {
		ent = (u_int32_t)off >> 24;
		if (ent >= NFS_DIRHASHSIZ)
			return NULL;
		ndhp = &np->n_dircache[ent];
	} else {
		ndhp = NFSDIRHASH(np, off);
	}

	if (hashent)
		*hashent = (int)(ndhp - np->n_dircache);

	NFSDC_LOCK(np);
	if (do32) {
		LIST_FOREACH(ndp, ndhp, dc_hash) {
			if (ndp->dc_cookie32 == (u_int32_t)off) {
				/*
				 * An invalidated entry will become the
				 * start of a new block fetched from
				 * the server.
				 */
				if (ndp->dc_flags & NFSDC_INVALID) {
					ndp->dc_blkcookie = ndp->dc_cookie;
					ndp->dc_entry = 0;
					ndp->dc_flags &= ~NFSDC_INVALID;
				}
				break;
			}
		}
	} else {
		LIST_FOREACH(ndp, ndhp, dc_hash) {
			if (ndp->dc_cookie == off)
				break;
		}
	}
	if (ndp != NULL)
		ndp->dc_refcnt++;
	NFSDC_UNLOCK(np);
	return ndp;
}


struct nfsdircache *
nfs_enterdircache(struct vnode *vp, off_t off, off_t blkoff, int en,
    daddr_t blkno)
{
	struct nfsnode *np = VTONFS(vp);
	struct nfsdirhashhead *ndhp;
	struct nfsdircache *ndp = NULL;
	struct nfsdircache *newndp = NULL;
	struct nfsmount *nmp = VFSTONFS(vp->v_mount);
	int hashent = 0, gen, overwrite;	/* XXX: GCC */

	/*
	 * XXX refuse entries for offset 0. amd(8) erroneously sets
	 * cookie 0 for the '.' entry, making this necessary. This
	 * isn't so bad, as 0 is a special case anyway.
	 */
	if (off == 0)
		/* XXXUNCONST */
		return (struct nfsdircache *)__UNCONST(&dzero);

	if (!np->n_dircache)
		/*
		 * XXX would like to do this in nfs_nget but vtype
		 * isn't known at that time.
		 */
		nfs_initdircache(vp);

	if ((nmp->nm_flag & NFSMNT_XLATECOOKIE) && !np->n_dirgens)
		nfs_initdirxlatecookie(vp);

retry:
	ndp = nfs_searchdircache(vp, off, 0, &hashent);

	NFSDC_LOCK(np);
	if (ndp && (ndp->dc_flags & NFSDC_INVALID) == 0) {
		/*
		 * Overwriting an old entry. Check if it's the same.
		 * If so, just return. If not, remove the old entry.
		 */
		if (ndp->dc_blkcookie == blkoff && ndp->dc_entry == en)
			goto done;
		nfs_unlinkdircache(np, ndp);
		nfs_putdircache_unlocked(np, ndp);
		ndp = NULL;
	}

	ndhp = &np->n_dircache[hashent];

	if (!ndp) {
		if (newndp == NULL) {
			NFSDC_UNLOCK(np);
			newndp = malloc(sizeof(*ndp), M_NFSDIROFF, M_WAITOK);
			newndp->dc_refcnt = 1;
			LIST_NEXT(newndp, dc_hash) = (void *)-1;
			goto retry;
		}
		ndp = newndp;
		newndp = NULL;
		overwrite = 0;
		if (nmp->nm_flag & NFSMNT_XLATECOOKIE) {
			/*
			 * We're allocating a new entry, so bump the
			 * generation number.
			 */
			KASSERT(np->n_dirgens);
			gen = ++np->n_dirgens[hashent];
			if (gen == 0) {
				np->n_dirgens[hashent]++;
				gen++;
			}
			ndp->dc_cookie32 = (hashent << 24) | (gen & 0xffffff);
		}
	} else
		overwrite = 1;

	ndp->dc_cookie = off;
	ndp->dc_blkcookie = blkoff;
	ndp->dc_entry = en;
	ndp->dc_flags = 0;

	if (overwrite)
		goto done;

	/*
	 * If the maximum directory cookie cache size has been reached
	 * for this node, take one off the front. The idea is that
	 * directories are typically read front-to-back once, so that
	 * the oldest entries can be thrown away without much performance
	 * loss.
	 */
	if (np->n_dircachesize == NFS_MAXDIRCACHE) {
		nfs_unlinkdircache(np, TAILQ_FIRST(&np->n_dirchain));
	} else
		np->n_dircachesize++;

	KASSERT(ndp->dc_refcnt == 1);
	LIST_INSERT_HEAD(ndhp, ndp, dc_hash);
	TAILQ_INSERT_TAIL(&np->n_dirchain, ndp, dc_chain);
	ndp->dc_refcnt++;
done:
	KASSERT(ndp->dc_refcnt > 0);
	NFSDC_UNLOCK(np);
	if (newndp)
		nfs_putdircache(np, newndp);
	return ndp;
}

void
nfs_invaldircache(vp, flags)
	struct vnode *vp;
	int flags;
{
	struct nfsnode *np = VTONFS(vp);
	struct nfsdircache *ndp = NULL;
	struct nfsmount *nmp = VFSTONFS(vp->v_mount);
	const bool forcefree = flags & NFS_INVALDIRCACHE_FORCE;

#ifdef DIAGNOSTIC
	if (vp->v_type != VDIR)
		panic("nfs: invaldircache: not dir");
#endif

	if ((flags & NFS_INVALDIRCACHE_KEEPEOF) == 0)
		np->n_flag &= ~NEOFVALID;

	if (!np->n_dircache)
		return;

	NFSDC_LOCK(np);
	if (!(nmp->nm_flag & NFSMNT_XLATECOOKIE) || forcefree) {
		while ((ndp = TAILQ_FIRST(&np->n_dirchain)) != NULL) {
			KASSERT(!forcefree || ndp->dc_refcnt == 1);
			nfs_unlinkdircache(np, ndp);
		}
		np->n_dircachesize = 0;
		if (forcefree && np->n_dirgens) {
			FREE(np->n_dirgens, M_NFSDIROFF);
			np->n_dirgens = NULL;
		}
	} else {
		TAILQ_FOREACH(ndp, &np->n_dirchain, dc_chain)
			ndp->dc_flags |= NFSDC_INVALID;
	}

	NFSDC_UNLOCK(np);
}

/*
 * Called once before VFS init to initialize shared and
 * server-specific data structures.
 */
static int
nfs_init0(void)
{

	nfsrtt.pos = 0;
	rpc_vers = txdr_unsigned(RPC_VER2);
	rpc_call = txdr_unsigned(RPC_CALL);
	rpc_reply = txdr_unsigned(RPC_REPLY);
	rpc_msgdenied = txdr_unsigned(RPC_MSGDENIED);
	rpc_msgaccepted = txdr_unsigned(RPC_MSGACCEPTED);
	rpc_mismatch = txdr_unsigned(RPC_MISMATCH);
	rpc_autherr = txdr_unsigned(RPC_AUTHERR);
	rpc_auth_unix = txdr_unsigned(RPCAUTH_UNIX);
	rpc_auth_kerb = txdr_unsigned(RPCAUTH_KERB4);
	nfs_prog = txdr_unsigned(NFS_PROG);
	nfs_true = txdr_unsigned(true);
	nfs_false = txdr_unsigned(false);
	nfs_xdrneg1 = txdr_unsigned(-1);
	nfs_ticks = (hz * NFS_TICKINTVL + 500) / 1000;
	if (nfs_ticks < 1)
		nfs_ticks = 1;
#ifdef NFSSERVER
	nfsrv_init(0);			/* Init server data structures */
	nfsrv_initcache();		/* Init the server request cache */
	{
		extern krwlock_t netexport_lock;	/* XXX */
		rw_init(&netexport_lock);
	}
#endif /* NFSSERVER */

#if defined(NFSSERVER) || (defined(NFS) && !defined(NFS_V2_ONLY))
	nfsdreq_init();
#endif /* defined(NFSSERVER) || (defined(NFS) && !defined(NFS_V2_ONLY)) */

	/*
	 * Initialize reply list and start timer
	 */
	TAILQ_INIT(&nfs_reqq);
	nfs_timer(nfs_timer);
	MOWNER_ATTACH(&nfs_mowner);

#ifdef NFS
	/* Initialize the kqueue structures */
	nfs_kqinit();
	/* Initialize the iod structures */
	nfs_iodinit();
#endif
	return 0;
}

void
nfs_init(void)
{
	static ONCE_DECL(nfs_init_once);

	RUN_ONCE(&nfs_init_once, nfs_init0);
}

#ifdef NFS
/*
 * Called once at VFS init to initialize client-specific data structures.
 */
void
nfs_vfs_init()
{
	/* Initialize NFS server / client shared data. */
	nfs_init();

	nfs_nhinit();			/* Init the nfsnode table */
	nfs_commitsize = uvmexp.npages << (PAGE_SHIFT - 4);
}

void
nfs_vfs_reinit()
{
	nfs_nhreinit();
}

void
nfs_vfs_done()
{
	nfs_nhdone();
}

/*
 * Attribute cache routines.
 * nfs_loadattrcache() - loads or updates the cache contents from attributes
 *	that are on the mbuf list
 * nfs_getattrcache() - returns valid attributes if found in cache, returns
 *	error otherwise
 */

/*
 * Load the attribute cache (that lives in the nfsnode entry) with
 * the values on the mbuf list and
 * Iff vap not NULL
 *    copy the attributes to *vaper
 */
int
nfsm_loadattrcache(vpp, mdp, dposp, vaper, flags)
	struct vnode **vpp;
	struct mbuf **mdp;
	char **dposp;
	struct vattr *vaper;
	int flags;
{
	int32_t t1;
	char *cp2;
	int error = 0;
	struct mbuf *md;
	int v3 = NFS_ISV3(*vpp);

	md = *mdp;
	t1 = (mtod(md, char *) + md->m_len) - *dposp;
	error = nfsm_disct(mdp, dposp, NFSX_FATTR(v3), t1, &cp2);
	if (error)
		return (error);
	return nfs_loadattrcache(vpp, (struct nfs_fattr *)cp2, vaper, flags);
}

int
nfs_loadattrcache(vpp, fp, vaper, flags)
	struct vnode **vpp;
	struct nfs_fattr *fp;
	struct vattr *vaper;
	int flags;
{
	struct vnode *vp = *vpp;
	struct vattr *vap;
	int v3 = NFS_ISV3(vp);
	enum vtype vtyp;
	u_short vmode;
	struct timespec mtime;
	struct timespec ctime;
	struct vnode *nvp;
	int32_t rdev;
	struct nfsnode *np;
	extern int (**spec_nfsv2nodeop_p) __P((void *));
	uid_t uid;
	gid_t gid;

	if (v3) {
		vtyp = nfsv3tov_type(fp->fa_type);
		vmode = fxdr_unsigned(u_short, fp->fa_mode);
		rdev = makedev(fxdr_unsigned(u_int32_t, fp->fa3_rdev.specdata1),
			fxdr_unsigned(u_int32_t, fp->fa3_rdev.specdata2));
		fxdr_nfsv3time(&fp->fa3_mtime, &mtime);
		fxdr_nfsv3time(&fp->fa3_ctime, &ctime);
	} else {
		vtyp = nfsv2tov_type(fp->fa_type);
		vmode = fxdr_unsigned(u_short, fp->fa_mode);
		if (vtyp == VNON || vtyp == VREG)
			vtyp = IFTOVT(vmode);
		rdev = fxdr_unsigned(int32_t, fp->fa2_rdev);
		fxdr_nfsv2time(&fp->fa2_mtime, &mtime);
		ctime.tv_sec = fxdr_unsigned(u_int32_t,
		    fp->fa2_ctime.nfsv2_sec);
		ctime.tv_nsec = 0;

		/*
		 * Really ugly NFSv2 kludge.
		 */
		if (vtyp == VCHR && rdev == 0xffffffff)
			vtyp = VFIFO;
	}

	vmode &= ALLPERMS;

	/*
	 * If v_type == VNON it is a new node, so fill in the v_type,
	 * n_mtime fields. Check to see if it represents a special
	 * device, and if so, check for a possible alias. Once the
	 * correct vnode has been obtained, fill in the rest of the
	 * information.
	 */
	np = VTONFS(vp);
	if (vp->v_type == VNON) {
		vp->v_type = vtyp;
		if (vp->v_type == VFIFO) {
			extern int (**fifo_nfsv2nodeop_p) __P((void *));
			vp->v_op = fifo_nfsv2nodeop_p;
		} else if (vp->v_type == VREG) {
			mutex_init(&np->n_commitlock, MUTEX_DEFAULT, IPL_NONE);
		} else if (vp->v_type == VCHR || vp->v_type == VBLK) {
			vp->v_op = spec_nfsv2nodeop_p;
			nvp = checkalias(vp, (dev_t)rdev, vp->v_mount);
			if (nvp) {
				/*
				 * Discard unneeded vnode, but save its nfsnode.
				 * Since the nfsnode does not have a lock, its
				 * vnode lock has to be carried over.
				 */
				/*
				 * XXX is the old node sure to be locked here?
				 */
				KASSERT(lockstatus(&vp->v_lock) ==
				    LK_EXCLUSIVE);
				nvp->v_data = vp->v_data;
				vp->v_data = NULL;
				VOP_UNLOCK(vp, 0);
				vp->v_op = spec_vnodeop_p;
				vrele(vp);
				vgone(vp);
				lockmgr(&nvp->v_lock, LK_EXCLUSIVE,
				    &nvp->v_interlock);
				/*
				 * Reinitialize aliased node.
				 */
				np->n_vnode = nvp;
				*vpp = vp = nvp;
			}
		}
		np->n_mtime = mtime;
	}
	uid = fxdr_unsigned(uid_t, fp->fa_uid);
	gid = fxdr_unsigned(gid_t, fp->fa_gid);
	vap = np->n_vattr;

	/*
	 * Invalidate access cache if uid, gid, mode or ctime changed.
	 */
	if (np->n_accstamp != -1 &&
	    (gid != vap->va_gid || uid != vap->va_uid || vmode != vap->va_mode
	    || timespeccmp(&ctime, &vap->va_ctime, !=)))
		np->n_accstamp = -1;

	vap->va_type = vtyp;
	vap->va_mode = vmode;
	vap->va_rdev = (dev_t)rdev;
	vap->va_mtime = mtime;
	vap->va_ctime = ctime;
	vap->va_fsid = vp->v_mount->mnt_stat.f_fsidx.__fsid_val[0];
	switch (vtyp) {
	case VDIR:
		vap->va_blocksize = NFS_DIRFRAGSIZ;
		break;
	case VBLK:
		vap->va_blocksize = BLKDEV_IOSIZE;
		break;
	case VCHR:
		vap->va_blocksize = MAXBSIZE;
		break;
	default:
		vap->va_blocksize = v3 ? vp->v_mount->mnt_stat.f_iosize :
		    fxdr_unsigned(int32_t, fp->fa2_blocksize);
		break;
	}
	if (v3) {
		vap->va_nlink = fxdr_unsigned(u_short, fp->fa_nlink);
		vap->va_uid = uid;
		vap->va_gid = gid;
		vap->va_size = fxdr_hyper(&fp->fa3_size);
		vap->va_bytes = fxdr_hyper(&fp->fa3_used);
		vap->va_fileid = fxdr_hyper(&fp->fa3_fileid);
		fxdr_nfsv3time(&fp->fa3_atime, &vap->va_atime);
		vap->va_flags = 0;
		vap->va_filerev = 0;
	} else {
		vap->va_nlink = fxdr_unsigned(u_short, fp->fa_nlink);
		vap->va_uid = uid;
		vap->va_gid = gid;
		vap->va_size = fxdr_unsigned(u_int32_t, fp->fa2_size);
		vap->va_bytes = fxdr_unsigned(int32_t, fp->fa2_blocks)
		    * NFS_FABLKSIZE;
		vap->va_fileid = fxdr_unsigned(int32_t, fp->fa2_fileid);
		fxdr_nfsv2time(&fp->fa2_atime, &vap->va_atime);
		vap->va_flags = 0;
		vap->va_gen = fxdr_unsigned(u_int32_t,fp->fa2_ctime.nfsv2_usec);
		vap->va_filerev = 0;
	}
	if (vap->va_size != np->n_size) {
		if ((np->n_flag & NMODIFIED) && vap->va_size < np->n_size) {
			vap->va_size = np->n_size;
		} else {
			np->n_size = vap->va_size;
			if (vap->va_type == VREG) {
				/*
				 * we can't free pages if NAC_NOTRUNC because
				 * the pages can be owned by ourselves.
				 */
				if (flags & NAC_NOTRUNC) {
					np->n_flag |= NTRUNCDELAYED;
				} else {
					genfs_node_wrlock(vp);
					simple_lock(&vp->v_interlock);
					(void)VOP_PUTPAGES(vp, 0,
					    0, PGO_SYNCIO | PGO_CLEANIT |
					    PGO_FREE | PGO_ALLPAGES);
					uvm_vnp_setsize(vp, np->n_size);
					genfs_node_unlock(vp);
				}
			}
		}
	}
	np->n_attrstamp = time_second;
	if (vaper != NULL) {
		memcpy((void *)vaper, (void *)vap, sizeof(*vap));
		if (np->n_flag & NCHG) {
			if (np->n_flag & NACC)
				vaper->va_atime = np->n_atim;
			if (np->n_flag & NUPD)
				vaper->va_mtime = np->n_mtim;
		}
	}
	return (0);
}

/*
 * Check the time stamp
 * If the cache is valid, copy contents to *vap and return 0
 * otherwise return an error
 */
int
nfs_getattrcache(vp, vaper)
	struct vnode *vp;
	struct vattr *vaper;
{
	struct nfsnode *np = VTONFS(vp);
	struct nfsmount *nmp = VFSTONFS(vp->v_mount);
	struct vattr *vap;

	if (np->n_attrstamp == 0 ||
	    (time_second - np->n_attrstamp) >= NFS_ATTRTIMEO(nmp, np)) {
		nfsstats.attrcache_misses++;
		return (ENOENT);
	}
	nfsstats.attrcache_hits++;
	vap = np->n_vattr;
	if (vap->va_size != np->n_size) {
		if (vap->va_type == VREG) {
			if ((np->n_flag & NMODIFIED) != 0 &&
			    vap->va_size < np->n_size) {
				vap->va_size = np->n_size;
			} else {
				np->n_size = vap->va_size;
			}
			genfs_node_wrlock(vp);
			uvm_vnp_setsize(vp, np->n_size);
			genfs_node_unlock(vp);
		} else
			np->n_size = vap->va_size;
	}
	memcpy((void *)vaper, (void *)vap, sizeof(struct vattr));
	if (np->n_flag & NCHG) {
		if (np->n_flag & NACC)
			vaper->va_atime = np->n_atim;
		if (np->n_flag & NUPD)
			vaper->va_mtime = np->n_mtim;
	}
	return (0);
}

void
nfs_delayedtruncate(vp)
	struct vnode *vp;
{
	struct nfsnode *np = VTONFS(vp);

	if (np->n_flag & NTRUNCDELAYED) {
		np->n_flag &= ~NTRUNCDELAYED;
		genfs_node_wrlock(vp);
		simple_lock(&vp->v_interlock);
		(void)VOP_PUTPAGES(vp, 0,
		    0, PGO_SYNCIO | PGO_CLEANIT | PGO_FREE | PGO_ALLPAGES);
		uvm_vnp_setsize(vp, np->n_size);
		genfs_node_unlock(vp);
	}
}

#define	NFS_WCCKLUDGE_TIMEOUT	(24 * 60 * 60)	/* 1 day */
#define	NFS_WCCKLUDGE(nmp, now) \
	(((nmp)->nm_iflag & NFSMNT_WCCKLUDGE) && \
	((now) - (nmp)->nm_wcckludgetime - NFS_WCCKLUDGE_TIMEOUT) < 0)

/*
 * nfs_check_wccdata: check inaccurate wcc_data
 *
 * => return non-zero if we shouldn't trust the wcc_data.
 * => NFS_WCCKLUDGE_TIMEOUT is for the case that the server is "fixed".
 */

int
nfs_check_wccdata(struct nfsnode *np, const struct timespec *ctime,
    struct timespec *mtime, bool docheck)
{
	int error = 0;

#if !defined(NFS_V2_ONLY)

	if (docheck) {
		struct vnode *vp = NFSTOV(np);
		struct nfsmount *nmp;
		long now = time_second;
		const struct timespec *omtime = &np->n_vattr->va_mtime;
		const struct timespec *octime = &np->n_vattr->va_ctime;
#if defined(DEBUG)
		const char *reason = NULL; /* XXX: gcc */
#endif

		if (timespeccmp(omtime, mtime, <=)) {
#if defined(DEBUG)
			reason = "mtime";
#endif
			error = EINVAL;
		}

		if (vp->v_type == VDIR && timespeccmp(octime, ctime, <=)) {
#if defined(DEBUG)
			reason = "ctime";
#endif
			error = EINVAL;
		}

		nmp = VFSTONFS(vp->v_mount);
		if (error) {

			/*
			 * despite of the fact that we've updated the file,
			 * timestamps of the file were not updated as we
			 * expected.
			 * it means that the server has incompatible
			 * semantics of timestamps or (more likely)
			 * the server time is not precise enough to
			 * track each modifications.
			 * in that case, we disable wcc processing.
			 *
			 * yes, strictly speaking, we should disable all
			 * caching.  it's a compromise.
			 */

			mutex_enter(&nmp->nm_lock);
#if defined(DEBUG)
			if (!NFS_WCCKLUDGE(nmp, now)) {
				printf("%s: inaccurate wcc data (%s) detected,"
				    " disabling wcc"
				    " (ctime %u.%09u %u.%09u,"
				    " mtime %u.%09u %u.%09u)\n",
				    vp->v_mount->mnt_stat.f_mntfromname,
				    reason,
				    (unsigned int)octime->tv_sec,
				    (unsigned int)octime->tv_nsec,
				    (unsigned int)ctime->tv_sec,
				    (unsigned int)ctime->tv_nsec,
				    (unsigned int)omtime->tv_sec,
				    (unsigned int)omtime->tv_nsec,
				    (unsigned int)mtime->tv_sec,
				    (unsigned int)mtime->tv_nsec);
			}
#endif
			nmp->nm_iflag |= NFSMNT_WCCKLUDGE;
			nmp->nm_wcckludgetime = now;
			mutex_exit(&nmp->nm_lock);
		} else if (NFS_WCCKLUDGE(nmp, now)) {
			error = EPERM; /* XXX */
		} else if (nmp->nm_iflag & NFSMNT_WCCKLUDGE) {
			mutex_enter(&nmp->nm_lock);
			if (nmp->nm_iflag & NFSMNT_WCCKLUDGE) {
#if defined(DEBUG)
				printf("%s: re-enabling wcc\n",
				    vp->v_mount->mnt_stat.f_mntfromname);
#endif
				nmp->nm_iflag &= ~NFSMNT_WCCKLUDGE;
			}
			mutex_exit(&nmp->nm_lock);
		}
	}

#endif /* !defined(NFS_V2_ONLY) */

	return error;
}

/*
 * Heuristic to see if the server XDR encodes directory cookies or not.
 * it is not supposed to, but a lot of servers may do this. Also, since
 * most/all servers will implement V2 as well, it is expected that they
 * may return just 32 bits worth of cookie information, so we need to
 * find out in which 32 bits this information is available. We do this
 * to avoid trouble with emulated binaries that can't handle 64 bit
 * directory offsets.
 */

void
nfs_cookieheuristic(vp, flagp, l, cred)
	struct vnode *vp;
	int *flagp;
	struct lwp *l;
	kauth_cred_t cred;
{
	struct uio auio;
	struct iovec aiov;
	char *tbuf, *cp;
	struct dirent *dp;
	off_t *cookies = NULL, *cop;
	int error, eof, nc, len;

	MALLOC(tbuf, void *, NFS_DIRFRAGSIZ, M_TEMP, M_WAITOK);

	aiov.iov_base = tbuf;
	aiov.iov_len = NFS_DIRFRAGSIZ;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_READ;
	auio.uio_resid = NFS_DIRFRAGSIZ;
	auio.uio_offset = 0;
	UIO_SETUP_SYSSPACE(&auio);

	error = VOP_READDIR(vp, &auio, cred, &eof, &cookies, &nc);

	len = NFS_DIRFRAGSIZ - auio.uio_resid;
	if (error || len == 0) {
		FREE(tbuf, M_TEMP);
		if (cookies)
			free(cookies, M_TEMP);
		return;
	}

	/*
	 * Find the first valid entry and look at its offset cookie.
	 */

	cp = tbuf;
	for (cop = cookies; len > 0; len -= dp->d_reclen) {
		dp = (struct dirent *)cp;
		if (dp->d_fileno != 0 && len >= dp->d_reclen) {
			if ((*cop >> 32) != 0 && (*cop & 0xffffffffLL) == 0) {
				*flagp |= NFSMNT_SWAPCOOKIE;
				nfs_invaldircache(vp, 0);
				nfs_vinvalbuf(vp, 0, cred, l, 1);
			}
			break;
		}
		cop++;
		cp += dp->d_reclen;
	}

	FREE(tbuf, M_TEMP);
	free(cookies, M_TEMP);
}
#endif /* NFS */

#ifdef NFSSERVER
/*
 * Set up nameidata for a lookup() call and do it.
 *
 * If pubflag is set, this call is done for a lookup operation on the
 * public filehandle. In that case we allow crossing mountpoints and
 * absolute pathnames. However, the caller is expected to check that
 * the lookup result is within the public fs, and deny access if
 * it is not.
 */
int
nfs_namei(ndp, nsfh, len, slp, nam, mdp, dposp, retdirp, l, kerbflag, pubflag)
	struct nameidata *ndp;
	nfsrvfh_t *nsfh;
	uint32_t len;
	struct nfssvc_sock *slp;
	struct mbuf *nam;
	struct mbuf **mdp;
	char **dposp;
	struct vnode **retdirp;
	struct lwp *l;
	int kerbflag, pubflag;
{
	int i, rem;
	struct mbuf *md;
	char *fromcp, *tocp, *cp;
	struct iovec aiov;
	struct uio auio;
	struct vnode *dp;
	int error, rdonly, linklen;
	struct componentname *cnp = &ndp->ni_cnd;

	*retdirp = NULL;

	if ((len + 1) > MAXPATHLEN)
		return (ENAMETOOLONG);
	if (len == 0)
		return (EACCES);
	cnp->cn_pnbuf = PNBUF_GET();

	/*
	 * Copy the name from the mbuf list to ndp->ni_pnbuf
	 * and set the various ndp fields appropriately.
	 */
	fromcp = *dposp;
	tocp = cnp->cn_pnbuf;
	md = *mdp;
	rem = mtod(md, char *) + md->m_len - fromcp;
	for (i = 0; i < len; i++) {
		while (rem == 0) {
			md = md->m_next;
			if (md == NULL) {
				error = EBADRPC;
				goto out;
			}
			fromcp = mtod(md, void *);
			rem = md->m_len;
		}
		if (*fromcp == '\0' || (!pubflag && *fromcp == '/')) {
			error = EACCES;
			goto out;
		}
		*tocp++ = *fromcp++;
		rem--;
	}
	*tocp = '\0';
	*mdp = md;
	*dposp = fromcp;
	len = nfsm_rndup(len)-len;
	if (len > 0) {
		if (rem >= len)
			*dposp += len;
		else if ((error = nfs_adv(mdp, dposp, len, rem)) != 0)
			goto out;
	}

	/*
	 * Extract and set starting directory.
	 */
	error = nfsrv_fhtovp(nsfh, false, &dp, ndp->ni_cnd.cn_cred, slp,
	    nam, &rdonly, kerbflag, pubflag);
	if (error)
		goto out;
	if (dp->v_type != VDIR) {
		vrele(dp);
		error = ENOTDIR;
		goto out;
	}

	if (rdonly)
		cnp->cn_flags |= RDONLY;

	*retdirp = dp;

	if (pubflag) {
		/*
		 * Oh joy. For WebNFS, handle those pesky '%' escapes,
		 * and the 'native path' indicator.
		 */
		cp = PNBUF_GET();
		fromcp = cnp->cn_pnbuf;
		tocp = cp;
		if ((unsigned char)*fromcp >= WEBNFS_SPECCHAR_START) {
			switch ((unsigned char)*fromcp) {
			case WEBNFS_NATIVE_CHAR:
				/*
				 * 'Native' path for us is the same
				 * as a path according to the NFS spec,
				 * just skip the escape char.
				 */
				fromcp++;
				break;
			/*
			 * More may be added in the future, range 0x80-0xff
			 */
			default:
				error = EIO;
				vrele(dp);
				PNBUF_PUT(cp);
				goto out;
			}
		}
		/*
		 * Translate the '%' escapes, URL-style.
		 */
		while (*fromcp != '\0') {
			if (*fromcp == WEBNFS_ESC_CHAR) {
				if (fromcp[1] != '\0' && fromcp[2] != '\0') {
					fromcp++;
					*tocp++ = HEXSTRTOI(fromcp);
					fromcp += 2;
					continue;
				} else {
					error = ENOENT;
					vrele(dp);
					PNBUF_PUT(cp);
					goto out;
				}
			} else
				*tocp++ = *fromcp++;
		}
		*tocp = '\0';
		PNBUF_PUT(cnp->cn_pnbuf);
		cnp->cn_pnbuf = cp;
	}

	ndp->ni_pathlen = (tocp - cnp->cn_pnbuf) + 1;
	ndp->ni_segflg = UIO_SYSSPACE;
	ndp->ni_rootdir = rootvnode;
	ndp->ni_erootdir = NULL;

	if (pubflag) {
		ndp->ni_loopcnt = 0;
		if (cnp->cn_pnbuf[0] == '/')
			dp = rootvnode;
	} else {
		cnp->cn_flags |= NOCROSSMOUNT;
	}

	cnp->cn_lwp = l;
	VREF(dp);
	vn_lock(dp, LK_EXCLUSIVE | LK_RETRY);

    for (;;) {
	cnp->cn_nameptr = cnp->cn_pnbuf;
	ndp->ni_startdir = dp;

	/*
	 * And call lookup() to do the real work
	 */
	error = lookup(ndp);
	if (error) {
		if (ndp->ni_dvp) {
			vput(ndp->ni_dvp);
		}
		PNBUF_PUT(cnp->cn_pnbuf);
		return (error);
	}

	/*
	 * Check for encountering a symbolic link
	 */
	if ((cnp->cn_flags & ISSYMLINK) == 0) {
		if ((cnp->cn_flags & LOCKPARENT) == 0 && ndp->ni_dvp) {
			if (ndp->ni_dvp == ndp->ni_vp) {
				vrele(ndp->ni_dvp);
			} else {
				vput(ndp->ni_dvp);
			}
		}
		if (cnp->cn_flags & (SAVENAME | SAVESTART))
			cnp->cn_flags |= HASBUF;
		else
			PNBUF_PUT(cnp->cn_pnbuf);
		return (0);
	} else {
		if (!pubflag) {
			error = EINVAL;
			break;
		}
		if (ndp->ni_loopcnt++ >= MAXSYMLINKS) {
			error = ELOOP;
			break;
		}
		if (ndp->ni_vp->v_mount->mnt_flag & MNT_SYMPERM) {
			error = VOP_ACCESS(ndp->ni_vp, VEXEC, cnp->cn_cred,
			    cnp->cn_lwp);
			if (error != 0)
				break;
		}
		if (ndp->ni_pathlen > 1)
			cp = PNBUF_GET();
		else
			cp = cnp->cn_pnbuf;
		aiov.iov_base = cp;
		aiov.iov_len = MAXPATHLEN;
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_offset = 0;
		auio.uio_rw = UIO_READ;
		auio.uio_resid = MAXPATHLEN;
		UIO_SETUP_SYSSPACE(&auio);
		error = VOP_READLINK(ndp->ni_vp, &auio, cnp->cn_cred);
		if (error) {
badlink:
			if (ndp->ni_pathlen > 1)
				PNBUF_PUT(cp);
			break;
		}
		linklen = MAXPATHLEN - auio.uio_resid;
		if (linklen == 0) {
			error = ENOENT;
			goto badlink;
		}
		if (linklen + ndp->ni_pathlen >= MAXPATHLEN) {
			error = ENAMETOOLONG;
			goto badlink;
		}
		if (ndp->ni_pathlen > 1) {
			memcpy(cp + linklen, ndp->ni_next, ndp->ni_pathlen);
			PNBUF_PUT(cnp->cn_pnbuf);
			cnp->cn_pnbuf = cp;
		} else
			cnp->cn_pnbuf[linklen] = '\0';
		ndp->ni_pathlen += linklen;
		vput(ndp->ni_vp);
		dp = ndp->ni_dvp;

		/*
		 * Check if root directory should replace current directory.
		 */
		if (cnp->cn_pnbuf[0] == '/') {
			vput(dp);
			dp = ndp->ni_rootdir;
			VREF(dp);
			vn_lock(dp, LK_EXCLUSIVE | LK_RETRY);
		}
	}
   }
	vput(ndp->ni_dvp);
	vput(ndp->ni_vp);
	ndp->ni_vp = NULL;
out:
	PNBUF_PUT(cnp->cn_pnbuf);
	return (error);
}
#endif /* NFSSERVER */

/*
 * A fiddled version of m_adj() that ensures null fill to a 32-bit
 * boundary and only trims off the back end
 *
 * 1. trim off 'len' bytes as m_adj(mp, -len).
 * 2. add zero-padding 'nul' bytes at the end of the mbuf chain.
 */
void
nfs_zeropad(mp, len, nul)
	struct mbuf *mp;
	int len;
	int nul;
{
	struct mbuf *m;
	int count;

	/*
	 * Trim from tail.  Scan the mbuf chain,
	 * calculating its length and finding the last mbuf.
	 * If the adjustment only affects this mbuf, then just
	 * adjust and return.  Otherwise, rescan and truncate
	 * after the remaining size.
	 */
	count = 0;
	m = mp;
	for (;;) {
		count += m->m_len;
		if (m->m_next == NULL)
			break;
		m = m->m_next;
	}

	KDASSERT(count >= len);

	if (m->m_len >= len) {
		m->m_len -= len;
	} else {
		count -= len;
		/*
		 * Correct length for chain is "count".
		 * Find the mbuf with last data, adjust its length,
		 * and toss data from remaining mbufs on chain.
		 */
		for (m = mp; m; m = m->m_next) {
			if (m->m_len >= count) {
				m->m_len = count;
				break;
			}
			count -= m->m_len;
		}
		KASSERT(m && m->m_next);
		m_freem(m->m_next);
		m->m_next = NULL;
	}

	KDASSERT(m->m_next == NULL);

	/*
	 * zero-padding.
	 */
	if (nul > 0) {
		char *cp;
		int i;

		if (M_ROMAP(m) || M_TRAILINGSPACE(m) < nul) {
			struct mbuf *n;

			KDASSERT(MLEN >= nul);
			n = m_get(M_WAIT, MT_DATA);
			MCLAIM(n, &nfs_mowner);
			n->m_len = nul;
			n->m_next = NULL;
			m->m_next = n;
			cp = mtod(n, void *);
		} else {
			cp = mtod(m, char *) + m->m_len;
			m->m_len += nul;
		}
		for (i = 0; i < nul; i++)
			*cp++ = '\0';
	}
	return;
}

/*
 * Make these functions instead of macros, so that the kernel text size
 * doesn't get too big...
 */
void
nfsm_srvwcc(nfsd, before_ret, before_vap, after_ret, after_vap, mbp, bposp)
	struct nfsrv_descript *nfsd;
	int before_ret;
	struct vattr *before_vap;
	int after_ret;
	struct vattr *after_vap;
	struct mbuf **mbp;
	char **bposp;
{
	struct mbuf *mb = *mbp;
	char *bpos = *bposp;
	u_int32_t *tl;

	if (before_ret) {
		nfsm_build(tl, u_int32_t *, NFSX_UNSIGNED);
		*tl = nfs_false;
	} else {
		nfsm_build(tl, u_int32_t *, 7 * NFSX_UNSIGNED);
		*tl++ = nfs_true;
		txdr_hyper(before_vap->va_size, tl);
		tl += 2;
		txdr_nfsv3time(&(before_vap->va_mtime), tl);
		tl += 2;
		txdr_nfsv3time(&(before_vap->va_ctime), tl);
	}
	*bposp = bpos;
	*mbp = mb;
	nfsm_srvpostopattr(nfsd, after_ret, after_vap, mbp, bposp);
}

void
nfsm_srvpostopattr(nfsd, after_ret, after_vap, mbp, bposp)
	struct nfsrv_descript *nfsd;
	int after_ret;
	struct vattr *after_vap;
	struct mbuf **mbp;
	char **bposp;
{
	struct mbuf *mb = *mbp;
	char *bpos = *bposp;
	u_int32_t *tl;
	struct nfs_fattr *fp;

	if (after_ret) {
		nfsm_build(tl, u_int32_t *, NFSX_UNSIGNED);
		*tl = nfs_false;
	} else {
		nfsm_build(tl, u_int32_t *, NFSX_UNSIGNED + NFSX_V3FATTR);
		*tl++ = nfs_true;
		fp = (struct nfs_fattr *)tl;
		nfsm_srvfattr(nfsd, after_vap, fp);
	}
	*mbp = mb;
	*bposp = bpos;
}

void
nfsm_srvfattr(nfsd, vap, fp)
	struct nfsrv_descript *nfsd;
	struct vattr *vap;
	struct nfs_fattr *fp;
{

	fp->fa_nlink = txdr_unsigned(vap->va_nlink);
	fp->fa_uid = txdr_unsigned(vap->va_uid);
	fp->fa_gid = txdr_unsigned(vap->va_gid);
	if (nfsd->nd_flag & ND_NFSV3) {
		fp->fa_type = vtonfsv3_type(vap->va_type);
		fp->fa_mode = vtonfsv3_mode(vap->va_mode);
		txdr_hyper(vap->va_size, &fp->fa3_size);
		txdr_hyper(vap->va_bytes, &fp->fa3_used);
		fp->fa3_rdev.specdata1 = txdr_unsigned(major(vap->va_rdev));
		fp->fa3_rdev.specdata2 = txdr_unsigned(minor(vap->va_rdev));
		fp->fa3_fsid.nfsuquad[0] = 0;
		fp->fa3_fsid.nfsuquad[1] = txdr_unsigned(vap->va_fsid);
		txdr_hyper(vap->va_fileid, &fp->fa3_fileid);
		txdr_nfsv3time(&vap->va_atime, &fp->fa3_atime);
		txdr_nfsv3time(&vap->va_mtime, &fp->fa3_mtime);
		txdr_nfsv3time(&vap->va_ctime, &fp->fa3_ctime);
	} else {
		fp->fa_type = vtonfsv2_type(vap->va_type);
		fp->fa_mode = vtonfsv2_mode(vap->va_type, vap->va_mode);
		fp->fa2_size = txdr_unsigned(vap->va_size);
		fp->fa2_blocksize = txdr_unsigned(vap->va_blocksize);
		if (vap->va_type == VFIFO)
			fp->fa2_rdev = 0xffffffff;
		else
			fp->fa2_rdev = txdr_unsigned(vap->va_rdev);
		fp->fa2_blocks = txdr_unsigned(vap->va_bytes / NFS_FABLKSIZE);
		fp->fa2_fsid = txdr_unsigned(vap->va_fsid);
		fp->fa2_fileid = txdr_unsigned(vap->va_fileid);
		txdr_nfsv2time(&vap->va_atime, &fp->fa2_atime);
		txdr_nfsv2time(&vap->va_mtime, &fp->fa2_mtime);
		txdr_nfsv2time(&vap->va_ctime, &fp->fa2_ctime);
	}
}

#ifdef NFSSERVER
/*
 * nfsrv_fhtovp() - convert a fh to a vnode ptr (optionally locked)
 * 	- look up fsid in mount list (if not found ret error)
 *	- get vp and export rights by calling VFS_FHTOVP()
 *	- if cred->cr_uid == 0 or MNT_EXPORTANON set it to credanon
 *	- if not lockflag unlock it with VOP_UNLOCK()
 */
int
nfsrv_fhtovp(nfsrvfh_t *nsfh, int lockflag, struct vnode **vpp,
    kauth_cred_t cred, struct nfssvc_sock *slp, struct mbuf *nam, int *rdonlyp,
    int kerbflag, int pubflag)
{
	struct mount *mp;
	kauth_cred_t credanon;
	int error, exflags;
	struct sockaddr_in *saddr;
	fhandle_t *fhp;

	fhp = NFSRVFH_FHANDLE(nsfh);
	*vpp = (struct vnode *)0;

	if (nfs_ispublicfh(nsfh)) {
		if (!pubflag || !nfs_pub.np_valid)
			return (ESTALE);
		fhp = nfs_pub.np_handle;
	}

	error = netexport_check(&fhp->fh_fsid, nam, &mp, &exflags, &credanon);
	if (error) {
		return error;
	}

	error = VFS_FHTOVP(mp, &fhp->fh_fid, vpp);
	if (error)
		return (error);

	if (!(exflags & (MNT_EXNORESPORT|MNT_EXPUBLIC))) {
		saddr = mtod(nam, struct sockaddr_in *);
		if ((saddr->sin_family == AF_INET) &&
		    ntohs(saddr->sin_port) >= IPPORT_RESERVED) {
			vput(*vpp);
			return (NFSERR_AUTHERR | AUTH_TOOWEAK);
		}
#ifdef INET6
		if ((saddr->sin_family == AF_INET6) &&
		    ntohs(saddr->sin_port) >= IPV6PORT_RESERVED) {
			vput(*vpp);
			return (NFSERR_AUTHERR | AUTH_TOOWEAK);
		}
#endif
	}
	/*
	 * Check/setup credentials.
	 */
	if (exflags & MNT_EXKERB) {
		if (!kerbflag) {
			vput(*vpp);
			return (NFSERR_AUTHERR | AUTH_TOOWEAK);
		}
	} else if (kerbflag) {
		vput(*vpp);
		return (NFSERR_AUTHERR | AUTH_TOOWEAK);
	} else if (kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER,
		    NULL) == 0 || (exflags & MNT_EXPORTANON)) {
		kauth_cred_clone(credanon, cred);
	}
	if (exflags & MNT_EXRDONLY)
		*rdonlyp = 1;
	else
		*rdonlyp = 0;
	if (!lockflag)
		VOP_UNLOCK(*vpp, 0);
	return (0);
}

/*
 * WebNFS: check if a filehandle is a public filehandle. For v3, this
 * means a length of 0, for v2 it means all zeroes.
 */
int
nfs_ispublicfh(const nfsrvfh_t *nsfh)
{
	const char *cp = (const void *)(NFSRVFH_DATA(nsfh));
	int i;

	if (NFSRVFH_SIZE(nsfh) == 0) {
		return true;
	}
	if (NFSRVFH_SIZE(nsfh) != NFSX_V2FH) {
		return false;
	}
	for (i = 0; i < NFSX_V2FH; i++)
		if (*cp++ != 0)
			return false;
	return true;
}
#endif /* NFSSERVER */

/*
 * This function compares two net addresses by family and returns true
 * if they are the same host.
 * If there is any doubt, return false.
 * The AF_INET family is handled as a special case so that address mbufs
 * don't need to be saved to store "struct in_addr", which is only 4 bytes.
 */
int
netaddr_match(family, haddr, nam)
	int family;
	union nethostaddr *haddr;
	struct mbuf *nam;
{
	struct sockaddr_in *inetaddr;

	switch (family) {
	case AF_INET:
		inetaddr = mtod(nam, struct sockaddr_in *);
		if (inetaddr->sin_family == AF_INET &&
		    inetaddr->sin_addr.s_addr == haddr->had_inetaddr)
			return (1);
		break;
#ifdef INET6
	case AF_INET6:
	    {
		struct sockaddr_in6 *sin6_1, *sin6_2;

		sin6_1 = mtod(nam, struct sockaddr_in6 *);
		sin6_2 = mtod(haddr->had_nam, struct sockaddr_in6 *);
		if (sin6_1->sin6_family == AF_INET6 &&
		    IN6_ARE_ADDR_EQUAL(&sin6_1->sin6_addr, &sin6_2->sin6_addr))
			return 1;
	    }
#endif
#ifdef ISO
	case AF_ISO:
	    {
		struct sockaddr_iso *isoaddr1, *isoaddr2;

		isoaddr1 = mtod(nam, struct sockaddr_iso *);
		isoaddr2 = mtod(haddr->had_nam, struct sockaddr_iso *);
		if (isoaddr1->siso_family == AF_ISO &&
		    isoaddr1->siso_nlen > 0 &&
		    isoaddr1->siso_nlen == isoaddr2->siso_nlen &&
		    SAME_ISOADDR(isoaddr1, isoaddr2))
			return (1);
		break;
	    }
#endif	/* ISO */
	default:
		break;
	};
	return (0);
}

/*
 * The write verifier has changed (probably due to a server reboot), so all
 * PG_NEEDCOMMIT pages will have to be written again. Since they are marked
 * as dirty or are being written out just now, all this takes is clearing
 * the PG_NEEDCOMMIT flag. Once done the new write verifier can be set for
 * the mount point.
 */
void
nfs_clearcommit(mp)
	struct mount *mp;
{
	struct vnode *vp;
	struct nfsnode *np;
	struct vm_page *pg;
	struct nfsmount *nmp = VFSTONFS(mp);

	rw_enter(&nmp->nm_writeverflock, RW_WRITER);

	TAILQ_FOREACH(vp, &mp->mnt_vnodelist, v_mntvnodes) {
		KASSERT(vp->v_mount == mp);
		if (vp->v_type != VREG)
			continue;
		np = VTONFS(vp);
		np->n_pushlo = np->n_pushhi = np->n_pushedlo =
		    np->n_pushedhi = 0;
		np->n_commitflags &=
		    ~(NFS_COMMIT_PUSH_VALID | NFS_COMMIT_PUSHED_VALID);
		simple_lock(&vp->v_uobj.vmobjlock);
		TAILQ_FOREACH(pg, &vp->v_uobj.memq, listq) {
			pg->flags &= ~PG_NEEDCOMMIT;
		}
		simple_unlock(&vp->v_uobj.vmobjlock);
	}
	mutex_enter(&nmp->nm_lock);
	nmp->nm_iflag &= ~NFSMNT_STALEWRITEVERF;
	mutex_exit(&nmp->nm_lock);
	rw_exit(&nmp->nm_writeverflock);
}

void
nfs_merge_commit_ranges(vp)
	struct vnode *vp;
{
	struct nfsnode *np = VTONFS(vp);

	KASSERT(np->n_commitflags & NFS_COMMIT_PUSH_VALID);

	if (!(np->n_commitflags & NFS_COMMIT_PUSHED_VALID)) {
		np->n_pushedlo = np->n_pushlo;
		np->n_pushedhi = np->n_pushhi;
		np->n_commitflags |= NFS_COMMIT_PUSHED_VALID;
	} else {
		if (np->n_pushlo < np->n_pushedlo)
			np->n_pushedlo = np->n_pushlo;
		if (np->n_pushhi > np->n_pushedhi)
			np->n_pushedhi = np->n_pushhi;
	}

	np->n_pushlo = np->n_pushhi = 0;
	np->n_commitflags &= ~NFS_COMMIT_PUSH_VALID;

#ifdef NFS_DEBUG_COMMIT
	printf("merge: committed: %u - %u\n", (unsigned)np->n_pushedlo,
	    (unsigned)np->n_pushedhi);
#endif
}

int
nfs_in_committed_range(vp, off, len)
	struct vnode *vp;
	off_t off, len;
{
	struct nfsnode *np = VTONFS(vp);
	off_t lo, hi;

	if (!(np->n_commitflags & NFS_COMMIT_PUSHED_VALID))
		return 0;
	lo = off;
	hi = lo + len;

	return (lo >= np->n_pushedlo && hi <= np->n_pushedhi);
}

int
nfs_in_tobecommitted_range(vp, off, len)
	struct vnode *vp;
	off_t off, len;
{
	struct nfsnode *np = VTONFS(vp);
	off_t lo, hi;

	if (!(np->n_commitflags & NFS_COMMIT_PUSH_VALID))
		return 0;
	lo = off;
	hi = lo + len;

	return (lo >= np->n_pushlo && hi <= np->n_pushhi);
}

void
nfs_add_committed_range(vp, off, len)
	struct vnode *vp;
	off_t off, len;
{
	struct nfsnode *np = VTONFS(vp);
	off_t lo, hi;

	lo = off;
	hi = lo + len;

	if (!(np->n_commitflags & NFS_COMMIT_PUSHED_VALID)) {
		np->n_pushedlo = lo;
		np->n_pushedhi = hi;
		np->n_commitflags |= NFS_COMMIT_PUSHED_VALID;
	} else {
		if (hi > np->n_pushedhi)
			np->n_pushedhi = hi;
		if (lo < np->n_pushedlo)
			np->n_pushedlo = lo;
	}
#ifdef NFS_DEBUG_COMMIT
	printf("add: committed: %u - %u\n", (unsigned)np->n_pushedlo,
	    (unsigned)np->n_pushedhi);
#endif
}

void
nfs_del_committed_range(vp, off, len)
	struct vnode *vp;
	off_t off, len;
{
	struct nfsnode *np = VTONFS(vp);
	off_t lo, hi;

	if (!(np->n_commitflags & NFS_COMMIT_PUSHED_VALID))
		return;

	lo = off;
	hi = lo + len;

	if (lo > np->n_pushedhi || hi < np->n_pushedlo)
		return;
	if (lo <= np->n_pushedlo)
		np->n_pushedlo = hi;
	else if (hi >= np->n_pushedhi)
		np->n_pushedhi = lo;
	else {
		/*
		 * XXX There's only one range. If the deleted range
		 * is in the middle, pick the largest of the
		 * contiguous ranges that it leaves.
		 */
		if ((np->n_pushedlo - lo) > (hi - np->n_pushedhi))
			np->n_pushedhi = lo;
		else
			np->n_pushedlo = hi;
	}
#ifdef NFS_DEBUG_COMMIT
	printf("del: committed: %u - %u\n", (unsigned)np->n_pushedlo,
	    (unsigned)np->n_pushedhi);
#endif
}

void
nfs_add_tobecommitted_range(vp, off, len)
	struct vnode *vp;
	off_t off, len;
{
	struct nfsnode *np = VTONFS(vp);
	off_t lo, hi;

	lo = off;
	hi = lo + len;

	if (!(np->n_commitflags & NFS_COMMIT_PUSH_VALID)) {
		np->n_pushlo = lo;
		np->n_pushhi = hi;
		np->n_commitflags |= NFS_COMMIT_PUSH_VALID;
	} else {
		if (lo < np->n_pushlo)
			np->n_pushlo = lo;
		if (hi > np->n_pushhi)
			np->n_pushhi = hi;
	}
#ifdef NFS_DEBUG_COMMIT
	printf("add: tobecommitted: %u - %u\n", (unsigned)np->n_pushlo,
	    (unsigned)np->n_pushhi);
#endif
}

void
nfs_del_tobecommitted_range(vp, off, len)
	struct vnode *vp;
	off_t off, len;
{
	struct nfsnode *np = VTONFS(vp);
	off_t lo, hi;

	if (!(np->n_commitflags & NFS_COMMIT_PUSH_VALID))
		return;

	lo = off;
	hi = lo + len;

	if (lo > np->n_pushhi || hi < np->n_pushlo)
		return;

	if (lo <= np->n_pushlo)
		np->n_pushlo = hi;
	else if (hi >= np->n_pushhi)
		np->n_pushhi = lo;
	else {
		/*
		 * XXX There's only one range. If the deleted range
		 * is in the middle, pick the largest of the
		 * contiguous ranges that it leaves.
		 */
		if ((np->n_pushlo - lo) > (hi - np->n_pushhi))
			np->n_pushhi = lo;
		else
			np->n_pushlo = hi;
	}
#ifdef NFS_DEBUG_COMMIT
	printf("del: tobecommitted: %u - %u\n", (unsigned)np->n_pushlo,
	    (unsigned)np->n_pushhi);
#endif
}

/*
 * Map errnos to NFS error numbers. For Version 3 also filter out error
 * numbers not specified for the associated procedure.
 */
int
nfsrv_errmap(nd, err)
	struct nfsrv_descript *nd;
	int err;
{
	const short *defaulterrp, *errp;

	if (nd->nd_flag & ND_NFSV3) {
	    if (nd->nd_procnum <= NFSPROC_COMMIT) {
		errp = defaulterrp = nfsrv_v3errmap[nd->nd_procnum];
		while (*++errp) {
			if (*errp == err)
				return (err);
			else if (*errp > err)
				break;
		}
		return ((int)*defaulterrp);
	    } else
		return (err & 0xffff);
	}
	if (err <= ELAST)
		return ((int)nfsrv_v2errmap[err - 1]);
	return (NFSERR_IO);
}

u_int32_t
nfs_getxid()
{
	static u_int32_t base;
	static u_int32_t nfs_xid = 0;
	static struct simplelock nfs_xidlock = SIMPLELOCK_INITIALIZER;
	u_int32_t newxid;

	simple_lock(&nfs_xidlock);
	/*
	 * derive initial xid from system time
	 * XXX time is invalid if root not yet mounted
	 */
	if (__predict_false(!base && (rootvp))) {
		struct timeval tv;

		microtime(&tv);
		base = tv.tv_sec << 12;
		nfs_xid = base;
	}

	/*
	 * Skip zero xid if it should ever happen.
	 */
	if (__predict_false(++nfs_xid == 0))
		nfs_xid++;
	newxid = nfs_xid;
	simple_unlock(&nfs_xidlock);

	return txdr_unsigned(newxid);
}

/*
 * assign a new xid for existing request.
 * used for NFSERR_JUKEBOX handling.
 */
void
nfs_renewxid(struct nfsreq *req)
{
	u_int32_t xid;
	int off;

	xid = nfs_getxid();
	if (req->r_nmp->nm_sotype == SOCK_STREAM)
		off = sizeof(u_int32_t); /* RPC record mark */
	else
		off = 0;

	m_copyback(req->r_mreq, off, sizeof(xid), (void *)&xid);
	req->r_xid = xid;
}

#if defined(NFSSERVER)
int
nfsrv_composefh(struct vnode *vp, nfsrvfh_t *nsfh, bool v3)
{
	int error;
	size_t fhsize;

	fhsize = NFSD_MAXFHSIZE;
	error = vfs_composefh(vp, (void *)NFSRVFH_DATA(nsfh), &fhsize);
	if (NFSX_FHTOOBIG_P(fhsize, v3)) {
		error = EOPNOTSUPP;
	}
	if (error != 0) {
		return error;
	}
	if (!v3 && fhsize < NFSX_V2FH) {
		memset((char *)NFSRVFH_DATA(nsfh) + fhsize, 0,
		    NFSX_V2FH - fhsize);
		fhsize = NFSX_V2FH;
	}
	if ((fhsize % NFSX_UNSIGNED) != 0) {
		return EOPNOTSUPP;
	}
	nsfh->nsfh_size = fhsize;
	return 0;
}

int
nfsrv_comparefh(const nfsrvfh_t *fh1, const nfsrvfh_t *fh2)
{

	if (NFSRVFH_SIZE(fh1) != NFSRVFH_SIZE(fh2)) {
		return NFSRVFH_SIZE(fh2) - NFSRVFH_SIZE(fh1);
	}
	return memcmp(NFSRVFH_DATA(fh1), NFSRVFH_DATA(fh2), NFSRVFH_SIZE(fh1));
}

void
nfsrv_copyfh(nfsrvfh_t *fh1, const nfsrvfh_t *fh2)
{
	size_t size;

	fh1->nsfh_size = size = NFSRVFH_SIZE(fh2);
	memcpy(NFSRVFH_DATA(fh1), NFSRVFH_DATA(fh2), size);
}
#endif /* defined(NFSSERVER) */
