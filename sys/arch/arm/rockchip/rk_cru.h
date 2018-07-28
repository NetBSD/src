/* $NetBSD: rk_cru.h,v 1.1.2.3 2018/07/28 04:37:29 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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

#ifndef _ARM_RK_CRU_H
#define _ARM_RK_CRU_H

#include <dev/clk/clk_backend.h>
#include <dev/fdt/syscon.h>

struct rk_cru_softc;
struct rk_cru_clk;

/*
 * Clocks
 */

enum rk_cru_clktype {
	RK_CRU_UNKNOWN,
	RK_CRU_PLL,
	RK_CRU_ARM,
	RK_CRU_COMPOSITE,
	RK_CRU_GATE,
	RK_CRU_MUX,
};

/* PLL clocks */

struct rk_cru_pll_rate {
	u_int		rate;
	u_int		refdiv;
	u_int		fbdiv;
	u_int		postdiv1;
	u_int		postdiv2;
	u_int		dsmpd;
	u_int		fracdiv;
};

#define	RK_PLL_RATE(_rate, _refdiv, _fbdiv, _postdiv1, _postdiv2, _dsmpd, _fracdiv) \
	{							\
		.rate = (_rate),				\
		.refdiv = (_refdiv),				\
		.fbdiv = (_fbdiv),				\
		.postdiv1 = (_postdiv1),			\
		.postdiv2 = (_postdiv2),			\
		.dsmpd = (_dsmpd),				\
		.fracdiv = (_fracdiv),				\
	}

struct rk_cru_pll {
	bus_size_t	con_base;
	bus_size_t	mode_reg;
	uint32_t	mode_mask;
	uint32_t	lock_mask;
	const struct rk_cru_pll_rate *rates;
	u_int		nrates;
	const char	*parent;
};

u_int	rk_cru_pll_get_rate(struct rk_cru_softc *, struct rk_cru_clk *);
int	rk_cru_pll_set_rate(struct rk_cru_softc *, struct rk_cru_clk *, u_int);
const char *rk_cru_pll_get_parent(struct rk_cru_softc *, struct rk_cru_clk *);

#define	RK_PLL(_id, _name, _parent, _con_base, _mode_reg, _mode_mask, _lock_mask, _rates) \
	{							\
		.id = (_id),					\
		.type = RK_CRU_PLL,				\
		.base.name = (_name),				\
		.base.flags = 0,				\
		.u.pll.parent = (_parent),			\
		.u.pll.con_base = (_con_base),			\
		.u.pll.mode_reg = (_mode_reg),			\
		.u.pll.mode_mask = (_mode_mask),		\
		.u.pll.lock_mask = (_lock_mask),		\
		.u.pll.rates = (_rates),			\
		.u.pll.nrates = __arraycount(_rates),		\
		.get_rate = rk_cru_pll_get_rate,		\
		.set_rate = rk_cru_pll_set_rate,		\
		.get_parent = rk_cru_pll_get_parent,		\
	}

/* ARM clocks */

struct rk_cru_arm_rate {
	u_int		rate;
	u_int		div;
};

#define	RK_ARM_RATE(_rate, _div)				\
	{							\
		.rate = (_rate),				\
		.div = (_div),					\
	}

struct rk_cru_arm {
	bus_size_t	reg;
	uint32_t	mux_mask;
	u_int		mux_main;
	u_int		mux_alt;
	uint32_t	div_mask;
	const char	**parents;
	u_int		nparents;
	const struct rk_cru_arm_rate *rates;
	u_int		nrates;
};

u_int	rk_cru_arm_get_rate(struct rk_cru_softc *, struct rk_cru_clk *);
int	rk_cru_arm_set_rate(struct rk_cru_softc *, struct rk_cru_clk *, u_int);
const char *rk_cru_arm_get_parent(struct rk_cru_softc *, struct rk_cru_clk *);
int	rk_cru_arm_set_parent(struct rk_cru_softc *, struct rk_cru_clk *, const char *);

#define	RK_ARM(_id, _name, _parents, _reg, _mux_mask, _mux_main, _mux_alt, _div_mask, _rates) \
	{							\
		.id = (_id),					\
		.type = RK_CRU_ARM,				\
		.base.name = (_name),				\
		.base.flags = 0,				\
		.u.arm.parents = (_parents),			\
		.u.arm.nparents = __arraycount(_parents),	\
		.u.arm.reg = (_reg),				\
		.u.arm.mux_mask = (_mux_mask),			\
		.u.arm.mux_main = (_mux_main),			\
		.u.arm.mux_alt = (_mux_alt),			\
		.u.arm.div_mask = (_div_mask),			\
		.u.arm.rates = (_rates),			\
		.u.arm.nrates = __arraycount(_rates),		\
		.get_rate = rk_cru_arm_get_rate,		\
		.set_rate = rk_cru_arm_set_rate,		\
		.get_parent = rk_cru_arm_get_parent,		\
		.set_parent = rk_cru_arm_set_parent,		\
	}

/* Composite clocks */

struct rk_cru_composite {
	bus_size_t	muxdiv_reg;
	uint32_t	mux_mask;
	uint32_t	div_mask;
	bus_size_t	gate_reg;
	uint32_t	gate_mask;
	const char	**parents;
	u_int		nparents;
	u_int		flags;
#define	RK_COMPOSITE_ROUND_DOWN		0x01
};

