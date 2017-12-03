/* $NetBSD: sunxi_ccu.h,v 1.15.2.2 2017/12/03 11:35:56 jdolecek Exp $ */

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

#ifndef _ARM_SUNXI_CCU_H
#define _ARM_SUNXI_CCU_H

#include <dev/clk/clk_backend.h>

struct sunxi_ccu_softc;
struct sunxi_ccu_clk;
struct sunxi_ccu_reset;

/*
 * Resets
 */

struct sunxi_ccu_reset {
	bus_size_t	reg;
	uint32_t	mask;
};

#define	SUNXI_CCU_RESET(_id, _reg, _bit)	\
	[_id] = {				\
		.reg = (_reg),			\
		.mask = __BIT(_bit),		\
	}

/*
 * Clocks
 */

enum sunxi_ccu_clktype {
	SUNXI_CCU_UNKNOWN,
	SUNXI_CCU_GATE,
	SUNXI_CCU_NM,
	SUNXI_CCU_NKMP,
	SUNXI_CCU_PREDIV,
	SUNXI_CCU_DIV,
	SUNXI_CCU_PHASE,
	SUNXI_CCU_FIXED_FACTOR,
};

struct sunxi_ccu_gate {
	bus_size_t	reg;
	uint32_t	mask;
	const char	*parent;
};

int	sunxi_ccu_gate_enable(struct sunxi_ccu_softc *,
			      struct sunxi_ccu_clk *, int);
const char *sunxi_ccu_gate_get_parent(struct sunxi_ccu_softc *,
				      struct sunxi_ccu_clk *);

#define	SUNXI_CCU_GATE(_id, _name, _pname, _reg, _bit)		\
	[_id] = {						\
		.type = SUNXI_CCU_GATE,				\
		.base.name = (_name),				\
		.base.flags = CLK_SET_RATE_PARENT,		\
		.u.gate.parent = (_pname),			\
		.u.gate.reg = (_reg),				\
		.u.gate.mask = __BIT(_bit),			\
		.enable = sunxi_ccu_gate_enable,		\
		.get_parent = sunxi_ccu_gate_get_parent,	\
	}

struct sunxi_ccu_nkmp_tbl {
	u_int		rate;
	uint32_t	n;
	uint32_t	k;
	uint32_t	m;
	uint32_t	p;
};

struct sunxi_ccu_nkmp {
	bus_size_t	reg;
	const char	*parent;
	uint32_t	n;
	uint32_t	k;
	uint32_t	m;
	uint32_t	p;
	uint32_t	lock;
	uint32_t	enable;
	uint32_t	flags;
	const struct sunxi_ccu_nkmp_tbl *table;
#define	SUNXI_CCU_NKMP_DIVIDE_BY_TWO		__BIT(0)
#define	SUNXI_CCU_NKMP_FACTOR_N_EXACT		__BIT(1)
#define	SUNXI_CCU_NKMP_SCALE_CLOCK		__BIT(2)
#define	SUNXI_CCU_NKMP_FACTOR_P_POW2		__BIT(3)
#define	SUNXI_CCU_NKMP_FACTOR_N_ZERO_IS_ONE	__BIT(4)
};

int	sunxi_ccu_nkmp_enable(struct sunxi_ccu_softc *,
			      struct sunxi_ccu_clk *, int);
u_int	sunxi_ccu_nkmp_get_rate(struct sunxi_ccu_softc *,
				struct sunxi_ccu_clk *);
int	sunxi_ccu_nkmp_set_rate(struct sunxi_ccu_softc *,
				struct sunxi_ccu_clk *, u_int);
const char *sunxi_ccu_nkmp_get_parent(struct sunxi_ccu_softc *,
				      struct sunxi_ccu_clk *);

