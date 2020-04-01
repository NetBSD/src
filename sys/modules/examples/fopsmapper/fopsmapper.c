/*	$NetBSD: fopsmapper.c,v 1.2 2020/04/01 11:45:53 kamil Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: fopsmapper.c,v 1.2 2020/04/01 11:45:53 kamil Exp $");

#include <sys/module.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <sys/conf.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/kmem.h>
#include <sys/mman.h>
#include <sys/mutex.h>
#include <uvm/uvm_extern.h>

/*
 * To use this module you need to:
 *
 * mknod /dev/fopsmapper c 351 0
 *
 */

dev_type_open(fopsmapper_open);

const struct cdevsw fopsmapper_cdevsw = {
	.d_open = fopsmapper_open,
	.d_close = noclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

static int fopsmapper_mmap(file_t *, off_t *, size_t,
	       	int, int *, int *,struct uvm_object **, int *);
static int fopsmapper_close(file_t *);

const struct fileops mapper_fileops = {
	.fo_read = fbadop_read,
	.fo_write = fbadop_write,
	.fo_ioctl = fbadop_ioctl,
	.fo_fcntl = fnullop_fcntl,
	.fo_poll = fnullop_poll,
	.fo_stat = fbadop_stat,
	.fo_close = fopsmapper_close,
	.fo_kqfilter = fnullop_kqfilter,
	.fo_restart = fnullop_restart,
	.fo_mmap = fopsmapper_mmap,
};

typedef struct fopsmapper_softc {
	char *buf;
	struct uvm_object *uobj;
	size_t bufsize;
} fops_t;

int
fopsmapper_open(dev_t dev, int flag, int mode, struct lwp *l)
{
	fops_t *fo;
	struct file *fp;
	int fd, error;

	if ((error = fd_allocfile(&fp, &fd)) != 0)
		return error;

	fo = kmem_zalloc(sizeof(*fo), KM_SLEEP);

	return fd_clone(fp, fd, flag, &mapper_fileops, fo);
}

int
fopsmapper_mmap(file_t * fp, off_t * offp, size_t size, int prot,
		int *flagsp, int *advicep, struct uvm_object **uobjp,
		int *maxprotp)
{
	fops_t *fo;
	int error;

	if (prot & PROT_EXEC)
		return EACCES;

	if (size != (size_t)PAGE_SIZE)
		return EINVAL;

	if ((fo = fp->f_data) == NULL)
		return ENXIO;

	fo->bufsize = size;
	fo->uobj = uao_create(size, 0);

	fo->buf = NULL;
	/* Map the uvm object into kernel */
	error =	uvm_map(kernel_map, (vaddr_t *) &fo->buf, fo->bufsize,
	                fo->uobj, 0, 0,
	                UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_RW,
	                            UVM_INH_SHARE,UVM_ADV_RANDOM, 0));

	if (error) {
		uao_detach(fo->uobj);
		return error;
	}
	snprintf(fo->buf, 13, "Hey There!");

	/* Get the reference of uobj */
	uao_reference(fo->uobj);

	*uobjp = fo->uobj;
	*maxprotp = prot;
	*advicep = UVM_ADV_RANDOM;

	return 0;
}

int
fopsmapper_close(file_t * fp)
{
	fops_t *fo;

	fo = fp->f_data;
	KASSERT(fo != NULL);

	if (fo->buf != NULL)
		uvm_deallocate(kernel_map, (vaddr_t)fo->buf, fo->bufsize);

	kmem_free(fo, sizeof(*fo));

	return 0;
}

MODULE(MODULE_CLASS_MISC, fopsmapper, NULL);

static int
fopsmapper_modcmd(modcmd_t cmd, void *arg __unused)
{
	int cmajor = 351, bmajor = -1;

	switch (cmd) {
	case MODULE_CMD_INIT:
		if (devsw_attach("fopsmapper", NULL, &bmajor,
		    &fopsmapper_cdevsw, &cmajor))
			return ENXIO;
		return 0;
	case MODULE_CMD_FINI:
		return EBUSY;
	default:
		return ENOTTY;
	}
}