int	rk_cru_composite_enable(struct rk_cru_softc *, struct rk_cru_clk *, int);
u_int	rk_cru_composite_get_rate(struct rk_cru_softc *, struct rk_cru_clk *);
int	rk_cru_composite_set_rate(struct rk_cru_softc *, struct rk_cru_clk *, u_int);
const char *rk_cru_composite_get_parent(struct rk_cru_softc *, struct rk_cru_clk *);
int	rk_cru_composite_set_parent(struct rk_cru_softc *, struct rk_cru_clk *, const char *);

#define	RK_COMPOSITE(_id, _name, _parents, _muxdiv_reg, _mux_mask, _div_mask, _gate_reg, _gate_mask, _flags) \
	{							\
		.id = (_id),					\
		.type = RK_CRU_COMPOSITE,			\
		.base.name = (_name),				\
		.base.flags = 0,				\
		.u.composite.parents = (_parents),		\
		.u.composite.nparents = __arraycount(_parents),	\
		.u.composite.muxdiv_reg = (_muxdiv_reg),	\
		.u.composite.mux_mask = (_mux_mask),		\
		.u.composite.div_mask = (_div_mask),		\
		.u.composite.gate_reg = (_gate_reg),		\
		.u.composite.gate_mask = (_gate_mask),		\
		.u.composite.flags = (_flags),			\
		.enable = rk_cru_composite_enable,		\
		.get_rate = rk_cru_composite_get_rate,		\
		.set_rate = rk_cru_composite_set_rate,		\
		.get_parent = rk_cru_composite_get_parent,	\
		.set_parent = rk_cru_composite_set_parent,	\
	}

/* Gate clocks */

struct rk_cru_gate {
	bus_size_t	reg;
	uint32_t	mask;
	const char	*parent;
};

int	rk_cru_gate_enable(struct rk_cru_softc *,
			   struct rk_cru_clk *, int);
const char *rk_cru_gate_get_parent(struct rk_cru_softc *,
				   struct rk_cru_clk *);

#define	RK_GATE(_id, _name, _pname, _reg, _bit)			\
	{							\
		.id = (_id),					\
		.type = RK_CRU_GATE,				\
		.base.name = (_name),				\
		.base.flags = CLK_SET_RATE_PARENT,		\
		.u.gate.parent = (_pname),			\
		.u.gate.reg = (_reg),				\
		.u.gate.mask = __BIT(_bit),			\
		.enable = rk_cru_gate_enable,			\
		.get_parent = rk_cru_gate_get_parent,		\
	}

/* Mux clocks */

struct rk_cru_mux {
	bus_size_t	reg;
	uint32_t	mask;
	const char	**parents;
	u_int		nparents;
	u_int		flags;
#define	RK_MUX_GRF			0x01
};

const char *rk_cru_mux_get_parent(struct rk_cru_softc *, struct rk_cru_clk *);
int	rk_cru_mux_set_parent(struct rk_cru_softc *, struct rk_cru_clk *, const char *);

#define	RK_MUX_FLAGS(_id, _name, _parents, _reg, _mask, _flags)	\
	{							\
		.id = (_id),					\
		.type = RK_CRU_MUX,				\
		.base.name = (_name),				\
		.base.flags = CLK_SET_RATE_PARENT,		\
		.u.mux.parents = (_parents),			\
		.u.mux.nparents = __arraycount(_parents),	\
		.u.mux.reg = (_reg),				\
		.u.mux.mask = (_mask),				\
		.u.mux.flags = (_flags),			\
		.set_parent = rk_cru_mux_set_parent,		\
		.get_parent = rk_cru_mux_get_parent,		\
	}
#define	RK_MUX(_id, _name, _parents, _reg, _mask)		\
	RK_MUX_FLAGS(_id, _name, _parents, _reg, _mask, 0)
#define	RK_MUXGRF(_id, _name, _parents, _reg, _mask)		\
	RK_MUX_FLAGS(_id, _name, _parents, _reg, _mask, RK_MUX_GRF)

/*
 * Rockchip clock definition
 */

struct rk_cru_clk {
	struct clk	base;
	u_int		id;
	enum rk_cru_clktype type;
	union {
		struct rk_cru_pll pll;
		struct rk_cru_arm arm;
		struct rk_cru_composite composite;
		struct rk_cru_gate gate;
		struct rk_cru_mux mux;
	} u;

	int		(*enable)(struct rk_cru_softc *,
				  struct rk_cru_clk *, int);
	u_int		(*get_rate)(struct rk_cru_softc *,
				    struct rk_cru_clk *);
	int		(*set_rate)(struct rk_cru_softc *,
				    struct rk_cru_clk *, u_int);
	u_int		(*round_rate)(struct rk_cru_softc *,
				    struct rk_cru_clk *, u_int);
	const char *	(*get_parent)(struct rk_cru_softc *,
				      struct rk_cru_clk *);
	int		(*set_parent)(struct rk_cru_softc *,
				      struct rk_cru_clk *,
				      const char *);
};

/*
 * Driver state
 */

struct rk_cru_softc {
	device_t		sc_dev;
	int			sc_phandle;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	struct syscon		*sc_grf;

	struct clk_domain	sc_clkdom;

	struct rk_cru_clk	*sc_clks;
	u_int			sc_nclks;
};

int	rk_cru_attach(struct rk_cru_softc *);
struct rk_cru_clk *rk_cru_clock_find(struct rk_cru_softc *,
				     const char *);
void	rk_cru_print(struct rk_cru_softc *);

#define CRU_READ(sc, reg)	\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define CRU_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

#define	HAS_GRF(sc)	((sc)->sc_grf != NULL)

#endif /* _ARM_RK_CRU_H */
