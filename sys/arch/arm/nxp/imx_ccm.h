/* $NetBSD: imx_ccm.h,v 1.1 2020/12/23 14:42:38 skrll Exp $ */

/*-
 * Copyright (c) 2020 Jared McNeill <jmcneill@invisible.ca>
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

#ifndef _ARM_IMX_CCM_H
#define _ARM_IMX_CCM_H

#include <dev/clk/clk_backend.h>
#include <dev/fdt/syscon.h>

struct imx_ccm_softc;
struct imx_ccm_clk;

/*
 * Clocks
 */

enum imx_ccm_clktype {
	IMX_CCM_UNKNOWN,
	IMX_CCM_EXTCLK,
	IMX_CCM_GATE,
	IMX_CCM_COMPOSITE,
	IMX_CCM_PLL,
	IMX_CCM_FIXED,
	IMX_CCM_FIXED_FACTOR,
	IMX_CCM_MUX,
	IMX_CCM_DIV,
};

/* External clocks */

int	imx_ccm_extclk_enable(struct imx_ccm_softc *, struct imx_ccm_clk *, int);
u_int	imx_ccm_extclk_get_rate(struct imx_ccm_softc *, struct imx_ccm_clk *);
int	imx_ccm_extclk_set_rate(struct imx_ccm_softc *, struct imx_ccm_clk *, u_int);
const char *imx_ccm_extclk_get_parent(struct imx_ccm_softc *, struct imx_ccm_clk *);

#define	IMX_EXTCLK(_id, _name)					\
	{							\
		.id = (_id),					\
		.type = IMX_CCM_EXTCLK,				\
		.base.name = (_name),				\
		.base.flags = 0,				\
		.u.extclk = (_name),				\
		.enable = imx_ccm_extclk_enable,		\
		.get_rate = imx_ccm_extclk_get_rate,		\
		.set_rate = imx_ccm_extclk_set_rate,		\
		.get_parent = imx_ccm_extclk_get_parent,	\
	}

/* Gate clocks */

struct imx_ccm_gate {
	bus_size_t	reg;
	uint32_t	mask;
	const char	*parent;
};

int	imx_ccm_gate_enable(struct imx_ccm_softc *,
			   struct imx_ccm_clk *, int);
const char *imx_ccm_gate_get_parent(struct imx_ccm_softc *,
				   struct imx_ccm_clk *);

#define	IMX_GATE(_id, _name, _pname, _reg, _mask) \
	IMX_GATE_INDEX(_id, 0, _name, _pname, _reg, _mask)
#define	IMX_GATE_INDEX(_id, _regidx, _name, _pname, _reg, _mask) \
	{							\
		.id = (_id),					\
		.regidx = (_regidx),				\
		.type = IMX_CCM_GATE,				\
		.base.name = (_name),				\
		.base.flags = CLK_SET_RATE_PARENT,		\
		.u.gate.parent = (_pname),			\
		.u.gate.reg = (_reg),				\
		.u.gate.mask = (_mask),				\
		.enable = imx_ccm_gate_enable,			\
		.get_parent = imx_ccm_gate_get_parent,		\
	}
#define	IMX_ROOT_GATE(_id, _name, _pname, _reg)			\
	IMX_ROOT_GATE_INDEX(_id, 0, _name, _pname, _reg)
#define	IMX_ROOT_GATE_INDEX(_id, _regidx, _name, _pname, _reg)	\
	IMX_GATE_INDEX(_id, _regidx, _name, _pname, _reg, __BITS(1,0))

/* Composite clocks */

struct imx_ccm_composite {
	bus_size_t	reg;
	const char	**parents;
	u_int		nparents;
	u_int		flags;
#define	IMX_COMPOSITE_ROUND_DOWN	0x01
#define	IMX_COMPOSITE_SET_RATE_PARENT	0x02
};

int	imx_ccm_composite_enable(struct imx_ccm_softc *, struct imx_ccm_clk *, int);
u_int	imx_ccm_composite_get_rate(struct imx_ccm_softc *, struct imx_ccm_clk *);
int	imx_ccm_composite_set_rate(struct imx_ccm_softc *, struct imx_ccm_clk *, u_int);
const char *imx_ccm_composite_get_parent(struct imx_ccm_softc *, struct imx_ccm_clk *);
int	imx_ccm_composite_set_parent(struct imx_ccm_softc *, struct imx_ccm_clk *, const char *);

