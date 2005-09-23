/*	$NetBSD: tmpfs.h,v 1.5 2005/09/23 12:10:32 jmmv Exp $	*/

/*
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal.
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

#if !defined(_TMPFS_H_)
#  define _TMPFS_H_
#else
#  error "tmpfs.h cannot be included multiple times."
#endif

/* ---------------------------------------------------------------------
 * KERNEL-SPECIFIC DEFINITIONS
 * --------------------------------------------------------------------- */

#if defined(_KERNEL)

#include <sys/dirent.h>
#include <sys/mount.h>
#include <sys/queue.h>
#include <sys/vnode.h>

#include <fs/tmpfs/tmpfs_pool.h>

/* --------------------------------------------------------------------- */

/*
 * This structure holds a directory entry.
 */
struct tmpfs_dirent {
	TAILQ_ENTRY(tmpfs_dirent)	td_entries;
	uint16_t			td_namelen;
	char *				td_name;
	struct tmpfs_node *		td_node;
};
TAILQ_HEAD(tmpfs_dir, tmpfs_dirent);

#define	TMPFS_DIRCOOKIE(dirent)	((off_t)(uintptr_t)(dirent))
#define	TMPFS_DIRCOOKIE_DOT	0
#define	TMPFS_DIRCOOKIE_DOTDOT	1
#define	TMPFS_DIRCOOKIE_EOF	2

/* --------------------------------------------------------------------- */

/*
 * This structure represents a node within tmpfs.
 */
struct tmpfs_node {
	LIST_ENTRY(tmpfs_node)	tn_entries;

	enum vtype		tn_type;
	ino_t			tn_id;

#define	TMPFS_NODE_ACCESSED	(1 << 1)
#define	TMPFS_NODE_MODIFIED	(1 << 2)
#define	TMPFS_NODE_CHANGED	(1 << 3)
	int			tn_status;

	off_t			tn_size;

	/* Attributes. */
	uid_t			tn_uid;
	gid_t			tn_gid;
	mode_t			tn_mode;
	int			tn_flags;
	nlink_t			tn_links;
	struct timespec		tn_atime;
	struct timespec		tn_mtime;
	struct timespec		tn_ctime;
	struct timespec		tn_birthtime;
	unsigned long		tn_gen;

	struct vnode *		tn_vnode;

	/* Used by tmpfs_lookup to store the affected directory entry during
	 * DELETE and RENAME operations. */
	struct tmpfs_dirent *	tn_lookup_dirent;

	union {
		/* Valid when tn_type == VBLK || tn_type == VCHR. */
		struct {
			dev_t			tn_rdev;
		};

		/* Valid when tn_type == VDIR. */
		struct {
			struct tmpfs_node *	tn_parent;
			struct tmpfs_dir	tn_dir;

			/* Used by tmpfs_readdir to speed up lookups. */
			off_t			tn_readdir_lastn;
			struct tmpfs_dirent *	tn_readdir_lastp;
		};

		/* Valid when tn_type == VLNK. */
		struct {
			char *			tn_link;
		};

		/* Valid when tn_type == VREG. */
		struct {
			struct uvm_object *	tn_aobj;
			size_t			tn_aobj_pages;
		};
	};
};
LIST_HEAD(tmpfs_node_list, tmpfs_node);

/* --------------------------------------------------------------------- */

/*
 * This structure holds the information relative to a tmpfs instance.
 */
struct tmpfs_mount {
	size_t			tm_pages_max;
	size_t			tm_pages_used;

	struct tmpfs_node *	tm_root;

	ino_t			tm_nodes_max;
	ino_t			tm_nodes_last;
	struct tmpfs_node_list	tm_nodes_used;
	struct tmpfs_node_list	tm_nodes_avail;

	struct tmpfs_pool	tm_dirent_pool;
	struct tmpfs_pool	tm_node_pool;
	struct tmpfs_str_pool	tm_str_pool;
};

/* --------------------------------------------------------------------- */

/*
 * This structure maps a file identifier to a tmpfs node.  Used by the
 * NFS code.
 */
struct tmpfs_fid {
	uint16_t		tf_len;
	uint16_t		tf_pad;
	ino_t			tf_id;
	unsigned long		tf_gen;
};

/* --------------------------------------------------------------------- */

/*
 * Prototypes for tmpfs_subr.c.
 */

int	tmpfs_alloc_node(struct tmpfs_mount *, enum vtype,
	    uid_t uid, gid_t gid, mode_t mode, struct tmpfs_node *,
	    char *, dev_t, struct proc *, struct tmpfs_node **);
void	tmpfs_free_node(struct tmpfs_mount *, struct tmpfs_node *);
int	tmpfs_alloc_dirent(struct tmpfs_mount *, struct tmpfs_node *,
	    const char *, uint16_t, struct tmpfs_dirent **);
void	tmpfs_free_dirent(struct tmpfs_mount *, struct tmpfs_dirent *,
	    boolean_t);
int	tmpfs_alloc_vp(struct mount *, struct tmpfs_node *, struct vnode **);
void	tmpfs_free_vp(struct vnode *);
int	tmpfs_alloc_file(struct vnode *, struct vnode **, struct vattr *,
	    struct componentname *, char *);
void	tmpfs_dir_attach(struct vnode *, struct tmpfs_dirent *);
void	tmpfs_dir_detach(struct vnode *, struct tmpfs_dirent *);
struct tmpfs_dirent *	tmpfs_dir_lookup(struct tmpfs_node *node,
			    struct componentname *cnp);