#define	SUNXI_CCU_NKMP_TABLE(_id, _name, _parent, _reg, _n, _k, _m, \
		       _p, _enable, _lock, _tbl, _flags)	\
	[_id] = {						\
		.type = SUNXI_CCU_NKMP,				\
		.base.name = (_name),				\
		.u.nkmp.reg = (_reg),				\
		.u.nkmp.parent = (_parent),			\
		.u.nkmp.n = (_n),				\
		.u.nkmp.k = (_k),				\
		.u.nkmp.m = (_m),				\
		.u.nkmp.p = (_p),				\
		.u.nkmp.enable = (_enable),			\
		.u.nkmp.flags = (_flags),			\
		.u.nkmp.lock = (_lock),				\
		.u.nkmp.table = (_tbl),				\
		.enable = sunxi_ccu_nkmp_enable,		\
		.get_rate = sunxi_ccu_nkmp_get_rate,		\
		.set_rate = sunxi_ccu_nkmp_set_rate,		\
		.get_parent = sunxi_ccu_nkmp_get_parent,	\
	}

#define	SUNXI_CCU_NKMP(_id, _name, _parent, _reg, _n, _k, _m,	\
		       _p, _enable, _flags)			\
	SUNXI_CCU_NKMP_TABLE(_id, _name, _parent, _reg, _n, _k, _m, \
			     _p, _enable, 0, NULL, _flags)


struct sunxi_ccu_nm {
	bus_size_t	reg;
	const char	**parents;
	u_int		nparents;
	uint32_t	n;
	uint32_t	m;
	uint32_t	sel;
	uint32_t	enable;
	uint32_t	flags;
#define	SUNXI_CCU_NM_POWER_OF_TWO	__BIT(0)
#define	SUNXI_CCU_NM_ROUND_DOWN		__BIT(1)
#define	SUNXI_CCU_NM_DIVIDE_BY_TWO	__BIT(2)
};

int	sunxi_ccu_nm_enable(struct sunxi_ccu_softc *,
			    struct sunxi_ccu_clk *, int);
u_int	sunxi_ccu_nm_get_rate(struct sunxi_ccu_softc *,
			      struct sunxi_ccu_clk *);
int	sunxi_ccu_nm_set_rate(struct sunxi_ccu_softc *,
			      struct sunxi_ccu_clk *, u_int);
int	sunxi_ccu_nm_set_parent(struct sunxi_ccu_softc *,
				struct sunxi_ccu_clk *,
				const char *);
const char *sunxi_ccu_nm_get_parent(struct sunxi_ccu_softc *,
				    struct sunxi_ccu_clk *);

#define	SUNXI_CCU_NM(_id, _name, _parents, _reg, _n, _m, _sel,	\
		     _enable, _flags)				\
	[_id] = {						\
		.type = SUNXI_CCU_NM,				\
		.base.name = (_name),				\
		.u.nm.reg = (_reg),				\
		.u.nm.parents = (_parents),			\
		.u.nm.nparents = __arraycount(_parents),	\
		.u.nm.n = (_n),					\
		.u.nm.m = (_m),					\
		.u.nm.sel = (_sel),				\
		.u.nm.enable = (_enable),			\
		.u.nm.flags = (_flags),				\
		.enable = sunxi_ccu_nm_enable,			\
		.get_rate = sunxi_ccu_nm_get_rate,		\
		.set_rate = sunxi_ccu_nm_set_rate,		\
		.set_parent = sunxi_ccu_nm_set_parent,		\
		.get_parent = sunxi_ccu_nm_get_parent,		\
	}

struct sunxi_ccu_div {
	bus_size_t	reg;
	const char	**parents;
	u_int		nparents;
	uint32_t	div;
	uint32_t	sel;
	uint32_t	enable;
	uint32_t	flags;
#define	SUNXI_CCU_DIV_POWER_OF_TWO	__BIT(0)
#define	SUNXI_CCU_DIV_ZERO_IS_ONE	__BIT(1)
#define	SUNXI_CCU_DIV_TIMES_TWO		__BIT(2)
#define	SUNXI_CCU_DIV_SET_RATE_PARENT	__BIT(3)
};

int	sunxi_ccu_div_enable(struct sunxi_ccu_softc *,
			     struct sunxi_ccu_clk *, int);
