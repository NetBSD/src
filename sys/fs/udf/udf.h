/* $NetBSD: udf.h,v 1.3 2006/02/02 15:52:23 reinoud Exp $ */

/*
 * Copyright (c) 2006 Reinoud Zandijk
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */


#include <sys/queue.h>
#include <sys/uio.h>

#include "udf_osta.h"
#include "ecma167-udf.h"
#include <sys/cdio.h>
#include <miscfs/genfs/genfs_node.h>

#ifndef _FS_UDF_UDF_H_
#define _FS_UDF_UDF_H_


/* TODO make `udf_verbose' set by sysctl */
/* debug section */
extern int udf_verbose;

/* initial value of udf_verbose */
#define UDF_DEBUGGING		0x000

/* debug categories */
#define UDF_DEBUG_VOLUMES	0x001
#define UDF_DEBUG_LOCKING	0x002
#define UDF_DEBUG_NODE		0x004
#define UDF_DEBUG_LOOKUP	0x008
#define UDF_DEBUG_READDIR	0x010
#define UDF_DEBUG_FIDS		0x020
#define UDF_DEBUG_DESCRIPTOR	0x040
#define UDF_DEBUG_TRANSLATE	0x080
#define UDF_DEBUG_STRATEGY	0x100
#define UDF_DEBUG_READ		0x200
#define UDF_DEBUG_CALL		0x400
#define UDF_DEBUG_NOTIMPL	UDF_DEBUG_CALL


#ifdef DEBUG
#define DPRINTF(name, arg) { \
		if (udf_verbose & UDF_DEBUG_##name) {\
			printf arg;\
		};\
	}
#define DPRINTFIF(name, cond, arg) { \
		if (udf_verbose & UDF_DEBUG_##name) { \
			if (cond) printf arg;\
		};\
	}
#else
#define DPRINTF(name, arg) {}
#define DPRINTFIF(name, cond, arg) {}
#endif


/* constants to identify what kind of identifier we are dealing with */
#define UDF_REGID_DOMAIN		 1
#define UDF_REGID_UDF			 2
#define UDF_REGID_IMPLEMENTATION	 3
#define UDF_REGID_APPLICATION		 4
#define UDF_REGID_NAME			99


/* DON'T change these: they identify 13thmonkey's UDF implementation */
#define APP_NAME		"*NetBSD UDF"
#define APP_VERSION_MAIN	1
#define APP_VERSION_SUB		0
#define IMPL_NAME		"*13thMonkey.org"


/* Configuration values */
#define UDF_INODE_HASHBITS 	10
#define UDF_INODE_HASHSIZE	(1<<UDF_INODE_HASHBITS)
#define UDF_INODE_HASHMASK	(UDF_INODE_HASHSIZE - 1)


/* structure space */
#define UDF_ANCHORS		4	/* 256, 512, N-256, N */
#define UDF_PARTITIONS		4	/* overkill */
#define UDF_PMAPS		4	/* overkill */


/* constants */
#define UDF_MAX_NAMELEN		255			/* as per SPEC */
#define UDF_TRANS_ZERO		((uint64_t) -1)
#define UDF_TRANS_UNMAPPED	((uint64_t) -2)
#define UDF_TRANS_INTERN	((uint64_t) -3)
#define UDF_MAX_SECTOR		((uint64_t) -10)	/* high water mark */


/* malloc pools */
MALLOC_DECLARE(M_UDFMNT);
MALLOC_DECLARE(M_UDFVOLD);
MALLOC_DECLARE(M_UDFTEMP);

struct pool udf_node_pool;

struct udf_node;

/* pre cleanup */
struct udf_mount {
	struct mount		*vfs_mountp;
	struct vnode		*devvp;	
	struct mmc_discinfo	 discinfo;
	struct udf_args		 mount_args;

	/* read in structures */
	struct anchor_vdp	*anchors[UDF_ANCHORS];	/* anchors to VDS    */
	struct pri_vol_desc	*primary_vol;		/* identification    */
	struct logvol_desc	*logical_vol;		/* main mapping v->p */
	struct unalloc_sp_desc	*unallocated;		/* free UDF space    */
	struct impvol_desc	*implementation;	/* likely reduntant  */
	struct logvol_int_desc	*logvol_integrity;	/* current integrity */
	struct part_desc	*partitions[UDF_PARTITIONS]; /* partitions   */

