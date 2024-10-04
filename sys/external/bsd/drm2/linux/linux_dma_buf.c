/*	$NetBSD: linux_dma_buf.c,v 1.15.4.2 2024/10/04 11:40:50 martin Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_dma_buf.c,v 1.15.4.2 2024/10/04 11:40:50 martin Exp $");

#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/kmem.h>
#include <sys/mutex.h>

#include <linux/dma-buf.h>
#include <linux/err.h>
#include <linux/dma-resv.h>

static int	dmabuf_fop_poll(struct file *, int);
static int	dmabuf_fop_close(struct file *);
static int	dmabuf_fop_kqfilter(struct file *, struct knote *);
static int	dmabuf_fop_mmap(struct file *, off_t *, size_t, int, int *,
		    int *, struct uvm_object **, int *);
static int	dmabuf_fop_seek(struct file *fp, off_t delta, int whence,
		    off_t *newoffp, int flags);

static const struct fileops dmabuf_fileops = {
	.fo_name = "dmabuf",
	.fo_read = fbadop_read,
	.fo_write = fbadop_write,
	.fo_ioctl = fbadop_ioctl,
	.fo_fcntl = fnullop_fcntl,
	.fo_poll = dmabuf_fop_poll,
	.fo_stat = fbadop_stat,
	.fo_close = dmabuf_fop_close,
	.fo_kqfilter = dmabuf_fop_kqfilter,
	.fo_restart = fnullop_restart,
	.fo_mmap = dmabuf_fop_mmap,
	.fo_seek = dmabuf_fop_seek,
};

struct dma_buf *
dma_buf_export(struct dma_buf_export_info *info)
{
	struct dma_buf *dmabuf;

	if (info->resv == NULL) {
		dmabuf = kmem_zalloc(offsetof(struct dma_buf, db_resv_int[1]),
		    KM_SLEEP);
	} else {
		dmabuf = kmem_zalloc(sizeof(*dmabuf), KM_SLEEP);
	}

	dmabuf->priv = info->priv;
	dmabuf->ops = info->ops;
	dmabuf->size = info->size;
	dmabuf->resv = info->resv;

	mutex_init(&dmabuf->db_lock, MUTEX_DEFAULT, IPL_NONE);
	dmabuf->db_refcnt = 1;
	dma_resv_poll_init(&dmabuf->db_resv_poll);

	if (dmabuf->resv == NULL) {
		dmabuf->resv = &dmabuf->db_resv_int[0];
		dma_resv_init(dmabuf->resv);
	}

	return dmabuf;
}

int
dma_buf_fd(struct dma_buf *dmabuf, int flags)
{
	struct file *file;
	int fd;
	unsigned refcnt __diagused;
	int ret;

	/* XXX errno NetBSD->Linux */
	ret = -fd_allocfile(&file, &fd);
	if (ret)
		goto out0;

	refcnt = atomic_inc_uint_nv(&dmabuf->db_refcnt);
	KASSERT(refcnt > 1);

	file->f_type = DTYPE_MISC;
	file->f_flag = 0;	/* XXX DRM code allows only O_CLOEXEC.  */
	file->f_ops = &dmabuf_fileops;
	file->f_data = dmabuf;
	fd_set_exclose(curlwp, fd, (flags & O_CLOEXEC) != 0);
	fd_affix(curproc, file, fd);

	ret = fd;
out0:	return ret;
}

struct dma_buf *
dma_buf_get(int fd)
{
	struct file *file;
	struct dma_buf *dmabuf;
	unsigned refcnt __diagused;
	int error;

	if ((file = fd_getfile(fd)) == NULL) {
		error = EBADF;
		goto fail0;
	}
	if (file->f_type != DTYPE_MISC || file->f_ops != &dmabuf_fileops) {
		error = EINVAL;
		goto fail1;
	}

	dmabuf = file->f_data;
	refcnt = atomic_inc_uint_nv(&dmabuf->db_refcnt);
	KASSERT(refcnt > 1);
	fd_putfile(fd);
	return dmabuf;

fail1:	fd_putfile(fd);
fail0:	KASSERT(error);
	return ERR_PTR(-error);
}

void
get_dma_buf(struct dma_buf *dmabuf)
{
	unsigned refcnt __diagused;

	refcnt = atomic_inc_uint_nv(&dmabuf->db_refcnt);
	KASSERT(refcnt > 1);
}

void
dma_buf_put(struct dma_buf *dmabuf)
{

	membar_release();
	if (atomic_dec_uint_nv(&dmabuf->db_refcnt) != 0)
		return;
	membar_acquire();

	dma_resv_poll_fini(&dmabuf->db_resv_poll);
	mutex_destroy(&dmabuf->db_lock);
	if (dmabuf->resv == &dmabuf->db_resv_int[0]) {
		dma_resv_fini(dmabuf->resv);
		kmem_free(dmabuf, offsetof(struct dma_buf, db_resv_int[1]));
	} else {
		kmem_free(dmabuf, sizeof(*dmabuf));
	}
}

