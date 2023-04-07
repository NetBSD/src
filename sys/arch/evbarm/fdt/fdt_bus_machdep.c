/* $NetBSD: fdt_bus_machdep.c,v 1.3 2023/04/07 08:55:31 skrll Exp $ */

/*-
 * Copyright (c) 2021 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: fdt_bus_machdep.c,v 1.3 2023/04/07 08:55:31 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>

#include <arm/fdt/arm_fdtvar.h>

extern struct bus_space arm_generic_bs_tag;

static int
nonposted_mmio_bs_map(void *t, bus_addr_t bpa, bus_size_t size, int flag,
    bus_space_handle_t *bshp)
{
	if (flag == 0) {
		flag |= BUS_SPACE_MAP_NONPOSTED;
	}

	return bus_space_map(&arm_generic_bs_tag, bpa, size, flag, bshp);
}

bus_space_tag_t
fdtbus_bus_tag_create(int phandle, uint32_t flags)
{
	const struct fdt_platform *plat = fdt_platform_find();
	struct bus_space *tagp;
	struct fdt_attach_args faa;

	plat->fp_init_attach_args(&faa);

	tagp = kmem_alloc(sizeof(*tagp), KM_SLEEP);
	*tagp = *faa.faa_bst;
	if ((flags & FDT_BUS_SPACE_FLAG_NONPOSTED_MMIO) != 0) {
		tagp->bs_map = nonposted_mmio_bs_map;
	}

	return tagp;
}
