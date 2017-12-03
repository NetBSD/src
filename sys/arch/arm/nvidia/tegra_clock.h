/* $NetBSD: tegra_clock.h,v 1.1.18.2 2017/12/03 11:35:54 jdolecek Exp $ */

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

#ifndef _ARM_TEGRA_CLOCK_H
#define _ARM_TEGRA_CLOCK_H

enum tegra_clk_type {
	TEGRA_CLK_FIXED,
	TEGRA_CLK_PLL,
	TEGRA_CLK_MUX,
	TEGRA_CLK_FIXED_DIV,
	TEGRA_CLK_DIV,
	TEGRA_CLK_GATE
};

struct tegra_fixed_clk {
	u_int rate;
};

struct tegra_fixed_div_clk {
	u_int div;
};

struct tegra_pll_clk {
	u_int base_reg;
	u_int divm_mask;
	u_int divn_mask;
	u_int divp_mask;
};

struct tegra_mux_clk {
	const char **parents;
	u_int nparents;
	u_int reg;
	u_int bits;
};

struct tegra_div_clk {
	u_int reg;
	u_int bits;
};

struct tegra_gate_clk {
	u_int set_reg;
	u_int clr_reg;
	u_int bits;
};

struct tegra_clk {
	struct clk base;		/* must be first */
	u_int id;
	const char *parent;
	enum tegra_clk_type type;
	u_int refcnt;
	union {
		struct tegra_fixed_clk fixed;
		struct tegra_pll_clk pll;
		struct tegra_mux_clk mux;
		struct tegra_fixed_div_clk fixed_div;
		struct tegra_div_clk div;
		struct tegra_gate_clk gate;
	} u;
};

#define TEGRA_CLK_BASE(_tclk)	((_tclk) ? &(_tclk)->base : NULL)
#define TEGRA_CLK_PRIV(_clk)	((struct tegra_clk *)(_clk))

#endif /* _ARM_TEGRA_CLOCK_H */
