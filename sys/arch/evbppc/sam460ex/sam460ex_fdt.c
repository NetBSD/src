/*	$NetBSD: sam460ex_fdt.c,v 1.1 2026/06/16 21:51:20 rkujawa Exp $	*/

/*
 * Copyright (c) 2012, 2014, 2024, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Minimal flattened-device-tree parsing for the Sam460ex.
 * Extract just enough for early bootstrap.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sam460ex_fdt.c,v 1.1 2026/06/16 21:51:20 rkujawa Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/sam460ex.h>

#include <libfdt.h>

struct sam460ex_fdt_info sam460ex_fdt_info;

static char bootargs_buf[256];

static uint32_t
fdt_getprop_u32(const void *fdt, int node, const char *prop, uint32_t defval)
{
	const fdt32_t *p;
	int len;

	p = fdt_getprop(fdt, node, prop, &len);
	if (p == NULL || len < (int)sizeof(*p))
		return defval;
	return fdt32_to_cpu(*p);
}

/*
 * Parse the FDT at fdt_pa
 */
bool
sam460ex_fdt_parse(paddr_t fdt_pa)
{
	struct sam460ex_fdt_info *info = &sam460ex_fdt_info;
	const void *fdt = (const void *)fdt_pa;
	const fdt32_t *reg;
	const char *args;
	int node, len, addr_cells, size_cells;

	memset(info, 0, sizeof(*info));

	if (fdt_pa == 0 || fdt_pa >= 0x10000000 ||
	    fdt_check_header(fdt) != 0)
		return false;

	/* /memory reg = <addr-cells..., size-cells...> */
	addr_cells = fdt_address_cells(fdt, 0);
	size_cells = fdt_size_cells(fdt, 0);
	node = fdt_path_offset(fdt, "/memory");
	if (node >= 0 && addr_cells > 0 && size_cells > 0) {
		reg = fdt_getprop(fdt, node, "reg", &len);
		if (reg != NULL &&
		    len >= (int)((addr_cells + size_cells) * sizeof(*reg))) {
			/* memory starts at 0; sum is the low size word */
			info->fi_memsize =
			    fdt32_to_cpu(reg[addr_cells + size_cells - 1]);
		}
	}

	node = fdt_path_offset(fdt, "/cpus/cpu@0");
	if (node >= 0) {
		info->fi_cpu_freq =
		    fdt_getprop_u32(fdt, node, "clock-frequency", 0);
		info->fi_timebase_freq =
		    fdt_getprop_u32(fdt, node, "timebase-frequency", 0);
	}

	node = fdt_path_offset(fdt, "/plb/opb");
	if (node >= 0)
		info->fi_opb_freq =
		    fdt_getprop_u32(fdt, node, "clock-frequency", 0);

	node = fdt_node_offset_by_compatible(fdt, -1, "ns16550");
	if (node >= 0)
		info->fi_uart_freq =
		    fdt_getprop_u32(fdt, node, "clock-frequency", 0);

	/*
	 * EMAC MAC addresses
	 */
	node = -1;
	while ((node = fdt_node_offset_by_compatible(fdt, node,
	    "ibm,emac4sync")) >= 0) {
		const uint8_t *mac;
		uint32_t idx;
		int j;

		idx = fdt_getprop_u32(fdt, node, "cell-index", ~0U);
		if (idx >= SAM460EX_NEMAC)
			continue;
		mac = fdt_getprop(fdt, node, "local-mac-address", &len);
		if (mac == NULL || len < 6)
			continue;
		for (j = 0; j < 6; j++)
			if (mac[j] != 0)
				break;
		if (j == 6)
			continue;	/* all-zero: not fixed up */
		memcpy(info->fi_enaddr[idx], mac, 6);
		info->fi_enaddr_valid[idx] = true;
	}

	node = fdt_path_offset(fdt, "/chosen");
	if (node >= 0) {
		args = fdt_getprop(fdt, node, "bootargs", &len);
		if (args != NULL && len > 0) {
			strlcpy(bootargs_buf, args,
			    uimin(sizeof(bootargs_buf), (size_t)len + 1));
			info->fi_bootargs = bootargs_buf;
		}
	}

	return true;
}
