/*	$NetBSD: zs.c,v 1.30.2.2 2007/05/27 14:27:01 ad Exp $	*/

/*-
 * Copyright (c) 1998 Minoura Makoto
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
 * X68k uses one Z8530 built-in. Channel A is for RS-232C serial port;
 * while channel B is dedicated to the mouse.
 * Extra Z8530's can be installed for serial ports.  This driver
 * supports up to 5 chips including the built-in one.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zs.c,v 1.30.2.2 2007/05/27 14:27:01 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/time.h>
#include <sys/syslog.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <arch/x68k/dev/intiovar.h>
#include <machine/z8530var.h>

#include <dev/ic/z8530reg.h>

#include "zsc.h"	/* NZSC */
#include "opt_zsc.h"
#ifndef ZSCN_SPEED
#define ZSCN_SPEED 9600
#endif
#include "zstty.h"


extern void Debugger(void);

/*
 * Some warts needed by z8530tty.c -
 * The default parity REALLY needs to be the same as the PROM uses,
 * or you can not see messages done with printf during boot-up...
 */
int zs_def_cflag = (CREAD | CS8 | HUPCL);
int zscn_def_cflag = (CREAD | CS8 | HUPCL);

/*
 * X68k provides a 5.0 MHz clock to the ZS chips.
 */
#define PCLK	(5 * 1000 * 1000)	/* PCLK pin input clock rate */


/* Default physical addresses. */
#define ZS_MAXDEV 5
static bus_addr_t zs_physaddr[ZS_MAXDEV] = {
	0x00e98000,
	0x00eafc00,
	0x00eafc10,
	0x00eafc20,
	0x00eafc30
};

static u_char zs_init_reg[16] = {
	0,	/* 0: CMD (reset, etc.) */
	0,	/* 1: No interrupts yet. */
	0x70,	/* 2: XXX: IVECT */
	ZSWR3_RX_8 | ZSWR3_RX_ENABLE,
	ZSWR4_CLK_X16 | ZSWR4_ONESB | ZSWR4_EVENP,
	ZSWR5_TX_8 | ZSWR5_TX_ENABLE,
	0,	/* 6: TXSYNC/SYNCLO */
	0,	/* 7: RXSYNC/SYNCHI */
	0,	/* 8: alias for data port */
	ZSWR9_MASTER_IE,
	ZSWR10_NRZ,	/*10: Misc. TX/RX control bits */
	ZSWR11_TXCLK_BAUD | ZSWR11_RXCLK_BAUD,
	((PCLK/32)/9600)-2,	/*12: BAUDLO (default=9600) */
	0,			/*13: BAUDHI (default=9600) */
	ZSWR14_BAUD_ENA | ZSWR14_BAUD_FROM_PCLK,
	ZSWR15_BREAK_IE,
};

static volatile struct zschan *conschan = 0;


/****************************************************************
 * Autoconfig
 ****************************************************************/

/* Definition of the driver for autoconfig. */
static int	zs_match(struct device *, struct cfdata *, void *);
static void	zs_attach(struct device *, struct device *, void *);
static int	zs_print(void *, const char *name);

CFATTACH_DECL(zsc, sizeof(struct zsc_softc),
    zs_match, zs_attach, NULL, NULL);

extern struct cfdriver zsc_cd;

static int zshard(void *);
static int zs_get_speed(struct zs_chanstate *);


/*
 * Is the zs chip present?
 */
static int
zs_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct intio_attach_args *ia = aux;
	struct zsdevice *zsaddr = (void *)ia->ia_addr;
	int i;

	if (strcmp(ia->ia_name, "zsc") != 0)
		return 0;

	for (i = 0; i < ZS_MAXDEV; i++)
		if (zsaddr == (void *)zs_physaddr[i]) /* XXX */
			break;

	ia->ia_size = 8;
	if (intio_map_allocate_region(parent, ia, INTIO_MAP_TESTONLY))
		return 0;

	if (zsaddr != (void *)zs_physaddr[i])
		return 0;
	if (badaddr(INTIO_ADDR(zsaddr)))
		return 0;

	return (1);
}

/*
 * Attach a found zs.
 */
