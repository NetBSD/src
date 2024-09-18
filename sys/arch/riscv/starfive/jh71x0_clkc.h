/* $NetBSD: jh71x0_clkc.h,v 1.4 2024/09/18 10:37:03 skrll Exp $ */

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

#ifndef _STARFIVE_JH71X0CLKC_H
#define _STARFIVE_JH71X0CLKC_H

#include <dev/clk/clk_backend.h>
#include <dev/fdt/syscon.h>

/*
 * Each clock has a 32-bit register indexed from the register base with
 * the following bit field definitions depending on type.
 */

/* register fields */
#define JH71X0_CLK_ENABLE	__BIT(31)
#define JH71X0_CLK_INVERT	__BIT(30)
#define JH71X0_CLK_MUX_MASK	__BITS(27, 24)
#define JH71X0_CLK_DIV_MASK	__BITS(23, 0)
#define JH71X0_CLK_FRAC_MASK	__BITS(15, 8)
#define JH71X0_CLK_INT_MASK	__BITS(7, 0)

/* fractional divider min/max */
#define JH71X0_CLK_FRAC_MIN	100UL
#define JH71X0_CLK_FRAC_MAX	(26600UL - 1)


struct jh71x0_clkc_clk;

struct jh71x0_clkc_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;
	struct clk_domain	sc_clkdom;

	struct jh71x0_clkc_clk *sc_clk;
	size_t			sc_nclks;

	// JH7110 only
	size_t			sc_nrsts;
	bus_size_t		sc_reset_assert;
	bus_size_t		sc_reset_status;

	kmutex_t		sc_lock;
};

struct jh71x0_clkc_clk;

// MDIV

enum jh71x0_clkc_clktype {
	JH71X0CLK_UNKNOWN,
	JH71X0CLK_FIXED_FACTOR,
	JH71X0CLK_GATE,
	JH71X0CLK_DIV,
	JH71X0CLK_FRACDIV,
	JH71X0CLK_MUX,
	JH71X0CLK_MUXDIV,
	JH71X0CLK_INV,
};

/*
 * Fixed-factor clocks
 */

struct jh71x0_clkc_fixed_factor {
	const char *	jcff_parent;
	u_int		jcff_div;
	u_int		jcff_mult;
};

u_int	jh71x0_clkc_fixed_factor_get_rate(struct jh71x0_clkc_softc *,
	    struct jh71x0_clkc_clk *);
int	jh71x0_clkc_fixed_factor_set_rate(struct jh71x0_clkc_softc *,
	    struct jh71x0_clkc_clk *, u_int);
const char *
	jh71x0_clkc_fixed_factor_get_parent(struct jh71x0_clkc_softc *,
	    struct jh71x0_clkc_clk *);

extern struct jh71x0_clkc_clkops jh71x0_clkc_ffactor_ops;

#define	JH71X0CLKC_FIXED_FACTOR(_id, _name, _parent, _div, _mult)	      \
	[_id] = {							      \
		.jcc_type = JH71X0CLK_FIXED_FACTOR,			      \
		.jcc_clk.name = (_name),				      \
		.jcc_ffactor.jcff_parent = (_parent),			      \
		.jcc_ffactor.jcff_div = (_div),				      \
		.jcc_ffactor.jcff_mult = (_mult),			      \
		.jcc_ops = &jh71x0_clkc_ffactor_ops,			      \
	}

/*
 * Gate clocks
 */

struct jh71x0_clkc_gate {
	const char	*jcg_parent;
};

int	jh71x0_clkc_gate_enable(struct jh71x0_clkc_softc *,
	    struct jh71x0_clkc_clk *, int);
const char *
	jh71x0_clkc_gate_get_parent(struct jh71x0_clkc_softc *,
	    struct jh71x0_clkc_clk *);

extern struct jh71x0_clkc_clkops jh71x0_clkc_gate_ops;

#define	JH71X0CLKC_GATE(_id, _name, _pname)				      \
	[_id] = {							      \
		.jcc_type = JH71X0CLK_GATE,				      \
		.jcc_clk = {						      \
			.name = (_name),				      \
			.flags = CLK_SET_RATE_PARENT,			      \
		},							      \
		.jcc_reg = (_id) * sizeof(uint32_t),			      \
		.jcc_gate.jcg_parent = (_pname),			      \
		.jcc_ops = &jh71x0_clkc_gate_ops,			      \
	}

/*
 * Divider clocks
 */

struct jh71x0_clkc_div {
	bus_size_t	jcd_reg;
	const char *	jcd_parent;
	uint32_t	jcd_maxdiv;
	uint32_t	jcd_flags;
#define	JH71X0CLKC_DIV_GATE	__BIT(0)
};

u_int	jh71x0_clkc_div_get_rate(struct jh71x0_clkc_softc *,
	    struct jh71x0_clkc_clk *);
int	jh71x0_clkc_div_set_rate(struct jh71x0_clkc_softc *,
	    struct jh71x0_clkc_clk *, u_int);
