/*	$NetBSD: zs.c,v 1.3.64.1 2015/09/22 12:05:39 skrll Exp $	*/

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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zs.c,v 1.3.64.1 2015/09/22 12:05:39 skrll Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/tty.h>
#include <sys/systm.h>

#include <dev/cons.h>
#include <dev/ic/z8530reg.h>

#include <mips/cpuregs.h>

#include <machine/autoconf.h>
#include <machine/z8530var.h>

#include <cobalt/cobalt/console.h>

#include "ioconf.h"

/*
 * Some warts needed by z8530tty.c -
 * The default parity REALLY needs to be the same as the PROM uses,
 * or you can not see messages done with printf during boot-up...
 */
int zs_def_cflag = (CREAD | CS8 | HUPCL);

#define ZS_DEFSPEED	115200
#define PCLK		(115200 * 96)	/*  11.0592MHz */

#define ZS_DELAY()	delay(2)

/* The layout of this is hardware-dependent (padding, order). */
/* A/~B (Channel A/Channel B) pin is connected to DAdr0 */
#define ZS_CHAN_A	0x01
#define ZS_CHAN_B	0x00

/* D/~C (Data/Control) pin is connected to DAdr1 */
#define ZS_CSR		0x00		/* ctrl, status, and indirect access */
#define ZS_DATA		0x02		/* data */


/* Definition of the driver for autoconfig. */
static int  zs_match(device_t, cfdata_t, void *);
static void zs_attach(device_t, device_t, void *);
static int  zs_print(void *, const char *name);

CFATTACH_DECL_NEW(zsc, sizeof(struct zsc_softc),
    zs_match, zs_attach, NULL, NULL);

static int zshard(void *);
#if 0
static int zs_get_speed(struct zs_chanstate *);
#endif
static int  zs_getc(void *);
static void zs_putc(void *, int);

/* console status from cninit */
static struct zs_chanstate zs_conschan_store;
static struct zs_chanstate *zs_conschan;
static uint8_t *zs_cons;

/* default speed for all channels */
static int zs_defspeed = ZS_DEFSPEED;

static uint8_t zs_init_reg[16] = {
	0,					/* 0: CMD (reset, etc.) */
	0,					/* 1: No interrupts yet. */
	0,					/* 2: no IVECT */
	ZSWR3_RX_8 | ZSWR3_RX_ENABLE,		/* 3: RX params and ctrl */
	ZSWR4_CLK_X16 | ZSWR4_ONESB,		/* 4: TX/RX misc params */
	ZSWR5_TX_8 | ZSWR5_TX_ENABLE,		/* 5: TX params and ctrl */
	0,					/* 6: TXSYNC/SYNCLO */
	0,					/* 7: RXSYNC/SYNCHI */
	0,					/* 8: alias for data port */
	ZSWR9_MASTER_IE,			/* 9: Master interrupt ctrl */
	0,					/*10: Misc TX/RX ctrl */
	ZSWR11_TXCLK_BAUD | ZSWR11_RXCLK_BAUD,	/*11: Clock Mode ctrl */
	BPS_TO_TCONST((PCLK/16), ZS_DEFSPEED),	/*12: BAUDLO */
	0,					/*13: BAUDHI */
	ZSWR14_BAUD_ENA | ZSWR14_BAUD_FROM_PCLK, /*14: Misc ctrl */
	ZSWR15_BREAK_IE,			/*15: Ext/Status intr ctrl */
};

/* register address offset for each channel */
static const int chanoff[] = { ZS_CHAN_A, ZS_CHAN_B };


static int
zs_match(device_t parent, cfdata_t cf, void *aux)
{
	static int matched;

	/* only one zs */
	if (matched)
		return 0;

	/* only Qube 2700 could have Z85C30 serial */
	if (cobalt_id != COBALT_ID_QUBE2700)
		return 0;

	if (!console_present)
		return 0;

	matched = 1;
	return 1;
}

