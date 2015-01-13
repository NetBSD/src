/*	$NetBSD: obio.c,v 1.16 2015/01/13 10:37:38 jmcneill Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: obio.c,v 1.16 2015/01/13 10:37:38 jmcneill Exp $");

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

void	obio_init_grf(void);
void	obio_iomux(int, int);
void	obio_init_gpio(void);
void	obio_swporta(int, int, int);

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
	aprint_normal(": On-board I/O\n");

#ifdef ROCKCHIP_CLOCK_DEBUG
	obio_dump_clocks();
#endif

	obio_init_grf();
	obio_init_gpio();

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

	bus_space_subregion(&rockchip_bs_tag, rockchip_core1_bsh,
	    ROCKCHIP_GRF_OFFSET, ROCKCHIP_GRF_SIZE, &obio.obio_grf_bsh);

	switch (cf->cf_loc[OBIOCF_MULT]) {
	case 1:
		obio.obio_bst = &rockchip_bs_tag;
		break;
	case 4:
		obio.obio_bst = &rockchip_a4x_bs_tag;
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

#define RK3188_GRF_GPIO0C_IOMUX_OFFSET	0x0068
#define RK3188_GRF_GPIO0D_IOMUX_OFFSET	0x006C

#define RK3188_GRF_GPIO1A_IOMUX_OFFSET	0x0070
#define RK3188_GRF_GPIO1B_IOMUX_OFFSET	0x0074
#define RK3188_GRF_GPIO1C_IOMUX_OFFSET	0x0078
#define RK3188_GRF_GPIO1D_IOMUX_OFFSET	0x007C

#define RK3188_GRF_GPIO2A_IOMUX_OFFSET	0x0080
#define RK3188_GRF_GPIO2B_IOMUX_OFFSET	0x0084
#define RK3188_GRF_GPIO2C_IOMUX_OFFSET	0x0088
#define RK3188_GRF_GPIO2D_IOMUX_OFFSET	0x008C

#define RK3188_GRF_GPIO3A_IOMUX_OFFSET	0x0090
#define RK3188_GRF_GPIO3B_IOMUX_OFFSET	0x0094
#define RK3188_GRF_GPIO3C_IOMUX_OFFSET	0x0098
#define RK3188_GRF_GPIO3D_IOMUX_OFFSET	0x009C

#define RK3188_GRF_SOC_CON0_OFFSET	0x00A0
#define RK3188_GRF_SOC_CON1_OFFSET	0x00A4
#define RK3188_GRF_SOC_CON2_OFFSET	0x00A8
#define RK3188_GRF_SOC_STATUS_OFFSET	0x00AC

#define RK3188_GRF_IO_CON2_OFFSET	0x00FC
#define RK3188_GRF_IO_CON3_OFFSET	0x0100

#define GRF_GPIO0A_IOMUX_OFFSET	0x00a8
#define GRF_GPIO3A_IOMUX_OFFSET	0x00d8
#define GRF_GPIO3B_IOMUX_OFFSET	0x00dc

void obio_init_grf(void)
{
#if 1
	/* Radxa Rock */
	obio_iomux(RK3188_GRF_GPIO3A_IOMUX_OFFSET, 0x55555554); /* MMC0 */
	obio_iomux(RK3188_GRF_GPIO3B_IOMUX_OFFSET, 0x00050001); /* MMC0 */
	obio_iomux(RK3188_GRF_IO_CON2_OFFSET,      0x00c000c0); /* MMC0 */
	obio_iomux(RK3188_GRF_GPIO3D_IOMUX_OFFSET, 0x3c000000); /* VBUS */
	obio_iomux(RK3188_GRF_GPIO1D_IOMUX_OFFSET, 0x55555555); /* I2C[0124] */
	obio_iomux(RK3188_GRF_GPIO3B_IOMUX_OFFSET, 0xa000a000); /* I2C3 */
	obio_iomux(RK3188_GRF_SOC_CON1_OFFSET,	   0xf800f800);	/* I2C[01234] */

//	obio_iomux(RK3188_GRF_GPIO0C_IOMUX_OFFSET, 0x00030000); /* PHY */
	obio_iomux(RK3188_GRF_GPIO3C_IOMUX_OFFSET, 0xffffaaaa); /* PHY */
	obio_iomux(RK3188_GRF_GPIO3D_IOMUX_OFFSET, 0x003f000a); /* PHY */
	obio_iomux(RK3188_GRF_SOC_CON1_OFFSET,     0x00030002); /* VMAC */
	obio_iomux(RK3188_GRF_IO_CON3_OFFSET,      0x000f000f); /* VMAC */