const char *
	jh71x0_clkc_div_get_parent(struct jh71x0_clkc_softc *,
	    struct jh71x0_clkc_clk *);

extern struct jh71x0_clkc_clkops jh71x0_clkc_div_ops;

#define	JH71X0CLKC_DIV_FLAGS(_id, _name, _maxdiv, _parent, _flags)	      \
	[_id] = {							      \
		.jcc_type = JH71X0CLK_DIV,				      \
		.jcc_clk = {						      \
			.name = (_name),				      \
		},							      \
		.jcc_reg = (_id) * sizeof(uint32_t),			      \
		.jcc_div = {						      \
			.jcd_parent = (_parent),			      \
			.jcd_maxdiv = (_maxdiv),			      \
			.jcd_flags = (_flags),				      \
		},							      \
		.jcc_ops = &jh71x0_clkc_div_ops,			      \
	}

#define	JH71X0CLKC_DIV(_id, _n, _m, _p)					      \
    JH71X0CLKC_DIV_FLAGS((_id), (_n), (_m), (_p), 0)

#define	JH71X0CLKC_GATEDIV(_id, _n, _m, _p)				      \
    JH71X0CLKC_DIV_FLAGS((_id), (_n), (_m), (_p), JH71X0CLKC_DIV_GATE)

/*
 * Fractional Divider clocks
 */

struct jh71x0_clkc_fracdiv {
	bus_size_t	jcd_reg;
	const char *	jcd_parent;
	uint32_t	jcd_flags;
#define	JH71X0CLKC_DIV_GATE	__BIT(0)
};

u_int	jh71x0_clkc_fracdiv_get_rate(struct jh71x0_clkc_softc *,
	    struct jh71x0_clkc_clk *);
int	jh71x0_clkc_fracdiv_set_rate(struct jh71x0_clkc_softc *,
	    struct jh71x0_clkc_clk *, u_int);
const char *
	jh71x0_clkc_fracdiv_get_parent(struct jh71x0_clkc_softc *,
	    struct jh71x0_clkc_clk *);

extern struct jh71x0_clkc_clkops jh71x0_clkc_fracdiv_ops;

#define	JH71X0CLKC_FRACDIV(_id, _name, _parent)				      \
	[_id] = {							      \
		.jcc_type = JH71X0CLK_FRACDIV,				      \
		.jcc_clk = {						      \
			.name = (_name),				      \
		},							      \
		.jcc_reg = (_id) * sizeof(uint32_t),			      \
		.jcc_fracdiv = {					      \
			.jcd_parent = (_parent),			      \
		},							      \
		.jcc_ops = &jh71x0_clkc_fracdiv_ops,			      \
	}


/*
 * Mux clocks
 */

struct jh71x0_clkc_mux {
	size_t		jcm_nparents;
	const char **	jcm_parents;
	uint32_t	jcm_flags;
#define	JH71X0CLKC_MUX_GATE	__BIT(0)
};

int	jh71x0_clkc_mux_set_parent(struct jh71x0_clkc_softc *,
	    struct jh71x0_clkc_clk *, const char *);
const char *
	jh71x0_clkc_mux_get_parent(struct jh71x0_clkc_softc *,
	    struct jh71x0_clkc_clk *);

extern struct jh71x0_clkc_clkops jh71x0_clkc_mux_ops;

#define	JH71X0CLKC_MUX_FLAGSX2(_id, _name, _parents, _cflags, _mflags)	      \
	[_id] = {							      \
		.jcc_type = JH71X0CLK_MUX,				      \
		.jcc_clk = {						      \
			.name = (_name),				      \
			.flags = (_cflags),				      \
		},							      \
		.jcc_reg = (_id) * sizeof(uint32_t),			      \
		.jcc_mux = {						      \
			.jcm_parents = (_parents),			      \
			.jcm_nparents = __arraycount(_parents),		      \
			.jcm_flags = (_mflags),				      \
		},							      \
		.jcc_ops = &jh71x0_clkc_mux_ops,			      \
	}

#define	JH71X0CLKC_MUX(_id, _n, _p)					      \
    JH71X0CLKC_MUX_FLAGSX2((_id), (_n), (_p), 0, 0)

#define	JH71X0CLKC_MUX_FLAGS(_id, _n, _p, _f)				      \
    JH71X0CLKC_MUX_FLAGSX2((_id), (_n), (_p), (_f), 0)

#define	JH71X0CLKC_MUXGATE(_id, _n, _p)					      \
    JH71X0CLKC_MUX_FLAGSX2((_id), (_n), (_p), 0, JH71X0CLKC_MUX_GATE)

#define	JH71X0CLKC_MUXGATE_FLAGS(_id, _n, _p, _f)			      \
    JH71X0CLKC_MUX_FLAGSX2((_id), (_n), (_p), (_f), JH71X0CLKC_MUX_GATE)



/*
 * Mux divider clocks
 */

