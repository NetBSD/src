/* $NetBSD: jh71x0_clkc.c,v 1.3 2024/08/19 07:33:55 skrll Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: jh71x0_clkc.c,v 1.3 2024/08/19 07:33:55 skrll Exp $");

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/device.h>

#include <dev/clk/clk_backend.h>

#include <dev/fdt/fdtvar.h>

#include <riscv/starfive/jh71x0_clkc.h>

#define	RD4(sc, reg)							\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))


static void
jh71x0_clkc_update(struct jh71x0_clkc_softc * const sc,
    struct jh71x0_clkc_clk *jcc, uint32_t set, uint32_t clr)
{
	// lock
	uint32_t val = RD4(sc, jcc->jcc_reg);
	val &= ~clr;
	val |=  set;
	WR4(sc, jcc->jcc_reg, val);
}

/*
 * FIXED_FACTOR operations
 */

static u_int
jh71x0_clkc_fixed_factor_get_parent_rate(struct clk *clk)
{
	struct clk *clk_parent = clk_get_parent(clk);
	if (clk_parent == NULL)
		return 0;

	return clk_get_rate(clk_parent);
}

u_int
jh71x0_clkc_fixed_factor_get_rate(struct jh71x0_clkc_softc *sc,
    struct jh71x0_clkc_clk *jcc)
{
	KASSERT(jcc->jcc_type == JH71X0CLK_FIXED_FACTOR);

	struct jh71x0_clkc_fixed_factor * const jcff = &jcc->jcc_ffactor;
	struct clk *clk = &jcc->jcc_clk;

	uint64_t rate = jh71x0_clkc_fixed_factor_get_parent_rate(clk);
	if (rate == 0)
		return 0;

	rate *= jcff->jcff_mult;
	rate /= jcff->jcff_div;

	return rate;
}

static int
jh71x0_clkc_fixed_factor_set_parent_rate(struct clk *clk, u_int rate)
{
	struct clk *clk_parent = clk_get_parent(clk);
	if (clk_parent == NULL)
		return ENXIO;

	return clk_set_rate(clk_parent, rate);
}

int
jh71x0_clkc_fixed_factor_set_rate(struct jh71x0_clkc_softc *sc,
    struct jh71x0_clkc_clk *jcc, u_int rate)
{
	KASSERT(jcc->jcc_type == JH71X0CLK_FIXED_FACTOR);

	struct jh71x0_clkc_fixed_factor * const jcff = &jcc->jcc_ffactor;
	struct clk *clk = &jcc->jcc_clk;


	uint64_t tmp = rate;
	tmp *= jcff->jcff_div;
	tmp /= jcff->jcff_mult;

	return jh71x0_clkc_fixed_factor_set_parent_rate(clk, tmp);
}

const char *
jh71x0_clkc_fixed_factor_get_parent(struct jh71x0_clkc_softc *sc,
    struct jh71x0_clkc_clk *jcc)
{
	KASSERT(jcc->jcc_type == JH71X0CLK_FIXED_FACTOR);

	struct jh71x0_clkc_fixed_factor * const jcff = &jcc->jcc_ffactor;

	return jcff->jcff_parent;
}


/*
 * MUX operations
 */

int
jh71x0_clkc_mux_set_parent(struct jh71x0_clkc_softc *sc,
    struct jh71x0_clkc_clk *jcc, const char *name)
{
	KASSERT(jcc->jcc_type == JH71X0CLK_MUX);

	struct jh71x0_clkc_mux * const jcm = &jcc->jcc_mux;

	size_t i;
	for (i = 0; i < jcm->jcm_nparents; i++) {
		if (jcm->jcm_parents[i] != NULL &&
		    strcmp(jcm->jcm_parents[i], name) == 0)
			break;
	}
	if (i >= jcm->jcm_nparents)
		return EINVAL;

	KASSERT(i <= __SHIFTOUT_MASK(JH71X0_CLK_MUX_MASK));