#define	IMX_COMPOSITE(_id, _name, _parents, _reg, _flags)	\
	IMX_COMPOSITE_INDEX(_id, 0, _name, _parents, _reg, _flags)

#define	IMX_COMPOSITE_INDEX(_id, _regidx, _name, _parents, _reg, _flags) \
	{							\
		.id = (_id),					\
		.regidx = (_regidx),				\
		.type = IMX_CCM_COMPOSITE,			\
		.base.name = (_name),				\
		.base.flags = 0,				\
		.u.composite.parents = (_parents),		\
		.u.composite.nparents = __arraycount(_parents),	\
		.u.composite.reg = (_reg),			\
		.u.composite.flags = (_flags),			\
		.enable = imx_ccm_composite_enable,		\
		.get_rate = imx_ccm_composite_get_rate,		\
		.set_rate = imx_ccm_composite_set_rate,		\
		.set_parent = imx_ccm_composite_set_parent,	\
		.get_parent = imx_ccm_composite_get_parent,	\
	}

/* PLLs */

struct imx_ccm_pll {
	bus_size_t	reg;
	const char	*parent;
	uint32_t	div_mask;
	u_int		flags;
#define	IMX_PLL_ARM		__BIT(0)
#define	IMX_PLL_480M_528M	__BIT(1)
#define	IMX_PLL_ENET		__BIT(2)
};

int	imx_ccm_pll_enable(struct imx_ccm_softc *, struct imx_ccm_clk *, int);
u_int	imx_ccm_pll_get_rate(struct imx_ccm_softc *, struct imx_ccm_clk *);
const char *imx_ccm_pll_get_parent(struct imx_ccm_softc *, struct imx_ccm_clk *);

#define	IMX_PLL(_id, _name, _parent, _reg, _div_mask, _flags)	\
	IMX_PLL_INDEX(_id, 0, _name, _parent, _reg, _div_mask, _flags)
#define	IMX_PLL_INDEX(_id, _regidx, _name, _parent, _reg, _div_mask, _flags) \
	{							\
		.id = (_id),					\
		.regidx = (_regidx),				\
		.type = IMX_CCM_PLL,				\
		.base.name = (_name),				\
		.base.flags = 0,				\
		.u.pll.parent = (_parent),			\
		.u.pll.reg = (_reg),				\
		.u.pll.div_mask = (_div_mask),			\
		.u.pll.flags = (_flags),			\
		.enable = imx_ccm_pll_enable,			\
		.get_rate = imx_ccm_pll_get_rate,		\
		.get_parent = imx_ccm_pll_get_parent,		\
	}

/* Fixed clocks */

struct imx_ccm_fixed {
	u_int		rate;
};

u_int	imx_ccm_fixed_get_rate(struct imx_ccm_softc *, struct imx_ccm_clk *);

#define	IMX_FIXED(_id, _name, _rate)				\
	{							\
		.id = (_id),					\
		.type = IMX_CCM_FIXED,				\
		.base.name = (_name),				\
		.base.flags = 0,				\
		.u.fixed.rate = (_rate),			\
		.get_rate = imx_ccm_fixed_get_rate,		\
	}

/* Fixed factor clocks */

struct imx_ccm_fixed_factor {
	const char	*parent;
	u_int		mult;
	u_int		div;
};

u_int	imx_ccm_fixed_factor_get_rate(struct imx_ccm_softc *, struct imx_ccm_clk *);
int	imx_ccm_fixed_factor_set_rate(struct imx_ccm_softc *, struct imx_ccm_clk *, u_int);
const char *imx_ccm_fixed_factor_get_parent(struct imx_ccm_softc *, struct imx_ccm_clk *);

#define	IMX_FIXED_FACTOR(_id, _name, _parent, _mult, _div)	\
	{							\
		.id = (_id),					\
		.type = IMX_CCM_FIXED_FACTOR,			\
		.base.name = (_name),				\
		.base.flags = 0,				\
		.u.fixed_factor.parent = (_parent),		\
		.u.fixed_factor.mult = (_mult),			\
		.u.fixed_factor.div = (_div),			\
		.get_rate = imx_ccm_fixed_factor_get_rate,	\
		.set_rate = imx_ccm_fixed_factor_set_rate,	\
		.get_parent = imx_ccm_fixed_factor_get_parent,	\
	}

/* Mux clocks */

struct imx_ccm_mux {
	bus_size_t	reg;
	const char	**parents;
	u_int		nparents;
	uint32_t	sel;
};

