/* $NetBSD: ti_prcm.h,v 1.1 2017/10/26 23:28:15 jmcneill Exp $ */

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

#ifndef _ARM_TI_PRCM_H
#define	_ARM_TI_PRCM_H

#ifdef TI_PRCM_PRIVATE

#include <dev/clk/clk_backend.h>

struct ti_prcm_clk;
struct ti_prcm_softc;

enum ti_prcm_clktype {
	TI_PRCM_UNKNOWN,
	TI_PRCM_FIXED,
	TI_PRCM_FIXED_FACTOR,
	TI_PRCM_HWMOD,
};

struct ti_prcm_fixed {
	u_int			rate;
};

struct ti_prcm_fixed_factor {
	u_int			mult;
	u_int			div;
	const char		*parent;
};

struct ti_prcm_hwmod {
	bus_size_t		reg;
	const char		*parent;
};

struct ti_prcm_clk {
	struct clk		base;
	enum ti_prcm_clktype	type;
	union {
		struct ti_prcm_fixed fixed;
		struct ti_prcm_fixed_factor fixed_factor;
		struct ti_prcm_hwmod hwmod;
	} u;

	int		(*enable)(struct ti_prcm_softc *,
				  struct ti_prcm_clk *, int);
	u_int		(*get_rate)(struct ti_prcm_softc *,
				    struct ti_prcm_clk *);
	int		(*set_rate)(struct ti_prcm_softc *,
				    struct ti_prcm_clk *, u_int);
	const char *	(*get_parent)(struct ti_prcm_softc *,
				      struct ti_prcm_clk *);
	int		(*set_parent)(struct ti_prcm_softc *,
				      struct ti_prcm_clk *,
				      const char *);
};

int	ti_prcm_attach(struct ti_prcm_softc *);
struct ti_prcm_clk *ti_prcm_clock_find(struct ti_prcm_softc *, const char *);

static inline u_int
ti_prcm_fixed_get_rate(struct ti_prcm_softc *sc, struct ti_prcm_clk *tc)
{
	KASSERT(tc->type == TI_PRCM_FIXED);
	return tc->u.fixed.rate;
}

#define	TI_PRCM_FIXED(_name, _rate)					\
	{								\
		.type = TI_PRCM_FIXED, .base.name = (_name),		\
		.u.fixed.rate = (_rate),				\
		.get_rate = ti_prcm_fixed_get_rate,			\
	}

static inline u_int
ti_prcm_fixed_factor_get_rate(struct ti_prcm_softc *sc, struct ti_prcm_clk *tc)
{
	KASSERT(tc->type == TI_PRCM_FIXED_FACTOR);
	struct ti_prcm_clk *tc_parent;

	tc_parent = ti_prcm_clock_find(sc, tc->u.fixed_factor.parent);
	KASSERT(tc_parent != NULL);

	const u_int mult = tc->u.fixed_factor.mult;
	const u_int div = tc->u.fixed_factor.div;

	return (clk_get_rate(&tc_parent->base) * mult) / div;
}

static inline const char *
ti_prcm_fixed_factor_get_parent(struct ti_prcm_softc *sc, struct ti_prcm_clk *tc)
{
	KASSERT(tc->type == TI_PRCM_FIXED_FACTOR);
	return tc->u.fixed_factor.parent;
}

#define	TI_PRCM_FIXED_FACTOR(_name, _mult, _div, _parent)		\
	{								\
		.type = TI_PRCM_FIXED_FACTOR, .base.name = (_name),	\
		.u.fixed_factor.mult = (_mult),				\
		.u.fixed_factor.div = (_div),				\
		.u.fixed_factor.parent = (_parent),			\
		.get_rate = ti_prcm_fixed_factor_get_rate,		\
		.get_parent = ti_prcm_fixed_factor_get_parent,		\
	}

static inline const char *
ti_prcm_hwmod_get_parent(struct ti_prcm_softc *sc, struct ti_prcm_clk *tc)
{
	KASSERT(tc->type == TI_PRCM_HWMOD);
	return tc->u.hwmod.parent;
}

#define	TI_PRCM_HWMOD(_name, _reg, _parent, _enable)			\
	{								\
		.type = TI_PRCM_HWMOD, .base.name = (_name),		\
		.u.hwmod.reg = (_reg),					\
		.u.hwmod.parent = (_parent),				\
		.enable = (_enable),					\
		.get_parent = ti_prcm_hwmod_get_parent,			\
	}

struct ti_prcm_softc {
	device_t		sc_dev;
	int			sc_phandle;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct clk_domain	sc_clkdom;

	struct ti_prcm_clk	*sc_clks;
	u_int			sc_nclks;
};

#define	PRCM_READ(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	PRCM_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

#endif /* !TI_PRCM_PRIVATE */

struct clk *	ti_prcm_get_hwmod(const int, u_int);

#endif /* !_ARM_TI_PRCM_H */
