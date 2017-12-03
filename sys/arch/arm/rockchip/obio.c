/*	$NetBSD: obio.c,v 1.19.18.2 2017/12/03 11:35:55 jdolecek Exp $	*/

/*
 * Copyright (c) 2001, 2002, 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_rockchip.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: obio.c,v 1.19.18.2 2017/12/03 11:35:55 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <sys/bus.h>

#include <arm/mainbus/mainbus.h>
#include <arm/rockchip/rockchip_reg.h>
#include <arm/rockchip/rockchip_var.h>


#include "locators.h"

int	obio_match(device_t, cfdata_t, void *);
void	obio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(obio, 0,
    obio_match, obio_attach, NULL, NULL);

int	obio_print(void *, const char *);
int	obio_search(device_t, cfdata_t, const int *, void *);

static void	obio_init_rk3066(void);
static void	obio_init_rk3188(void);
static void	obio_grf_set(uint32_t, uint32_t);
static int	obio_gpio_set_out(u_int, u_int, u_int);

#ifdef ROCKCHIP_CLOCK_DEBUG
static void	obio_dump_clocks(void);
#endif

/* there can be only one */
bool	obio_found;

int
obio_match(device_t parent, cfdata_t cf, void *aux)
{

	if (obio_found)
		return 0;
	return 1;
}

void
obio_attach(device_t parent, device_t self, void *aux)
{
	obio_found = true;

	aprint_naive("\n");
	aprint_normal(": %s\n", rockchip_chip_name());

#ifdef ROCKCHIP_CLOCK_DEBUG
	obio_dump_clocks();
#endif

	switch (rockchip_chip_id()) {
	case ROCKCHIP_CHIP_ID_RK3066:
		obio_init_rk3066();
		break;
	case ROCKCHIP_CHIP_ID_RK3188:
	case ROCKCHIP_CHIP_ID_RK3188PLUS:
		obio_init_rk3188();
		break;
	default:
		break;
	}

	/*
	 * Attach all on-board devices as described in the kernel
	 * configuration file. Attach devices marked "crit 1" first.
	 */
	for (int crit = 1; crit >= 0; crit--) {
		config_search_ia(obio_search, self, "obio", &crit);
	}
}

int
obio_print(void *aux, const char *pnp)
{
	struct obio_attach_args *obio = aux;
	bus_addr_t addr = obio->obio_base + obio->obio_offset;

	aprint_normal(": addr 0x%08lx", addr);
	aprint_normal("-0x%08lx", addr + (obio->obio_size - 1));
	if (obio->obio_width != OBIOCF_WIDTH_DEFAULT)
		aprint_normal(" width %d", obio->obio_width);
	if (obio->obio_intr != OBIOCF_INTR_DEFAULT)
		aprint_normal(" intr %d", obio->obio_intr);
	if (obio->obio_mult != OBIOCF_MULT_DEFAULT)
		aprint_normal(" mult %d", obio->obio_mult);
	if (obio->obio_port != OBIOCF_PORT_DEFAULT)
		aprint_normal(" port %d", obio->obio_port);

	return UNCONF;
}

int
obio_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct obio_attach_args obio;
	bus_addr_t addr = cf->cf_loc[OBIOCF_ADDR];
	int crit = *(int *)aux;

	if (cf->cf_loc[OBIOCF_CRIT] != crit)
		return 0;

	if (addr >= ROCKCHIP_CORE0_BASE &&
	    addr < ROCKCHIP_CORE0_BASE + ROCKCHIP_CORE0_SIZE) {
		obio.obio_base = ROCKCHIP_CORE0_BASE;
		obio.obio_bsh = rockchip_core0_bsh;
	} else if (addr >= ROCKCHIP_CORE1_BASE &&
	    addr < ROCKCHIP_CORE1_BASE + ROCKCHIP_CORE1_SIZE) {
		obio.obio_base = ROCKCHIP_CORE1_BASE;
		obio.obio_bsh = rockchip_core1_bsh;
	} else {
		panic("addr %#llx is not in CORE0 or CORE1 space",
		    (long long unsigned int)addr);
	}
	obio.obio_offset = addr - obio.obio_base;
	obio.obio_size = cf->cf_loc[OBIOCF_SIZE];
	obio.obio_width = cf->cf_loc[OBIOCF_WIDTH];
	obio.obio_intr = cf->cf_loc[OBIOCF_INTR];
	obio.obio_mult = cf->cf_loc[OBIOCF_MULT];
	obio.obio_port = cf->cf_loc[OBIOCF_PORT];
	obio.obio_dmat = &rockchip_bus_dma_tag;

	bus_space_subregion(&armv7_generic_bs_tag, rockchip_core1_bsh,
	    ROCKCHIP_GRF_OFFSET, ROCKCHIP_GRF_SIZE, &obio.obio_grf_bsh);

	switch (cf->cf_loc[OBIOCF_MULT]) {
	case 1:
		obio.obio_bst = &armv7_generic_bs_tag;
		break;
	case 4:
		obio.obio_bst = &armv7_generic_a4x_bs_tag;
		break;
	default:
		panic("Unsupported EMIFS multiplier.");
		break;
	}
	if (config_match(parent, cf, &obio) > 0) {
		config_attach(parent, cf, &obio, obio_print);
	}
	return 0;
}