u_int	sunxi_ccu_div_get_rate(struct sunxi_ccu_softc *,
			       struct sunxi_ccu_clk *);
int	sunxi_ccu_div_set_rate(struct sunxi_ccu_softc *,
			       struct sunxi_ccu_clk *, u_int);
int	sunxi_ccu_div_set_parent(struct sunxi_ccu_softc *,
			         struct sunxi_ccu_clk *,
			         const char *);
const char *sunxi_ccu_div_get_parent(struct sunxi_ccu_softc *,
				     struct sunxi_ccu_clk *);

#define	SUNXI_CCU_DIV(_id, _name, _parents, _reg, _div,		\
		      _sel, _flags)				\
	SUNXI_CCU_DIV_GATE(_id, _name, _parents, _reg, _div,	\
			   _sel, 0, _flags)

#define	SUNXI_CCU_DIV_GATE(_id, _name, _parents, _reg, _div,	\
		      _sel, _enable, _flags)			\
	[_id] = {						\
		.type = SUNXI_CCU_DIV,				\
		.base.name = (_name),				\
		.u.div.reg = (_reg),				\
		.u.div.parents = (_parents),			\
		.u.div.nparents = __arraycount(_parents),	\
		.u.div.div = (_div),				\
		.u.div.sel = (_sel),				\
		.u.div.enable = (_enable),			\
		.u.div.flags = (_flags),			\
		.enable = sunxi_ccu_div_enable,			\
		.get_rate = sunxi_ccu_div_get_rate,		\
		.set_rate = sunxi_ccu_div_set_rate,		\
		.set_parent = sunxi_ccu_div_set_parent,		\
		.get_parent = sunxi_ccu_div_get_parent,		\
	}

struct sunxi_ccu_prediv {
	bus_size_t	reg;
	const char	**parents;
	u_int		nparents;
	uint32_t	prediv;
	uint32_t	prediv_sel;
	uint32_t	prediv_fixed;
	uint32_t	div;
	uint32_t	sel;
	uint32_t	flags;
#define	SUNXI_CCU_PREDIV_POWER_OF_TWO	__BIT(0)
#define	SUNXI_CCU_PREDIV_DIVIDE_BY_TWO	__BIT(1)
};

u_int	sunxi_ccu_prediv_get_rate(struct sunxi_ccu_softc *,
				  struct sunxi_ccu_clk *);
int	sunxi_ccu_prediv_set_rate(struct sunxi_ccu_softc *,
				  struct sunxi_ccu_clk *, u_int);
int	sunxi_ccu_prediv_set_parent(struct sunxi_ccu_softc *,
				    struct sunxi_ccu_clk *,
				    const char *);
const char *sunxi_ccu_prediv_get_parent(struct sunxi_ccu_softc *,
					struct sunxi_ccu_clk *);

#define	SUNXI_CCU_PREDIV(_id, _name, _parents, _reg, _prediv,	\
		     _prediv_sel, _div, _sel, _flags)		\
	SUNXI_CCU_PREDIV_FIXED(_id, _name, _parents, _reg, _prediv, \
		     _prediv_sel, 0, _div, _sel, _flags)

#define	SUNXI_CCU_PREDIV_FIXED(_id, _name, _parents, _reg, _prediv, \
		     _prediv_sel, _prediv_fixed, _div, _sel, _flags) \
	[_id] = {						\
		.type = SUNXI_CCU_PREDIV,			\
		.base.name = (_name),				\
		.u.prediv.reg = (_reg),				\
		.u.prediv.parents = (_parents),			\
		.u.prediv.nparents = __arraycount(_parents),	\
		.u.prediv.prediv = (_prediv),			\
		.u.prediv.prediv_sel = (_prediv_sel),		\
		.u.prediv.prediv_fixed = (_prediv_fixed),	\
		.u.prediv.div = (_div),				\
		.u.prediv.sel = (_sel),				\
		.u.prediv.flags = (_flags),			\
		.get_rate = sunxi_ccu_prediv_get_rate,		\
		.set_rate = sunxi_ccu_prediv_set_rate,		\
		.set_parent = sunxi_ccu_prediv_set_parent,	\
		.get_parent = sunxi_ccu_prediv_get_parent,	\
	}