struct dma_buf_attachment *
dma_buf_dynamic_attach(struct dma_buf *dmabuf, bus_dma_tag_t dmat,
    bool dynamic_mapping)
{
	struct dma_buf_attachment *attach;
	int ret = 0;

	attach = kmem_zalloc(sizeof(*attach), KM_SLEEP);
	attach->dmabuf = dmabuf;
	attach->dev = dmat;
	attach->dynamic_mapping = dynamic_mapping;

	mutex_enter(&dmabuf->db_lock);
	if (dmabuf->ops->attach)
		ret = dmabuf->ops->attach(dmabuf, attach);
	mutex_exit(&dmabuf->db_lock);
	if (ret)
		goto fail0;

	if (attach->dynamic_mapping != dmabuf->ops->dynamic_mapping)
		panic("%s: NYI", __func__);

	return attach;

fail0:	kmem_free(attach, sizeof(*attach));
	return ERR_PTR(ret);
}

struct dma_buf_attachment *
dma_buf_attach(struct dma_buf *dmabuf, bus_dma_tag_t dmat)
{

	return dma_buf_dynamic_attach(dmabuf, dmat, /*dynamic_mapping*/false);
}

void
dma_buf_detach(struct dma_buf *dmabuf, struct dma_buf_attachment *attach)
{

	mutex_enter(&dmabuf->db_lock);
	if (dmabuf->ops->detach)
		dmabuf->ops->detach(dmabuf, attach);
	mutex_exit(&dmabuf->db_lock);

	kmem_free(attach, sizeof(*attach));
}

struct sg_table *
dma_buf_map_attachment(struct dma_buf_attachment *attach,
    enum dma_data_direction dir)
{
	struct sg_table *sg;

	if (attach->dmabuf->ops->dynamic_mapping)
		dma_resv_lock(attach->dmabuf->resv, NULL);
	sg = attach->dmabuf->ops->map_dma_buf(attach, dir);
	if (attach->dmabuf->ops->dynamic_mapping)
		dma_resv_unlock(attach->dmabuf->resv);

	return sg;
}

void
dma_buf_unmap_attachment(struct dma_buf_attachment *attach,
    struct sg_table *sg, enum dma_data_direction dir)
{

	if (attach->dmabuf->ops->dynamic_mapping)
		dma_resv_lock(attach->dmabuf->resv, NULL);
	attach->dmabuf->ops->unmap_dma_buf(attach, sg, dir);
	if (attach->dmabuf->ops->dynamic_mapping)
		dma_resv_unlock(attach->dmabuf->resv);
}

static int
dmabuf_fop_close(struct file *file)
{
	struct dma_buf *dmabuf = file->f_data;

	dma_buf_put(dmabuf);
	return 0;
}

static int
dmabuf_fop_poll(struct file *file, int events)
{
	struct dma_buf *dmabuf = file->f_data;
	struct dma_resv_poll *rpoll = &dmabuf->db_resv_poll;

	return dma_resv_do_poll(dmabuf->resv, events, rpoll);
}

static int
dmabuf_fop_kqfilter(struct file *file, struct knote *kn)
{
	struct dma_buf *dmabuf = file->f_data;
	struct dma_resv_poll *rpoll = &dmabuf->db_resv_poll;

	return dma_resv_kqfilter(dmabuf->resv, kn, rpoll);
}

static int
dmabuf_fop_mmap(struct file *file, off_t *offp, size_t size, int prot,
    int *flagsp, int *advicep, struct uvm_object **uobjp, int *maxprotp)
{
	struct dma_buf *dmabuf = file->f_data;

	if (size > dmabuf->size || *offp < 0 || *offp > dmabuf->size - size)
		return EINVAL;

	return dmabuf->ops->mmap(dmabuf, offp, size, prot, flagsp, advicep,
	    uobjp, maxprotp);
}

/*
 * We don't actually do anything with the file offset; this is just how
 * libdrm_amdgpu expects to find the size of the DMA buf.  (Why it
 * doesn't use fstat is unclear, but it doesn't really matter.)
 */
static int
dmabuf_fop_seek(struct file *fp, off_t delta, int whence, off_t *newoffp,
    int flags)
{
	const off_t OFF_MAX = __type_max(off_t);
	struct dma_buf *dmabuf = fp->f_data;
	off_t base, newoff;
	int error;

	mutex_enter(&fp->f_lock);

	switch (whence) {
	case SEEK_CUR:
		base = fp->f_offset;
		break;
	case SEEK_END:
		base = dmabuf->size;
		break;
	case SEEK_SET:
		base = 0;
		break;
	default:
		error = EINVAL;
		goto out;
	}

	/* Check for arithmetic overflow and reject negative offsets.  */
	if (base < 0 || delta > OFF_MAX - base || base + delta < 0) {
		error = EINVAL;
		goto out;
	}

	/* Compute the new offset.  */
	newoff = base + delta;

	/* Success!  */
	if (newoffp)
		*newoffp = newoff;
	if (flags & FOF_UPDATE_OFFSET)
		fp->f_offset = newoff;
	error = 0;

out:	mutex_exit(&fp->f_lock);
	return error;
}
