/* $NetBSD: fdt_dma_machdep.c,v 1.1.6.2 2020/04/08 14:07:35 martin Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: fdt_dma_machdep.c,v 1.1.6.2 2020/04/08 14:07:35 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>

extern struct arm32_bus_dma_tag arm_generic_dma_tag;

bus_dma_tag_t
fdtbus_dma_tag_create(int phandle, const struct fdt_dma_range *ranges,
    u_int nranges)
{
	struct arm32_bus_dma_tag *tagp;
	u_int n;

	const int flags = of_hasprop(phandle, "dma-coherent") ?
	    _BUS_DMAMAP_COHERENT : 0;

	tagp = kmem_alloc(sizeof(*tagp), KM_SLEEP);
	*tagp = arm_generic_dma_tag;
	if (nranges == 0) {
		tagp->_nranges = 1;
		tagp->_ranges = kmem_alloc(sizeof(*tagp->_ranges), KM_SLEEP);
		tagp->_ranges[0].dr_sysbase = 0;
		tagp->_ranges[0].dr_busbase = 0;
		tagp->_ranges[0].dr_len = UINTPTR_MAX;
		tagp->_ranges[0].dr_flags = flags;
	} else {
		tagp->_nranges = nranges;
		tagp->_ranges = kmem_alloc(sizeof(*tagp->_ranges) * nranges,
		    KM_SLEEP);
		for (n = 0; n < nranges; n++) {
			tagp->_ranges[n].dr_sysbase = ranges[n].dr_sysbase;
			tagp->_ranges[n].dr_busbase = ranges[n].dr_busbase;
			tagp->_ranges[n].dr_len = ranges[n].dr_len;
			tagp->_ranges[n].dr_flags = flags;
		}
	}

	return tagp;
}