	/* derived; points *into* other structures */
	struct udf_logvol_info	*logvol_info;		/* integrity descr.  */

	/* fileset and root directories */
	struct fileset_desc	*fileset_desc;		/* normally one      */

	/* logical to physical translations */
	int 			 vtop[UDF_PMAPS+1];	/* vpartnr trans     */
	int			 vtop_tp[UDF_PMAPS+1];	/* type of trans     */

	uint32_t		 possible_vat_location;	/* predicted         */
	uint32_t		 vat_table_alloc_length;
	uint32_t		 vat_entries;
	uint32_t		 vat_offset;		/* offset in table   */
	uint8_t			*vat_table;		/* read in data      */

	uint32_t		 sparable_packet_len;
	struct udf_sparing_table*sparing_table;

	struct udf_node 	*metafile;
	struct udf_node 	*metabitmapfile;
	struct udf_node 	*metacopyfile;
	struct udf_node 	*metabitmapcopyfile;

	/* disc allocation */
	int			data_alloc, meta_alloc;	/* allocation scheme */

	struct mmc_trackinfo	datatrack;
	struct mmc_trackinfo	metadatatrack;
		/* TODO free space and usage per partition */
		/* ... [UDF_PARTITIONS]; */

	/* hash table to lookup ino_t -> udf_node */
	LIST_HEAD(, udf_node) udf_nodes[UDF_INODE_HASHSIZE];

	/* allocation pool for udf_node's descriptors */
	struct pool desc_pool;

	/* locks */
	struct simplelock ihash_slock;
	struct lock       get_node_lock;

	/* lists */
	STAILQ_HEAD(, udf_node) dirty_nodes;
	STAILQ_HEAD(udfmntpts, udf_mount) all_udf_mntpnts;
};


#define UDF_VTOP_RAWPART UDF_PMAPS	/* [0..UDF_PMAPS> are normal     */

/* virtual to physical mapping types */
#define UDF_VTOP_TYPE_RAW            0
#define UDF_VTOP_TYPE_UNKNOWN        0
#define UDF_VTOP_TYPE_PHYS           1
#define UDF_VTOP_TYPE_VIRT           2
#define UDF_VTOP_TYPE_SPARABLE       3
#define UDF_VTOP_TYPE_META           4

/* allocation strategies */
#define UDF_ALLOC_SPACEMAP           1  /* spacemaps                     */
#define UDF_ALLOC_SEQUENTIAL         2  /* linear on NWA                 */
#define UDF_ALLOC_VAT                3  /* VAT handling                  */
#define UDF_ALLOC_METABITMAP         4  /* metadata bitmap               */
#define UDF_ALLOC_METASEQUENTIAL     5  /* in chunks seq., nodes not seq */
#define UDF_ALLOC_RELAXEDSEQUENTIAL  6  /* only nodes not seq.           */

/* readdir cookies */
#define UDF_DIRCOOKIE_DOT 1


struct udf_node {
	struct genfs_node	i_gnode;		/* has to be first   */
	struct vnode		*vnode;			/* vnode associated  */
	struct udf_mount	*ump;

	/* one of `fe' or `efe' can be set, not both (UDF file entry dscr.)  */
	struct file_entry	*fe;
	struct extfile_entry	*efe;

	/* location found and recording location & hints */
	struct long_ad		 loc;			/* FID/hash loc.     */
	struct long_ad		 next_loc;		/* strat 4096 loc    */
	int			 needs_indirect;	/* has missing indr. */

	/* TODO support for allocation extents? */

	/* device number from extended attributes = makedev(min,maj) */
	dev_t			 rdev;

	/* misc */
	struct lockf		*lockf;			/* lock list         */

	/* possibly not needed */
	long			 refcnt;
	int			 dirty;
	int			 hold;

	struct udf_node		*extattr;
	struct udf_node		*streamdir;

	LIST_ENTRY(udf_node)	 hashchain;		/* all udf nodes     */
	STAILQ_ENTRY(udf_node)	 dirty_nodes;		/* dirty udf nodes   */
};

#endif /* !_FS_UDF_UDF_H_ */