#define GRF_GPIO0A_IOMUX_OFFSET	0x00a8
#define GRF_GPIO0B_IOMUX_OFFSET	0x00ac
#define GRF_GPIO0C_IOMUX_OFFSET	0x00b0
#define GRF_GPIO0D_IOMUX_OFFSET	0x00b4

#define GRF_GPIO1A_IOMUX_OFFSET	0x00b8
#define GRF_GPIO1B_IOMUX_OFFSET	0x00bc
#define GRF_GPIO1C_IOMUX_OFFSET	0x00c0
#define GRF_GPIO1D_IOMUX_OFFSET	0x00c4

#define GRF_GPIO2A_IOMUX_OFFSET	0x00c8
#define GRF_GPIO2B_IOMUX_OFFSET	0x00cc
#define GRF_GPIO2C_IOMUX_OFFSET	0x00d0
#define GRF_GPIO2D_IOMUX_OFFSET	0x00d4

#define GRF_GPIO3A_IOMUX_OFFSET	0x00d8
#define GRF_GPIO3B_IOMUX_OFFSET	0x00dc
#define GRF_GPIO3C_IOMUX_OFFSET	0x00e0
#define GRF_GPIO3D_IOMUX_OFFSET	0x00e4

#define GRF_GPIO4A_IOMUX_OFFSET	0x00e8
#define GRF_GPIO4B_IOMUX_OFFSET	0x00ec
#define GRF_GPIO4C_IOMUX_OFFSET	0x00f0
#define GRF_GPIO4D_IOMUX_OFFSET	0x00f4

#define GRF_GPIO6B_IOMUX_OFFSET	0x010c

#define GRF_SOC_CON0_OFFSET	0x0150
#define GRF_SOC_CON1_OFFSET	0x0154
#define GRF_SOC_CON2_OFFSET	0x0158

static void
obio_init_rk3066(void)
{
	/* dwctwo[01] */
	obio_grf_set(GRF_GPIO0A_IOMUX_OFFSET, 0x14001400);

	/* com2 */
	obio_grf_set(GRF_GPIO1B_IOMUX_OFFSET, 0x00050005);

	/* rkemac0 */
	obio_grf_set(GRF_GPIO1C_IOMUX_OFFSET, 0xffffaaaa);
	obio_grf_set(GRF_GPIO1D_IOMUX_OFFSET, 0x000f000a);
	obio_grf_set(GRF_SOC_CON1_OFFSET,     0x00030002);
	obio_grf_set(GRF_SOC_CON2_OFFSET,     0x00400040);

	/* rkiic[01234] */
	obio_grf_set(GRF_GPIO2D_IOMUX_OFFSET, 0x55005500);
	obio_grf_set(GRF_GPIO3A_IOMUX_OFFSET, 0x05550555);
	obio_grf_set(GRF_SOC_CON1_OFFSET,     0xf800f800);

	/* dwcmmc0 */
	obio_grf_set(GRF_GPIO3A_IOMUX_OFFSET, 0x50004000);
	obio_grf_set(GRF_GPIO3B_IOMUX_OFFSET, 0x55551555);

	/* ChipSPARK Rayeager PX2: dwcmmc2 */
	obio_grf_set(GRF_GPIO3D_IOMUX_OFFSET, 0xc0008000);
	obio_grf_set(GRF_GPIO4B_IOMUX_OFFSET, 0x003c0008);
	obio_grf_set(GRF_SOC_CON0_OFFSET,     0x08000800);

	/* ChipSPARK Rayeager PX2: umass0 */
	obio_grf_set(GRF_GPIO0B_IOMUX_OFFSET, 0x04000000);
	obio_grf_set(GRF_GPIO4C_IOMUX_OFFSET, 0x30000000);
	obio_gpio_set_out(0, 13, 1);
	obio_gpio_set_out(4, 22, 1);

	/* ChipSPARK Rayeager PX2: uhub2 */
	obio_grf_set(GRF_GPIO1D_IOMUX_OFFSET, 0x40000000);
	obio_gpio_set_out(1, 31, 1);

	/* ChipSPARK Rayeager PX2: ukphy0 */
	/* rtl8201f: clock must be configured before resetting */
	rockchip_mac_set_rate(50000000);
	obio_grf_set(GRF_GPIO1D_IOMUX_OFFSET, 0x10000000);
	obio_gpio_set_out(1, 30, 1);
	/* rtl8201f: need 1s delay here? */
}

