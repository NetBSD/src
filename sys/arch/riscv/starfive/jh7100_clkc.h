/* $NetBSD: jh7100_clkc.h,v 1.2 2024/01/17 07:05:35 skrll Exp $ */

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _STARFIVE_JH7100CLKC_H
#define _STARFIVE_JH7100CLKC_H

#include <dev/clk/clk_backend.h>
#include <dev/fdt/syscon.h>

/*
 * Each clock has a 32-bit register indexed from the register base with
 * the following bit field definitions depending on type.
 */

/* register fields */
#define JH7100_CLK_ENABLE	__BIT(31)
#define JH7100_CLK_INVERT	__BIT(30)
#define JH7100_CLK_MUX_MASK	__BITS(27, 24)
#define JH7100_CLK_DIV_MASK	__BITS(23, 0)
#define JH7100_CLK_FRAC_MASK	__BITS(15, 8)
#define JH7100_CLK_INT_MASK	__BITS(7, 0)

/* fractional divider min/max */
#define JH7100_CLK_FRAC_MIN	100UL
#define JH7100_CLK_FRAC_MAX	(26600UL - 1)

struct jh7100_clkc_softc;
struct jh7100_clkc_clk;

enum jh7100_clkc_clktype {
	JH7100CLK_UNKNOWN,
	JH7100CLK_FIXED_FACTOR,
	JH7100CLK_GATE,
	JH7100CLK_DIV,
	JH7100CLK_FRACDIV,
	JH7100CLK_MUX,
	JH7100CLK_INV,
};

/*
 * Fixed-factor clocks
 */

struct jh7100_clkc_fixed_factor {
	const char *	jcff_parent;
	u_int		jcff_div;
	u_int		jcff_mult;
};

u_int	jh7100_clkc_fixed_factor_get_rate(struct jh7100_clkc_softc *,
	    struct jh7100_clkc_clk *);
int	jh7100_clkc_fixed_factor_set_rate(struct jh7100_clkc_softc *,
	    struct jh7100_clkc_clk *, u_int);
const char *
	jh7100_clkc_fixed_factor_get_parent(struct jh7100_clkc_softc *,
	    struct jh7100_clkc_clk *);

#define	JH7100CLKC_FIXED_FACTOR(_id, _name, _parent, _div, _mult)	      \
	[_id] = {							      \
		.jcc_type = JH7100CLK_FIXED_FACTOR,			      \
		.jcc_clk.name = (_name),				      \
		.jcc_ffactor.jcff_parent = (_parent),			      \
		.jcc_ffactor.jcff_div = (_div),				      \
		.jcc_ffactor.jcff_mult = (_mult),			      \
		.jcc_ops = &jh7100_clkc_ffactor_ops,			      \
	}

/*
 * Gate clocks
 */

struct jh7100_clkc_gate {
	const char	*jcg_parent;
};

int	jh7100_clkc_gate_enable(struct jh7100_clkc_softc *,
	    struct jh7100_clkc_clk *, int);
const char *
	jh7100_clkc_gate_get_parent(struct jh7100_clkc_softc *,
	    struct jh7100_clkc_clk *);

#define	JH7100CLKC_GATE(_id, _name, _pname)				      \
	[_id] = {							      \
		.jcc_type = JH7100CLK_GATE,				      \
		.jcc_clk = {						      \
			.name = (_name),				      \
			.flags = CLK_SET_RATE_PARENT,			      \
		},							      \
		.jcc_reg = (_id) * sizeof(uint32_t),			      \
		.jcc_gate.jcg_parent = (_pname),			      \
		.jcc_ops = &jh7100_clkc_gate_ops,			      \
	}

/*
 * Divider clocks
 */

struct jh7100_clkc_div {
	bus_size_t	jcd_reg;
	const char *	jcd_parent;
	uint32_t	jcd_maxdiv;
	uint32_t	jcd_flags;
#define	JH7100CLKC_DIV_GATE	__BIT(0)
};

u_int	jh7100_clkc_div_get_rate(struct jh7100_clkc_softc *,
	    struct jh7100_clkc_clk *);
int	jh7100_clkc_div_set_rate(struct jh7100_clkc_softc *,
	    struct jh7100_clkc_clk *, u_int);
const char *
	jh7100_clkc_div_get_parent(struct jh7100_clkc_softc *,
	    struct jh7100_clkc_clk *);

#define	JH7100CLKC_DIV_FLAGS(_id, _name, _maxdiv, _parent, _flags)	      \
	[_id] = {							      \
		.jcc_type = JH7100CLK_DIV,				      \
		.jcc_clk = {						      \
			.name = (_name),				      \
		},							      \
		.jcc_reg = (_id) * sizeof(uint32_t),			      \
		.jcc_div = {						      \
			.jcd_parent = (_parent),			      \
			.jcd_maxdiv = (_maxdiv),			      \
			.jcd_flags = (_flags),				      \
		},							      \
		.jcc_ops = &jh7100_clkc_div_ops,			      \
	}

