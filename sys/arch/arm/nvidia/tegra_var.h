/* $NetBSD: tegra_var.h,v 1.31 2017/04/14 00:19:34 jmcneill Exp $ */

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
#include <sys/gpio.h>

#include "opt_tegra.h"

extern struct bus_space armv7_generic_bs_tag;
extern struct bus_space armv7_generic_a4x_bs_tag;
extern bus_space_handle_t tegra_ppsb_bsh;
extern bus_space_handle_t tegra_apb_bsh;
extern struct arm32_bus_dma_tag tegra_dma_tag;

#define CHIP_ID_TEGRA20		0x20
#define CHIP_ID_TEGRA30		0x30
#define CHIP_ID_TEGRA114	0x35
#define CHIP_ID_TEGRA124	0x40
#define CHIP_ID_TEGRA132	0x13

u_int	tegra_chip_id(void);
const char *tegra_chip_name(void);
void	tegra_bootstrap(void);
void	tegra_dma_bootstrap(psize_t);
void	tegra_cpuinit(void);

struct tegra_gpio_pin;
struct tegra_gpio_pin *tegra_gpio_acquire(const char *, u_int);
void	tegra_gpio_release(struct tegra_gpio_pin *);
int	tegra_gpio_read(struct tegra_gpio_pin *);
void	tegra_gpio_write(struct tegra_gpio_pin *, int);

struct tegra_mpio_padctlgrp {
	int	preemp;
	int	hsm;
	int	schmt;
	int	drv_type;
	int	drvdn;
	int	drvup;
	int	slwr;
	int	slwf;
};
void	tegra_mpio_padctlgrp_read(u_int, struct tegra_mpio_padctlgrp *);
void	tegra_mpio_padctlgrp_write(u_int, const struct tegra_mpio_padctlgrp *);

void	tegra_mpio_pinmux_set_config(u_int, int, const char *);
void	tegra_mpio_pinmux_set_io_reset(u_int, bool);
void	tegra_mpio_pinmux_set_rcv_sel(u_int, bool);
void	tegra_mpio_pinmux_get_config(u_int, int *, const char **);
const char *tegra_mpio_pinmux_get_pm(u_int);
bool	tegra_mpio_pinmux_get_io_reset(u_int);
bool	tegra_mpio_pinmux_get_rcv_sel(u_int);

void	tegra_pmc_reset(void);
void	tegra_pmc_power(u_int, bool);
void	tegra_pmc_remove_clamping(u_int);
void	tegra_pmc_hdmi_enable(void);

psize_t	tegra_mc_memsize(void);

uint32_t tegra_fuse_read(u_int);

void	tegra_xusbpad_sata_enable(void);
void	tegra_xusbpad_xhci_enable(void);

struct videomode;
int	tegra_dc_port(device_t);
int	tegra_dc_enable(device_t, device_t, const struct videomode *,
			const uint8_t *);
void	tegra_dc_hdmi_start(device_t);

#define TEGRA_CPUFREQ_MAX	16
struct tegra_cpufreq_func {
	u_int (*set_rate)(u_int);
	u_int (*get_rate)(void);
	size_t (*get_available)(u_int *, size_t);
};
void	tegra_cpufreq_register(const struct tegra_cpufreq_func *);
void	tegra_cpufreq_init(void);

#if defined(SOC_TEGRA124)
void	tegra124_cpuinit(void);
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
