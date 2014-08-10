/*	$NetBSD: zs.c,v 1.24.56.1 2014/08/10 06:54:03 tls Exp $	*/

/*-
 * Copyright (c) 1996, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross and Wayne Knowles
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
__KERNEL_RCSID(0, "$NetBSD: zs.c,v 1.24.56.1 2014/08/10 06:54:03 tls Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"

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
#include <sys/cpu.h>
#include <sys/intr.h>

#include <machine/mainboard.h>
#include <machine/autoconf.h>
#include <machine/prom.h>
#include <machine/z8530var.h>

#include <dev/cons.h>
#include <dev/ic/z8530reg.h>

#include "ioconf.h"
#include "zsc.h"	/* NZSC */
#define NZS NZSC

/* Make life easier for the initialized arrays here. */
#if NZS < 2
#undef  NZS
#define NZS 2
#endif

/*
 * Some warts needed by z8530tty.c -
 * The default parity REALLY needs to be the same as the PROM uses,
 * or you can not see messages done with printf during boot-up...
 */
int zs_def_cflag = (CREAD | CS8 | HUPCL);


#define PCLK		10000000	/* PCLK pin input clock rate */

#ifndef ZS_DEFSPEED
#define ZS_DEFSPEED	9600
#endif

/*
 * Define interrupt levels.
 */
#define ZSHARD_PRI 64

/* Register recovery time is 3.5 to 4 PCLK Cycles */
#define ZS_RECOVERY	1		/* 1us = 10 PCLK Cycles */
#define ZS_DELAY()	delay(ZS_RECOVERY)

/* The layout of this is hardware-dependent (padding, order). */
struct zschan {
	uint8_t pad1[3];
	volatile uint8_t zc_csr;	/* ctrl,status, and indirect access */
	uint8_t   pad2[3];
	volatile uint8_t zc_data;	/* data */
};
struct zsdevice {
	/* Yes, they are backwards. */
	struct	zschan zs_chan_b;
	struct	zschan zs_chan_a;
};

/* Return the byte offset of element within a structure */
#define OFFSET(struct_def, el)		((size_t)&((struct_def *)0)->el)

#define ZS_CHAN_A	OFFSET(struct zsdevice, zs_chan_a)
#define ZS_CHAN_B	OFFSET(struct zsdevice, zs_chan_b)
#define ZS_REG_CSR	OFFSET(struct zschan, zc_csr)
#define ZS_REG_DATA	OFFSET(struct zschan, zc_data)
static int zs_chan_offset[] = {ZS_CHAN_A, ZS_CHAN_B};

/* Flags from cninit() */
static int zs_hwflags[NZS][2];

/* Default speed for all channels */
static int zs_defspeed = ZS_DEFSPEED;
static volatile int zssoftpending;

static uint8_t zs_init_reg[16] = {
	0,				/* 0: CMD (reset, etc.) */
	0,				/* 1: No interrupts yet. */
	ZSHARD_PRI,			/* 2: IVECT */
	ZSWR3_RX_8 | ZSWR3_RX_ENABLE,
	ZSWR4_CLK_X16 | ZSWR4_ONESB,
	ZSWR5_TX_8 | ZSWR5_TX_ENABLE,
	0,				/* 6: TXSYNC/SYNCLO */
	0,				/* 7: RXSYNC/SYNCHI */
	0,				/* 8: alias for data port */
	ZSWR9_MASTER_IE,
	0,				/*10: Misc. TX/RX control bits */
	ZSWR11_TXCLK_BAUD | ZSWR11_RXCLK_BAUD | ZSWR11_TRXC_OUT_ENA,
	BPS_TO_TCONST(PCLK/16, ZS_DEFSPEED), /*12: BAUDLO (default=9600) */
	0,				/*13: BAUDHI (default=9600) */
	ZSWR14_BAUD_ENA | ZSWR14_BAUD_FROM_PCLK,
	ZSWR15_BREAK_IE,
};


