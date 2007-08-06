/*	$NetBSD: genfs.c,v 1.2 2007/08/06 20:46:28 pooka Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by Google Summer of Code.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/vnode.h>

#include <miscfs/genfs/genfs_node.h>
#include <miscfs/genfs/genfs.h>

#include "rump.h"
#include "rumpuser.h"

void
genfs_node_init(struct vnode *vp, const struct genfs_ops *ops)
{
	struct genfs_node *gp = VTOG(vp);

	gp->g_op = ops;
}

void
genfs_node_destroy(struct vnode *vp)
{

	return;
}

void
genfs_node_unlock(struct vnode *vp)
{

}

void
genfs_node_wrlock(struct vnode *vp)
{

}

void
genfs_directio(struct vnode *vp, struct uio *uio, int ioflag)
{

	panic("%s: not implemented", __func__);
}

int
genfs_getpages(void *v)
{
	struct vop_getpages_args /* {
		struct vnode *a_vp;
		voff_t a_offset;
		struct vm_page **a_m;
		int *a_count;
		int a_centeridx;
		vm_prot_t a_access_type;
		int a_advice;
		int a_flags;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct buf buf;
	struct vm_page *pg;
	voff_t curoff, endoff;
	size_t remain, bufoff, xfersize;
	uint8_t *tmpbuf;
	int bshift = vp->v_mount->mnt_fs_bshift;
	int bsize = 1<<bshift;
	int count = *ap->a_count;
	int i, error;

	/* we'll allocate pages using other means */
	if (ap->a_flags & PGO_NOBLOCKALLOC) {
		*ap->a_count = 0;
		return 0;
	}

	if (ap->a_centeridx != 0)
		panic("%s: centeridx != not supported", __func__);

	curoff = ap->a_offset & ~PAGE_MASK;
	for (i = 0; i < count; i++, curoff += PAGE_SIZE) {
		pg = rumpvm_findpage(&vp->v_uobj, curoff);
		if (pg == NULL)
			break;
		ap->a_m[i] = pg;
	}

	/* got everything?  if so, just return */
	if (i == count)
		return 0;

	/*
	 * else, transfer entire range for simplicity and copy into
	 * page buffers
	 */

	/* align to boundaries */
	endoff = ap->a_offset + (count << PAGE_SHIFT);
	endoff = MIN(round_page(endoff), ((vp->v_writesize+bsize)&~(bsize-1)));
	curoff = ap->a_offset & ~(MAX(bsize,PAGE_SIZE)-1);
	remain = endoff - curoff;

	printf("a_offset: %x, startoff: 0x%x, endoff 0x%x\n", (int)ap->a_offset, (int)curoff, (int)endoff);

	/* read everything into a buffer */
	tmpbuf = rumpuser_malloc(remain, 0);
	memset(tmpbuf, 0, remain);
	for (bufoff = 0; remain; remain -= xfersize, bufoff+=xfersize) {
		struct vnode *devvp;
		daddr_t lbn, bn;
		int run;

		lbn = (curoff + bufoff) >> bshift;
		/* XXX: assume eof */
		error = VOP_BMAP(vp, lbn, &devvp, &bn, &run);
		if (error)
			panic("%s: VOP_BMAP & lazy bum: %d", __func__, error);

		printf("lbn %d run %d foo %d\n", (int)lbn, run, (int)(curoff+bufoff));
		xfersize = MIN(((lbn+1+run)<<bshift)-(curoff+bufoff), remain);
		buf.b_data = tmpbuf + bufoff;
		buf.b_bcount = xfersize;
		buf.b_blkno = bn;
		buf.b_lblkno = 0;
		buf.b_flags = B_READ;

		VOP_STRATEGY(devvp, &buf);
		if (buf.b_error)
			panic("%s: VOP_STRATEGY, lazy bum", __func__);
	}

	/* skip to beginning of pages we're interested in */
	bufoff = 0;
	while (round_page(curoff + bufoff) < trunc_page(ap->a_offset))
		bufoff += PAGE_SIZE;

	printf("first page offset 0x%x\n", (int)(curoff + bufoff));

	for (i = 0; i < count; i++, bufoff += PAGE_SIZE) {
		pg = rumpvm_findpage(&vp->v_uobj, curoff + bufoff);
		printf("got page %p (off 0x%x)\n", pg, (int)(curoff+bufoff));
		if (pg == NULL) {
			pg = rumpvm_makepage(&vp->v_uobj, curoff + bufoff, 1);
			memcpy((void *)pg->uobject, tmpbuf+bufoff, PAGE_SIZE);
			pg->flags = PG_CLEAN;
		}
		ap->a_m[i] = pg;
	}

	rumpuser_free(tmpbuf);

	return 0;
}