	uint32_t val = RD4(sc, jcc->jcc_reg);
	val &= ~JH71X0_CLK_MUX_MASK;
	val |= __SHIFTIN(i, JH71X0_CLK_MUX_MASK);
	WR4(sc, jcc->jcc_reg, val);

	return 0;
}


const char *
jh71x0_clkc_mux_get_parent(struct jh71x0_clkc_softc *sc,
    struct jh71x0_clkc_clk *jcc)
{
	KASSERT(jcc->jcc_type == JH71X0CLK_MUX);

	uint32_t val = RD4(sc, jcc->jcc_reg);
	size_t pindex = __SHIFTOUT(val, JH71X0_CLK_MUX_MASK);

	if (pindex >= jcc->jcc_mux.jcm_nparents)
		return NULL;

	return jcc->jcc_mux.jcm_parents[pindex];
}


/*
 * GATE operations
 */

int
jh71x0_clkc_gate_enable(struct jh71x0_clkc_softc *sc,
    struct jh71x0_clkc_clk *jcc, int enable)
{
	KASSERT(jcc->jcc_type == JH71X0CLK_GATE);

	jh71x0_clkc_update(sc, jcc,
	    (enable ? JH71X0_CLK_ENABLE : 0), JH71X0_CLK_ENABLE);

	return 0;
}

const char *
jh71x0_clkc_gate_get_parent(struct jh71x0_clkc_softc *sc,
    struct jh71x0_clkc_clk *jcc)
{
	KASSERT(jcc->jcc_type == JH71X0CLK_GATE);

	struct jh71x0_clkc_gate *jcc_gate = &jcc->jcc_gate;

	return jcc_gate->jcg_parent;
}


/*
 * DIVIDER operations
 */

u_int
jh71x0_clkc_div_get_rate(struct jh71x0_clkc_softc *sc,
    struct jh71x0_clkc_clk *jcc)
{
	KASSERT(jcc->jcc_type == JH71X0CLK_DIV);

	struct clk * const clk = &jcc->jcc_clk;
	struct clk * const clk_parent = clk_get_parent(clk);

	if (clk_parent == NULL)
		return 0;

	u_int rate = clk_get_rate(clk_parent);
	if (rate == 0)
		return 0;

	uint32_t val = RD4(sc, jcc->jcc_reg);
	uint32_t div = __SHIFTOUT(val, JH71X0_CLK_DIV_MASK);

	return rate / div;
}

int
jh71x0_clkc_div_set_rate(struct jh71x0_clkc_softc *sc,
    struct jh71x0_clkc_clk *jcc, u_int new_rate)
{
	KASSERT(jcc->jcc_type == JH71X0CLK_DIV);

	struct jh71x0_clkc_div * const jcc_div = &jcc->jcc_div;
	struct clk * const clk = &jcc->jcc_clk;
	struct clk * const clk_parent = clk_get_parent(clk);

	if (clk_parent == NULL)
		return ENXIO;

	if (jcc_div->jcd_maxdiv == 0)
		return ENXIO;

	u_int parent_rate = clk_get_rate(clk_parent);
	if (parent_rate == 0) {
		return (new_rate == 0) ? 0 : ERANGE;
	}
	u_int ratio = howmany(parent_rate, new_rate);
	u_int div = uimin(ratio, jcc_div->jcd_maxdiv);

	KASSERT(div <= __SHIFTOUT_MASK(JH71X0_CLK_DIV_MASK));

	jh71x0_clkc_update(sc, jcc,
	    __SHIFTIN(div, JH71X0_CLK_DIV_MASK), JH71X0_CLK_DIV_MASK);

	return 0;
}

const char *
jh71x0_clkc_div_get_parent(struct jh71x0_clkc_softc *sc,
    struct jh71x0_clkc_clk *jcc)
{
	KASSERT(jcc->jcc_type == JH71X0CLK_DIV);

	struct jh71x0_clkc_div *jcc_div = &jcc->jcc_div;

	return jcc_div->jcd_parent;
}


/*
 * FRACTIONAL DIVIDER operations
 */

