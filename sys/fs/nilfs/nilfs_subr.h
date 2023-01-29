/* $NetBSD: nilfs_subr.h,v 1.5 2023/01/29 16:07:14 reinoud Exp $ */

/*
 * Copyright (c) 2008, 2009 Reinoud Zandijk
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

#ifndef _FS_NILFS_NILFS_SUBR_H_
#define _FS_NILFS_NILFS_SUBR_H_

/* handies */
#define	VFSTONILFS(mp)	((struct nilfs_mount *)mp->mnt_data)

/* basic calculators */
uint64_t nilfs_get_segnum_of_block(struct nilfs_device *nilfsdev, uint64_t blocknr);
void nilfs_get_segment_range(struct nilfs_device *nilfsdev, uint64_t segnum,
	uint64_t *seg_start, uint64_t *seg_end);
void nilfs_calc_mdt_consts(struct nilfs_device *nilfsdev,
	struct nilfs_mdt *mdt, int entry_size);
uint32_t crc32_le(uint32_t crc, const uint8_t *buf, size_t len);

/* log reading / volume helpers */
int nilfs_get_segment_log(struct nilfs_device *nilfsdev, uint64_t *blocknr,
	uint64_t *offset, struct buf **bpp, int len, void *blob);
void nilfs_search_super_root(struct nilfs_device *nilfsdev);

/* reading */
int nilfs_bread(struct nilfs_node *node, uint64_t blocknr,
	int flags, struct buf **bpp);

/* btree operations */
int nilfs_btree_nlookup(struct nilfs_node *node, uint64_t from, uint64_t blks, uint64_t *l2vmap);

/* vtop operations */
void nilfs_mdt_trans(struct nilfs_mdt *mdt, uint64_t index, uint64_t *blocknr, uint32_t *entry_in_block);
int nilfs_nvtop(struct nilfs_node *node, uint64_t blks, uint64_t *l2vmap, uint64_t *v2pmap);

/* node action implementators */
int nilfs_get_node_raw(struct nilfs_device *nilfsdev, struct nilfs_mount *ump, uint64_t ino, struct nilfs_inode *inode, struct nilfs_node **nodep);
void nilfs_dispose_node(struct nilfs_node **node);

int nilfs_grow_node(struct nilfs_node *node, uint64_t new_size);
int nilfs_shrink_node(struct nilfs_node *node, uint64_t new_size);
void nilfs_itimes(struct nilfs_node *nilfs_node, struct timespec *acc,
	struct timespec *mod, struct timespec *birth);
int  nilfs_update(struct vnode *node, struct timespec *acc,
	struct timespec *mod, struct timespec *birth, int updflags);

/* return vpp? */
int nilfs_lookup_name_in_dir(struct vnode *dvp, const char *name, int namelen, uint64_t *ino, int *found);
int nilfs_create_node(struct vnode *dvp, struct vnode **vpp, struct vattr *vap, struct componentname *cnp);
void nilfs_delete_node(struct nilfs_node *nilfs_node);

int nilfs_chsize(struct vnode *vp, u_quad_t newsize, kauth_cred_t cred);
int nilfs_dir_detach(struct nilfs_mount *ump, struct nilfs_node *dir_node, struct nilfs_node *nilfs_node, struct componentname *cnp);
int nilfs_dir_attach(struct nilfs_mount *ump, struct nilfs_node *dir_node, struct nilfs_node *nilfs_node, struct vattr *vap, struct componentname *cnp);


/* vnode operations */
int nilfs_inactive(void *v);
int nilfs_reclaim(void *v);
int nilfs_readdir(void *v);
int nilfs_getattr(void *v);
int nilfs_setattr(void *v);
int nilfs_pathconf(void *v);
int nilfs_open(void *v);
int nilfs_close(void *v);
int nilfs_access(void *v);
int nilfs_read(void *v);
int nilfs_write(void *v);
int nilfs_trivial_bmap(void *v);
int nilfs_vfsstrategy(void *v);
int nilfs_lookup(void *v);
int nilfs_create(void *v);
int nilfs_mknod(void *v);
int nilfs_link(void *);
int nilfs_symlink(void *v);
int nilfs_readlink(void *v);
int nilfs_rename(void *v);
int nilfs_remove(void *v);
int nilfs_mkdir(void *v);
int nilfs_rmdir(void *v);
int nilfs_fsync(void *v);
int nilfs_advlock(void *v);

#endif	/* !_FS_NILFS_NILFS_SUBR_H_ */
