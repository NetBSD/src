/* $NetBSD: exynos_clock.h,v 1.1.18.2 2017/12/03 11:35:56 jdolecek Exp $ */

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

#ifndef _ARM_EXYNOS_CLOCK_H
#define _ARM_EXYNOS_CLOCK_H

#include <dev/clk/clk.h>

enum exynos_clk_type {
	EXYNOS_CLK_FIXED,
	EXYNOS_CLK_PLL,
	EXYNOS_CLK_MUX,
	EXYNOS_CLK_DIV,
	EXYNOS_CLK_GATE
};

struct exynos_fixed_clk {
	u_int rate;
};

struct exynos_pll_clk {
	u_int lock_reg;
	u_int con0_reg;
};

struct exynos_mux_clk {
	const char **parents;
	u_int nparents;
	u_int reg;
	u_int bits;
};

struct exynos_div_clk {
	u_int reg;
	u_int bits;
};

struct exynos_gate_clk {
	u_int reg;
	u_int bits;
};

struct exynos_clk {
	struct clk base;		/* must be first */
	enum exynos_clk_type type;
	const char *alias;
	const char *parent;
	u_int refcnt;
	union {
		struct exynos_fixed_clk fixed;
		struct exynos_pll_clk pll;
		struct exynos_mux_clk mux;
		struct exynos_div_clk div;
		struct exynos_gate_clk gate;
	} u;
};

#endif /* _ARM_EXYNOS_CLOCK_H */
