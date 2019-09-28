/* $NetBSD: tegra_pinmux.h,v 1.1.10.1 2019/09/28 12:21:29 martin Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _ARM_TEGRA_PINMUX_H
#define	_ARM_TEGRA_PINMUX_H

#include "opt_tegra.h"

#define TEGRA_PINMUX_MAXFUNC	4

struct tegra_pinmux_pins {
	const char *tpp_name;
	u_int tpp_reg;
	enum tegra_pin_type {
		TEGRA_PINMUX,
		TEGRA_PADCTRL
	} tpp_type;
	union {
		const char *tpp_functions[TEGRA_PINMUX_MAXFUNC];
		struct {
			uint32_t	drvdn_mask;
			uint32_t	drvup_mask;
			uint32_t	slwrr_mask;
			uint32_t	slwrf_mask;
		} tpp_dg;
	};
};

struct tegra_pinmux_conf {
	uint32_t npins;
	const struct tegra_pinmux_pins *pins;
};

#ifdef SOC_TEGRA210
extern const struct tegra_pinmux_conf tegra210_pinmux_conf;
#endif

#endif /* _ARM_TEGRA_PINMUX_H */
