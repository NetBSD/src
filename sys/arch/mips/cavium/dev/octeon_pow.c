/*	$NetBSD: octeon_pow.c,v 1.10 2020/06/23 05:15:33 simonb Exp $	*/

/*
 * Copyright (c) 2007 Internet Initiative Japan, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon_pow.c,v 1.10 2020/06/23 05:15:33 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <mips/include/locore.h>
#include <mips/cavium/octeonvar.h>
#include <mips/cavium/include/iobusvar.h>
#include <mips/cavium/dev/octeon_powreg.h>
#include <mips/cavium/dev/octeon_powvar.h>

void			octpow_bootstrap(struct octeon_config *);

static void		octpow_init(struct octpow_softc *);
static void		octpow_init_regs(struct octpow_softc *);
static inline void      octpow_config_int(struct octpow_softc *, int,
			    uint64_t, uint64_t, uint64_t);

struct octpow_softc	octpow_softc;

/* -------------------------------------------------------------------------- */

/* ---- initialization and configuration */

void
octpow_bootstrap(struct octeon_config *mcp)
{
	struct octpow_softc *sc = &octpow_softc;

	sc->sc_regt = &mcp->mc_iobus_bust;
	/* XXX */

	octpow_init(sc);
}

static inline void
octpow_config_int(struct octpow_softc *sc, int group, uint64_t tc_thr,
    uint64_t ds_thr, uint64_t iq_thr)
{
	uint64_t wq_int_thr =
	    POW_WQ_INT_THRX_TC_EN |
	    __SHIFTIN(tc_thr, POW_WQ_INT_THRX_TC_THR) |
	    __SHIFTIN(ds_thr, POW_WQ_INT_THRX_DS_THR) |
	    __SHIFTIN(iq_thr, POW_WQ_INT_THRX_IQ_THR);

	_POW_WR8(sc, POW_WQ_INT_THR0_OFFSET + (group * 8), wq_int_thr);
}

/*
 * interrupt threshold configuration
 *
 * => DS / IQ
 *    => ...
 * => time counter threshold
 *    => unit is 1msec
 *    => each group can set timeout
 * => temporary disable bit
 *    => use CIU generic timer
 */

void
octpow_config(struct octpow_softc *sc, int group)
{

	octpow_config_int(sc, group,
	    0x0f,		/* TC */
	    0x00,		/* DS */
	    0x00);		/* IQ */
}

void
octpow_init(struct octpow_softc *sc)
{
	octpow_init_regs(sc);

	sc->sc_int_pc_base = 10000;
	octpow_config_int_pc(sc, sc->sc_int_pc_base);
}

void
octpow_init_regs(struct octpow_softc *sc)
{
	int status;

	status = bus_space_map(sc->sc_regt, POW_BASE, POW_SIZE, 0,
	    &sc->sc_regh);
	if (status != 0)
		panic("can't map %s space", "pow register");
}
