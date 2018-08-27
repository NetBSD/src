/*	$NetBSD: linux_dma_buf.c,v 1.1 2018/08/27 15:22:54 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_dma_buf.c,v 1.1 2018/08/27 15:22:54 riastradh Exp $");

#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/kmem.h>
#include <sys/mutex.h>

#include <linux/dma-buf.h>
#include <linux/err.h>
#include <linux/fence.h>
#include <linux/reservation.h>

struct dma_buf_file {
	struct dma_buf	*dbf_dmabuf;
};

static int	dmabuf_fop_poll(struct file *, int);
static int	dmabuf_fop_close(struct file *);
static int	dmabuf_fop_kqfilter(struct file *, struct knote *);
static int	dmabuf_fop_mmap(struct file *, off_t *, size_t, int, int *,
		    int *, struct uvm_object **, int *);

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

	if (dmabuf->resv == NULL) {
		dmabuf->resv = &dmabuf->db_resv_int[0];
		reservation_object_init(dmabuf->resv);
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

	fd_putfile(fd);
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
		goto fail1;
	}
	if (file->f_type != DTYPE_MISC || file->f_ops != &dmabuf_fileops) {
		error = EINVAL;
		goto fail0;
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

	if (atomic_dec_uint_nv(&dmabuf->db_refcnt) != 0)
		return;

	if (dmabuf->resv == &dmabuf->db_resv_int[0])
		reservation_object_fini(dmabuf->resv);
	if (dmabuf->resv == &dmabuf->db_resv_int[0]) {
		kmem_free(dmabuf, offsetof(struct dma_buf, db_resv_int[1]));
	} else {
		kmem_free(dmabuf, sizeof(*dmabuf));
	}
}

struct dma_buf_attachment *
dma_buf_attach(struct dma_buf *dmabuf, struct device *dev)
{
	struct dma_buf_attachment *attach;
	int ret = 0;

	attach = kmem_zalloc(sizeof(*attach), KM_SLEEP);
	attach->dmabuf = dmabuf;

	mutex_enter(&dmabuf->db_lock);
	if (dmabuf->ops->attach)
		ret = dmabuf->ops->attach(dmabuf, dev, attach);
	mutex_exit(&dmabuf->db_lock);
	if (ret)
		goto fail0;

	return attach;

fail0:	kmem_free(attach, sizeof(*attach));
	return ERR_PTR(ret);
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

	return attach->dmabuf->ops->map_dma_buf(attach, dir);
}

void
dma_buf_unmap_attachment(struct dma_buf_attachment *attach,
    struct sg_table *sg, enum dma_data_direction dir)
{

	return attach->dmabuf->ops->unmap_dma_buf(attach, sg, dir);
}

static int
dmabuf_fop_close(struct file *file)
{
	struct dma_buf_file *dbf = file->f_data;
	struct dma_buf *dmabuf = dbf->dbf_dmabuf;

	dma_buf_put(dmabuf);
	return 0;
}

static int
dmabuf_fop_poll(struct file *file, int events)
{
#ifdef notyet
	struct dma_buf_file *dbf = file->f_data;
	struct dma_buf *dmabuf = dbf->dbf_dmabuf;

	return reservation_object_poll(dmabuf->resv, events);
#else
	return -ENOSYS;
#endif
}

static int
dmabuf_fop_kqfilter(struct file *file, struct knote *knote)
{
#ifdef notyet
	struct dma_buf_file *dbf = file->f_data;
	struct dma_buf *dmabuf = dbf->dbf_dmabuf;

	return reservation_object_kqfilter(dmabuf->resv, knote);
#else
	return -ENOSYS;
#endif
}

static int
dmabuf_fop_mmap(struct file *file, off_t *offp, size_t size, int prot,
    int *flagsp, int *advicep, struct uvm_object **uobjp, int *maxprotp)
{
	struct dma_buf_file *dbf = file->f_data;
	struct dma_buf *dmabuf = dbf->dbf_dmabuf;

	if (size > dmabuf->size)
		return EINVAL;

	return dmabuf->ops->mmap(dmabuf, offp, size, prot, flagsp, advicep,
	    uobjp, maxprotp);
}
