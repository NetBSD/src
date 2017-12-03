/* $NetBSD: rockchip_var.h,v 1.13.18.2 2017/12/03 11:35:55 jdolecek Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Hiroshi Tokuda
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

#ifndef _ARM_ROCKCHIP_ROCKCHIP_VAR_H_
#define _ARM_ROCKCHIP_ROCKCHIP_VAR_H_

#include "opt_rockchip.h"

#include <sys/types.h>
#include <sys/bus.h>

struct obio_attach_args {
	bus_space_tag_t	obio_bst;	/* bus space tag */
	bus_space_handle_t obio_bsh;	/* bus space handle */
	bus_space_handle_t obio_grf_bsh; /* GRF bus space handle */
	bus_addr_t	obio_base;	/* base address of handle */
	bus_addr_t	obio_offset;	/* address of device */
	bus_size_t	obio_size;	/* size of device */
	int		obio_intr;	/* irq */
	int		obio_width;	/* bus width */
	unsigned int	obio_mult;	/* multiplier */
	int		obio_port;	/* port */
	bus_dma_tag_t	obio_dmat;
	const char	*obio_name;
};

extern struct bus_space armv7_generic_bs_tag;
extern struct bus_space armv7_generic_a4x_bs_tag;
extern struct arm32_bus_dma_tag	rockchip_bus_dma_tag;
extern bus_space_handle_t rockchip_core0_bsh;
extern bus_space_handle_t rockchip_core1_bsh;

void rockchip_bootstrap(void);

void rockchip_cpufreq_init(void);

uint32_t rockchip_chip_id(void);
#define ROCKCHIP_CHIP_ID_RK3066		0x30660000
#define ROCKCHIP_CHIP_ID_RK3188		0x31880000
#define ROCKCHIP_CHIP_ID_RK3188PLUS	0x31880001
const char *rockchip_chip_name(void);

u_int rockchip_apll_get_rate(void);
u_int rockchip_cpll_get_rate(void);
u_int rockchip_dpll_get_rate(void);
u_int rockchip_gpll_get_rate(void);
u_int rockchip_cpu_get_rate(void);
u_int rockchip_ahb_get_rate(void);
u_int rockchip_apb_get_rate(void);
u_int rockchip_pclk_cpu_get_rate(void);
u_int rockchip_a9periph_get_rate(void);
u_int rockchip_mmc0_get_rate(void);
u_int rockchip_mmc0_set_div(u_int);
u_int rockchip_i2c_get_rate(u_int);
u_int rockchip_mac_get_rate(void);
u_int rockchip_mac_set_rate(u_int);

#endif /* _ARM_ROCKCHIP_ROCKCHIP_VAR_H_ */
