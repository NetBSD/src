/* $NetBSD: meson_clk.h,v 1.3 2019/02/25 19:30:17 jmcneill Exp $ */

/*-
 * Copyright (c) 2017-2019 Jared McNeill <jmcneill@invisible.ca>
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

#ifndef _ARM_MESON_CLK_H
#define _ARM_MESON_CLK_H

#include <dev/clk/clk_backend.h>
#include <dev/fdt/syscon.h>

struct meson_clk_softc;
struct meson_clk_clk;
struct meson_clk_reset;

/*
 * Resets
 */

struct meson_clk_reset {
	bus_size_t	reg;
	uint32_t	mask;
};

#define	MESON_CLK_RESET(_id, _reg, _bit)	\
	[_id] = {				\
		.reg = (_reg),			\
		.mask = __BIT(_bit),		\
	}

/*
 * Clocks
 */

enum meson_clk_clktype {
	MESON_CLK_UNKNOWN,
	MESON_CLK_FIXED,
	MESON_CLK_GATE,
	MESON_CLK_PLL,
	MESON_CLK_MPLL,
	MESON_CLK_DIV,
	MESON_CLK_FIXED_FACTOR,
	MESON_CLK_MUX,
};

/*
 * Fixed clocks
 */

struct meson_clk_fixed {
	u_int		rate;
};

u_int	meson_clk_fixed_get_rate(struct meson_clk_softc *, struct meson_clk_clk *);

#define	MESON_CLK_FIXED(_id, _name, _rate)			\
	[_id] = {						\
		.type = MESON_CLK_FIXED,			\
		.base.name = (_name),				\
		.base.flags = 0,				\
		.u.fixed.rate = (_rate),			\
		.get_rate = meson_clk_fixed_get_rate,		\
	}

/*
 * Gate clocks
 */

struct meson_clk_gate {
	bus_size_t	reg;
	uint32_t	mask;
	const char	*parent;
	uint32_t	flags;
#define	MESON_CLK_GATE_SET_TO_DISABLE		__BIT(0)
};

int	meson_clk_gate_enable(struct meson_clk_softc *,
			      struct meson_clk_clk *, int);
const char *meson_clk_gate_get_parent(struct meson_clk_softc *,
				      struct meson_clk_clk *);

#define	MESON_CLK_GATE_FLAGS(_id, _name, _pname, _reg, _bit, _flags)	\
	[_id] = {						\
		.type = MESON_CLK_GATE,				\
		.base.name = (_name),				\
		.base.flags = CLK_SET_RATE_PARENT,		\
		.u.gate.parent = (_pname),			\
		.u.gate.reg = (_reg),				\
		.u.gate.mask = __BIT(_bit),			\
		.u.gate.flags = (_flags),			\
		.enable = meson_clk_gate_enable,		\
		.get_parent = meson_clk_gate_get_parent,	\
	}

#define	MESON_CLK_GATE(_id, _name, _pname, _reg, _bit)		\
	MESON_CLK_GATE_FLAGS(_id, _name, _pname, _reg, _bit, 0)

/*
 * Divider clocks
 */

struct meson_clk_div {
	bus_size_t	reg;
	const char	*parent;
	uint32_t	div;
	uint32_t	flags;
#define	MESON_CLK_DIV_POWER_OF_TWO	__BIT(0)
#define	MESON_CLK_DIV_SET_RATE_PARENT	__BIT(1)
#define	MESON_CLK_DIV_CPU_SCALE_TABLE	__BIT(2)
};

u_int	meson_clk_div_get_rate(struct meson_clk_softc *,
			       struct meson_clk_clk *);
int	meson_clk_div_set_rate(struct meson_clk_softc *,
			       struct meson_clk_clk *, u_int);
const char *meson_clk_div_get_parent(struct meson_clk_softc *,
				     struct meson_clk_clk *);

#define	MESON_CLK_DIV(_id, _name, _parent, _reg, _div, _flags)	\
	[_id] = {						\
		.type = MESON_CLK_DIV,				\
		.base.name = (_name),				\
		.u.div.reg = (_reg),				\
		.u.div.parent = (_parent),			\
		.u.div.div = (_div),				\
		.u.div.flags = (_flags),			\
		.get_rate = meson_clk_div_get_rate,		\
		.set_rate = meson_clk_div_set_rate,		\
		.get_parent = meson_clk_div_get_parent,		\
	}

