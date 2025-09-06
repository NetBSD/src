/*	$NetBSD: fdt_stub.c,v 1.1 2025/09/06 15:44:04 thorpej Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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
 * Stub routines for modular FDT binding functionality referenced from
 * common code.  If the real routines are present, these weak aliases are
 * overridden by the linker.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fdt_stub.c,v 1.1 2025/09/06 15:44:04 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

bus_dma_tag_t	fdtbus_iommu_map_nullop(int, u_int, bus_dma_tag_t);

bus_dma_tag_t
fdtbus_iommu_map_nullop(int phandle __unused, u_int index __unused,
    bus_dma_tag_t dmat)
{
	return dmat;
}
__weak_alias(fdtbus_iommu_map, fdtbus_iommu_map_nullop);

/* Must be in the same compilation unit, sigh. */
int	fdtbus_nullop(void);

int
fdtbus_nullop(void)
{
	return 0;
}

__weak_alias(fdtbus_pinctrl_set_config, fdtbus_nullop);
__weak_alias(fdtbus_pinctrl_has_config, fdtbus_nullop);

__weak_alias(fdtbus_power_reset, fdtbus_nullop);
__weak_alias(fdtbus_power_poweroff, fdtbus_nullop);

__weak_alias(fdtbus_powerdomain_enable_index, fdtbus_nullop);
__weak_alias(fdtbus_powerdomain_disable_index, fdtbus_nullop);
__weak_alias(fdtbus_powerdomain_enable, fdtbus_nullop);
__weak_alias(fdtbus_powerdomain_disable, fdtbus_nullop);
