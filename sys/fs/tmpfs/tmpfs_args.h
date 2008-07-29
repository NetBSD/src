/*	$NetBSD: tmpfs_args.h,v 1.1.2.3 2008/07/29 07:00:30 simonb Exp $	*/

/*
 * Copyright (c) 2005, 2006, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal, developed as part of Google's Summer of Code
 * 2005 program.
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

#ifndef _FS_TMPFS_TMPFS_ARGS_H_
#define _FS_TMPFS_TMPFS_ARGS_H_

#include <sys/vnode.h>

/*
 * Internal representation of a tmpfs directory entry.
 */
struct tmpfs_dirent {
	TAILQ_ENTRY(tmpfs_dirent)	td_entries;

	/* Length of the name stored in this directory entry.  This avoids
	 * the need to recalculate it every time the name is used. */
	uint16_t			td_namelen;

	/* The name of the entry, allocated from a string pool.  This
	* string is not required to be zero-terminated; therefore, the
	* td_namelen field must always be used when accessing its value. */
	char *				td_name;

	/* Pointer to the node this entry refers to. */
	struct tmpfs_node *		td_node;
};

/* A directory in tmpfs holds a sorted list of directory entries, which in
 * turn point to other files (which can be directories themselves).
 *
 * In tmpfs, this list is managed by a tail queue, whose head is defined by
 * the struct tmpfs_dir type.
 *
 * It is imporant to notice that directories do not have entries for . and
 * .. as other file systems do.  These can be generated when requested
 * based on information available by other means, such as the pointer to
 * the node itself in the former case or the pointer to the parent directory
 * in the latter case.  This is done to simplify tmpfs's code and, more
 * importantly, to remove redundancy. */
TAILQ_HEAD(tmpfs_dir, tmpfs_dirent);

/*
 * Internal representation of a tmpfs file system node.
 *
 * This structure is splitted in two parts: one holds attributes common
 * to all file types and the other holds data that is only applicable to
 * a particular type.  The code must be careful to only access those
 * attributes that are actually allowed by the node's type.
 */
struct tmpfs_node {
	/* Doubly-linked list entry which links all existing nodes for a
	 * single file system.  This is provided to ease the removal of
	 * all nodes during the unmount operation. */
	LIST_ENTRY(tmpfs_node)	tn_entries;

	/* The node's type.  Any of 'VBLK', 'VCHR', 'VDIR', 'VFIFO',
	 * 'VLNK', 'VREG' and 'VSOCK' is allowed.  The usage of vnode
	 * types instead of a custom enumeration is to make things simpler
	 * and faster, as we do not need to convert between two types. */
	enum vtype		tn_type;

	/* Node identifier. */
	ino_t			tn_id;

	/* Node's internal status.  This is used by several file system
	 * operations to do modifications to the node in a delayed
	 * fashion. */
	int			tn_status;
#define	TMPFS_NODE_ACCESSED	(1 << 1)
#define	TMPFS_NODE_MODIFIED	(1 << 2)
#define	TMPFS_NODE_CHANGED	(1 << 3)

	/* The node size.  It does not necessarily match the real amount
	 * of memory consumed by it. */
	off_t			tn_size;

	/* Generic node attributes. */
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

	/* Head of byte-level lock list (used by tmpfs_advlock). */
	struct lockf *		tn_lockf;

	/* As there is a single vnode for each active file within the
	 * system, care has to be taken to avoid allocating more than one
	 * vnode per file.  In order to do this, a bidirectional association
	 * is kept between vnodes and nodes.
	 *
	 * Whenever a vnode is allocated, its v_data field is updated to
	 * point to the node it references.  At the same time, the node's
	 * tn_vnode field is modified to point to the new vnode representing
	 * it.  Further attempts to allocate a vnode for this same node will
	 * result in returning a new reference to the value stored in
	 * tn_vnode.
	 *
	 * May be NULL when the node is unused (that is, no vnode has been
	 * allocated for it or it has been reclaimed). */
	kmutex_t		tn_vlock;
	struct vnode *		tn_vnode;

	union {
		/* Valid when tn_type == VBLK || tn_type == VCHR. */
		struct {
			dev_t			tn_rdev;
		} tn_dev;

		/* Valid when tn_type == VDIR. */
		struct {
			/* Pointer to the parent directory.  The root
			 * directory has a pointer to itself in this field;
			 * this property identifies the root node. */
			struct tmpfs_node *	tn_parent;

			/* Head of a tail-queue that links the contents of
			 * the directory together.  See above for a
			 * description of its contents. */
			struct tmpfs_dir	tn_dir;

			/* Number and pointer of the first directory entry
			 * returned by the readdir operation if it were
			 * called again to continue reading data from the
			 * same directory as before.  This is used to speed
			 * up reads of long directories, assuming that no
			 * more than one read is in progress at a given time.
			 * Otherwise, these values are discarded and a linear
			 * scan is performed from the beginning up to the
			 * point where readdir starts returning values. */
			off_t			tn_readdir_lastn;
			struct tmpfs_dirent *	tn_readdir_lastp;
		} tn_dir;

		/* Valid when tn_type == VLNK. */
		struct tn_lnk {
			/* The link's target, allocated from a string pool. */
			char *			tn_link;
		} tn_lnk;

		/* Valid when tn_type == VREG. */
		struct tn_reg {
			/* The contents of regular files stored in a tmpfs
			 * file system are represented by a single anonymous
			 * memory object (aobj, for short).  The aobj provides
			 * direct access to any position within the file,
			 * because its contents are always mapped in a
			 * contiguous region of virtual memory.  It is a task
			 * of the memory management subsystem (see uvm(9)) to
			 * issue the required page ins or page outs whenever
			 * a position within the file is accessed. */
			struct uvm_object *	tn_aobj;
			size_t			tn_aobj_pages;
		} tn_reg;
	} tn_spec;
};

static __inline
struct tmpfs_node *
VP_TO_TMPFS_NODE(struct vnode *vp)
{
	struct tmpfs_node *node;

#ifdef KASSERT
	KASSERT((vp) != NULL && (vp)->v_data != NULL);
#endif
	node = (struct tmpfs_node *)vp->v_data;
	return node;
}

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

#endif /* _FS_TMPFS_TMPFS_ARGS_H_ */
