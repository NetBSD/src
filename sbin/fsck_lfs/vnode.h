/* $NetBSD: vnode.h,v 1.1 2003/03/28 08:09:55 perseant Exp $ */
/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Konrad E. Schroder <perseant@hhhh.org>.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
#define VNODE_CACHE_SIZE 1000

LIST_HEAD(ubuflists, ubuf);
LIST_HEAD(uvnodelst, uvnode);
LIST_HEAD(vgrlst,    vget_reg);

struct uvnode {
	int v_fd;
	int (*v_strategy_op) (struct ubuf *);
	int (*v_bwrite_op) (struct ubuf *);
	int (*v_bmap_op) (struct uvnode *, daddr_t, daddr_t *);
#define VDIROP          0x1000	/* vnode is involved in a directory op */
#define	VDIRTY		0x8000	/* vnode is dirty */
	u_int32_t v_flag;
	void *v_fs;
	void *v_data;
#undef v_usecount
	int v_usecount;
	struct ubuflists v_cleanblkhd;	/* clean blocklist head */
	struct ubuflists v_dirtyblkhd;	/* dirty blocklist head */
	LIST_ENTRY(uvnode) v_mntvnodes;
	LIST_ENTRY(uvnode) v_getvnodes;
};

struct vget_reg {
	void *vgr_fs;
	struct uvnode *(*vgr_func) (void *, ino_t);
	LIST_ENTRY(vget_reg) vgr_list;
};

#define VOP_STRATEGY(bp)          ((bp)->b_vp->v_strategy_op(bp))
#define VOP_BWRITE(bp)            ((bp)->b_vp->v_bwrite_op(bp))
#define VOP_BMAP(vp, lbn, daddrp) ((vp)->v_bmap_op((vp), (lbn), (daddrp)))

extern int fsdirty;
extern struct uvnodelst vnodelist;

int raw_vop_strategy(struct ubuf *);
int raw_vop_bwrite(struct ubuf *);
int raw_vop_bmap(struct uvnode *, daddr_t, daddr_t *);

void vnode_destroy(struct uvnode *);
struct uvnode *vget(void *, ino_t);
void register_vget(void *, struct uvnode *(void *, ino_t));
void vfs_init(void);
