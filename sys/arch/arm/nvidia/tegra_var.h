/* $NetBSD: tegra_var.h,v 1.42.4.1 2018/07/28 04:37:28 pgoyette Exp $ */

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

extern struct bus_space arm_generic_bs_tag;
extern struct bus_space arm_generic_a4x_bs_tag;
extern bus_space_handle_t tegra_ppsb_bsh;
extern bus_space_handle_t tegra_apb_bsh;

void	tegra_bootstrap(void);

struct tegra_gpio_pin;
struct tegra_gpio_pin *tegra_gpio_acquire(const char *, u_int);
void	tegra_gpio_release(struct tegra_gpio_pin *);
int	tegra_gpio_read(struct tegra_gpio_pin *);
void	tegra_gpio_write(struct tegra_gpio_pin *, int);

void	tegra_pmc_reset(void);
void	tegra_pmc_power(u_int, bool);
void	tegra_pmc_remove_clamping(u_int);
void	tegra_pmc_hdmi_enable(void);

void	tegra210_car_xusbio_enable_hw_control(void);
void	tegra210_car_xusbio_enable_hw_seq(void);

uint32_t tegra_fuse_read(u_int);

void	tegra_timer_delay(u_int);

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

#if defined(SOC_TEGRA124)
void	tegra124_mpinit(void);
#endif
#if defined(SOC_TEGRA210)
void	tegra210_mpinit(void);
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