#define	JH7100CLKC_DIV(_id, _n, _m, _p)		  			      \
    JH7100CLKC_DIV_FLAGS((_id), (_n), (_m), (_p), 0)

#define	JH7100CLKC_GATEDIV(_id, _n, _m, _p)				      \
    JH7100CLKC_DIV_FLAGS((_id), (_n), (_m), (_p), JH7100CLKC_DIV_GATE)

/*
 * Fractional Divider clocks
 */

struct jh7100_clkc_fracdiv {
	bus_size_t	jcd_reg;
	const char *	jcd_parent;
	uint32_t	jcd_flags;
#define	JH7100CLKC_DIV_GATE	__BIT(0)
};

u_int	jh7100_clkc_fracdiv_get_rate(struct jh7100_clkc_softc *,
	    struct jh7100_clkc_clk *);
int	jh7100_clkc_fracdiv_set_rate(struct jh7100_clkc_softc *,
	    struct jh7100_clkc_clk *, u_int);
const char *
	jh7100_clkc_fracdiv_get_parent(struct jh7100_clkc_softc *,
	    struct jh7100_clkc_clk *);

#define	JH7100CLKC_FRACDIV(_id, _name, _parent)				      \
	[_id] = {							      \
		.jcc_type = JH7100CLK_FRACDIV,				      \
		.jcc_clk = {						      \
			.name = (_name),				      \
		},							      \
		.jcc_reg = (_id) * sizeof(uint32_t),			      \
		.jcc_fracdiv = {					      \
			.jcd_parent = (_parent),			      \
		},							      \
		.jcc_ops = &jh7100_clkc_fracdiv_ops,			      \
	}


/*
 * Mux clocks
 */

struct jh7100_clkc_mux {
	size_t		jcm_nparents;
	const char **	jcm_parents;
};

int	jh7100_clkc_mux_set_parent(struct jh7100_clkc_softc *,
	    struct jh7100_clkc_clk *, const char *);
const char *
	jh7100_clkc_mux_get_parent(struct jh7100_clkc_softc *,
	    struct jh7100_clkc_clk *);

#define	JH7100CLKC_MUX_FLAGS(_id, _name, _parents, _cflags)		      \
	[_id] = {							      \
		.jcc_type = JH7100CLK_MUX,				      \
		.jcc_clk = {						      \
			.name = (_name),				      \
			.flags = (_cflags),				      \
		},							      \
		.jcc_reg = (_id) * sizeof(uint32_t),			      \
		.jcc_mux = {						      \
			.jcm_parents = (_parents),			      \
			.jcm_nparents = __arraycount(_parents),		      \
		},							      \
		.jcc_ops = &jh7100_clkc_mux_ops,			      \
	}

#define	JH7100CLKC_MUX(_id, _n, _p)		  			      \
    JH7100CLKC_MUX_FLAGS((_id), (_n), (_p), 0)

struct jh7100_clkc_inv {
	const char *	jci_parent;
};

const char *
jh7100_clkc_inv_get_parent(struct jh7100_clkc_softc *sc,
    struct jh7100_clkc_clk *jcc);

#define	JH7100CLKC_INV(_id, _name, _pname)				      \
	[_id] = {							      \
		.jcc_type = JH7100CLK_INV,				      \
		.jcc_clk = {						      \
			.name = (_name),				      \
			.flags = CLK_SET_RATE_PARENT,			      \
		},							      \
		.jcc_reg = (_id) * sizeof(uint32_t),			      \
		.jcc_inv.jci_parent = (_pname),				      \
		.jcc_ops = &jh7100_clkc_inv_ops,			      \
	}


struct jh7100_clkc_clkops {

	int		(*jcco_enable)(struct jh7100_clkc_softc *,
			    struct jh7100_clkc_clk *, int);
	u_int		(*jcco_getrate)(struct jh7100_clkc_softc *,
			    struct jh7100_clkc_clk *);
	int		(*jcco_setrate)(struct jh7100_clkc_softc *,
			    struct jh7100_clkc_clk *, u_int);
	const char *    (*jcco_getparent)(struct jh7100_clkc_softc *,
			    struct jh7100_clkc_clk *);
	int		(*jcco_setparent)(struct jh7100_clkc_softc *,
			    struct jh7100_clkc_clk *, const char *);
};


struct jh7100_clkc_clk {
	struct clk		 		jcc_clk;
	enum jh7100_clkc_clktype 		jcc_type;
	bus_size_t				jcc_reg;
	union {
		struct jh7100_clkc_gate		jcc_gate;
		struct jh7100_clkc_div		jcc_div;
		struct jh7100_clkc_fracdiv	jcc_fracdiv;
		struct jh7100_clkc_fixed_factor jcc_ffactor;
		struct jh7100_clkc_mux		jcc_mux;
		struct jh7100_clkc_inv		jcc_inv;
	};
	struct jh7100_clkc_clkops *		jcc_ops;
};

#endif
