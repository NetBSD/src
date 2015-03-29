/* $NetBSD: tegra_var.h,v 1.2 2015/03/29 22:27:04 jmcneill Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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

#ifndef _ARM_TEGRA_VAR_H
#define _ARM_TEGRA_VAR_H

#include <sys/types.h>
#include <sys/bus.h>

#include "opt_tegra.h"

struct tegra_locators {
	const char *loc_name;
	bus_addr_t loc_offset;
	bus_size_t loc_size;
	int loc_port;
	int loc_intr;
#define TEGRAIO_INTR_DEFAULT	0
};

struct tegraio_attach_args {
	struct tegra_locators tio_loc;
	bus_space_tag_t tio_bst;
	bus_space_tag_t tio_a4x_bst;
	bus_space_handle_t tio_bsh;
	bus_dma_tag_t tio_dmat;
};

extern struct bus_space armv7_generic_bs_tag;
extern struct bus_space armv7_generic_a4x_bs_tag;
extern bus_space_handle_t tegra_host1x_bsh;
extern bus_space_handle_t tegra_apb_bsh;
extern bus_space_handle_t tegra_ahb_a2_bsh;
extern struct arm32_bus_dma_tag tegra_dma_tag;

#define CHIP_ID_TEGRA20		0x20
#define CHIP_ID_TEGRA30		0x30
#define CHIP_ID_TEGRA114	0x35
#define CHIP_ID_TEGRA124	0x40
#define CHIP_ID_TEGRA132	0x13

u_int	tegra_chip_id(void);
const char *tegra_chip_name(void);
void	tegra_bootstrap(void);

void	tegra_pmc_reset(void);

psize_t	tegra_mc_memsize(void);

#if defined(SOC_TEGRA124)
void	tegra124_mpinit(void);
#endif

static void inline
tegra_reg_set_clear(bus_space_tag_t bst, bus_space_handle_t bsh,
    bus_size_t o, uint32_t set_mask, uint32_t clr_mask)
{
	const uint32_t old = bus_space_read_4(bst, bsh, o);
	const uint32_t new = set_mask | (old & ~clr_mask);
	if (old != new) {
		bus_space_write_4(bst, bsh, o, new);
	}
}

#endif /* _ARM_TEGRA_VAR_H */
