/*	$NetBSD: kernfs.h,v 1.22 2004/05/07 15:33:17 cl Exp $	*/

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
	KFSkern,		/* the filesystem itself (.) */
	KFSroot,		/* the filesystem root (..) */
	KFSnull,		/* none aplicable */
	KFStime,		/* boottime */
	KFSint,			/* integer */
	KFSstring,		/* string */
	KFShostname,	/* hostname */
	KFSavenrun,		/* loadavg */
	KFSdevice,		/* device file (rootdev/rrootdev) */
	KFSmsgbuf,		/* msgbuf */
	KFSipsecsadir,	/* ipsec security association (top dir) */
	KFSipsecspdir,	/* ipsec security policy (top dir) */
	KFSipsecsa,		/* ipsec security association entry */
	KFSipsecsp,		/* ipsec security policy entry */
	KFSsubdir,		/* directory */
	KFSlasttype,		/* last used type */
	KFSmaxtype = (1<<6) - 1	/* last possible type */
} kfstype;

/*
 * Control data for the kern file system.
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

struct dyn_kern_target {
	struct kern_target		dkt_kt;
	SIMPLEQ_ENTRY(dyn_kern_target)	dkt_queue;
};

struct kernfs_subdir {
	SIMPLEQ_HEAD(,dyn_kern_target)	ks_entries;
	unsigned int			ks_nentries;
	unsigned int			ks_dirs;
	const struct kern_target	*ks_parent;
};

struct kernfs_node {
	LIST_ENTRY(kernfs_node) kfs_hash; /* hash chain */
	TAILQ_ENTRY(kernfs_node) kfs_list; /* flat list */
	struct vnode	*kfs_vnode;	/* vnode associated with this pfsnode */
	kfstype		kfs_type;	/* type of procfs node */
	mode_t		kfs_mode;	/* mode bits for stat() */
	long		kfs_fileno;	/* unique file id */
	u_int32_t	kfs_value;	/* SA id or SP id (KFSint) */
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
	((kt >= &kern_targets[0] && kt < &kern_targets[static_nkern_targets]) \
	    ? 2 + ((kt) - &kern_targets[0]) \
	      : (((cookie + 1) << 6) | (typ)))
#define KERNFS_TYPE_FILENO(typ, cookie) \
	(((cookie + 1) << 6) | (typ))

#define VFSTOKERNFS(mp)	((struct kernfs_mount *)((mp)->mnt_data))
#define	VTOKERN(vp)	((struct kernfs_node *)(vp)->v_data)
#define KERNFSTOV(kfs)	((kfs)->kfs_vnode)

extern const struct kern_target kern_targets[];
extern int nkern_targets;
extern const int static_nkern_targets;
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

/*
 * Data types for the kernfs file operations.
 */
typedef enum {
	KERNFS_XWRITE,
	KERNFS_FILEOP_CLOSE,
	KERNFS_FILEOP_GETATTR,
	KERNFS_FILEOP_IOCTL,
	KERNFS_FILEOP_MMAP,
	KERNFS_FILEOP_OPEN,
	KERNFS_FILEOP_WRITE,
} kfsfileop;

struct kernfs_fileop {
	kfstype				kf_type;
	kfsfileop			kf_fileop;
	union {
		void			*_kf_genop;
		int			(*_kf_vop)(void *);
		int			(*_kf_xwrite)
			(const struct kernfs_node *, char *, size_t);
	} _kf_opfn;
	SPLAY_ENTRY(kernfs_fileop)	kf_node;
};
#define	kf_genop	_kf_opfn
#define	kf_vop		_kf_opfn._kf_vop
#define	kf_xwrite	_kf_opfn._kf_xwrite

typedef struct kern_target kernfs_parentdir_t;
typedef struct dyn_kern_target kernfs_entry_t;

/*
 * Functions for adding kernfs datatypes and nodes.
 */
kfstype kernfs_alloctype(int, const struct kernfs_fileop *);
#define	KERNFS_ALLOCTYPE(kf) kernfs_alloctype(sizeof((kf)) / \
	sizeof((kf)[0]), (kf))
#define	KERNFS_ALLOCENTRY(dkt, m_type, m_flags)				\
	dkt = (struct dyn_kern_target *)malloc(				\
		sizeof(struct dyn_kern_target), (m_type), (m_flags))
#define	KERNFS_INITENTRY(dkt, type, name, data, tag, vtype, mode) do {	\
	(dkt)->dkt_kt.kt_type = (type);					\
	(dkt)->dkt_kt.kt_namlen = strlen((name));			\
	(dkt)->dkt_kt.kt_name = (name);					\
	(dkt)->dkt_kt.kt_data = (data);					\
	(dkt)->dkt_kt.kt_tag = (tag);					\
	(dkt)->dkt_kt.kt_vtype = (vtype);				\
	(dkt)->dkt_kt.kt_mode = (mode);					\
} while (/*CONSTCOND*/0)
#define	KERNFS_ENTOPARENTDIR(dkt) &(dkt)->dkt_kt
int kernfs_addentry __P((kernfs_parentdir_t *, kernfs_entry_t *));

#endif /* _KERNEL */