struct jh71x0_clkc_muxdiv {
	size_t		jcmd_nparents;
	const char **	jcmd_parents;
	uint32_t	jcmd_maxdiv;
	uint32_t	jcmd_flags;
#define	JH71X0CLKC_MUXDIV_GATE	__BIT(0)
};

u_int	jh71x0_clkc_muxdiv_get_rate(struct jh71x0_clkc_softc *,
	    struct jh71x0_clkc_clk *);
int	jh71x0_clkc_muxdiv_set_rate(struct jh71x0_clkc_softc *,
	    struct jh71x0_clkc_clk *, u_int);

int	jh71x0_clkc_muxdiv_set_parent(struct jh71x0_clkc_softc *,
	    struct jh71x0_clkc_clk *, const char *);
const char *
	jh71x0_clkc_muxdiv_get_parent(struct jh71x0_clkc_softc *,
	    struct jh71x0_clkc_clk *);
extern struct jh71x0_clkc_clkops jh71x0_clkc_muxdiv_ops;

#define	JH71X0CLKC_MUXDIV_FLAGSX2(_id, _name, _maxdiv, _parents, _cf, _mf)    \
	[_id] = {							      \
		.jcc_type = JH71X0CLK_MUXDIV,				      \
		.jcc_clk = {						      \
			.name = (_name),				      \
			.flags = (_cf),					      \
		},							      \
		.jcc_reg = (_id) * sizeof(uint32_t),			      \
		.jcc_muxdiv = {						      \
			.jcmd_parents = (_parents),			      \
			.jcmd_nparents = __arraycount(_parents),	      \
			.jcmd_maxdiv = (_maxdiv),			      \
			.jcmd_flags = (_mf),				      \
		},							      \
		.jcc_ops = &jh71x0_clkc_muxdiv_ops,			      \
	}

#define	JH71X0CLKC_MUXDIV(_id, _n, _m, _p)					      \
    JH71X0CLKC_MUXDIV_FLAGSX2((_id), (_n), (_m), (_p), 0, 0)

#define	JH71X0CLKC_MUXDIV_FLAGS(_id, _n, _m, _p, _f)				      \
    JH71X0CLKC_MUXDIV_FLAGSX2((_id), (_n), (_m), (_p), (_f), 0)

#define	JH71X0CLKC_MUXDIVGATE(_id, _n, _m, _p)					      \
    JH71X0CLKC_MUXDIV_FLAGSX2((_id), (_n), (_m), (_p), 0, JH71X0CLKC_MUX_GATE)

#define	JH71X0CLKC_MUXDIVGATE_FLAGS(_id, _n, _m, _p, _f)			      \
    JH71X0CLKC_MUXDIV_FLAGSX2((_id), (_n), (_m), (_p), (_f), JH71X0CLKC_MUX_GATE)


struct jh71x0_clkc_inv {
	const char *	jci_parent;
};

const char *
jh71x0_clkc_inv_get_parent(struct jh71x0_clkc_softc *sc,
    struct jh71x0_clkc_clk *jcc);

extern struct jh71x0_clkc_clkops jh71x0_clkc_inv_ops;

#define	JH71X0CLKC_INV(_id, _name, _pname)				      \
	[_id] = {							      \
		.jcc_type = JH71X0CLK_INV,				      \
		.jcc_clk = {						      \
			.name = (_name),				      \
			.flags = CLK_SET_RATE_PARENT,			      \
		},							      \
		.jcc_reg = (_id) * sizeof(uint32_t),			      \
		.jcc_inv.jci_parent = (_pname),				      \
		.jcc_ops = &jh71x0_clkc_inv_ops,			      \
	}


struct jh71x0_clkc_clkops {

	int		(*jcco_enable)(struct jh71x0_clkc_softc *,
			    struct jh71x0_clkc_clk *, int);
	u_int		(*jcco_getrate)(struct jh71x0_clkc_softc *,
			    struct jh71x0_clkc_clk *);
	int		(*jcco_setrate)(struct jh71x0_clkc_softc *,
			    struct jh71x0_clkc_clk *, u_int);
	const char *    (*jcco_getparent)(struct jh71x0_clkc_softc *,
			    struct jh71x0_clkc_clk *);
	int		(*jcco_setparent)(struct jh71x0_clkc_softc *,
			    struct jh71x0_clkc_clk *, const char *);
};


struct jh71x0_clkc_clk {
	struct clk				jcc_clk;
	enum jh71x0_clkc_clktype		jcc_type;
	bus_size_t				jcc_reg;
	union {
		struct jh71x0_clkc_gate		jcc_gate;
		struct jh71x0_clkc_div		jcc_div;
		struct jh71x0_clkc_fracdiv	jcc_fracdiv;
		struct jh71x0_clkc_fixed_factor jcc_ffactor;
		struct jh71x0_clkc_mux		jcc_mux;
		struct jh71x0_clkc_muxdiv	jcc_muxdiv;
		struct jh71x0_clkc_inv		jcc_inv;
	};
	struct jh71x0_clkc_clkops *		jcc_ops;
};

extern const struct clk_funcs jh71x0_clkc_funcs;

#endif