struct sunxi_ccu_phase {
	bus_size_t	reg;
	const char	*parent;
	uint32_t	mask;
};

u_int	sunxi_ccu_phase_get_rate(struct sunxi_ccu_softc *,
				 struct sunxi_ccu_clk *);
int	sunxi_ccu_phase_set_rate(struct sunxi_ccu_softc *,
				 struct sunxi_ccu_clk *, u_int);
const char *sunxi_ccu_phase_get_parent(struct sunxi_ccu_softc *,
				       struct sunxi_ccu_clk *);

#define	SUNXI_CCU_PHASE(_id, _name, _parent, _reg, _mask)	\
	[_id] = {						\
		.type = SUNXI_CCU_PHASE,			\
		.base.name = (_name),				\
		.u.phase.reg = (_reg),				\
		.u.phase.parent = (_parent),			\
		.u.phase.mask = (_mask),			\
		.get_rate = sunxi_ccu_phase_get_rate,		\
		.set_rate = sunxi_ccu_phase_set_rate,		\
		.get_parent = sunxi_ccu_phase_get_parent,	\
	}

struct sunxi_ccu_fixed_factor {
	const char	*parent;
	u_int		div;
	u_int		mult;
};

u_int	sunxi_ccu_fixed_factor_get_rate(struct sunxi_ccu_softc *,
					struct sunxi_ccu_clk *);
const char *sunxi_ccu_fixed_factor_get_parent(struct sunxi_ccu_softc *,
					      struct sunxi_ccu_clk *);

#define	SUNXI_CCU_FIXED_FACTOR(_id, _name, _parent, _div, _mult)	\
	[_id] = {							\
		.type = SUNXI_CCU_FIXED_FACTOR,				\
		.base.name = (_name),					\
		.u.fixed_factor.parent = (_parent),			\
		.u.fixed_factor.div = (_div),				\
		.u.fixed_factor.mult = (_mult),				\
		.get_rate = sunxi_ccu_fixed_factor_get_rate,		\
		.get_parent = sunxi_ccu_fixed_factor_get_parent,	\
	}

struct sunxi_ccu_clk {
	struct clk	base;
	enum sunxi_ccu_clktype type;
	union {
		struct sunxi_ccu_gate gate;
		struct sunxi_ccu_nm nm;
		struct sunxi_ccu_nkmp nkmp;
		struct sunxi_ccu_prediv prediv;
		struct sunxi_ccu_div div;
		struct sunxi_ccu_phase phase;
		struct sunxi_ccu_fixed_factor fixed_factor;
	} u;

	int		(*enable)(struct sunxi_ccu_softc *,
				  struct sunxi_ccu_clk *, int);
	u_int		(*get_rate)(struct sunxi_ccu_softc *,
				    struct sunxi_ccu_clk *);
	int		(*set_rate)(struct sunxi_ccu_softc *,
				    struct sunxi_ccu_clk *, u_int);
	const char *	(*get_parent)(struct sunxi_ccu_softc *,
				      struct sunxi_ccu_clk *);
	int		(*set_parent)(struct sunxi_ccu_softc *,
				      struct sunxi_ccu_clk *,
				      const char *);
};

struct sunxi_ccu_softc {
	device_t		sc_dev;
	int			sc_phandle;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct clk_domain	sc_clkdom;

	struct sunxi_ccu_reset *sc_resets;
	u_int			sc_nresets;

	struct sunxi_ccu_clk	*sc_clks;
	u_int			sc_nclks;
};

int	sunxi_ccu_attach(struct sunxi_ccu_softc *);
struct sunxi_ccu_clk *sunxi_ccu_clock_find(struct sunxi_ccu_softc *,
					   const char *);
void	sunxi_ccu_print(struct sunxi_ccu_softc *);

#define CCU_READ(sc, reg)	\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define CCU_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

#endif /* _ARM_SUNXI_CCU_H */
