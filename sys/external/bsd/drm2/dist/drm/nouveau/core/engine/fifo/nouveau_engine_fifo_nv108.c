/*	$NetBSD: nouveau_engine_fifo_nv108.c,v 1.1.1.1.2.2 2014/08/10 06:55:32 tls Exp $	*/

/*
 * Copyright 2013 Red Hat Inc.
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
__KERNEL_RCSID(0, "$NetBSD: nouveau_engine_fifo_nv108.c,v 1.1.1.1.2.2 2014/08/10 06:55:32 tls Exp $");

#include "nve0.h"

struct nouveau_oclass *
nv108_fifo_oclass = &(struct nve0_fifo_impl) {
	.base.handle = NV_ENGINE(FIFO, 0x08),
	.base.ofuncs = &(struct nouveau_ofuncs) {
		.ctor = nve0_fifo_ctor,
		.dtor = nve0_fifo_dtor,
		.init = nve0_fifo_init,
		.fini = _nouveau_fifo_fini,
	},
	.channels = 1024,
}.base;