/****************************************************************
 * Autoconfig
 ****************************************************************/

/* Definition of the driver for autoconfig. */
static int	zs_match(device_t, cfdata_t, void *);
static void	zs_attach(device_t, device_t, void *);
static int	zs_print(void *, const char *name);

CFATTACH_DECL_NEW(zsc, sizeof(struct zsc_softc),
    zs_match, zs_attach, NULL, NULL);

static int	zshard(void *);
void		zssoft(void *);
static int	zs_get_speed(struct zs_chanstate *);
struct		zschan *zs_get_chan_addr(int zs_unit, int channel);
int		zs_getc(void *);
void		zs_putc(void *, int);

/*
 * Is the zs chip present?
 */
static int
zs_match(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;
	void *va;

	if (strcmp(ca->ca_name, "zsc"))
		return 0;

	va = (void *)cf->cf_addr;

	/* This returns -1 on a fault (bus error). */
	if (badaddr(va, 1))
		return 0;
	return 1;
}

/*
 * Attach a found zs.
 *
 * Match slave number to zs unit number, so that misconfiguration will
 * not set up the keyboard as ttya, etc.
 */
static void
zs_attach(device_t parent, device_t self, void *aux)
{
	struct zsc_softc *zsc = device_private(self);
	struct confargs *ca = aux;
	struct zsc_attach_args zsc_args;
	struct zs_chanstate *cs;
	struct zs_channel *ch;
	int    zs_unit, channel, s;

	zsc->zsc_dev = self;
	zsc->zsc_bustag = ca->ca_bustag;
	if (bus_space_map(ca->ca_bustag, ca->ca_addr,
			  sizeof(struct zsdevice),
			  BUS_SPACE_MAP_LINEAR,
			  &zsc->zsc_base) != 0) {
		aprint_error(": cannot map registers\n");
		return;
	}

	zs_unit = device_unit(self);
	aprint_normal("\n");

	/*
	 * Initialize software state for each channel.
	 */
	for (channel = 0; channel < 2; channel++) {
		zsc_args.channel = channel;
		zsc_args.hwflags = zs_hwflags[zs_unit][channel];
		ch = &zsc->zsc_cs_store[channel];
		cs = zsc->zsc_cs[channel] = (struct zs_chanstate *)ch;

		zs_lock_init(cs);
		cs->cs_reg_csr = NULL;
		cs->cs_reg_data = NULL;
		cs->cs_channel = channel;
		cs->cs_private = NULL;
		cs->cs_ops = &zsops_null;
		cs->cs_brg_clk = PCLK / 16;

		if (bus_space_subregion(ca->ca_bustag, zsc->zsc_base,
					zs_chan_offset[channel],
					sizeof(struct zschan),
					&ch->cs_regs) != 0) {
			aprint_error_dev(self, ": cannot map regs\n");
			return;
		}
		ch->cs_bustag = ca->ca_bustag;

		memcpy(cs->cs_creg, zs_init_reg, 16);
		memcpy(cs->cs_preg, zs_init_reg, 16);

		if (zsc_args.hwflags & ZS_HWFLAG_CONSOLE)
			cs->cs_defspeed = zs_get_speed(cs);
		else
			cs->cs_defspeed = zs_defspeed;
		cs->cs_defcflag = zs_def_cflag;

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
			zs_write_reg(cs, 9, 0);
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


	zsc->sc_si = softint_establish(SOFTINT_SERIAL, zssoft, zsc);
	bus_intr_establish(zsc->zsc_bustag, SYS_INTR_SCC0, 0, 0, zshard, NULL);

	evcnt_attach_dynamic(&zsc->zs_intrcnt, EVCNT_TYPE_INTR, NULL,
			     device_xname(self), "intr");

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
 * Our ZS chips all share a common, autovectored interrupt,
 * so we have to look at all of them on each interrupt.
 */
static int
zshard(void *arg)
{
	struct zsc_softc *zsc;
	int unit, rval, softreq;

	rval = 0;
	for (unit = 0; unit < zsc_cd.cd_ndevs; unit++) {
		zsc = device_lookup_private(&zsc_cd, unit);
		if (zsc == NULL)
			continue;
		rval |= zsc_intr_hard(zsc);
		softreq = zsc->zsc_cs[0]->cs_softreq;
		softreq |= zsc->zsc_cs[1]->cs_softreq;
		if (softreq && (zssoftpending == 0)) {
		    zssoftpending = 1;
		    softint_schedule(zsc->sc_si);
		}
		zsc->zs_intrcnt.ev_count++;
	}
	return rval;
}

/*
 * Similar scheme as for zshard (look at all of them)
 */
void
zssoft(void *arg)
{
	struct zsc_softc *zsc;
	int s, unit;

	/* This is not the only ISR on this IPL. */
	if (zssoftpending == 0)
		return;

	/*
	 * The soft intr. bit will be set by zshard only if
	 * the variable zssoftpending is zero.  The order of
	 * these next two statements prevents our clearing
	 * the soft intr bit just after zshard has set it.
	 */
	/*isr_soft_clear(ZSSOFT_PRI);*/
	zssoftpending = 0;

	/* Make sure we call the tty layer at spltty. */
	s = spltty();
	for (unit = 0; unit < zsc_cd.cd_ndevs; unit++) {
		zsc = device_lookup_private(&zsc_cd, unit);
		if (zsc == NULL)
			continue;
		(void)zsc_intr_soft(zsc);
	}
	splx(s);
	return;
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
zs_set_speed(struct zs_chanstate *cs, int bps)
{
	int tconst;
#if 0
	int real_bps;
#endif

#if 0
	while (!(zs_read_csr(cs) & ZSRR0_TX_READY))
	        {/*nop*/}
#endif
	/* Wait for transmit buffer to empty */
	if (bps == 0) {
		return (0);
	}

#ifdef	DIAGNOSTIC
	if (cs->cs_brg_clk == 0)
		panic("zs_set_speed");
#endif

	tconst = BPS_TO_TCONST(cs->cs_brg_clk, bps);
	if (tconst < 0)
		return (EINVAL);

#if 0
	/* Convert back to make sure we can do it. */
	real_bps = TCONST_TO_BPS(cs->cs_brg_clk, tconst);

	/* XXX - Allow some tolerance here? */
	if (real_bps != bps)
		return (EINVAL);
#endif

	cs->cs_preg[12] = tconst;
	cs->cs_preg[13] = tconst >> 8;

	/* Caller will stuff the pending registers. */
	return (0);
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
	return (0);
}


/*
 * Read or write the chip with suitable delays.
 */

uint8_t
zs_read_reg(struct zs_chanstate *cs, uint8_t reg)
{
	uint8_t val;
	struct zs_channel *zsc = (struct zs_channel *)cs;

	bus_space_write_1(zsc->cs_bustag, zsc->cs_regs, ZS_REG_CSR, reg);
	ZS_DELAY();
	val = bus_space_read_1(zsc->cs_bustag, zsc->cs_regs, ZS_REG_CSR);
	ZS_DELAY();
	return val;
}

void
zs_write_reg(struct zs_chanstate *cs, uint8_t reg, uint8_t val)
{
	struct zs_channel *zsc = (struct zs_channel *)cs;

	bus_space_write_1(zsc->cs_bustag, zsc->cs_regs, ZS_REG_CSR, reg);
	ZS_DELAY();
	bus_space_write_1(zsc->cs_bustag, zsc->cs_regs, ZS_REG_CSR, val);
	ZS_DELAY();
}

uint8_t
zs_read_csr(struct zs_chanstate *cs)
{
	struct zs_channel *zsc = (struct zs_channel *)cs;
	uint8_t val;

	val = bus_space_read_1(zsc->cs_bustag, zsc->cs_regs, ZS_REG_CSR);
	ZS_DELAY();
	return val;
}

void
zs_write_csr(struct zs_chanstate *cs, uint8_t val)
{
	struct zs_channel *zsc = (struct zs_channel *)cs;

	bus_space_write_1(zsc->cs_bustag, zsc->cs_regs, ZS_REG_CSR, val);
	ZS_DELAY();
}

uint8_t
zs_read_data(struct zs_chanstate *cs)
{
	struct zs_channel *zsc = (struct zs_channel *)cs;
	uint8_t val;

	val = bus_space_read_1(zsc->cs_bustag, zsc->cs_regs, ZS_REG_DATA);
	ZS_DELAY();
	return val;
}

void
zs_write_data(struct zs_chanstate *cs, uint8_t val)
{
	struct zs_channel *zsc = (struct zs_channel *)cs;

	bus_space_write_1(zsc->cs_bustag, zsc->cs_regs, ZS_REG_DATA, val);
	ZS_DELAY();
}

void
zs_abort(struct zs_chanstate *cs)
{

#if defined(KGDB)
	zskgdb(cs);
#elif defined(DDB)
	Debugger();
#endif
}


/*********************************************************/
/*  Polled character I/O functions for console and KGDB  */
/*********************************************************/

struct zschan *
zs_get_chan_addr(int zs_unit, int channel)
{
        struct zsdevice *addr;
        struct zschan *zc;

        if (zs_unit >= NZS)
                return NULL;
 
        addr = (struct zsdevice *) ZS0_ADDR;
        
        if (channel == 0) {
                zc = &addr->zs_chan_a;
        } else {
                zc = &addr->zs_chan_b;
        }
        return (zc);
}      

int
zs_getc(void *arg)
{
	volatile struct zschan *zc = arg;
	int s, c;
	uint8_t rr0;

	s = splhigh();
	/* Wait for a character to arrive. */
	do {
		rr0 = zc->zc_csr;
		ZS_DELAY();
	} while ((rr0 & ZSRR0_RX_READY) == 0);

	c = zc->zc_data;
	ZS_DELAY();
	splx(s);

	return (c);
}

/*
 * Polled output char.
 */
void
zs_putc(void *arg, int c)
{
	volatile struct zschan *zc = arg;
	int s;
	uint8_t rr0;

	s = splhigh();
	/* Wait for transmitter to become ready. */
	do {
		rr0 = zc->zc_csr;
		ZS_DELAY();
	} while ((rr0 & ZSRR0_TX_READY) == 0);

	zc->zc_data = c;
	wbflush();
	ZS_DELAY();
	splx(s);
}

/***************************************************************/

static void zscnprobe(struct consdev *);
static void zscninit(struct consdev *);
static int  zscngetc(dev_t);
static void zscnputc(dev_t, int);
static void zscnpollc(dev_t, int);

static int  cons_port;

struct consdev consdev_zs = {
	zscnprobe,
	zscninit,
	zscngetc,
	zscnputc,
	zscnpollc
};

void
zscnprobe(struct consdev *cn)
{
}

void
zscninit(struct consdev *cn)
{
	extern const struct cdevsw zstty_cdevsw;

	cons_port = prom_getconsole(); 
	cn->cn_dev = makedev(cdevsw_lookup_major(&zstty_cdevsw), cons_port);
	cn->cn_pri = CN_REMOTE;
	zs_hwflags[0][cons_port] = ZS_HWFLAG_CONSOLE;
}

int
zscngetc(dev_t dev)
{
	struct zschan *zs;

	zs = zs_get_chan_addr(0, cons_port);
	return zs_getc(zs);
}

void
zscnputc(dev_t dev, int c)
{
	struct zschan *zs;

	zs = zs_get_chan_addr(0, cons_port);
	zs_putc(zs, c);
}

void
zscnpollc(dev_t dev, int on)
{
}