u_int
jh71x0_clkc_fracdiv_get_rate(struct jh71x0_clkc_softc *sc,
    struct jh71x0_clkc_clk *jcc)
{
	KASSERT(jcc->jcc_type == JH71X0CLK_FRACDIV);

	struct clk * const clk = &jcc->jcc_clk;
	struct clk * const clk_parent = clk_get_parent(clk);

	if (clk_parent == NULL)
		return 0;


	u_int rate = clk_get_rate(clk_parent);
	if (rate == 0)
		return 0;

	uint32_t val = RD4(sc, jcc->jcc_reg);
	unsigned long div100 =
	    100UL * __SHIFTOUT(val, JH71X0_CLK_INT_MASK) +
		    __SHIFTOUT(val, JH71X0_CLK_FRAC_MASK);

	return (div100 >= JH71X0_CLK_FRAC_MIN) ? 100UL * rate / div100 : 0;
}

int
jh71x0_clkc_fracdiv_set_rate(struct jh71x0_clkc_softc *sc,
    struct jh71x0_clkc_clk *jcc, u_int new_rate)
{
	KASSERT(jcc->jcc_type == JH71X0CLK_FRACDIV);

	struct clk * const clk = &jcc->jcc_clk;
	struct clk * const clk_parent = clk_get_parent(clk);

	if (clk_parent == NULL)
		return ENXIO;

#if 0
	if (jcc_div->jcd_maxdiv == 0)
		return ENXIO;

	u_int parent_rate = clk_get_rate(clk_parent);

	if (parent_rate == 0) {
		return (new_rate == 0) ? 0 : ERANGE;
	}
	u_int ratio = howmany(parent_rate, new_rate);
	u_int div = uimin(ratio, jcc_div->jcd_maxdiv);

	KASSERT(div <= __SHIFTOUT_MASK(JH71X0_CLK_DIV_MASK));

	jh71x0_clkc_update(sc, jcc,
	    __SHIFTIN(div, JH71X0_CLK_DIV_MASK), JH71X0_CLK_DIV_MASK);
#endif

	return 0;
}

const char *
jh71x0_clkc_fracdiv_get_parent(struct jh71x0_clkc_softc *sc,
    struct jh71x0_clkc_clk *jcc)
{
	KASSERT(jcc->jcc_type == JH71X0CLK_FRACDIV);

	struct jh71x0_clkc_fracdiv *jcc_fracdiv = &jcc->jcc_fracdiv;

	return jcc_fracdiv->jcd_parent;
}


/*
 * MUXDIV operations
 */


int
jh71x0_clkc_muxdiv_set_parent(struct jh71x0_clkc_softc *sc,
    struct jh71x0_clkc_clk *jcc, const char *name)
{
	KASSERT(jcc->jcc_type == JH71X0CLK_MUXDIV);

	struct jh71x0_clkc_muxdiv * const jcmd = &jcc->jcc_muxdiv;

	size_t i;
	for (i = 0; i < jcmd->jcmd_nparents; i++) {
		if (jcmd->jcmd_parents[i] != NULL &&
		    strcmp(jcmd->jcmd_parents[i], name) == 0)
			break;
	}
	if (i >= jcmd->jcmd_nparents)
		return EINVAL;

	KASSERT(i <= __SHIFTOUT_MASK(JH71X0_CLK_MUX_MASK));

	uint32_t val = RD4(sc, jcc->jcc_reg);
	val &= ~JH71X0_CLK_MUX_MASK;
	val |= __SHIFTIN(i, JH71X0_CLK_MUX_MASK);
	WR4(sc, jcc->jcc_reg, val);

	return 0;
}


const char *
jh71x0_clkc_muxdiv_get_parent(struct jh71x0_clkc_softc *sc,
    struct jh71x0_clkc_clk *jcc)
{
	KASSERT(jcc->jcc_type == JH71X0CLK_MUXDIV);

	uint32_t val = RD4(sc, jcc->jcc_reg);
	size_t pindex = __SHIFTOUT(val, JH71X0_CLK_MUX_MASK);

