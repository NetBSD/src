/* $NetBSD: sunxi_gpio.h,v 1.8 2018/02/18 10:28:37 jmcneill Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
 * Copyright (c) 2016 Emmanuel Vadot <manu@bidouilliste.com>
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
 *
 * $FreeBSD$
 */

#ifndef _ARM_SUNXI_GPIO_H
#define	_ARM_SUNXI_GPIO_H

#include "opt_soc.h"

#define SUNXI_GPIO_MAXFUNC	8

struct sunxi_gpio_pins {
	const char *name;
	uint8_t port;
	uint8_t pin;
	const char *functions[SUNXI_GPIO_MAXFUNC];
	uint8_t eint_func;
	uint8_t eint_num;
	uint8_t eint_bank;
};

struct sunxi_gpio_padconf {
	uint32_t npins;
	const struct sunxi_gpio_pins *pins;
};

#ifdef SOC_SUN4I_A10
extern const struct sunxi_gpio_padconf sun4i_a10_padconf;
#endif

#ifdef SOC_SUN5I_A13
extern const struct sunxi_gpio_padconf sun5i_a13_padconf;
#endif

#ifdef SOC_SUN6I_A31
extern const struct sunxi_gpio_padconf sun6i_a31_padconf;
extern const struct sunxi_gpio_padconf sun6i_a31_r_padconf;
#endif

#ifdef SOC_SUN7I_A20
extern const struct sunxi_gpio_padconf sun7i_a20_padconf;
#endif

#ifdef SOC_SUN8I_A83T
extern const struct sunxi_gpio_padconf sun8i_a83t_padconf;
extern const struct sunxi_gpio_padconf sun8i_a83t_r_padconf;
#endif

#ifdef SOC_SUN8I_H3
extern const struct sunxi_gpio_padconf sun8i_h3_padconf;
extern const struct sunxi_gpio_padconf sun8i_h3_r_padconf;
#endif

#ifdef SOC_SUN9I_A80
extern const struct sunxi_gpio_padconf sun9i_a80_padconf;
extern const struct sunxi_gpio_padconf sun9i_a80_r_padconf;
#endif

#ifdef SOC_SUN50I_A64
extern const struct sunxi_gpio_padconf sun50i_a64_padconf;
extern const struct sunxi_gpio_padconf sun50i_a64_r_padconf;
#endif

#ifdef SOC_SUN50I_H6
extern const struct sunxi_gpio_padconf sun50i_h6_padconf;
extern const struct sunxi_gpio_padconf sun50i_h6_r_padconf;
#endif

#endif /* _ARM_SUNXI_GPIO_H */
