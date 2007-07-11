/*	$NetBSD: zs.c,v 1.24.8.1 2007/07/11 20:01:02 mjf Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * news68k/dev/zs.c - based on {newsmips,x68k,mvme68k}/dev/zs.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zs.c,v 1.24.8.1 2007/07/11 20:01:02 mjf Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/tty.h>

#include <machine/cpu.h>
#include <machine/z8530var.h>

#include <dev/cons.h>
#include <dev/ic/z8530reg.h>

#include <news68k/dev/hbvar.h>

#include "ioconf.h"

int  zs_getc(void *);
void zs_putc(void *, int);

/*
 * Some warts needed by z8530tty.c -
 * The default parity REALLY needs to be the same as the PROM uses,
 * or you can not see messages done with printf during boot-up...
 */
int zs_def_cflag = (CREAD | CS8 | HUPCL);

/*
 * The news68k machines use three different clocks for the ZS chips.
 */
#define NPCLK	3
#define PCLK0	(9600 * 416)	/*  news1700: 3.9936MHz */
#define PCLK1	(9600 * 512)	/*  news1200: 4.9152MHz */
#define PCLK2	(9600 * 384)	/*  external: 3.6864MHz */

static const u_int pclk[NPCLK] = {
	PCLK0,
	PCLK1,
	PCLK2,
};

/*
 * Define interrupt levels.
 */
#define ZSHARD_PRI 5
#define ZS_IVECT 64

#define ZS_DELAY() /* delay(2) */

/* The layout of this is hardware-dependent (padding, order). */
struct zschan {
	volatile u_char	zc_csr;		/* ctrl,status, and indirect access */
	volatile u_char	zc_data;	/* data */
};
struct zsdevice {
	/* Yes, they are backwards. */
	struct	zschan zs_chan_b;
	struct	zschan zs_chan_a;
};

/* Default speed for all channels */
static int zs_defspeed = 9600;

/* console status from cninit */
static struct zs_chanstate zs_conschan_store;
static struct zs_chanstate *zs_conschan;
static struct zschan *zc_cons;

static u_char zs_init_reg[16] = {
	0,	/* 0: CMD (reset, etc.) */
	0,	/* 1: No interrupts yet. */
	ZS_IVECT,	/* IVECT */
	ZSWR3_RX_8 | ZSWR3_RX_ENABLE,
	ZSWR4_CLK_X16 | ZSWR4_ONESB | ZSWR4_EVENP,
	ZSWR5_TX_8 | ZSWR5_TX_ENABLE,
	0,	/* 6: TXSYNC/SYNCLO */
	0,	/* 7: RXSYNC/SYNCHI */
	0,	/* 8: alias for data port */
	ZSWR9_MASTER_IE,
	0,	/*10: Misc. TX/RX control bits */
	ZSWR11_TXCLK_BAUD | ZSWR11_RXCLK_BAUD,
	BPS_TO_TCONST((PCLK0/16), 9600), /*12: BAUDLO (default=9600) */
	0,			/*13: BAUDHI (default=9600) */
	ZSWR14_BAUD_ENA | ZSWR14_BAUD_FROM_PCLK,
	ZSWR15_BREAK_IE,
};


/****************************************************************
 * Autoconfig
 ****************************************************************/

/* Definition of the driver for autoconfig. */
static int  zs_match(struct device *, struct cfdata *, void *);
static void zs_attach(struct device *, struct device *, void *);
static int  zs_print(void *, const char *name);

CFATTACH_DECL(zsc, sizeof(struct zsc_softc),
    zs_match, zs_attach, NULL, NULL);

static int zshard(void *);
#if 0
static int zs_get_speed(struct zs_chanstate *);
#endif

/*
 * Is the zs chip present?
 */
static int
zs_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct hb_attach_args *ha = aux;
	u_int addr;

	if (strcmp(ha->ha_name, "zsc"))
		return 0;

	/* XXX no default address */
	if (ha->ha_address == (u_int)-1)
		return 0;

	addr = IIOV(ha->ha_address);
	/* This returns -1 on a fault (bus error). */
	if (badaddr((void *)addr, 1))
		return 0;

	return 1;
}

/*
 * Attach a found zs.
 */