	if (pindex >= jcc->jcc_muxdiv.jcmd_nparents)
		return NULL;

	return jcc->jcc_muxdiv.jcmd_parents[pindex];
}



u_int
jh71x0_clkc_muxdiv_get_rate(struct jh71x0_clkc_softc *sc,
    struct jh71x0_clkc_clk *jcc)
{
	KASSERT(jcc->jcc_type == JH71X0CLK_MUXDIV);

	struct clk * const clk = &jcc->jcc_clk;
	struct clk * const clk_parent = clk_get_parent(clk);

	if (clk_parent == NULL)
		return 0;

	u_int rate = clk_get_rate(clk_parent);
	if (rate == 0)
		return 0;

	uint32_t val = RD4(sc, jcc->jcc_reg);
	uint32_t div = __SHIFTOUT(val, JH71X0_CLK_DIV_MASK);

	return rate / div;
}

int
jh71x0_clkc_muxdiv_set_rate(struct jh71x0_clkc_softc *sc,
    struct jh71x0_clkc_clk *jcc, u_int new_rate)
{
	KASSERT(jcc->jcc_type == JH71X0CLK_MUXDIV);

	struct jh71x0_clkc_muxdiv * const jcc_muxdiv = &jcc->jcc_muxdiv;
	struct clk * const clk = &jcc->jcc_clk;
	struct clk * const clk_parent = clk_get_parent(clk);

	if (clk_parent == NULL)
		return ENXIO;

	if (jcc_muxdiv->jcmd_maxdiv == 0)
		return ENXIO;

	u_int parent_rate = clk_get_rate(clk_parent);
	if (parent_rate == 0) {
		return (new_rate == 0) ? 0 : ERANGE;
	}
	u_int ratio = howmany(parent_rate, new_rate);
	u_int div = uimin(ratio, jcc_muxdiv->jcmd_maxdiv);

	KASSERT(div <= __SHIFTOUT_MASK(JH71X0_CLK_DIV_MASK));

	jh71x0_clkc_update(sc, jcc,
	    __SHIFTIN(div, JH71X0_CLK_DIV_MASK), JH71X0_CLK_DIV_MASK);

	return 0;
}


/*
 * INV operations
 */
const char *
jh71x0_clkc_inv_get_parent(struct jh71x0_clkc_softc *sc,
    struct jh71x0_clkc_clk *jcc)
{
	KASSERT(jcc->jcc_type == JH71X0CLK_INV);

	struct jh71x0_clkc_inv * const jci = &jcc->jcc_inv;

	return jci->jci_parent;
}


struct jh71x0_clkc_clkops jh71x0_clkc_gate_ops = {
	.jcco_enable = jh71x0_clkc_gate_enable,
	.jcco_getparent = jh71x0_clkc_gate_get_parent,
};

struct jh71x0_clkc_clkops jh71x0_clkc_div_ops = {
	.jcco_setrate = jh71x0_clkc_div_set_rate,
	.jcco_getrate = jh71x0_clkc_div_get_rate,
	.jcco_getparent = jh71x0_clkc_div_get_parent,
};

struct jh71x0_clkc_clkops jh71x0_clkc_fracdiv_ops = {
	.jcco_setrate = jh71x0_clkc_fracdiv_set_rate,
	.jcco_getrate = jh71x0_clkc_fracdiv_get_rate,
	.jcco_getparent = jh71x0_clkc_fracdiv_get_parent,
};

struct jh71x0_clkc_clkops jh71x0_clkc_ffactor_ops = {
	.jcco_setrate = jh71x0_clkc_fixed_factor_set_rate,
	.jcco_getrate = jh71x0_clkc_fixed_factor_get_rate,
	.jcco_getparent = jh71x0_clkc_fixed_factor_get_parent,
};


struct jh71x0_clkc_clkops jh71x0_clkc_mux_ops = {
	.jcco_setparent = jh71x0_clkc_mux_set_parent,
	.jcco_getparent = jh71x0_clkc_mux_get_parent,
};

