/* $NetBSD: fdt_dma_machdep.c,v 1.1 2020/07/16 11:49:38 jmcneill Exp $ */

/*-
 * Copyright (c) 2020 Jared McNeill <jmcneill@invisible.ca>
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fdt_dma_machdep.c,v 1.1 2020/07/16 11:49:38 jmcneill Exp $");

#define	_MIPS_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>

static struct mips_bus_dma_tag fdtbus_dma_tag = {
	._cookie = NULL,
	._wbase = 0,
	._bounce_alloc_lo = 0,
	._bounce_alloc_hi = 0,
	._dmamap_ops = _BUS_DMAMAP_OPS_INITIALIZER,
	._dmamem_ops = _BUS_DMAMEM_OPS_INITIALIZER,
	._dmatag_ops = _BUS_DMATAG_OPS_INITIALIZER,
};

bus_dma_tag_t
fdtbus_dma_tag_create(int phandle, const struct fdt_dma_range *ranges,
    u_int nranges)
{
	bus_dma_tag_t newtag;
	bus_addr_t min_addr, max_addr;
	int error;

	if (nranges == 0)
		return &fdtbus_dma_tag;

	KASSERT(nranges == 1);
	KASSERT(ranges[0].dr_sysbase == ranges[0].dr_busbase);

	min_addr = ranges[0].dr_sysbase;
	max_addr = min_addr + ranges[0].dr_len - 1;
	error = bus_dmatag_subregion(&fdtbus_dma_tag, min_addr, max_addr,
	    &newtag, 0);
	KASSERT(error == 0);

	return newtag;
}