static void
zs_attach(struct device *parent, struct device *self, void *aux)
{
	struct zsc_softc *zsc = (void *) self;
	struct cfdata *cf = device_cfdata(self);
	struct hb_attach_args *ha = aux;
	struct zsc_attach_args zsc_args;
	struct zsdevice *zs;
	struct zschan *zc;
	struct zs_chanstate *cs;
	int s, channel, clk;

	zs = (void *)IIOV(ha->ha_address);

	clk = cf->cf_flags;
	if (clk < 0 || clk >= NPCLK)
		clk = 0;

	printf("\n");

	/*
	 * Initialize software state for each channel.
	 */
	for (channel = 0; channel < 2; channel++) {
		zsc_args.channel = channel;
		cs = &zsc->zsc_cs_store[channel];

		zsc->zsc_cs[channel] = cs;
		zc = (channel == 0) ? &zs->zs_chan_a : &zs->zs_chan_b;

		if (ha->ha_vect != -1)
			zs_init_reg[2] = ha->ha_vect;

		if (zc == zc_cons) {
			memcpy(cs, zs_conschan, sizeof(struct zs_chanstate));
			zs_conschan = cs;
			zsc_args.hwflags = ZS_HWFLAG_CONSOLE;
		} else {
			cs->cs_reg_csr  = &zc->zc_csr;
			cs->cs_reg_data = &zc->zc_data;
			memcpy(cs->cs_creg, zs_init_reg, 16);
			memcpy(cs->cs_preg, zs_init_reg, 16);
			cs->cs_defspeed = zs_defspeed;
			zsc_args.hwflags = 0;
		}

		simple_lock_init(&cs->cs_lock);
		cs->cs_defcflag = zs_def_cflag;

		cs->cs_channel = channel;
		cs->cs_private = NULL;
		cs->cs_ops = &zsops_null;
		cs->cs_brg_clk = pclk[clk] / 16;

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
			u_char reset = (channel == 0) ?
				ZSWR9_A_RESET : ZSWR9_B_RESET;
			s = splhigh();
			zs_write_reg(cs,  9, reset);
			splx(s);
		}
	}

	/*
	 * Now safe to install interrupt handlers.
	 */
	hb_intr_establish(zs_init_reg[2], zshard, ZSHARD_PRI, zsc);
	zsc->zsc_softintr_cookie = softintr_establish(IPL_SOFTSERIAL,
	    (void (*)(void *))zsc_intr_soft, zsc);

	/*
	 * Set the master interrupt enable and interrupt vector.
	 * (common to both channels, do it on A)
	 */
	cs = zsc->zsc_cs[0];
	s = splhigh();
	/* interrupt vector */
	zs_write_reg(cs, 2, zs_init_reg[2]);
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

/*
 * For news68k-port, we don't use autovectored interrupt.
 * We do not need to look at all of the zs chips.
 */
static int
zshard(void *arg)
{
	struct zsc_softc *zsc = arg;
	int rval;

	rval = zsc_intr_hard(zsc);

	/* We are at splzs here, so no need to lock. */
	if (zsc->zsc_cs[0]->cs_softreq || zsc->zsc_cs[1]->cs_softreq) {
		softintr_schedule(zsc->zsc_softintr_cookie);
	}

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

	tconst = zs_read_reg(cs, 12);
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

u_char
zs_read_reg(struct zs_chanstate *cs, u_char reg)
{
	u_char val;

	*cs->cs_reg_csr = reg;
	ZS_DELAY();
	val = *cs->cs_reg_csr;
	ZS_DELAY();
	return val;
}

void
zs_write_reg(struct zs_chanstate *cs, u_char reg, u_char val)
{

	*cs->cs_reg_csr = reg;
	ZS_DELAY();
	*cs->cs_reg_csr = val;
	ZS_DELAY();
}

u_char
zs_read_csr(struct zs_chanstate *cs)
{
	u_char val;

	val = *cs->cs_reg_csr;
	ZS_DELAY();
	return val;
}

void
zs_write_csr(struct zs_chanstate *cs, u_char val)
{

	*cs->cs_reg_csr = val;
	ZS_DELAY();
}

u_char
zs_read_data(struct zs_chanstate *cs)
{
	u_char val;

	val = *cs->cs_reg_data;
	ZS_DELAY();
	return val;
}

void
zs_write_data(struct zs_chanstate *cs, u_char val)
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
	int s, c, rr0;

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
	int s, rr0;

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

/*****************************************************************/

static void zscnprobe(struct consdev *);
static void zscninit(struct consdev *);
static int  zscngetc(dev_t);
static void zscnputc(dev_t, int);

struct consdev consdev_zs = {
	zscnprobe,
	zscninit,
	zscngetc,
	zscnputc,
	nullcnpollc,
	NULL,
	NULL,
	NULL,
	NODEV,
	CN_DEAD
};

static void
zscnprobe(struct consdev *cn)
{
}

static void
zscninit(struct consdev *cn)
{
	struct zs_chanstate *cs;

	extern const struct cdevsw zstty_cdevsw;
	extern int tty00_is_console;
	extern uint32_t sccport0a;

	cn->cn_dev = makedev(cdevsw_lookup_major(&zstty_cdevsw), 0);
	if (tty00_is_console)
		cn->cn_pri = CN_REMOTE;
	else
		cn->cn_pri = CN_NORMAL;

	zc_cons = (struct zschan *)sccport0a; /* XXX */

	zs_conschan = cs = &zs_conschan_store;

	/* Setup temporary chanstate. */
	cs->cs_reg_csr  = &zc_cons->zc_csr;
	cs->cs_reg_data = &zc_cons->zc_data;

	/* Initialize the pending registers. */
	memcpy(cs->cs_preg, zs_init_reg, 16);
	cs->cs_preg[5] |= ZSWR5_DTR | ZSWR5_RTS;

	cs->cs_preg[12] = BPS_TO_TCONST(pclk[systype] / 16, 9600); /* XXX */
	cs->cs_preg[13] = 0;
	cs->cs_defspeed = 9600;

	/* Clear the master interrupt enable. */
	zs_write_reg(cs, 9, 0);

	/* Reset the whole SCC chip. */
	zs_write_reg(cs, 9, ZSWR9_HARD_RESET);

	/* Copy "pending" to "current" and H/W */
	zs_loadchannelregs(cs);
}

static int
zscngetc(dev_t dev)
{

	return zs_getc((void *)zs_conschan);
}

static void
zscnputc(dev_t dev, int c)
{

	zs_putc((void *)zs_conschan, c);
}
