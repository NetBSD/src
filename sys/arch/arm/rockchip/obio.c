/*	$NetBSD: obio.c,v 1.1 2014/12/26 16:53:33 jmcneill Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: obio.c,v 1.1 2014/12/26 16:53:33 jmcneill Exp $");

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

	obio_init_grf();
	obio_init_gpio();

	/*
	 * Attach all on-board devices as described in the kernel
	 * configuration file.
	 */
	config_search_ia(obio_search, self, "obio", NULL);
}

int
obio_print(void *aux, const char *pnp)
{
	struct obio_attach_args *obio = aux;

	aprint_normal(": addr 0x%08lx", obio->obio_addr);
	aprint_normal("-0x%08lx", obio->obio_addr + (obio->obio_size - 1));
	if (obio->obio_width != OBIOCF_WIDTH_DEFAULT)
		aprint_normal(" width %d", obio->obio_width);
	if (obio->obio_intr != OBIOCF_INTR_DEFAULT)
		aprint_normal(" intr %d", obio->obio_intr);
	aprint_normal(" mult %d", obio->obio_mult);

	return UNCONF;
}

int
obio_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct obio_attach_args obio;

	obio.obio_addr = cf->cf_loc[OBIOCF_ADDR];
	obio.obio_size = cf->cf_loc[OBIOCF_SIZE];
	obio.obio_width = cf->cf_loc[OBIOCF_WIDTH];
	obio.obio_intr = cf->cf_loc[OBIOCF_INTR];
	obio.obio_mult = cf->cf_loc[OBIOCF_MULT];
	obio.obio_dmat = &rockchip_bus_dma_tag;


	switch (cf->cf_loc[OBIOCF_MULT]) {
	case 1:
		obio.obio_iot = &rockchip_bs_tag;
		break;
	case 4:
		obio.obio_iot = &rockchip_a4x_bs_tag;
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

#define GRF_GPIO3A_IOMUX_OFFSET	0x0090
#define GRF_GPIO3B_IOMUX_OFFSET	0x0094
#define GRF_GPIO3C_IOMUX_OFFSET	0x0098
#define GRF_GPIO3D_IOMUX_OFFSET	0x009C

void obio_init_grf(void)
{
	obio_iomux(GRF_GPIO3A_IOMUX_OFFSET, 0xffff5555);
	obio_iomux(GRF_GPIO3B_IOMUX_OFFSET, 0xffff0004);
	obio_iomux(GRF_GPIO3D_IOMUX_OFFSET, 0xffff1400);
}

void obio_iomux(int offset, int new)
{
	bus_space_handle_t bh;
	bus_space_tag_t bt = &rockchip_bs_tag;
	int old, renew;

	if (bus_space_map(bt, ROCKCHIP_GRF_BASE, ROCKCHIP_GRF_SIZE, 0, &bh))
		panic("GRF can not be mapped.");

	old = bus_space_read_4(bt, bh, offset);
	bus_space_write_4(bt, bh, offset, (old | new | 0xffff0000));
	renew = bus_space_read_4(bt, bh, offset);

	printf("grf iomux: old %08x, new %08x, renew %08x\n", old, new, renew);

	bus_space_unmap(bt, bh, ROCKCHIP_GRF_SIZE);
}

#define GPIO_SWPORTA_DR_OFFSET	0x00
#define GPIO_SWPORTA_DD_OFFSET	0x04

void obio_init_gpio(void)
{
	obio_swporta(ROCKCHIP_GPIO0_BASE, GPIO_SWPORTA_DR_OFFSET, __BIT(3));
	obio_swporta(ROCKCHIP_GPIO0_BASE, GPIO_SWPORTA_DD_OFFSET, __BIT(3));
}

void obio_swporta(int gpio_base, int offset, int new)
{
	bus_space_handle_t bh;
	bus_space_tag_t bt = &rockchip_bs_tag;
	int old, renew;
	int gpio_size = 0x100; /* XXX */

	if (bus_space_map(bt, gpio_base, gpio_size, 0, &bh))
		panic("gpio can not be mapped.");

	old = bus_space_read_4(bt, bh, offset);
	bus_space_write_4(bt, bh, offset, old | new);
	renew = bus_space_read_4(bt, bh, offset);

	printf("gpio: 0x%08x 0x%08x -> 0x%08x\n", gpio_base + offset, old, renew);

	bus_space_unmap(bt, bh, gpio_size);
}
