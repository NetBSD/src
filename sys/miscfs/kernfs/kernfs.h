/*	$NetBSD: kernfs.h,v 1.18 2003/09/08 06:51:53 itojun Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
 * Jan-Simon Pendry.
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
 *	@(#)kernfs.h	8.6 (Berkeley) 3/29/95
 */

#define	_PATH_KERNFS	"/kern"		/* Default mountpoint */

#ifdef _KERNEL
#include <sys/queue.h>

/*
 * The different types of node in a kernfs filesystem
 */
typedef enum {
	Pkern,		/* the filesystem itself (.) */
	Proot,		/* the filesystem root (..) */
	Pnull,		/* none aplicable */
	Ptime,		/* boottime */
	Pint,		/* integer */
	Pstring,	/* string */
	Phostname,	/* hostname */
	Pavenrun,	/* loadavg */
	Pdevice,	/* device file (rootdev/rrootdev) */
	Pmsgbuf,	/* msgbuf */
	Pipsecsadir,	/* ipsec security association (top dir) */
	Pipsecspdir,	/* ipsec security policy (top dir) */
	Pipsecsa,	/* ipsec security association entry */
	Pipsecsp,	/* ipsec security policy entry */
} kfstype;

/*
 * control data for the kern file system.
 */
struct kern_target {
	u_char		kt_type;
	u_char		kt_namlen;
	const char	*kt_name;
	void		*kt_data;
	kfstype		kt_tag;
	u_char		kt_vtype;
	mode_t		kt_mode;
};

struct kernfs_node {
	LIST_ENTRY(kernfs_node) kfs_hash; /* hash chain */
	TAILQ_ENTRY(kernfs_node) kfs_list; /* flat list */
	struct vnode	*kfs_vnode;	/* vnode associated with this pfsnode */
	kfstype		kfs_type;	/* type of procfs node */
	mode_t		kfs_mode;	/* mode bits for stat() */
	long		kfs_fileno;	/* unique file id */
	u_int32_t	kfs_value;	/* SA id or SP id (Pint) */
	const struct kern_target *kfs_kt;
	void		*kfs_v;		/* pointer to secasvar/secpolicy/mbuf */
	long		kfs_cookie;	/* fileno cookie */
};

struct kernfs_mount {
	TAILQ_HEAD(, kernfs_node) nodelist;
	long fileno_cookie;
};

#define UIO_MX	32

#define KERNFS_FILENO(kt, typ, cookie) \
	((kt) ? 2 + ((kt) - &kern_targets[0]) \
	      : (((cookie) << 6) | ((typ) + nkern_targets)))

#define VFSTOKERNFS(mp)	((struct kernfs_mount *)((mp)->mnt_data))
#define	VTOKERN(vp)	((struct kernfs_node *)(vp)->v_data)
#define KERNFSTOV(kfs)	((kfs)->kfs_vnode)

extern const struct kern_target kern_targets[];
extern int nkern_targets;
extern int (**kernfs_vnodeop_p) __P((void *));
extern struct vfsops kernfs_vfsops;
extern dev_t rrootdev;

struct secasvar;
struct secpolicy;

int kernfs_root __P((struct mount *, struct vnode **));

void kernfs_hashinit __P((void));
void kernfs_hashreinit __P((void));
void kernfs_hashdone __P((void));
int kernfs_freevp __P((struct vnode *));
int kernfs_allocvp __P((struct mount *, struct vnode **, kfstype,
	const struct kern_target *, u_int32_t));

void kernfs_revoke_sa __P((struct secasvar *));
void kernfs_revoke_sp __P((struct secpolicy *));
#endif /* _KERNEL */
