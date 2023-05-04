/*	$NetBSD: imx6_ccmvar.h,v 1.3 2023/05/04 13:25:07 bouyer Exp $	*/
/*
 * Copyright (c) 2012,2019  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_ARM_NXP_IMX6_CCMVAR_H_
#define	_ARM_NXP_IMX6_CCMVAR_H_

#include <dev/clk/clk.h>
#include <dev/clk/clk_backend.h>

struct imx6ccm_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_space_handle_t sc_ioh_analog;

	struct clk_domain sc_clkdom;
	struct imx6_clk *sc_imx6_clks;
	int sc_imx6_clksize;
};

struct imxccm_init_parent;

void imx6ccm_attach_common(device_t, struct imx6_clk *, int,
    struct imxccm_init_parent *);

struct clk *imx6_get_clock(struct imx6ccm_softc *, const char *);
struct imx6_clk *imx6_clk_find(struct imx6ccm_softc *sc, const char *);

struct imxccm_init_parent {
	const char *clock;
	const char *parent;
};


enum imx6_clk_type {
	IMX6_CLK_FIXED,
	IMX6_CLK_FIXED_FACTOR,
	IMX6_CLK_PLL,
	IMX6_CLK_MUX,
	IMX6_CLK_GATE,
	IMX6_CLK_PFD,
	IMX6_CLK_DIV,
};

enum imx6_clk_reg {
	IMX6_CLK_REG_CCM,
	IMX6_CLK_REG_CCM_ANALOG,
};

enum imx6_clk_pll_type {
	IMX6_CLK_PLL_GENERIC,
	IMX6_CLK_PLL_SYS,
	IMX6_CLK_PLL_USB,
	IMX6_CLK_PLL_AUDIO_VIDEO,
	IMX6_CLK_PLL_ENET,
};

enum imx6_clk_div_type {
	IMX6_CLK_DIV_NORMAL,
	IMX6_CLK_DIV_BUSY,
	IMX6_CLK_DIV_TABLE,
};

enum imx6_clk_mux_type {
	IMX6_CLK_MUX_NORMAL,
	IMX6_CLK_MUX_BUSY,
};

struct imx6_clk_fixed {
	u_int rate;
};

struct imx6_clk_fixed_factor {
	u_int div;
	u_int mult;
};

struct imx6_clk_pfd {
	uint32_t reg;
	int index;
};

struct imx6_clk_pll {
	enum imx6_clk_pll_type type;
	uint32_t reg;
	uint32_t mask;
	uint32_t powerdown;
	unsigned long ref;
};

struct imx6_clk_div {
	enum imx6_clk_div_type type;
	enum imx6_clk_reg base;
	uint32_t reg;
	uint32_t mask;
	uint32_t busy_reg;
	uint32_t busy_mask;
	const int *tbl;
};

struct imx6_clk_mux {
	enum imx6_clk_mux_type type;
	enum imx6_clk_reg base;
	uint32_t reg;
	uint32_t mask;
	const char **parents;
	u_int nparents;
	uint32_t busy_reg;
	uint32_t busy_mask;
};

struct imx6_clk_gate {
	enum imx6_clk_reg base;
	uint32_t reg;
	uint32_t mask;
	uint32_t exclusive_mask;
};

struct imx6_clk {
	struct clk base;		/* must be first */

	const char *parent;
	u_int refcnt;

	enum imx6_clk_type type;
	union {
		struct imx6_clk_fixed fixed;
		struct imx6_clk_fixed_factor fixed_factor;
		struct imx6_clk_pfd pfd;
		struct imx6_clk_pll pll;
		struct imx6_clk_div div;
		struct imx6_clk_mux mux;
		struct imx6_clk_gate gate;
	} clk;
};

#define CLK_FIXED(_name, _rate) {				\
	.base = { .name = (_name) },				\
	.type = IMX6_CLK_FIXED,					\
	.clk = {						\
		.fixed = {					\
			.rate = (_rate),			\
		}						\
	}							\
}

#define CLK_FIXED_FACTOR(_name, _parent, _div, _mult) {		\
	.base = { .name = (_name) },				\
	.type = IMX6_CLK_FIXED_FACTOR,				\
	.parent = (_parent),					\
	.clk = {						\
		.fixed_factor = {				\
			.div = (_div),				\
			.mult = (_mult),			\
		}						\
	}							\
}

#define CLK_PFD(_name, _parent, _reg, _index) {			\
	.base = { .name = (_name) },				\
	.type = IMX6_CLK_PFD,					\
	.parent = (_parent),					\
	.clk = {						\
		.pfd = {					\
			.reg = (CCM_ANALOG_##_reg),		\
			.index = (_index),			\
		}						\
	}							\
}

