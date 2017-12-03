/* $NetBSD: amlogic_var.h,v 1.12.16.2 2017/12/03 11:35:51 jdolecek Exp $ */

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

#ifndef _ARM_AMLOGIC_VAR_H
#define _ARM_AMLOGIC_VAR_H

#include <sys/types.h>
#include <sys/bus.h>

struct amlogic_locators {
	const char *loc_name;
	bus_addr_t loc_offset;
	bus_size_t loc_size;
	int loc_port;
	int loc_intr;
#define AMLOGICIO_INTR_DEFAULT	0
};

struct amlogicio_attach_args {
	struct amlogic_locators aio_loc;
	bus_space_tag_t aio_core_bst;
	bus_space_tag_t aio_core_a4x_bst;
	bus_space_handle_t aio_bsh;
	bus_dma_tag_t aio_dmat;
};

extern struct bus_space armv7_generic_bs_tag;
extern struct bus_space armv7_generic_a4x_bs_tag;
extern bus_space_handle_t amlogic_core_bsh;
extern struct arm32_bus_dma_tag amlogic_dma_tag;

void	amlogic_bootstrap(void);

void	amlogic_cpufreq_bootstrap(void);
void	amlogic_cpufreq_init(void);

void	amlogic_wdog_init(void);
void	amlogic_usbphy_init(int);
void	amlogic_eth_init(void);
void	amlogic_rng_init(void);
void	amlogic_sdhc_init(void);
void	amlogic_sdio_init(void);

int	amlogic_sdhc_select_port(int);
void	amlogic_sdhc_reset_port(int);
bool	amlogic_sdhc_is_removable(int);
bool	amlogic_sdhc_is_card_present(int);
#define AMLOGIC_SDHC_PORT_A	0
#define AMLOGIC_SDHC_PORT_B	1
#define AMLOGIC_SDHC_PORT_C	2
void	amlogic_sdhc_set_voltage(int, int);
#define AMLOGIC_SDHC_VOL_330	0
#define AMLOGIC_SDHC_VOL_180	1

int	amlogic_sdio_select_port(int);
#define AMLOGIC_SDIO_PORT_A	0
#define AMLOGIC_SDIO_PORT_B	1
#define AMLOGIC_SDIO_PORT_C	2

uint32_t amlogic_get_rate_xtal(void);
uint32_t amlogic_get_rate_sys(void);
uint32_t amlogic_get_rate_fixed(void);
uint32_t amlogic_get_rate_a9(void);
uint32_t amlogic_get_rate_a9periph(void);
uint32_t amlogic_get_rate_clk81(void);

void	amlogic_genfb_ddb_trap_callback(int);
void	amlogic_genfb_set_console_dev(device_t);

static void inline
amlogic_reg_set_clear(bus_space_tag_t bst, bus_space_handle_t bsh,
    bus_size_t o, uint32_t set_mask, uint32_t clr_mask)
{
	const uint32_t old = bus_space_read_4(bst, bsh, o);
	const uint32_t new = set_mask | (old & ~clr_mask);
	if (old != new) {
		bus_space_write_4(bst, bsh, o, new);
	}
}

#endif /* _ARM_AMLOGIC_VAR_H */