struct jh71x0_clkc_clkops jh71x0_clkc_muxdiv_ops = {
	.jcco_setrate = jh71x0_clkc_muxdiv_set_rate,
	.jcco_getrate = jh71x0_clkc_muxdiv_get_rate,
	.jcco_setparent = jh71x0_clkc_muxdiv_set_parent,
	.jcco_getparent = jh71x0_clkc_muxdiv_get_parent,
};


struct jh71x0_clkc_clkops jh71x0_clkc_inv_ops = {
	.jcco_getparent = jh71x0_clkc_inv_get_parent,
};

static struct clk *
jh71x0_clkc_get(void *priv, const char *name)
{
	struct jh71x0_clkc_softc * const sc = priv;

	for (u_int id = 0; id < sc->sc_nclks; id++) {
		struct jh71x0_clkc_clk * const jcc = &sc->sc_clk[id];

		if (strcmp(name, jcc->jcc_clk.name) == 0) {
			return &jcc->jcc_clk;
		}
	}

	return NULL;
}

static void
jh71x0_clkc_put(void *priv, struct clk *clk)
{
}


static int
jh71x0_clkc_set_rate(void *priv, struct clk *clk, u_int rate)
{
	struct jh71x0_clkc_softc * const sc = priv;
	struct jh71x0_clkc_clk * const jcc =
	    container_of(clk, struct jh71x0_clkc_clk, jcc_clk);

	if (clk->flags & CLK_SET_RATE_PARENT) {
		struct clk *clk_parent = clk_get_parent(clk);
		if (clk_parent == NULL) {
			aprint_debug("%s: no parent for %s\n", __func__,
			    jcc->jcc_clk.name);
			return ENXIO;
		}
		return clk_set_rate(clk_parent, rate);
	}

	if (jcc->jcc_ops->jcco_setrate)
		return jcc->jcc_ops->jcco_setrate(sc, jcc, rate);

	return ENXIO;
}

static u_int
jh71x0_clkc_get_rate(void *priv, struct clk *clk)
{
	struct jh71x0_clkc_softc * const sc = priv;
	struct jh71x0_clkc_clk * const jcc =
	    container_of(clk, struct jh71x0_clkc_clk, jcc_clk);

	if (jcc->jcc_ops->jcco_getrate)
		return jcc->jcc_ops->jcco_getrate(sc, jcc);

	struct clk * const clk_parent = clk_get_parent(clk);
	if (clk_parent == NULL) {
		aprint_debug("%s: no parent for %s\n", __func__,
		    jcc->jcc_clk.name);
		return 0;
	}

	return clk_get_rate(clk_parent);
}

static int
jh71x0_clkc_enable(void *priv, struct clk *clk)
{
	struct jh71x0_clkc_softc * const sc = priv;
	struct jh71x0_clkc_clk * const jcc =
	    container_of(clk, struct jh71x0_clkc_clk, jcc_clk);

	struct clk * const clk_parent = clk_get_parent(clk);
	if (clk_parent != NULL) {
		int error = clk_enable(clk_parent);
		if (error != 0)
			return error;
	}

	switch (jcc->jcc_type) {
	case JH71X0CLK_GATE:
		jh71x0_clkc_update(sc, jcc, JH71X0_CLK_ENABLE, 0);
		break;

	case JH71X0CLK_DIV: {
		struct jh71x0_clkc_div * const jcc_div = &jcc->jcc_div;
		if (jcc_div->jcd_flags & JH71X0CLKC_DIV_GATE) {
			jh71x0_clkc_update(sc, jcc, JH71X0_CLK_ENABLE, 0);
		}
		break;
	    }

	case JH71X0CLK_MUX: {
		struct jh71x0_clkc_mux * const jcc_mux = &jcc->jcc_mux;
		if (jcc_mux->jcm_flags & JH71X0CLKC_MUX_GATE) {
			jh71x0_clkc_update(sc, jcc, JH71X0_CLK_ENABLE, 0);
		}
		break;
	    }

	case JH71X0CLK_FIXED_FACTOR:
	case JH71X0CLK_INV:
	case JH71X0CLK_MUXDIV:
		break;

	default:
		printf("%s: type %d\n", __func__, jcc->jcc_type);
		return ENXIO;
	}
	return 0;
}