static void
zs_attach(struct device *parent, struct device *self, void *aux)
{
	struct zsc_softc *zsc = (void *)self;
	struct intio_attach_args *ia = aux;
	struct zsc_attach_args zsc_args;
	volatile struct zschan *zc;
	struct zs_chanstate *cs;
	int r, s, zs_unit, channel;

	zs_unit = device_unit(&zsc->zsc_dev);
	zsc->zsc_addr = (void *)ia->ia_addr;

	ia->ia_size = 8;
	r = intio_map_allocate_region(parent, ia, INTIO_MAP_ALLOCATE);
#ifdef DIAGNOSTIC
	if (r)
		panic("zs: intio IO map corruption");
#endif

	printf("\n");

	/*
	 * Initialize software state for each channel.
	 */
	for (channel = 0; channel < 2; channel++) {
		struct device *child;

		zsc_args.channel = channel;
		zsc_args.hwflags = 0;
		cs = &zsc->zsc_cs_store[channel];
		zsc->zsc_cs[channel] = cs;

		simple_lock_init(&cs->cs_lock);
		cs->cs_channel = channel;
		cs->cs_private = NULL;
		cs->cs_ops = &zsops_null;
		cs->cs_brg_clk = PCLK / 16;

		if (channel == 0)
			zc = (volatile void *)INTIO_ADDR(&zsc->zsc_addr->zs_chan_a);
		else
			zc = (volatile void *)INTIO_ADDR(&zsc->zsc_addr->zs_chan_b);
		cs->cs_reg_csr  = &zc->zc_csr;
		cs->cs_reg_data = &zc->zc_data;

		zs_init_reg[2] = ia->ia_intr;
		memcpy(cs->cs_creg, zs_init_reg, 16);
		memcpy(cs->cs_preg, zs_init_reg, 16);

		if (zc == conschan) {
			zsc_args.hwflags |= ZS_HWFLAG_CONSOLE;
			cs->cs_defspeed = zs_get_speed(cs);
			cs->cs_defcflag = zscn_def_cflag;
		} else {
			cs->cs_defspeed = 9600;
			cs->cs_defcflag = zs_def_cflag;
		}

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
			s = splzs();
			zs_write_reg(cs, 9, 0);
			splx(s);
		}

		/*
		 * Look for a child driver for this channel.
		 * The child attach will setup the hardware.
		 */
		child = config_found(self, (void *)&zsc_args, zs_print);
#if ZSTTY > 0
		if (zc == conschan &&
		    ((child && strcmp(child->dv_xname, "zstty0")) ||
		     child == NULL)) /* XXX */
			panic("zs_attach: console device mismatch");
#endif
		if (child == NULL) {
			/* No sub-driver.  Just reset it. */
			u_char reset = (channel == 0) ?
				ZSWR9_A_RESET : ZSWR9_B_RESET;
			s = splzs();
			zs_write_reg(cs,  9, reset);
			splx(s);
		}
	}

	/*
	 * Now safe to install interrupt handlers.
	 */
	if (intio_intr_establish(ia->ia_intr, "zs", zshard, zsc))
		panic("zs_attach: interrupt vector busy");
	zsc->zsc_softintr_cookie = softintr_establish(IPL_SOFTSERIAL,
	    (void (*)(void *))zsc_intr_soft, zsc);
	/* XXX; evcnt_attach() ? */

	/*
	 * Set the master interrupt enable and interrupt vector.
	 * (common to both channels, do it on A)
	 */
	cs = zsc->zsc_cs[0];
	s = splzs();
	/* interrupt vector */
	zs_write_reg(cs, 2, ia->ia_intr);
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
 * For x68k-port, we don't use autovectored interrupt.
 * We do not need to look at all of the zs chips.
 */
static int
zshard(void *arg)
{
	struct zsc_softc *zsc = arg;
	int rval;
	int s;

	/*
	 * Actually, zs hardware ipl is 5.
	 * Here we disable all interrupts to shorten the zshard
	 * handling time.  Otherwise, too many characters are
	 * dropped.
	 */
	s = splhigh();
	rval = zsc_intr_hard(zsc);

	/* We are at splzs here, so no need to lock. */
	if (zsc->zsc_cs[0]->cs_softreq || zsc->zsc_cs[1]->cs_softreq)
		softintr_schedule(zsc->zsc_softintr_cookie);

	return (rval);
}

/*
 * Compute the current baud rate given a ZS channel.
 */
static int
zs_get_speed(struct zs_chanstate *cs)
{
	int tconst;

	tconst = zs_read_reg(cs, 12);
	tconst |= zs_read_reg(cs, 13) << 8;
	return (TCONST_TO_BPS(cs->cs_brg_clk, tconst));
}

/*
 * MD functions for setting the baud rate and control modes.
 */
