/* $NetBSD: meson_pinctrl.h,v 1.3 2019/04/19 19:07:56 jmcneill Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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

#ifndef _MESON_PINCTRL_H
#define _MESON_PINCTRL_H

#include "opt_soc.h"

#include <sys/bus.h>

#define	MESON_PINCTRL_MAXBANK	8

enum meson_pinctrl_regtype {
	MESON_PINCTRL_REGTYPE_PULL,
	MESON_PINCTRL_REGTYPE_PULL_ENABLE,
	MESON_PINCTRL_REGTYPE_GPIO,
};

struct meson_pinctrl_gpioreg {
	enum meson_pinctrl_regtype type;
	bus_size_t reg;
	uint32_t mask;
};

struct meson_pinctrl_gpio {
	u_int id;
	const char *name;
	struct meson_pinctrl_gpioreg oen;
	struct meson_pinctrl_gpioreg out;
	struct meson_pinctrl_gpioreg in;
	struct meson_pinctrl_gpioreg pupden;
	struct meson_pinctrl_gpioreg pupd;
};

struct meson_pinctrl_group {
	const char *name;
	bus_size_t reg;
	u_int bit;
	u_int bank[MESON_PINCTRL_MAXBANK];
	u_int nbank;
};

struct meson_pinctrl_config {
	const char *name;
	const struct meson_pinctrl_group *groups;
	u_int ngroups;
	const struct meson_pinctrl_gpio *gpios;
	u_int ngpios;
};

#ifdef SOC_MESON8B
extern const struct meson_pinctrl_config meson8b_aobus_pinctrl_config;
extern const struct meson_pinctrl_config meson8b_cbus_pinctrl_config;
#endif

#ifdef SOC_MESONGXBB
extern const struct meson_pinctrl_config mesongxbb_aobus_pinctrl_config;
extern const struct meson_pinctrl_config mesongxbb_periphs_pinctrl_config;
#endif

#ifdef SOC_MESONGXL
extern const struct meson_pinctrl_config mesongxl_aobus_pinctrl_config;
extern const struct meson_pinctrl_config mesongxl_periphs_pinctrl_config;
#endif

#endif /* !_MESON_PINCTRL_H */
