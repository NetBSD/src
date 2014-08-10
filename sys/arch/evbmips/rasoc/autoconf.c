/*	$NetBSD: autoconf.c,v 1.4.10.1 2014/08/10 06:53:56 tls Exp $	*/
/*-
 * Copyright (c) 2011 CradlePoint Technology, Inc.
 * All rights reserved.
 *
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
 * THIS SOFTWARE IS PROVIDED BY CRADLEPOINT TECHNOLOGY, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.4.10.1 2014/08/10 06:53:56 tls Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <mips/ralink/ralink_reg.h>
#include <mips/ralink/ralink_var.h>

/*
 * Configure all devices on system
 */     
void
cpu_configure(void)
{
	intr_init();

	/* Kick off autoconfiguration. */
	if (config_rootfound("mainbus", NULL) == NULL)
		panic("no mainbus found");

	/*
	 * Hardware interrupts will be enabled in
	 * sys/arch/mips/mips/mips3_clockintr.c:mips3_initclocks()
	 * to avoid hardclock(9) by CPU INT5 before softclockintr is
	 * initialized in initclocks().
	 */
}

void
cpu_rootconf(void)
{
	rootconf();
}

static const struct cfg_info {
	const char *map_name;
	uint32_t map_rst;
	uint32_t map_clkcfg1;
} map_info[] = {
#if defined(MT7620)
	{ "rpci", RST_PCIE0_7620, SYSCTL_CLKCFG1_PCIE_CLK_EN_7620 },
	{ "ohci", RST_UHST0_7620|RST_UHST, SYSCTL_CLKCFG1_UPHY0_CLK_EN_7620 },
	{ "ehci", RST_UHST0_7620|RST_UHST, SYSCTL_CLKCFG1_UPHY0_CLK_EN_7620 },
	{ "sdhc", RST_SDHC_7620, SYSCTL_CLKCFG1_SDHC_CLK_EN },
	{ "rsw", RST_ESW_7620, SYSCTL_CLKCFG1_ESW_CLK_EN },
#endif
#if defined(RT3883)
	{ "rpci", RST_PCI_3883 | RST_PCIPCIE_3883,
	    SYSCTL_CLKCFG1_PCI_CLK_EN|SYSCTL_CLKCFG1_PCIE_CLK_EN_3883 },
	{ "ohci", RST_UHST, SYSCTL_CLKCFG1_UPHY0_CLK_EN_3883 },
	{ "ehci", RST_UHST, SYSCTL_CLKCFG1_UPHY0_CLK_EN_3883 },
#endif
};

static void
ra_device_fixup(bus_space_tag_t bst, const struct cfg_info *map)
{
	const uint32_t clkcfg1 = bus_space_read_4(bst, ra_sysctl_bsh,
	    RA_SYSCTL_CLKCFG1);
	if ((clkcfg1 & map->map_clkcfg1) != map->map_clkcfg1) {
		bus_space_write_4(bst, ra_sysctl_bsh, RA_SYSCTL_CLKCFG1,
		    clkcfg1 | map->map_clkcfg1);
		delay(10000);
	}

	const uint32_t rst = bus_space_read_4(bst, ra_sysctl_bsh,
	    RA_SYSCTL_RST);
	if ((rst & map->map_rst) != 0) {
		bus_space_write_4(bst, ra_sysctl_bsh, RA_SYSCTL_RST,
		    rst & ~map->map_rst);
		delay(10000);
	}
}

void
device_register(device_t self, void *aux)
{
	device_t parent = device_parent(self);

	if (parent != NULL && device_is_a(parent, "mainbus")) {
		// If we are attaching a mainbus device, see if we know how
		// to bring it out of reset.
		struct mainbus_attach_args * const ma = aux;
		for (const struct cfg_info *map = map_info;
		     map < map_info + __arraycount(map_info);
		     map++) {
			if (device_is_a(self, map->map_name)) {
				ra_device_fixup(ma->ma_memt, map);
				delay(1000);
				break;
			}
		}

#if defined(RT3883) || defined(MT7620)
		if (device_is_a(self, "ohci") || device_is_a(self, "ehci")) {
			const uint32_t cfg1 = bus_space_read_4(ma->ma_memt,
			    ra_sysctl_bsh, RA_SYSCTL_CFG1);
			if ((cfg1 & SYSCTL_CFG1_USB0_HOST_MODE) == 0) {
				bus_space_write_4(ma->ma_memt, ra_sysctl_bsh,
				    RA_SYSCTL_CFG1,
				    cfg1 | SYSCTL_CFG1_USB0_HOST_MODE);
				delay(10);
			}
		}
#endif
	}
}