const char *imx_ccm_mux_get_parent(struct imx_ccm_softc *, struct imx_ccm_clk *);
int	imx_ccm_mux_set_parent(struct imx_ccm_softc *, struct imx_ccm_clk *, const char *);

#define	IMX_MUX(_id, _name, _parents, _reg, _sel)			\
	IMX_MUX_INDEX(_id, 0, _name, _parents, _reg, _sel)

#define	IMX_MUX_INDEX(_id, _regidx, _name, _parents, _reg, _sel)	\
	{								\
		.id = (_id),						\
		.regidx = (_regidx),					\
		.type = IMX_CCM_MUX,					\
		.base.name = (_name),					\
		.base.flags = CLK_SET_RATE_PARENT,			\
		.u.mux.parents = (_parents),				\
		.u.mux.nparents = __arraycount(_parents),		\
		.u.mux.reg = (_reg),					\
		.u.mux.sel = (_sel),					\
		.get_parent = imx_ccm_mux_get_parent,			\
		.set_parent = imx_ccm_mux_set_parent,			\
	}

/* Divider clocks */

struct imx_ccm_div {
	bus_size_t	reg;
	const char	*parent;
	uint32_t	mask;
	u_int		flags;
#define	IMX_DIV_SET_RATE_PARENT		__BIT(0)
#define	IMX_DIV_ROUND_DOWN		__BIT(1)
};

u_int	imx_ccm_div_get_rate(struct imx_ccm_softc *, struct imx_ccm_clk *);
int	imx_ccm_div_set_rate(struct imx_ccm_softc *, struct imx_ccm_clk *, u_int);
const char *imx_ccm_div_get_parent(struct imx_ccm_softc *, struct imx_ccm_clk *);

#define	IMX_DIV(_id, _name, _parent, _reg, _mask, _flags)		\
	IMX_DIV_INDEX(_id, 0, _name, _parent, _reg, _mask, _flags)
#define	IMX_DIV_INDEX(_id, _regidx, _name, _parent, _reg, _mask, _flags) \
	{								\
		.id = (_id),						\
		.regidx = (_regidx),					\
		.type = IMX_CCM_DIV,					\
		.base.name = (_name),					\
		.base.flags = 0,					\
		.u.div.parent = (_parent),				\
		.u.div.reg = (_reg),					\
		.u.div.mask = (_mask),					\
		.u.div.flags = (_flags),				\
		.get_rate = imx_ccm_div_get_rate,			\
		.set_rate = imx_ccm_div_set_rate,			\
		.get_parent = imx_ccm_div_get_parent,			\
	}

/*
 * IMX clock definition
 */

struct imx_ccm_clk {
	struct clk	base;
	u_int		id;
	u_int		regidx;
	enum imx_ccm_clktype type;
	union {
		struct imx_ccm_gate gate;
		struct imx_ccm_composite composite;
		struct imx_ccm_pll pll;
		struct imx_ccm_fixed fixed;
		struct imx_ccm_fixed_factor fixed_factor;
		struct imx_ccm_mux mux;
		struct imx_ccm_div div;
		const char *extclk;
	} u;

	int		(*enable)(struct imx_ccm_softc *,
				  struct imx_ccm_clk *, int);
	u_int		(*get_rate)(struct imx_ccm_softc *,
				    struct imx_ccm_clk *);
	int		(*set_rate)(struct imx_ccm_softc *,
				    struct imx_ccm_clk *, u_int);
	u_int		(*round_rate)(struct imx_ccm_softc *,
				    struct imx_ccm_clk *, u_int);
	const char *	(*get_parent)(struct imx_ccm_softc *,
				      struct imx_ccm_clk *);
	int		(*set_parent)(struct imx_ccm_softc *,
				      struct imx_ccm_clk *,
				      const char *);
};

/*
 * Driver state
 */

struct imx_ccm_softc {
	device_t		sc_dev;
	int			sc_phandle;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh[2];

	bus_addr_t		sc_baseaddr;

	struct clk_domain	sc_clkdom;

	struct imx_ccm_clk	*sc_clks;
	u_int			sc_nclks;
};

int	imx_ccm_attach(struct imx_ccm_softc *);
struct imx_ccm_clk *imx_ccm_clock_find(struct imx_ccm_softc *,
				     const char *);
void	imx_ccm_print(struct imx_ccm_softc *);

extern const struct clk_funcs imx_ccm_clock_funcs;

#define CCM_READ(sc, idx, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh[idx], (reg))
#define CCM_WRITE(sc, idx, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh[idx], (reg), (val))

#endif /* _ARM_IMX_CCM_H */