/*
 * Attach a found zs.
 */
static void
zs_attach(device_t parent, device_t self, void *aux)
{
	struct zsc_softc *zsc = device_private(self);
	struct mainbus_attach_args *maa = aux;
	struct zsc_attach_args zsc_args;
	uint8_t *zs_base;
	struct zs_chanstate *cs;
	int s, channel;

	zsc->zsc_dev = self;

	/* XXX: MI z8530 doesn't use bus_space(9) yet */
	zs_base = (void *)MIPS_PHYS_TO_KSEG1(maa->ma_addr);

	aprint_normal(": optional Z85C30 serial port\n");

	/*
	 * Initialize software state for each channel.
	 */
	for (channel = 0; channel < 2; channel++) {
		zsc_args.channel = channel;
		cs = &zsc->zsc_cs_store[channel];

		zsc->zsc_cs[channel] = cs;

		zs_init_reg[2] = 0;

		if ((zs_base + chanoff[channel]) == zs_cons) {
			memcpy(cs, zs_conschan, sizeof(struct zs_chanstate));
			zs_conschan = cs;
			zsc_args.hwflags = ZS_HWFLAG_CONSOLE;
		} else {
			cs->cs_reg_csr  = zs_base + chanoff[channel] + ZS_CSR;
			cs->cs_reg_data = zs_base + chanoff[channel] + ZS_DATA;
			memcpy(cs->cs_creg, zs_init_reg, 16);
			memcpy(cs->cs_preg, zs_init_reg, 16);
			cs->cs_defspeed = zs_defspeed;
			zsc_args.hwflags = 0;
		}

		zs_lock_init(cs);
		cs->cs_defcflag = zs_def_cflag;

		cs->cs_channel = channel;
		cs->cs_private = NULL;
		cs->cs_ops = &zsops_null;
		cs->cs_brg_clk = PCLK / 16;

		/* Make these correspond to cs_defcflag (-crtscts) */
		cs->cs_rr0_dcd = ZSRR0_DCD;
		cs->cs_rr0_cts = 0;
		cs->cs_wr5_dtr = ZSWR5_DTR | ZSWR5_RTS;
		cs->cs_wr5_rts = 0;

		/*
		 * Clear the master interrupt enable.
		 * The INTENA is common to both channels,
		 * so just do it on the A channel.
		 */
		if (channel == 0) {
			s = splhigh();
			zs_write_reg(cs, 9, 0);
			splx(s);
		}

		/*
		 * Look for a child driver for this channel.
		 * The child attach will setup the hardware.
		 */
		if (!config_found(self, (void *)&zsc_args, zs_print)) {
			/* No sub-driver.  Just reset it. */
			uint8_t reset = (channel == 0) ?
			    ZSWR9_A_RESET : ZSWR9_B_RESET;
			s = splhigh();
			zs_write_reg(cs,  9, reset);
			splx(s);
		}
	}

	/*
	 * Now safe to install interrupt handlers.
	 */
	icu_intr_establish(maa->ma_irq, IST_EDGE, IPL_SERIAL, zshard, zsc);
	zsc->zsc_softintr_cookie = softint_establish(SOFTINT_SERIAL,
	    (void (*)(void *))zsc_intr_soft, zsc);

	/*
	 * Set the master interrupt enable and interrupt vector.
	 * (common to both channels, do it on A)
	 */
	cs = zsc->zsc_cs[0];
	s = splhigh();
	/* interrupt vector */
	zs_write_reg(cs, 2, 0);
	/* master interrupt control (enable) */
	zs_write_reg(cs, 9, zs_init_reg[9]);
	splx(s);
}

static int
zs_print(void *aux, const char *name)
{
	struct zsc_attach_args *args = aux;

	if (name != NULL)
		aprint_normal("%s: ", name);

	if (args->channel != -1)
		aprint_normal(" channel %d", args->channel);

	return UNCONF;
}

