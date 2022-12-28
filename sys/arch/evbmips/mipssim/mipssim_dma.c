/*	$NetBSD: mipssim_dma.c,v 1.2 2022/12/28 11:40:35 he Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

/*
 * Platform-specific DMA support for the mipssim.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mipssim_dma.c,v 1.2 2022/12/28 11:40:35 he Exp $");

#include <sys/param.h>
#include <sys/device.h>

#define	_MIPS_BUS_DMA_PRIVATE
#include <sys/bus.h>

#include <dev/isa/isavar.h>

#include <evbmips/mipssim/mipssimreg.h>
#include <evbmips/mipssim/mipssimvar.h>

void
mipssim_dma_init(struct mipssim_config *mcp)
{
	bus_dma_tag_t t;

	t = &mcp->mc_dmat;
	t->_cookie = mcp;
	t->_wbase = MIPSSIM_DMA_BASE;
	t->_bounce_alloc_lo = MIPSSIM_DMA_PHYSBASE;
	t->_bounce_alloc_hi = MIPSSIM_DMA_PHYSBASE + MIPSSIM_DMA_SIZE;
	t->_bounce_thresh = t->_bounce_alloc_hi; /* as an approximation */
	t->_dmamap_ops = mips_bus_dmamap_ops;
	t->_dmamem_ops = mips_bus_dmamem_ops;
	t->_dmatag_ops = mips_bus_dmatag_ops;
}