#else
	/* ChipSPARK Rayeager PX2 */
	obio_iomux(GRF_GPIO0A_IOMUX_OFFSET, 0x14000000); /* VBUS */
	obio_iomux(GRF_GPIO3A_IOMUX_OFFSET, 0x50004000); /* MMC0 */
	obio_iomux(GRF_GPIO3B_IOMUX_OFFSET, 0x55551555); /* MMC0 */
#endif
}

void obio_iomux(int offset, int new)
{
	bus_space_handle_t bh;
	bus_space_tag_t bt = &rockchip_bs_tag;
	int old, renew;

	bus_space_subregion(bt, rockchip_core1_bsh, ROCKCHIP_GRF_OFFSET,
	    ROCKCHIP_GRF_SIZE, &bh);

	old = bus_space_read_4(bt, bh, offset);
	bus_space_write_4(bt, bh, offset, new);
	renew = bus_space_read_4(bt, bh, offset);

	printf("grf iomux: old %08x, new %08x, renew %08x\n", old, new, renew);
}

#define GPIO_SWPORTA_DR_OFFSET	0x00
#define GPIO_SWPORTA_DD_OFFSET	0x04

void obio_init_gpio(void)
{
#if 1
	/* Radxa Rock */
	obio_swporta(ROCKCHIP_GPIO0_OFFSET, GPIO_SWPORTA_DR_OFFSET, __BIT(3));
	obio_swporta(ROCKCHIP_GPIO0_OFFSET, GPIO_SWPORTA_DD_OFFSET, __BIT(3));
	obio_swporta(ROCKCHIP_GPIO2_OFFSET, GPIO_SWPORTA_DR_OFFSET, __BIT(31));
	obio_swporta(ROCKCHIP_GPIO2_OFFSET, GPIO_SWPORTA_DD_OFFSET, __BIT(31));

	/* PHY */
	obio_swporta(ROCKCHIP_GPIO3_OFFSET, GPIO_SWPORTA_DR_OFFSET, __BIT(26));
	obio_swporta(ROCKCHIP_GPIO3_OFFSET, GPIO_SWPORTA_DD_OFFSET, __BIT(26));

	/* Minix Neo X7 USB ethernet */
	obio_swporta(ROCKCHIP_GPIO0_OFFSET, GPIO_SWPORTA_DR_OFFSET, __BIT(30));
	obio_swporta(ROCKCHIP_GPIO0_OFFSET, GPIO_SWPORTA_DD_OFFSET, __BIT(30));

	/* IT66121 HDMI */
	obio_swporta(ROCKCHIP_GPIO3_OFFSET, GPIO_SWPORTA_DR_OFFSET, __BIT(10));
	obio_swporta(ROCKCHIP_GPIO3_OFFSET, GPIO_SWPORTA_DD_OFFSET, __BIT(10));
#else
	/* ChipSPARK Rayeager PX2 */
	obio_swporta(ROCKCHIP_GPIO0_OFFSET, GPIO_SWPORTA_DR_OFFSET, __BIT(5));
	obio_swporta(ROCKCHIP_GPIO0_OFFSET, GPIO_SWPORTA_DD_OFFSET, __BIT(5));
	obio_swporta(ROCKCHIP_GPIO0_OFFSET, GPIO_SWPORTA_DR_OFFSET, __BIT(6));
	obio_swporta(ROCKCHIP_GPIO0_OFFSET, GPIO_SWPORTA_DD_OFFSET, __BIT(6));
	obio_swporta(ROCKCHIP_GPIO1_OFFSET, GPIO_SWPORTA_DR_OFFSET, __BIT(31));
	obio_swporta(ROCKCHIP_GPIO1_OFFSET, GPIO_SWPORTA_DD_OFFSET, __BIT(31));
#endif
}

void obio_swporta(int gpio_base, int offset, int new)
{
	bus_space_handle_t bh;
	bus_space_tag_t bt = &rockchip_bs_tag;
	int old, renew;
	int gpio_size = 0x100; /* XXX */

	bus_space_subregion(bt, rockchip_core1_bsh, gpio_base, gpio_size, &bh);

	old = bus_space_read_4(bt, bh, offset);
	bus_space_write_4(bt, bh, offset, old | new);
	renew = bus_space_read_4(bt, bh, offset);

	printf("gpio: 0x%08x 0x%08x -> 0x%08x\n", gpio_base + offset, old, renew);
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