/*
 * simplesimplesimple: we put all pages every time.
 */
int
genfs_putpages(void *v)
{
	struct vop_putpages_args /* {
		struct vnode *a_vp;
		voff_t a_offlo;
		voff_t a_offhi;
		int a_flags;
	} */ *ap = v;
	char databuf[MAXPHYS];
	struct buf buf;
	struct vnode *vp = ap->a_vp;
	struct uvm_object *uobj = &vp->v_uobj;
	struct vm_page *pg, *pg_next;
	voff_t smallest = -1;
	voff_t curoff, bufoff;
	size_t xfersize;
	int bshift = vp->v_mount->mnt_fs_bshift;

 restart:
	/* check if all pages are clean */
	for (pg = TAILQ_FIRST(&uobj->memq); pg; pg = pg_next) {
		pg_next = TAILQ_NEXT(pg, listq);
		if (pg->flags & PG_CLEAN) {
			rumpvm_freepage(uobj, pg);
			continue;
		}

		if (pg->offset < smallest || smallest == -1)
			smallest = pg->offset;
	}

	/* all done? */
	if (TAILQ_EMPTY(&uobj->memq))
		return 0;

	/* we need to flush */
	for (curoff = smallest; curoff < vp->v_writesize; curoff += PAGE_SIZE) {
		if (curoff - smallest > MAXPHYS)
			break;
		pg = rumpvm_findpage(uobj, curoff);
		if (pg == NULL)
			break;
		memcpy(databuf + (curoff-smallest),
		    (void *)pg->uobject, PAGE_SIZE);
		pg->flags |= PG_CLEAN;
	}
	assert(curoff > smallest);

	/* then we write */
	for (bufoff = 0; bufoff < curoff - smallest; bufoff += xfersize) {
		struct vnode *devvp;
		daddr_t bn, lbn;
		int run, error;

		lbn = (smallest + bufoff) >> bshift;
		error = VOP_BMAP(vp, lbn, &devvp, &bn, &run);
		if (error)
			panic("%s: VOP_BMAP failed: %d", __func__, error);

		xfersize = MIN(((lbn+1+run) << bshift) - (smallest+bufoff),
		     curoff - (smallest+bufoff));

		/* only write max what we are allowed to write */
		buf.b_bcount = xfersize;
		if (smallest + bufoff + xfersize > vp->v_writesize)
			buf.b_bcount -= (smallest+bufoff+xfersize)
			    - vp->v_writesize;

		buf.b_lblkno = 0;
		buf.b_blkno = bn;
		buf.b_data = databuf + bufoff;
		buf.b_flags = B_WRITE;

		VOP_STRATEGY(devvp, &buf);
		if (buf.b_error)
			panic("%s: VOP_STRATEGY lazy bum %d",
			    __func__, buf.b_error);
	}

	goto restart;
}

void
genfs_size(struct vnode *vp, off_t size, off_t *eobp, int flags)
{
	int fs_bsize = 1 << vp->v_mount->mnt_fs_bshift;

	*eobp = (size + fs_bsize - 1) & ~(fs_bsize - 1);
}

int
genfs_compat_gop_write(struct vnode *vp, struct vm_page **pgs,
	int npages, int flags)
{

	panic("%s: not implemented", __func__);
}

int
genfs_gop_write(struct vnode *vp, struct vm_page **pgs, int npages, int flags)
{

	panic("%s: not implemented", __func__);
}