static int
zshard(void *arg)
{
	struct zsc_softc *zsc = arg;
	int rval;

	rval = zsc_intr_hard(zsc);

#if 1
	/* XXX: there is some race condition? */
	if (rval)
		while (zsc_intr_hard(zsc))
			;
#endif

	/* We are at splzs here, so no need to lock. */
	if (zsc->zsc_cs[0]->cs_softreq || zsc->zsc_cs[1]->cs_softreq)
		softint_schedule(zsc->zsc_softintr_cookie);

	return rval;
}

/*
 * Compute the current baud rate given a ZS channel.
 */
#if 0
static int
zs_get_speed(struct zs_chanstate *cs)
{
	int tconst;

	tconst =  zs_read_reg(cs, 12);
	tconst |= zs_read_reg(cs, 13) << 8;
	return TCONST_TO_BPS(cs->cs_brg_clk, tconst);
}
#endif

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

	/* Allow ~4% tolerance here */
	if (abs(real_bps - bps) >= bps * 4 / 100)
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
	s = splzs();
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

/*
 * Polled input char.
 */
int
zs_getc(void *arg)
{
	struct zs_chanstate *cs = arg;
	int s, c;
	uint8_t rr0;

	s = splhigh();
	/* Wait for a character to arrive. */
	do {
		rr0 = *cs->cs_reg_csr;
		ZS_DELAY();
	} while ((rr0 & ZSRR0_RX_READY) == 0);

	c = *cs->cs_reg_data;
	ZS_DELAY();
	splx(s);

	return c;
}

/*
 * Polled output char.
 */
void
zs_putc(void *arg, int c)
{
	struct zs_chanstate *cs = arg;
	int s;
	uint8_t rr0;

	s = splhigh();
	/* Wait for transmitter to become ready. */
	do {
		rr0 = *cs->cs_reg_csr;
		ZS_DELAY();
	} while ((rr0 & ZSRR0_TX_READY) == 0);

	*cs->cs_reg_data = c;
	ZS_DELAY();
	splx(s);
}

void
zscnprobe(struct consdev *cn)
{

	cn->cn_pri = (console_present != 0 && cobalt_id == COBALT_ID_QUBE2700)
	    ? CN_NORMAL : CN_DEAD;
}

void
zscninit(struct consdev *cn)
{
	struct zs_chanstate *cs;

	extern const struct cdevsw zstty_cdevsw;

	cn->cn_dev = makedev(cdevsw_lookup_major(&zstty_cdevsw), 0);

	zs_cons = (uint8_t *)MIPS_PHYS_TO_KSEG1(ZS_BASE) + ZS_CHAN_A; /* XXX */

	zs_conschan = cs = &zs_conschan_store;

	/* Setup temporary chanstate. */
	cs->cs_reg_csr  = zs_cons + ZS_CSR;
	cs->cs_reg_data = zs_cons + ZS_DATA;

	/* Initialize the pending registers. */
	memcpy(cs->cs_preg, zs_init_reg, 16);
	cs->cs_preg[5] |= ZSWR5_DTR | ZSWR5_RTS;

	cs->cs_preg[12] = BPS_TO_TCONST(PCLK / 16, ZS_DEFSPEED);
	cs->cs_preg[13] = 0;
	cs->cs_defspeed = ZS_DEFSPEED;

	/* Clear the master interrupt enable. */
	zs_write_reg(cs, 9, 0);

	/* Reset the whole SCC chip. */
	zs_write_reg(cs, 9, ZSWR9_HARD_RESET);

	/* Copy "pending" to "current" and H/W */
	zs_loadchannelregs(cs);
}

int
zscngetc(dev_t dev)
{

	return zs_getc((void *)zs_conschan);
}

void
zscnputc(dev_t dev, int c)
{

	zs_putc((void *)zs_conschan, c);
}