int	tmpfs_dir_getdotdent(struct tmpfs_node *, struct uio *);
int	tmpfs_dir_getdotdotdent(struct tmpfs_node *, struct uio *);
struct tmpfs_dirent *	tmpfs_dir_lookupbycookie(struct tmpfs_node *, off_t);
int	tmpfs_dir_getdents(struct tmpfs_node *, struct uio *, off_t *);
int	tmpfs_reg_resize(struct vnode *, off_t);
size_t	tmpfs_mem_info(boolean_t);
int	tmpfs_chflags(struct vnode *, int, struct ucred *, struct proc *);
int	tmpfs_chmod(struct vnode *, mode_t, struct ucred *, struct proc *);
int	tmpfs_chown(struct vnode *, uid_t, gid_t, struct ucred *,
	    struct proc *);
int	tmpfs_chsize(struct vnode *, u_quad_t, struct ucred *, struct proc *);
int	tmpfs_chtimes(struct vnode *, struct timespec *, struct timespec *,
	    int, struct ucred *, struct proc *);

/* --------------------------------------------------------------------- */

/*
 * Convenience macros to simplify some logical expressions.
 */
#define IMPLIES(a, b) (!(a) || (b))
#define IFF(a, b) (IMPLIES(a, b) && IMPLIES(b, a))

/* --------------------------------------------------------------------- */

/*
 * Checks that the directory entry pointed by 'de' matches the name 'name'
 * with a length of 'len'.
 */
#define TMPFS_DIRENT_MATCHES(de, name, len) \
    (de->td_namelen == (uint16_t)len && \
    memcmp((de)->td_name, (name), (de)->td_namelen) == 0)

/* --------------------------------------------------------------------- */

/*
 * Ensures that the node pointed by 'node' is a directory and that its
 * contents are consistent with respect to directories.
 */
#define TMPFS_VALIDATE_DIR(node) \
    KASSERT((node)->tn_type == VDIR); \
    KASSERT((node)->tn_size % sizeof(struct tmpfs_dirent) == 0); \
    KASSERT((node)->tn_readdir_lastp == NULL || \
	TMPFS_DIRCOOKIE((node)->tn_readdir_lastp) == (node)->tn_readdir_lastn);

/* --------------------------------------------------------------------- */

/*
 * Memory management stuff.
 */

/* Amount of memory pages to reserve for the system (e.g., to not use by
 * tmpfs).
 * XXX: Should this be tunable through sysctl, for instance? */
#define TMPFS_PAGES_RESERVED (4 * 1024 * 1024 / PAGE_SIZE)

/* Returns the available memory for the given file system, which can be
 * its limit (set during mount time) or the amount of free memory, whichever
 * is lower. */
static inline size_t
TMPFS_PAGES_MAX(struct tmpfs_mount *tmp)
{
	size_t freepages;

	freepages = tmpfs_mem_info(FALSE);
	if (freepages < TMPFS_PAGES_RESERVED)
		freepages = 0;
	else
		freepages -= TMPFS_PAGES_RESERVED;

	return MIN(tmp->tm_pages_max, freepages + tmp->tm_pages_used);
}

#define TMPFS_PAGES_AVAIL(tmp) (TMPFS_PAGES_MAX(tmp) - (tmp)->tm_pages_used)

/* --------------------------------------------------------------------- */

/*
 * Macros/functions to convert from generic data structures to tmpfs
 * specific ones.
 *
 * Macros are used when no sanity checks have to be done, as they provide
 * the fastest conversion.  On the other hand, inlined functions are used
 * when expensive sanity checks are enabled, mostly because the checks
 * have to be done separately from the return value.
 */

#if defined(DIAGNOSTIC)
static inline
struct tmpfs_mount *
VFS_TO_TMPFS(struct mount *mp)
{
	struct tmpfs_mount *tmp;

	KASSERT((mp) != NULL && (mp)->mnt_data != NULL);
	tmp = (struct tmpfs_mount *)(mp)->mnt_data;
	KASSERT(TMPFS_PAGES_MAX(tmp) >= tmp->tm_pages_used);
	return tmp;
}

static inline
struct tmpfs_node *
VP_TO_TMPFS_NODE(struct vnode *vp)
{
	struct tmpfs_node *node;

	KASSERT((vp) != NULL && (vp)->v_data != NULL);
	node = (struct tmpfs_node *)vp->v_data;
	return node;
}

static inline
struct tmpfs_node *
VP_TO_TMPFS_DIR(struct vnode *vp)
{
	struct tmpfs_node *node;

	node = VP_TO_TMPFS_NODE(vp);
	TMPFS_VALIDATE_DIR(node);
	return node;
}
#else
#	define VFS_TO_TMPFS(mp) ((struct tmpfs_mount *)mp->mnt_data)
#	define VP_TO_TMPFS_NODE(vp) ((struct tmpfs_node *)vp->v_data)
#	define VP_TO_TMPFS_DIR(vp) VP_TO_TMPFS_NODE(vp)
#endif

#endif /* _KERNEL */

/* ---------------------------------------------------------------------
 * USER AND KERNEL DEFINITIONS
 * --------------------------------------------------------------------- */

/*
 * This structure is used to communicate mount parameters between userland
 * and kernel space.
 */
#define TMPFS_ARGS_VERSION	1
struct tmpfs_args {
	int			ta_version;

	/* Size counters. */
	ino_t			ta_nodes_max;
	off_t			ta_size_max;

	/* Root node attributes. */
	uid_t			ta_root_uid;
	gid_t			ta_root_gid;
	mode_t			ta_root_mode;
};
