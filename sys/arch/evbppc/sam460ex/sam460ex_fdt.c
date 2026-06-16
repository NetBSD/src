/*	$NetBSD: sam460ex_fdt.c,v 1.2 2026/06/16 23:37:49 rkujawa Exp $	*/

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
 * Flattened-device-tree bootstrap for the Sam460ex.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sam460ex_fdt.c,v 1.2 2026/06/16 23:37:49 rkujawa Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/sam460ex.h>

#include <dev/ofw/openfirm.h>
#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_console.h>

#include <libfdt.h>

struct sam460ex_fdt_info sam460ex_fdt_info;

/* see consinit() */
static int
sam460ex_fdt_console_nomatch(int phandle)
{
	return 0;
}

static const struct fdt_console sam460ex_fdt_console_none = {
	.match = sam460ex_fdt_console_nomatch,
	.consinit = NULL,
};
FDT_CONSOLE(sam460ex_none, &sam460ex_fdt_console_none);

/*
 * Our own copy of the device tree, kept live for the lifetime of the
 * kernel. 
 */
#define	SAM460EX_FDT_BUFSIZE	(64 * 1024)
static uint64_t fdt_buf[SAM460EX_FDT_BUFSIZE / sizeof(uint64_t)];

static char bootargs_buf[256];

static const struct device_compatible_entry emac_compat[] = {
	{ .compat = "ibm,emac4sync" },
	DEVICE_COMPAT_EOL
};

/*
 * Recursively collect the per-EMAC MAC addresses.
 */
static void
sam460ex_fdt_emac(int node)
{
	int child;

	for (child = OF_child(node); child; child = OF_peer(child)) {
		uint32_t idx;
		uint8_t mac[6];
		int j;

		if (of_compatible_match(child, emac_compat) != 0 &&
		    of_getprop_uint32(child, "cell-index", &idx) == 0 &&
		    idx < SAM460EX_NEMAC &&
		    OF_getprop(child, "local-mac-address", mac, sizeof(mac)) ==
		    (int)sizeof(mac)) {
			for (j = 0; j < 6; j++)
				if (mac[j] != 0)
					break;
			if (j != 6) {
				memcpy(sam460ex_fdt_info.fi_enaddr[idx], mac, 6);
				sam460ex_fdt_info.fi_enaddr_valid[idx] = true;
			}
		}
		sam460ex_fdt_emac(child);
	}
}

/*
 * Parse the device tree at the physical address U-Boot passed in r3.
 */
bool
sam460ex_fdt_parse(paddr_t fdt_pa)
{
	struct sam460ex_fdt_info *info = &sam460ex_fdt_info;
	const void *src = (const void *)fdt_pa;
	uint32_t ac, sc;
	int root, node, len;

	memset(info, 0, sizeof(*info));

	if (fdt_pa == 0 || fdt_pa >= 0x10000000 ||
	    fdt_check_header(src) != 0 ||
	    (size_t)fdt_totalsize(src) > sizeof(fdt_buf))
		return false;

	/*
	 * Copy the blob into our own buf
	 */
	if (fdt_open_into(src, fdt_buf, sizeof(fdt_buf)) != 0 ||
	    !fdtbus_init(fdt_buf))
		return false;

	root = OF_finddevice("/");

	/*
	 * RAM size: /memory "reg" = <address... size...>.  
	 */
	ac = 2;
	sc = 1;
	of_getprop_uint32(root, "#address-cells", &ac);
	of_getprop_uint32(root, "#size-cells", &sc);
	node = OF_finddevice("/memory");
	if (node >= 0 && ac > 0 && sc > 0 && ac + sc <= 4) {
		uint32_t reg[4];

		if (of_getprop_uint32_array(node, "reg", reg, ac + sc) == 0)
			info->fi_memsize = reg[ac + sc - 1];
	}

	/* CPU and timebase clocks (the timebase runs at the CPU clock). */
	node = OF_finddevice("/cpus/cpu@0");
	if (node >= 0) {
		of_getprop_uint32(node, "clock-frequency", &info->fi_cpu_freq);
		of_getprop_uint32(node, "timebase-frequency",
		    &info->fi_timebase_freq);
	}

	/* OPB (peripheral) clock. */
	node = OF_finddevice("/plb/opb");
	if (node >= 0)
		of_getprop_uint32(node, "clock-frequency", &info->fi_opb_freq);

	/* UART input clock: the first ns16550 in the tree. */
	node = of_find_bycompat(root, "ns16550");
	if (node != -1)
		of_getprop_uint32(node, "clock-frequency", &info->fi_uart_freq);

	/* Per-EMAC MAC addresses. */
	sam460ex_fdt_emac(root);

	/* Kernel command line from /chosen. */
	node = OF_finddevice("/chosen");
	if (node >= 0) {
		len = OF_getprop(node, "bootargs", bootargs_buf,
		    sizeof(bootargs_buf));
		if (len > 1)	/* length includes the terminating NUL */
			info->fi_bootargs = bootargs_buf;
	}

	return true;
}