/*
 * Fixed-factor clocks
 */

struct meson_clk_fixed_factor {
	const char	*parent;
	u_int		div;
	u_int		mult;
};

u_int	meson_clk_fixed_factor_get_rate(struct meson_clk_softc *,
					struct meson_clk_clk *);
int	meson_clk_fixed_factor_set_rate(struct meson_clk_softc *,
					struct meson_clk_clk *, u_int);
const char *meson_clk_fixed_factor_get_parent(struct meson_clk_softc *,
					      struct meson_clk_clk *);

#define	MESON_CLK_FIXED_FACTOR(_id, _name, _parent, _div, _mult)	\
	[_id] = {							\
		.type = MESON_CLK_FIXED_FACTOR,				\
		.base.name = (_name),					\
		.u.fixed_factor.parent = (_parent),			\
		.u.fixed_factor.div = (_div),				\
		.u.fixed_factor.mult = (_mult),				\
		.get_rate = meson_clk_fixed_factor_get_rate,		\
		.get_parent = meson_clk_fixed_factor_get_parent,	\
		.set_rate = meson_clk_fixed_factor_set_rate,		\
	}

/*
 * Mux clocks
 */

struct meson_clk_mux {
	bus_size_t	reg;
	const char	**parents;
	u_int		nparents;
	uint32_t	sel;
	uint32_t	flags;
};

const char *meson_clk_mux_get_parent(struct meson_clk_softc *,
			       struct meson_clk_clk *);

#define	MESON_CLK_MUX(_id, _name, _parents, _reg, _sel, _flags)		\
	[_id] = {							\
		.type = MESON_CLK_MUX,					\
		.base.name = (_name),					\
		.base.flags = CLK_SET_RATE_PARENT,			\
		.u.mux.parents = (_parents),				\
		.u.mux.nparents = __arraycount(_parents),		\
		.u.mux.reg = (_reg),					\
		.u.mux.sel = (_sel),					\
		.u.mux.flags = (_flags),				\
		.get_parent = meson_clk_mux_get_parent,			\
	}

/*
 * PLL clocks
 */

struct meson_clk_pll_reg {
	bus_size_t	reg;
	uint32_t	mask;
};

#define	MESON_CLK_PLL_REG(_reg, _mask)					\
	{ .reg = (_reg), .mask = (_mask) }
#define	MESON_CLK_PLL_REG_INVALID	MESON_CLK_PLL_REG(0,0)

struct meson_clk_pll {
	struct meson_clk_pll_reg	enable;
	struct meson_clk_pll_reg	m;
	struct meson_clk_pll_reg	n;
	struct meson_clk_pll_reg	frac;
	struct meson_clk_pll_reg	l;
	struct meson_clk_pll_reg	reset;
	const char			*parent;
	uint32_t			flags;
};

u_int	meson_clk_pll_get_rate(struct meson_clk_softc *,
			       struct meson_clk_clk *);
const char *meson_clk_pll_get_parent(struct meson_clk_softc *,
				     struct meson_clk_clk *);

#define	MESON_CLK_PLL_RATE(_id, _name, _parent, _enable, _m, _n, _frac, _l,	\
		      _reset, _setratefn, _flags)			\
	[_id] = {							\
		.type = MESON_CLK_PLL,					\
		.base.name = (_name),					\
		.u.pll.parent = (_parent),				\
		.u.pll.enable = _enable,				\
		.u.pll.m = _m,						\
		.u.pll.n = _n,						\
		.u.pll.frac = _frac,					\
		.u.pll.l = _l,						\
		.u.pll.reset = _reset,					\
		.u.pll.flags = (_flags),				\
		.set_rate = (_setratefn),				\
		.get_rate = meson_clk_pll_get_rate,			\
		.get_parent = meson_clk_pll_get_parent,			\
	}