#define CLK_PLL(_name, _parent, _type, _reg, _mask, _powerdown, _ref) { \
	.base = { .name = (_name) },				\
	.type = IMX6_CLK_PLL,					\
	.parent = (_parent),					\
	.clk = {						\
		.pll = {					\
			.type = (IMX6_CLK_PLL_##_type),		\
			.reg = (CCM_ANALOG_##_reg),		\
			.mask = (CCM_ANALOG_##_reg##_##_mask),	\
			.powerdown = (CCM_ANALOG_##_reg##_##_powerdown), \
			.ref = (_ref),				\
		}						\
	}							\
}

#define CLK_DIV(_name, _parent, _reg, _mask) {			\
	.base = { .name = (_name) },				\
	.type = IMX6_CLK_DIV,					\
	.parent = (_parent),					\
	.clk = {						\
		.div = {					\
			.type = (IMX6_CLK_DIV_NORMAL),		\
			.base = (IMX6_CLK_REG_CCM),		\
			.reg = (CCM_##_reg),			\
			.mask = (CCM_##_reg##_##_mask),		\
		}						\
	}							\
}

#define CLK_DIV_BUSY(_name, _parent, _reg, _mask, _busy_reg, _busy_mask) { \
	.base = { .name = (_name) },				\
	.type = IMX6_CLK_DIV,					\
	.parent = (_parent),					\
	.clk = {						\
		.div = {					\
			.type = (IMX6_CLK_DIV_BUSY),		\
			.base = (IMX6_CLK_REG_CCM),		\
			.reg = (CCM_##_reg),			\
			.mask = (CCM_##_reg##_##_mask),	\
			.busy_reg = (CCM_##_busy_reg),		\
			.busy_mask = (CCM_##_busy_reg##_##_busy_mask) \
		}						\
	}							\
}

#define CLK_DIV_TABLE(_name, _parent, _reg, _mask, _tbl) {	\
	.base = { .name = (_name) },				\
	.type = IMX6_CLK_DIV,					\
	.parent = (_parent),					\
	.clk = {						\
		.div = {					\
			.type = (IMX6_CLK_DIV_TABLE),		\
			.base = (IMX6_CLK_REG_CCM_ANALOG),	\
			.reg = (CCM_ANALOG_##_reg),		\
			.mask = (CCM_ANALOG_##_reg##_##_mask),	\
			.tbl = (_tbl)				\
		}						\
	}							\
}

#define CLK_MUX(_name, _parents, _base, _reg, _mask) {		\
	.base = { .name = (_name), .flags = CLK_SET_RATE_PARENT }, \
	.type = IMX6_CLK_MUX,					\
	.clk = {						\
		.mux = {					\
			.type = (IMX6_CLK_MUX_NORMAL),		\
			.base = (IMX6_CLK_REG_##_base),		\
			.reg = (_base##_##_reg),		\
			.mask = (_base##_##_reg##_##_mask),	\
			.parents = (_parents),			\
			.nparents = __arraycount(_parents)	\
		}						\
	}							\
}

#define CLK_MUX_BUSY(_name, _parents, _reg, _mask, _busy_reg, _busy_mask) { \
	.base = { .name = (_name), .flags = CLK_SET_RATE_PARENT }, \
	.type = IMX6_CLK_MUX,					\
	.clk = {						\
		.mux = {					\
			.type = (IMX6_CLK_MUX_BUSY),		\
			.base = (IMX6_CLK_REG_CCM),		\
			.reg = (CCM_##_reg),			\
			.mask = (CCM_##_reg##_##_mask),		\
			.parents = (_parents),			\
			.nparents = __arraycount(_parents),	\
			.busy_reg = (CCM_##_busy_reg),		\
			.busy_mask = (CCM_##_busy_reg##_##_busy_mask) \
		}						\
	}							\
}

#define CLK_GATE(_name, _parent, _base, _reg, _mask) {		\
	.base = { .name = (_name), .flags = CLK_SET_RATE_PARENT }, \
	.type = IMX6_CLK_GATE,					\
	.parent = (_parent),					\
	.clk = {						\
		.gate = {					\
			.base = (IMX6_CLK_REG_##_base),		\
			.reg = (_base##_##_reg),		\
			.mask = (_base##_##_reg##_##_mask),	\
			.exclusive_mask = 0			\
		}						\
	}							\
}

#define CLK_GATE_EXCLUSIVE(_name, _parent, _base, _reg, _mask, _exclusive_mask) {  \
	.base = { .name = (_name), .flags = CLK_SET_RATE_PARENT }, \
	.type = IMX6_CLK_GATE,					\
	.parent = (_parent),					\
	.clk = {						\
		.gate = {					\
			.base = (IMX6_CLK_REG_##_base),		\
			.reg = (_base##_##_reg),		\
			.mask = (_base##_##_reg##_##_mask),     \
			.exclusive_mask = (_base##_##_reg##_##_exclusive_mask) \
		}						\
	}							\
}

#endif	/* _ARM_NXP_IMX6_CCMVAR_H_ */