int
zs_set_speed(struct zs_chanstate *cs, int bps	/* bits per second */)
{
	int tconst, real_bps;

	if (bps == 0)
		return (0);

#ifdef	DIAGNOSTIC
	if (cs->cs_brg_clk == 0)
		panic("zs_set_speed");
#endif

	tconst = BPS_TO_TCONST(cs->cs_brg_clk, bps);
	if (tconst < 0)
		return (EINVAL);

	/* Convert back to make sure we can do it. */
	real_bps = TCONST_TO_BPS(cs->cs_brg_clk, tconst);

#if 0				/* XXX */
	/* XXX - Allow some tolerance here? */
	if (real_bps != bps)
		return (EINVAL);
#else
	/*
	 * Since our PCLK has somewhat strange value,
	 * we have to allow tolerance here.
	 */
	if (BPS_TO_TCONST(cs->cs_brg_clk, real_bps) != tconst)
		return (EINVAL);
#endif

	cs->cs_preg[12] = tconst;
	cs->cs_preg[13] = tconst >> 8;

	/* Caller will stuff the pending registers. */
	return (0);
}

int
zs_set_modes(struct zs_chanstate *cs, int cflag	/* bits per second */)
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
	return (0);
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


static struct zs_chanstate zscn_cs;

/****************************************************************
 * Console support functions (x68k specific!)
 * Note: this code is allowed to know about the layout of
 * the chip registers, and uses that to keep things simple.
 * XXX - I think I like the mvme167 code better. -gwr
 ****************************************************************/

/*
 * Handle user request to enter kernel debugger.
 */
void
zs_abort(struct zs_chanstate *cs)
{
	int rr0;

	/* Wait for end of break to avoid PROM abort. */
	/* XXX - Limit the wait? */
	do {
		rr0 = *cs->cs_reg_csr;
		ZS_DELAY();
	} while (rr0 & ZSRR0_BREAK);

#ifdef DDB
	Debugger();
#else
	printf("BREAK!!\n");
#endif
}


#if NZSTTY > 0

#include <dev/cons.h>
cons_decl(zs);

static int zs_getc(void);
static void zs_putc(int);

/*
 * Polled input char.
 */
static int
zs_getc(void)
{
	int s, c, rr0;

	s = splzs();
	/* Wait for a character to arrive. */
	do {
		rr0 = zs_read_csr(&zscn_cs);
	} while ((rr0 & ZSRR0_RX_READY) == 0);

	c = zs_read_data(&zscn_cs);
	splx(s);

	/*
	 * This is used by the kd driver to read scan codes,
	 * so don't translate '\r' ==> '\n' here...
	 */
	return (c);
}

/*
 * Polled output char.
 */
static void
zs_putc(int c)
{
	int s, rr0;

	s = splzs();
	/* Wait for transmitter to become ready. */
	do {
		rr0 = zs_read_csr(&zscn_cs);
	} while ((rr0 & ZSRR0_TX_READY) == 0);

	zs_write_data(&zscn_cs, c);
	splx(s);
}

void
zscninit(struct consdev *cn)
{
	volatile struct zschan *cnchan = (volatile void *)INTIO_ADDR(ZSCN_PHYSADDR);
	int s;

	memset(&zscn_cs, 0, sizeof(struct zs_chanstate));
	zscn_cs.cs_reg_csr = &cnchan->zc_csr;
	zscn_cs.cs_reg_data = &cnchan->zc_data;
	zscn_cs.cs_channel = 0;
	zscn_cs.cs_brg_clk = PCLK / 16;
	memcpy(zscn_cs.cs_preg, zs_init_reg, 16);
	zscn_cs.cs_preg[4] = ZSWR4_CLK_X16 | ZSWR4_ONESB; /* XXX */
	zscn_cs.cs_preg[9] = 0;
	zs_set_speed(&zscn_cs, ZSCN_SPEED);
	s = splzs();
	zs_loadchannelregs(&zscn_cs);
	splx(s);
	conschan = cnchan;
}

/*
 * Polled console input putchar.
 */
int
zscngetc(dev_t dev)
{
	return (zs_getc());
}

/*
 * Polled console output putchar.
 */
void
zscnputc(dev_t dev, int c)
{
	zs_putc(c);
}

void
zscnprobe(struct consdev *cd)
{
	int maj;
	extern const struct cdevsw zstty_cdevsw;

	/* locate the major number */
	maj = cdevsw_lookup_major(&zstty_cdevsw);
	/* XXX: minor number is 0 */

	if (maj == -1)
		cd->cn_pri = CN_DEAD;
	else {
#ifdef ZSCONSOLE
		cd->cn_pri = CN_REMOTE;	/* higher than ITE (CN_INTERNAL) */
#else
		cd->cn_pri = CN_NORMAL;
#endif
		cd->cn_dev = makedev(maj, 0);
	}
}

void
zscnpollc(dev_t dev, int on)
{
}

#endif