#define	MESON_CLK_PLL(_id, _name, _parent, _enable, _m, _n, _frac, _l,	\
		      _reset, _flags)					\
	[_id] = {							\
		.type = MESON_CLK_PLL,					\
		.base.name = (_name),					\
		.u.pll.parent = (_parent),				\
		.u.pll.enable = _enable,				\
		.u.pll.m = _m,						\
		.u.pll.n = _n,						\
		.u.pll.frac = _frac,					\
		.u.pll.l = _l,						\
		.u.pll.reset = _reset,					\
		.u.pll.flags = (_flags),				\
		.get_rate = meson_clk_pll_get_rate,			\
		.get_parent = meson_clk_pll_get_parent,			\
	}

/*
 * MPLL clocks
 */

struct meson_clk_mpll {
	struct meson_clk_pll_reg	sdm;
	struct meson_clk_pll_reg	sdm_enable;
	struct meson_clk_pll_reg	n2;
	struct meson_clk_pll_reg	ssen;
	const char			*parent;
	uint32_t			flags;
};

u_int	meson_clk_mpll_get_rate(struct meson_clk_softc *,
				struct meson_clk_clk *);
const char *meson_clk_mpll_get_parent(struct meson_clk_softc *,
				      struct meson_clk_clk *);

#define	MESON_CLK_MPLL(_id, _name, _parent, _sdm, _sdm_enable, _n2,	\
		       _ssen, _flags)					\
	[_id] = {							\
		.type = MESON_CLK_MPLL,					\
		.base.name = (_name),					\
		.u.mpll.parent = (_parent),				\
		.u.mpll.sdm = _sdm,					\
		.u.mpll.sdm_enable = _sdm_enable,			\
		.u.mpll.n2 = _n2,					\
		.u.mpll.ssen = _ssen,					\
		.u.mpll.flags = (_flags),				\
		.get_rate = meson_clk_mpll_get_rate,			\
		.get_parent = meson_clk_mpll_get_parent,		\
	}



struct meson_clk_clk {
	struct clk	base;
	enum meson_clk_clktype type;
	union {
		struct meson_clk_fixed fixed;
		struct meson_clk_gate gate;
		struct meson_clk_div div;
		struct meson_clk_fixed_factor fixed_factor;
		struct meson_clk_mux mux;
		struct meson_clk_pll pll;
		struct meson_clk_mpll mpll;
	} u;

	int		(*enable)(struct meson_clk_softc *,
				  struct meson_clk_clk *, int);
	u_int		(*get_rate)(struct meson_clk_softc *,
				    struct meson_clk_clk *);
	int		(*set_rate)(struct meson_clk_softc *,
				    struct meson_clk_clk *, u_int);
	u_int		(*round_rate)(struct meson_clk_softc *,
				    struct meson_clk_clk *, u_int);
	const char *	(*get_parent)(struct meson_clk_softc *,
				      struct meson_clk_clk *);
	int		(*set_parent)(struct meson_clk_softc *,
				      struct meson_clk_clk *,
				      const char *);
};

struct meson_clk_softc {
	device_t		sc_dev;
	int			sc_phandle;

	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct syscon		*sc_syscon;

	struct clk_domain	sc_clkdom;

	struct meson_clk_reset *sc_resets;
	u_int			sc_nresets;

	struct meson_clk_clk	*sc_clks;
	u_int			sc_nclks;
};

void	meson_clk_attach(struct meson_clk_softc *);
struct meson_clk_clk *meson_clk_clock_find(struct meson_clk_softc *,
					   const char *);
void	meson_clk_print(struct meson_clk_softc *);

void	meson_clk_lock(struct meson_clk_softc *);
void	meson_clk_unlock(struct meson_clk_softc *);
uint32_t meson_clk_read(struct meson_clk_softc *, bus_size_t);
void	meson_clk_write(struct meson_clk_softc *, bus_size_t, uint32_t);

#define	CLK_LOCK	meson_clk_lock
#define	CLK_UNLOCK	meson_clk_unlock
#define	CLK_READ	meson_clk_read
#define	CLK_WRITE	meson_clk_write

#endif /* _ARM_MESON_CLK_H */
