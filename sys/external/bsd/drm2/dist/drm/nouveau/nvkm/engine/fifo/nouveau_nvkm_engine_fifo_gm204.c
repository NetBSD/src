/*	$NetBSD: nouveau_nvkm_engine_fifo_gm204.c,v 1.2 2018/08/27 04:58:31 riastradh Exp $	*/

/*
 * Copyright 2015 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Ben Skeggs
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nouveau_nvkm_engine_fifo_gm204.c,v 1.2 2018/08/27 04:58:31 riastradh Exp $");

#include "gk104.h"
#include "changk104.h"

static const struct nvkm_fifo_func
gm204_fifo = {
	.dtor = gk104_fifo_dtor,
	.oneinit = gk104_fifo_oneinit,
	.init = gk104_fifo_init,
	.fini = gk104_fifo_fini,
	.intr = gk104_fifo_intr,
	.uevent_init = gk104_fifo_uevent_init,
	.uevent_fini = gk104_fifo_uevent_fini,
	.chan = {
		&gm204_fifo_gpfifo_oclass,
		NULL
	},
};

int
gm204_fifo_new(struct nvkm_device *device, int index, struct nvkm_fifo **pfifo)
{
	return gk104_fifo_new_(&gm204_fifo, device, index, 4096, pfifo);
}