static int
jh71x0_clkc_disable(void *priv, struct clk *clk)
{
	struct jh71x0_clkc_softc * const sc = priv;
	struct jh71x0_clkc_clk * const jcc =
	    container_of(clk, struct jh71x0_clkc_clk, jcc_clk);

	switch (jcc->jcc_type) {
	case JH71X0CLK_GATE:
		jh71x0_clkc_update(sc, jcc, 0, JH71X0_CLK_ENABLE);
		break;

	case JH71X0CLK_DIV: {
		struct jh71x0_clkc_div * const jcc_div = &jcc->jcc_div;
		if (jcc_div->jcd_flags & JH71X0CLKC_DIV_GATE) {
			jh71x0_clkc_update(sc, jcc, 0, JH71X0_CLK_ENABLE);
		}
		break;
	    }

	case JH71X0CLK_MUX: {
		struct jh71x0_clkc_mux * const jcc_mux = &jcc->jcc_mux;
		if (jcc_mux->jcm_flags & JH71X0CLKC_MUX_GATE) {
			jh71x0_clkc_update(sc, jcc, 0, JH71X0_CLK_ENABLE);
		}
		break;
	    }

	case JH71X0CLK_FIXED_FACTOR:
	case JH71X0CLK_INV:
	case JH71X0CLK_MUXDIV:
		break;

	default:
		return ENXIO;
	}
	return 0;
}



static struct jh71x0_clkc_clk *
jh71x0_clkc_clock_find(struct jh71x0_clkc_softc *sc, const char *name)
{
	for (size_t id = 0; id < sc->sc_nclks; id++) {
		struct jh71x0_clkc_clk * const jcc = &sc->sc_clk[id];

		if (jcc->jcc_clk.name == NULL)
			continue;
		if (strcmp(jcc->jcc_clk.name, name) == 0)
			return jcc;
	}

	return NULL;
}



static int
jh71x0_clkc_set_parent(void *priv, struct clk *clk,
    struct clk *clk_parent)
{
	struct jh71x0_clkc_softc * const sc = priv;
	struct jh71x0_clkc_clk * const jcc =
	    container_of(clk, struct jh71x0_clkc_clk, jcc_clk);

	if (jcc->jcc_ops->jcco_setparent == NULL)
		return EINVAL;

	return jcc->jcc_ops->jcco_setparent(sc, jcc, clk_parent->name);
}


static struct clk *
jh71x0_clkc_get_parent(void *priv, struct clk *clk)
{
	struct jh71x0_clkc_softc * const sc = priv;
	struct jh71x0_clkc_clk * const jcc =
	    container_of(clk, struct jh71x0_clkc_clk, jcc_clk);

	if (jcc->jcc_ops->jcco_getparent == NULL)
		return NULL;

	const char *parent = jcc->jcc_ops->jcco_getparent(sc, jcc);
	if (parent == NULL)
		return NULL;

	struct jh71x0_clkc_clk *jcc_parent = jh71x0_clkc_clock_find(sc, parent);
	if (jcc_parent != NULL)
		return &jcc_parent->jcc_clk;

	/* No parent in this domain, try FDT */
	return fdtbus_clock_get(sc->sc_phandle, parent);
}


const struct clk_funcs jh71x0_clkc_funcs = {
	.get = jh71x0_clkc_get,
	.put = jh71x0_clkc_put,
	.set_rate = jh71x0_clkc_set_rate,
	.get_rate = jh71x0_clkc_get_rate,
	.enable = jh71x0_clkc_enable,
	.disable = jh71x0_clkc_disable,
	.set_parent = jh71x0_clkc_set_parent,
	.get_parent = jh71x0_clkc_get_parent,
};