#define RK3188_GRF_GPIO0C_IOMUX_OFFSET	0x0068
#define RK3188_GRF_GPIO0D_IOMUX_OFFSET	0x006c

#define RK3188_GRF_GPIO1A_IOMUX_OFFSET	0x0070
#define RK3188_GRF_GPIO1B_IOMUX_OFFSET	0x0074
#define RK3188_GRF_GPIO1C_IOMUX_OFFSET	0x0078
#define RK3188_GRF_GPIO1D_IOMUX_OFFSET	0x007c

#define RK3188_GRF_GPIO2A_IOMUX_OFFSET	0x0080
#define RK3188_GRF_GPIO2B_IOMUX_OFFSET	0x0084
#define RK3188_GRF_GPIO2C_IOMUX_OFFSET	0x0088
#define RK3188_GRF_GPIO2D_IOMUX_OFFSET	0x008c

#define RK3188_GRF_GPIO3A_IOMUX_OFFSET	0x0090
#define RK3188_GRF_GPIO3B_IOMUX_OFFSET	0x0094
#define RK3188_GRF_GPIO3C_IOMUX_OFFSET	0x0098
#define RK3188_GRF_GPIO3D_IOMUX_OFFSET	0x009c

#define RK3188_GRF_SOC_CON0_OFFSET	0x00a0
#define RK3188_GRF_SOC_CON1_OFFSET	0x00a4
#define RK3188_GRF_SOC_CON2_OFFSET	0x00a8

#define RK3188_GRF_IO_CON2_OFFSET	0x00fc
#define RK3188_GRF_IO_CON3_OFFSET	0x0100

static void
obio_init_rk3188(void)
{
	/* com2 */
	obio_grf_set(RK3188_GRF_GPIO1B_IOMUX_OFFSET, 0x000f0005);

	/* rkiic[01234] */
	obio_grf_set(RK3188_GRF_GPIO1D_IOMUX_OFFSET, 0x55555555);
	obio_grf_set(RK3188_GRF_GPIO3B_IOMUX_OFFSET, 0xa000a000);
	obio_grf_set(RK3188_GRF_SOC_CON1_OFFSET,     0xf800f800);

	/* dwcmmc0 */
	obio_grf_set(RK3188_GRF_GPIO3A_IOMUX_OFFSET, 0x55555554);
	obio_grf_set(RK3188_GRF_GPIO3B_IOMUX_OFFSET, 0x00050001);
	//obio_grf_set(RK3188_GRF_IO_CON2_OFFSET,      0x00c000c0);

	/* rkemac0 */
	obio_grf_set(RK3188_GRF_GPIO3C_IOMUX_OFFSET, 0xffffaaaa);
	obio_grf_set(RK3188_GRF_GPIO3D_IOMUX_OFFSET, 0x000f000a);
	obio_grf_set(RK3188_GRF_SOC_CON1_OFFSET,     0x00030002);
	obio_grf_set(RK3188_GRF_SOC_CON2_OFFSET,     0x00400040);
	obio_grf_set(RK3188_GRF_IO_CON3_OFFSET,      0x000f000f);

	/* dwcotg[01] */
	obio_grf_set(RK3188_GRF_GPIO3D_IOMUX_OFFSET, 0x3c003c00);

	/* Radxa Rock: dwcotg[01] */
	//obio_grf_set(RK3188_GRF_GPIO3D_IOMUX_OFFSET, 0x3c000000);
	obio_grf_set(RK3188_GRF_GPIO2D_IOMUX_OFFSET, 0x40000000);
	obio_gpio_set_out(0, 3, 1);
	obio_gpio_set_out(2, 31, 1);

	/* Radxa Rock: IT66121 HDMI */
	obio_gpio_set_out(3, 10, 1);

	/* Radxa Rock: ukphy0 */
	obio_gpio_set_out(3, 26, 1); /* XXX: RMII_INT, input, active low */

	/* Minix Neo X7: cdce0 */
	obio_gpio_set_out(0, 30, 1);
}

static void
obio_grf_set(uint32_t offset, uint32_t value)
{
	bus_space_handle_t bh;
	bus_space_tag_t bt = &armv7_generic_bs_tag;
	uint32_t old, new;

	bus_space_subregion(bt, rockchip_core1_bsh, ROCKCHIP_GRF_OFFSET,
	    ROCKCHIP_GRF_SIZE, &bh);

	old = bus_space_read_4(bt, bh, offset);
	bus_space_write_4(bt, bh, offset, value);
	new = bus_space_read_4(bt, bh, offset);

	printf("grf: %04x: 0x%08x 0x%08x -> 0x%08x\n", offset, old, value, new);
}

