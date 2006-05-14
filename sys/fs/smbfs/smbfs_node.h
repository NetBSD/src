/*	$NetBSD: smbfs_node.h,v 1.11 2006/05/14 21:31:52 elad Exp $	*/

/*
 * Copyright (c) 2000-2001, Boris Popov
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by Boris Popov.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * FreeBSD: src/sys/fs/smbfs/smbfs_node.h,v 1.1 2001/04/10 07:59:05 bp Exp
 */
#ifndef _FS_SMBFS_SMBFS_NODE_H_
#define _FS_SMBFS_SMBFS_NODE_H_

#include <sys/hash.h>			/* for hash32_strn() */
#include <miscfs/genfs/genfs_node.h>	/* for struct genfs_node */

#define	SMBFS_ROOT_INO		2	/* just like in UFS */

/* Bits for smbnode.n_flag */
#define	NFLUSHINPROG		0x0001
#define	NFLUSHWANT		0x0002	/* they should gone ... */
#define	NMODIFIED		0x0004	/* bogus, until async IO implemented */
/*efine	NNEW			0x0008*//* smb/vnode has been allocated */
#define	NREFPARENT		0x0010	/* node holds parent from recycling */
#define	NOPEN			0x2000	/* file is open */

#define SMBFS_ATTRTIMO		5	/* Attribute cache timeout in sec */

struct smbfs_fctx;

struct smbnode {
	struct genfs_node	n_gnode;
	int			n_flag;
	struct smbnode *	n_parent;
	struct vnode *		n_vnode;
	struct smbmount *	n_mount;
	time_t			n_attrage;	/* attributes cache time */
	time_t			n_ctime;	/* Prev create time. */
	time_t			n_nctime;	/* last neg cache entry (dir) */
	struct timespec		n_mtime;	/* modify time */
	struct timespec		n_atime;	/* last access time */
	u_quad_t		n_size;
	long			n_ino;
	int			n_dosattr;
	u_int16_t		n_fid;		/* file handle */
	int			n_rwstate;	/* granted access mode */
	u_char			n_nmlen;
	u_char *		n_name;
	struct smbfs_fctx *	n_dirseq;	/* ff context */
	long			n_dirofs;	/* last ff offset */
	struct lockf *		n_lockf;	/* Locking records of file */
	LIST_ENTRY(smbnode)	n_hash;
};

#define VTOSMB(vp)	((struct smbnode *)(vp)->v_data)
#define SMBTOV(np)	((struct vnode *)(np)->n_vnode)

struct smbfattr;

int  smbfs_inactive(void *);
int  smbfs_reclaim(void *);
int smbfs_nget(struct mount *, struct vnode *, const char *, int,
    struct smbfattr *, struct vnode **);
#define	smbfs_hash(x, y)	hash32_strn((x), (y), HASH32_STR_INIT)

int  smbfs_readvnode(struct vnode *, struct uio *, kauth_cred_t);
int  smbfs_writevnode(struct vnode *, struct uio *, kauth_cred_t, int);
void smbfs_attr_cacheenter(struct vnode *, struct smbfattr *);
int  smbfs_attr_cachelookup(struct vnode * ,struct vattr *);

#define smbfs_attr_cacheremove(vp)	VTOSMB(vp)->n_attrage = 0

#endif /* _FS_SMBFS_SMBFS_NODE_H_ */
