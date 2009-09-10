/*	$NetBSD: irix_kmem.c,v 1.8.18.1 2009/09/10 01:52:34 matt Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
__KERNEL_RCSID(0, "$NetBSD: irix_kmem.c,v 1.8.18.1 2009/09/10 01:52:34 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/vnode.h>
#include <sys/conf.h>

#include <compat/irix/irix_kmem.h>
#include <compat/irix/irix_sysmp.h>

/*
 * We have no idea of theses value's meaning yet, but time will come
 */
long irix_kernel_var[32] = { 0x7fff2fc8, 0x0fb72194, 0x00000000, 0x0fb68890,
			     0x7fff2f74, 0x7fff2fc8, 0x7fff2fc8, 0x7fff2f74,
			     0x7fff2f7c, 0x0fb7246c, 0x0fbd9178, 0x0fb910d4,
			     0x0fb65440, 0x00000800, 0x00400034, 0x0000000a,
			     0x00000008, 0x0fb73fb0, 0x00000000, 0x00000000,
			     0x00000000, 0x7fff3000, 0x00000001, 0x7fff2f74,
			     0x7fff2d24, 0x0fa85880, 0x00000001, 0x0fb60188,
			     0x00000004, 0x00000000, 0x00000000, 0x00000001,
			   };
struct irix_kmem_softc {
	struct device irix_kmem_dev;
};

dev_type_open(irix_kmemopen);
dev_type_close(irix_kmemclose);
dev_type_read(irix_kmemread);
dev_type_write(irix_kmemwrite);

const struct cdevsw irix_kmem_cdevsw = {
	irix_kmemopen, irix_kmemclose, irix_kmemread, irix_kmemwrite,
	noioctl, nostop, notty, nopoll, nommap, nokqfilter,
};

void
irix_kmemattach(struct device *parent, struct device *self, void *aux)
{
	return;
}

int
irix_kmemopen(dev, flags, fmt, l)
	dev_t dev;
	int flags, fmt;
	struct lwp *l;
{
	return 0;
}

int
irix_kmemread(dev_t dev, struct uio *uio, int flag)
{
	void *buf = NULL;
	off_t buflen = 0;
	void *offset;
	int error = 0;

#ifdef DEBUG_IRIX
	printf("irix_kmemread(): offset = 0x%08lx\n", (long)uio->uio_offset);
	printf("irix_kmemread(): addr = %p\n", uio->uio_iov->iov_base);
	printf("irix_kmemread(): len = 0x%08lx\n", (long)uio->uio_iov->iov_len);
#endif
	offset = (void *)(uintptr_t)uio->uio_offset; /* XXX */
	if (offset == &averunnable) { /* load average */
		struct irix_loadavg iav;
		int scale = averunnable.fscale / IRIX_LOADAVG_SCALE;

		(void)memset(&iav, 0, sizeof(iav));
		iav.ldavg[0] = averunnable.ldavg[0] / scale;
		iav.ldavg[1] = averunnable.ldavg[1] / scale;
		iav.ldavg[2] = averunnable.ldavg[2] / scale;
		buf = (void *)&iav;
		buflen = sizeof(iav);
	}

	if (offset == &irix_kernel_var) { /* kernel variables */
		buf = &irix_kernel_var;
		buflen = sizeof(irix_kernel_var);
	}

	if (buflen != 0)
		error = uiomove(buf, buflen, uio);
	return error;
}

int
irix_kmemwrite(dev_t dev, struct uio *uio, int flag)
{
	return 0;
}

int
irix_kmemclose(dev, flags, fmt, l)
	dev_t dev;
	int flags, fmt;
	struct lwp *l;
{
	return 0;
}

