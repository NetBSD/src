/*	$NetBSD: zs.c,v 1.26.8.1 2012/10/30 17:20:06 yamt Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
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

/*
 * Zilog Z8530 Dual UART driver (machine-dependent part)
 *
 * Runs two serial lines per chip using slave drivers.
 * Plain tty/async lines use the zs_async slave.
 * Sun keyboard/mouse uses the zs_kbd/zs_ms slaves.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zs.c,v 1.26.8.1 2012/10/30 17:20:06 yamt Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/tty.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/intr.h>

#include <machine/adrsmap.h>
#include <machine/z8530var.h>

#include <dev/ic/z8530reg.h>

#include "ioconf.h"

void (*zs_delay)(void);

#define ZS_DELAY() (*zs_delay)()

/*
 * Some warts needed by z8530tty.c -
 * The default parity REALLY needs to be the same as the PROM uses,
 * or you can not see messages done with printf during boot-up...
 */
int zs_def_cflag = (CREAD | CS8 | HUPCL);

int
zs_print(void *aux, const char *name)
{
	struct zsc_attach_args *args = aux;

	if (name != NULL)
		aprint_normal("%s: ", name);

	if (args->channel != -1)
		aprint_normal(" channel %d", args->channel);

	return UNCONF;
}

/*
 * Our ZS chips all share a common interrupt level,
 * but we establish zshard handler per each ZS chips
 * to avoid holding unnecessary locks in interrupt context.
 */
int
zshard(void *arg)
{
	struct zsc_softc *zsc = arg;
	int rval;

	rval = zsc_intr_hard(zsc);
	if (zsc->zsc_cs[0]->cs_softreq || zsc->zsc_cs[1]->cs_softreq)
		softint_schedule(zsc->zsc_si);

	return rval;
}

/*
 * Compute the current baud rate given a ZS channel.
 */
int
zs_get_speed(struct zs_chanstate *cs)
{
	int tconst;

	tconst = zs_read_reg(cs, 12);
	tconst |= zs_read_reg(cs, 13) << 8;
	return TCONST_TO_BPS(cs->cs_brg_clk, tconst);
}

/*
 * MD functions for setting the baud rate and control modes.
 */
int
zs_set_speed(struct zs_chanstate *cs, int bps)
{
	int tconst, real_bps;

	if (bps == 0)
		return 0;

#ifdef	DIAGNOSTIC
	if (cs->cs_brg_clk == 0)
		panic("zs_set_speed");
#endif

	tconst = BPS_TO_TCONST(cs->cs_brg_clk, bps);
	if (tconst < 0)
		return EINVAL;

	/* Convert back to make sure we can do it. */
	real_bps = TCONST_TO_BPS(cs->cs_brg_clk, tconst);

	/* XXX - Allow some tolerance here? */
	if (real_bps != bps)
		return EINVAL;

	cs->cs_preg[12] = tconst;
	cs->cs_preg[13] = tconst >> 8;

	/* Caller will stuff the pending registers. */
	return 0;
}

int
zs_set_modes(struct zs_chanstate *cs, int cflag)
{
	int s;

	/*
	 * Output hardware flow control on the chip is horrendous:
	 * if carrier detect drops, the receiver is disabled, and if
	 * CTS drops, the transmitter is stoped IN MID CHARACTER!
	 * Therefore, NEVER set the HFC bit, and instead use the
	 * status interrupt to detect CTS changes.
	 */
	s = splserial();
	cs->cs_rr0_pps = 0;
	if ((cflag & (CLOCAL | MDMBUF)) != 0) {
		cs->cs_rr0_dcd = 0;
		if ((cflag & MDMBUF) == 0)
			cs->cs_rr0_pps = ZSRR0_DCD;
	} else
		cs->cs_rr0_dcd = ZSRR0_DCD;
	if ((cflag & CRTSCTS) != 0) {
		cs->cs_wr5_dtr = ZSWR5_DTR;
		cs->cs_wr5_rts = ZSWR5_RTS;
		cs->cs_rr0_cts = ZSRR0_CTS;
	} else if ((cflag & MDMBUF) != 0) {
		cs->cs_wr5_dtr = 0;
		cs->cs_wr5_rts = ZSWR5_DTR;
		cs->cs_rr0_cts = ZSRR0_DCD;
	} else {
		cs->cs_wr5_dtr = ZSWR5_DTR | ZSWR5_RTS;
		cs->cs_wr5_rts = 0;
		cs->cs_rr0_cts = 0;
	}
	splx(s);

	/* Caller will stuff the pending registers. */
	return 0;
}

/*
 * Read or write the chip with suitable delays.
 */

uint8_t
zs_read_reg(struct zs_chanstate *cs, uint8_t reg)
{
	uint8_t val;

	*cs->cs_reg_csr = reg;
	ZS_DELAY();
	val = *cs->cs_reg_csr;
	ZS_DELAY();
	return val;
}

void
zs_write_reg(struct zs_chanstate *cs, uint8_t reg, uint8_t val)
{

	*cs->cs_reg_csr = reg;
	ZS_DELAY();
	*cs->cs_reg_csr = val;
	ZS_DELAY();
}

uint8_t
zs_read_csr(struct zs_chanstate *cs)
{
	uint8_t val;

	val = *cs->cs_reg_csr;
	ZS_DELAY();
	return val;
}

void
zs_write_csr(struct zs_chanstate *cs, uint8_t val)
{

	*cs->cs_reg_csr = val;
	ZS_DELAY();
}

uint8_t
 zs_read_data(struct zs_chanstate *cs)
{
	uint8_t val;

	val = *cs->cs_reg_data;
	ZS_DELAY();
	return val;
}

void
zs_write_data(struct zs_chanstate *cs, uint8_t val)
{

	*cs->cs_reg_data = val;
	ZS_DELAY();
}

void
zs_abort(struct zs_chanstate *cs)
{

#ifdef DDB
	Debugger();
#endif
}