#define GPIO_SWPORTA_DR_OFFSET	0x00
#define GPIO_SWPORTA_DDR_OFFSET	0x04

static int
obio_gpio_set_out(u_int unit, u_int pin, u_int value)
{
	bus_space_handle_t bh;
	bus_space_tag_t bt = &armv7_generic_bs_tag;
	uint32_t gpio_base = 0, gpio_size = 0;
	uint32_t old, new;

	switch (rockchip_chip_id()) {
	case ROCKCHIP_CHIP_ID_RK3066:
		switch (unit) {
		case 0:
			gpio_base = ROCKCHIP_GPIO0_OFFSET;
			gpio_size = ROCKCHIP_GPIO0_SIZE;
			break;
		case 1:
			gpio_base = ROCKCHIP_GPIO1_OFFSET;
			gpio_size = ROCKCHIP_GPIO1_SIZE;
			break;
		case 2:
			gpio_base = ROCKCHIP_GPIO2_OFFSET;
			gpio_size = ROCKCHIP_GPIO2_SIZE;
			break;
		case 3:
			gpio_base = ROCKCHIP_GPIO3_OFFSET;
			gpio_size = ROCKCHIP_GPIO3_SIZE;
			break;
		case 4:
			gpio_base = ROCKCHIP_GPIO4_OFFSET;
			gpio_size = ROCKCHIP_GPIO4_SIZE;
			break;
		case 6:
			gpio_base = ROCKCHIP_GPIO6_OFFSET;
			gpio_size = ROCKCHIP_GPIO6_SIZE;
			break;
		}
		break;
	case ROCKCHIP_CHIP_ID_RK3188:
	case ROCKCHIP_CHIP_ID_RK3188PLUS:
		switch (unit) {
		case 0:
			gpio_base = ROCKCHIP_RK3188_GPIO0_OFFSET;
			gpio_size = ROCKCHIP_RK3188_GPIO0_SIZE;
			break;
		case 1:
			gpio_base = ROCKCHIP_GPIO1_OFFSET;
			gpio_size = ROCKCHIP_GPIO1_SIZE;
			break;
		case 2:
			gpio_base = ROCKCHIP_GPIO2_OFFSET;
			gpio_size = ROCKCHIP_GPIO2_SIZE;
			break;
		case 3:
			gpio_base = ROCKCHIP_GPIO3_OFFSET;
			gpio_size = ROCKCHIP_GPIO3_SIZE;
			break;
		}
		break;
	}

	if (gpio_base == 0 || gpio_size == 0 || pin > 31)
		return EINVAL;

	bus_space_subregion(bt, rockchip_core1_bsh, gpio_base, gpio_size, &bh);

	old = bus_space_read_4(bt, bh, GPIO_SWPORTA_DR_OFFSET);
	if (value)
		new = old | __BIT(pin);
	else
		new = old & ~__BIT(pin);
	bus_space_write_4(bt, bh, GPIO_SWPORTA_DR_OFFSET, new);
	new = bus_space_read_4(bt, bh, GPIO_SWPORTA_DR_OFFSET);
	printf("gpio%d: dr 0x%08x -> 0x%08x\n", unit, old, new);

	old = bus_space_read_4(bt, bh, GPIO_SWPORTA_DDR_OFFSET);
	bus_space_write_4(bt, bh, GPIO_SWPORTA_DDR_OFFSET, old | __BIT(pin));
	new = bus_space_read_4(bt, bh, GPIO_SWPORTA_DDR_OFFSET);
	printf("gpio%d: ddr 0x%08x -> 0x%08x\n", unit, old, new);

	return 0;
}

#ifdef ROCKCHIP_CLOCK_DEBUG
static void
obio_dump_clocks(void)
{
	printf("APLL: %u Hz\n", rockchip_apll_get_rate());
	printf("CPLL: %u Hz\n", rockchip_cpll_get_rate());
	printf("DPLL: %u Hz\n", rockchip_dpll_get_rate());
	printf("GPLL: %u Hz\n", rockchip_gpll_get_rate());
	printf("CPU: %u Hz\n", rockchip_cpu_get_rate());
	printf("AHB: %u Hz\n", rockchip_ahb_get_rate());
	printf("APB: %u Hz\n", rockchip_apb_get_rate());
	printf("PCLK_CPU: %u Hz\n", rockchip_pclk_cpu_get_rate());
	printf("A9PERIPH: %u Hz\n", rockchip_a9periph_get_rate());
}
#endif
