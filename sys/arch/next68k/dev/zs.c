/*	$NetBSD: zs.c,v 1.14 2001/05/13 16:55:38 chs Exp $	*/

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
 * Sun keyboard/mouse uses the zs_kbd/zs_ms slaves.
 */

/* This was snarfed from the netbsd sparc/dev/zs.c at version 1.56
 * and then updated to reflect changes in 1.59
 * by Darrin B Jewell <jewell@mit.edu>  Mon Mar 30 20:24:46 1998
 */

#include "opt_ddb.h"
#include "opt_serial.h"

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

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/psl.h>

#include <dev/cons.h>

#include <dev/ic/z8530reg.h>
#include <machine/z8530var.h>

#include <next68k/next68k/isr.h>
#include <next68k/dev/zs_cons.h>

#include "zsc.h" 	/* NZSC */

#if (NZSC < 0)
#error "No serial controllers?"
#endif

/*
 * Some warts needed by z8530tty.c -
 * The default parity REALLY needs to be the same as the PROM uses,
 * or you can not see messages done with printf during boot-up...
 */
int zs_def_cflag = (CREAD | CS8 | HUPCL);
int zs_major = 12;

/*
 * The NeXT provides a 3.686400 MHz clock to the ZS chips.
 */
#define PCLK	(9600 * 384)		/* PCLK pin input clock rate */

#define	ZS_DELAY()		delay(2)

/* The layout of this is hardware-dependent (padding, order). */
struct zschan {
	volatile u_char	zc_csr;		/* ctrl,status, and indirect access */
	u_char		zc_xxx0;
	volatile u_char	zc_data;	/* data */
	u_char		zc_xxx1;
};

static char *zsaddr[NZSC];

/* Flags from cninit() */
static int zs_hwflags[NZSC][2];

/* Default speed for each channel */
static int zs_defspeed[NZSC][2] = {
	{ 9600, 	/* ttya */
	  9600 },	/* ttyb */
};

static u_char zs_init_reg[16] = {
	0,	/* 0: CMD (reset, etc.) */
	0,	/* 1: No interrupts yet. */
	0x18 + NEXT_I_IPL(NEXT_I_SCC),	/* 2: IVECT */
	ZSWR3_RX_8 | ZSWR3_RX_ENABLE,
	ZSWR4_CLK_X16 | ZSWR4_ONESB | ZSWR4_EVENP,
	ZSWR5_TX_8 | ZSWR5_TX_ENABLE,
	0,	/* 6: TXSYNC/SYNCLO */
	0,	/* 7: RXSYNC/SYNCHI */
	0,	/* 8: alias for data port */
	ZSWR9_MASTER_IE,
	0,	/*10: Misc. TX/RX control bits */
	ZSWR11_TXCLK_BAUD | ZSWR11_RXCLK_BAUD,
	((PCLK/32)/9600)-2,	/*12: BAUDLO (default=9600) */
	0,			/*13: BAUDHI (default=9600) */
	ZSWR14_BAUD_ENA | ZSWR14_BAUD_FROM_PCLK,
	ZSWR15_BREAK_IE,
};

struct zschan *
zs_get_chan_addr(zs_unit, channel)
	int zs_unit, channel;
{
	char *addr;
	struct zschan *zc;

	if (zs_unit >= NZSC)
		return (NULL);
	addr = zsaddr[zs_unit];
	if (addr == NULL)
		return (NULL);
	if (channel == 0) {
		/* handle the fact the ports are intertwined. */
		zc = (struct zschan *)(addr+1);
	} else {
		zc = (struct zschan *)(addr);
	}
	return (zc);
}


/****************************************************************
 * Autoconfig
 ****************************************************************/

/* Definition of the driver for autoconfig. */
static int	zs_match __P((struct device *, struct cfdata *, void *));
static void	zs_attach __P((struct device *, struct device *, void *));
static int  zs_print __P((void *, const char *name));

extern int  zs_getc __P((void *arg));
extern void zs_putc __P((void *arg, int c));

struct cfattach zsc_ca = {
	sizeof(struct zsc_softc), zs_match, zs_attach
};

extern struct cfdriver zsc_cd;

/* Interrupt handlers. */
static int zshard __P((void *));
static void zssoft __P((void *));

static int zs_get_speed __P((struct zs_chanstate *));


/*
 * Is the zs chip present?
 */
static int
zs_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return(1);
}

/*
 * Attach a found zs.
 *
 * USE ROM PROPERTIES port-a-ignore-cd AND port-b-ignore-cd FOR
 * SOFT CARRIER, AND keyboard PROPERTY FOR KEYBOARD/MOUSE?
 */
static void
zs_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct zsc_softc *zsc = (void *) self;
	struct zsc_attach_args zsc_args;
	volatile struct zschan *zc;
	struct zs_chanstate *cs;
	int s, zs_unit, channel;

	printf("\n");

	zs_unit = zsc->zsc_dev.dv_unit;

	if (zs_unit == 0) {
		zsaddr[0] = (void *)IIOV(NEXT_P_SCC);
	}

	if (zsaddr[zs_unit] == NULL)
		panic("zs_attach: zs%d not mapped\n", zs_unit);

	/*
	 * Initialize software state for each channel.
	 */
	for (channel = 0; channel < 2; channel++) {
		zsc_args.channel = channel;
		zsc_args.hwflags = zs_hwflags[zs_unit][channel];
		cs = &zsc->zsc_cs_store[channel];
		zsc->zsc_cs[channel] = cs;

		cs->cs_channel = channel;
		cs->cs_private = NULL;
		cs->cs_ops = &zsops_null;
		cs->cs_brg_clk = PCLK / 16;

		zc = zs_get_chan_addr(zs_unit, channel);
		cs->cs_reg_csr  = &zc->zc_csr;
		cs->cs_reg_data = &zc->zc_data;

		bcopy(zs_init_reg, cs->cs_creg, 16);
		bcopy(zs_init_reg, cs->cs_preg, 16);

		/* XXX: Get these from the PROM properties! */
		/* XXX: See the mvme167 code.  Better. */
		if (zsc_args.hwflags & ZS_HWFLAG_CONSOLE)
			cs->cs_defspeed = zs_get_speed(cs);
		else
			cs->cs_defspeed = zs_defspeed[zs_unit][channel];
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
			u_char reset = (channel == 0) ?
				ZSWR9_A_RESET : ZSWR9_B_RESET;
			s = splzs();
			zs_write_reg(cs,  9, reset);
			splx(s);
		}
	}

	isrlink_autovec(zshard, NULL, NEXT_I_IPL(NEXT_I_SCC), 0);
	INTR_ENABLE(NEXT_I_SCC);

	{
		int sir;
		sir = allocate_sir(zssoft, zsc);
		if (sir != SIR_SERIAL) {
			panic("Unexpected zssoft sir");
		}
	}

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
zs_print(aux, name)
	void *aux;
	const char *name;
{
	struct zsc_attach_args *args = aux;

	if (name != NULL)
		printf("%s: ", name);

	if (args->channel != -1)
		printf(" channel %d", args->channel);

	return (UNCONF);
}

static volatile int zssoftpending;

/*
 * Our ZS chips all share a common, autovectored interrupt,
 * so we have to look at all of them on each interrupt.
 */
static int
zshard(arg)
	void *arg;
{
	register struct zsc_softc *zsc;
	register int unit, rr3, rval, softreq;
  if (!INTR_OCCURRED(NEXT_I_SCC)) return 0;

	rval = softreq = 0;
	for (unit = 0; unit < zsc_cd.cd_ndevs; unit++) {
		zsc = zsc_cd.cd_devs[unit];
		if (zsc == NULL)
			continue;
		rr3 = zsc_intr_hard(zsc);
		/* Count up the interrupts. */
		if (rr3) {
			rval |= rr3;
			zsc->zsc_intrcnt.ev_count++;
		}
		softreq |= zsc->zsc_cs[0]->cs_softreq;
		softreq |= zsc->zsc_cs[1]->cs_softreq;
	}

	/* We are at splzs here, so no need to lock. */
	if (softreq && (zssoftpending == 0)) {
		zssoftpending = 1;
		setsoftserial();
	}
	return(1);
}

/*
 * Similar scheme as for zshard (look at all of them)
 */
static void
zssoft(arg)
	void *arg;
{
	register struct zsc_softc *zsc;
	register int s, unit;

	/* This is not the only ISR on this IPL. */
	if (zssoftpending == 0)
		panic("zssoft not pending");

	/*
	 * The soft intr. bit will be set by zshard only if
	 * the variable zssoftpending is zero.  The order of
	 * these next two statements prevents our clearing
	 * the soft intr bit just after zshard has set it.
	 */
	/* ienab_bic(IE_ZSSOFT); */
	zssoftpending = 0;

	/* Make sure we call the tty layer at spltty. */
	s = spltty();
	for (unit = 0; unit < zsc_cd.cd_ndevs; unit++) {
		zsc = zsc_cd.cd_devs[unit];
		if (zsc == NULL)
			continue;
		(void)zsc_intr_soft(zsc);
	}
	splx(s);
}


/*
 * Compute the current baud rate given a ZS channel.
 */
static int
zs_get_speed(cs)
	struct zs_chanstate *cs;
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
zs_set_speed(cs, bps)
	struct zs_chanstate *cs;
	int bps;	/* bits per second */
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

	/* XXX - Allow some tolerance here? */
	if (real_bps != bps)
		return (EINVAL);

	cs->cs_preg[12] = tconst;
	cs->cs_preg[13] = tconst >> 8;

	/* Caller will stuff the pending registers. */
	return (0);
}

int
zs_set_modes(cs, cflag)
	struct zs_chanstate *cs;
	int cflag;	/* bits per second */
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
	} else if ((cflag & CDTRCTS) != 0) {
		cs->cs_wr5_dtr = 0;
		cs->cs_wr5_rts = ZSWR5_DTR;
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
zs_read_reg(cs, reg)
	struct zs_chanstate *cs;
	u_char reg;
{
	u_char val;

	*cs->cs_reg_csr = reg;
	ZS_DELAY();
	val = *cs->cs_reg_csr;
	ZS_DELAY();
	return (val);
}

void
zs_write_reg(cs, reg, val)
	struct zs_chanstate *cs;
	u_char reg, val;
{
	*cs->cs_reg_csr = reg;
	ZS_DELAY();
	*cs->cs_reg_csr = val;
	ZS_DELAY();
}

u_char
zs_read_csr(cs)
	struct zs_chanstate *cs;
{
	register u_char val;

	val = *cs->cs_reg_csr;
	ZS_DELAY();
	return (val);
}

void  zs_write_csr(cs, val)
	struct zs_chanstate *cs;
	u_char val;
{
	*cs->cs_reg_csr = val;
	ZS_DELAY();
}

u_char zs_read_data(cs)
	struct zs_chanstate *cs;
{
	register u_char val;

	val = *cs->cs_reg_data;
	ZS_DELAY();
	return (val);
}

void  zs_write_data(cs, val)
	struct zs_chanstate *cs;
	u_char val;
{
	*cs->cs_reg_data = val;
	ZS_DELAY();
}

/****************************************************************
 * Console support functions (Sun specific!)
 * Note: this code is allowed to know about the layout of
 * the chip registers, and uses that to keep things simple.
 * XXX - I think I like the mvme167 code better. -gwr
 ****************************************************************/

extern void Debugger __P((void));
void *zs_conschan;
int	zs_consunit = 0;

/*
 * Handle user request to enter kernel debugger.
 */
void
zs_abort(cs)
	struct zs_chanstate *cs;
{
#if defined(ZS_CONSOLE_ABORT)
	register volatile struct zschan *zc = zs_conschan;
	int rr0;

	/* Wait for end of break to avoid PROM abort. */
	/* XXX - Limit the wait? */
	do {
		rr0 = zc->zc_csr;
		ZS_DELAY();
	} while (rr0 & ZSRR0_BREAK);

#if defined(KGDB)
	zskgdb(cs);
#elif defined(DDB)
	Debugger();
#else
	/* XXX eventually, drop into next rom monitor here */
	printf("stopping on keyboard abort not supported without DDB or KGDB\n");
#endif
#else /* !ZS_CONSOLE_ABORT */
	return;
#endif
}

/*
 * Polled input char.
 */
int
zs_getc(arg)
	void *arg;
{
	register volatile struct zschan *zc = arg;
	register int s, c, rr0;

	s = splhigh();
	/* Wait for a character to arrive. */
	do {
		rr0 = zc->zc_csr;
		ZS_DELAY();
	} while ((rr0 & ZSRR0_RX_READY) == 0);

	c = zc->zc_data;
	ZS_DELAY();
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
void
zs_putc(arg, c)
	void *arg;
	int c;
{
	register volatile struct zschan *zc = arg;
	register int s, rr0;

	s = splhigh();
	/* Wait for transmitter to become ready. */
	do {
		rr0 = zc->zc_csr;
		ZS_DELAY();
	} while ((rr0 & ZSRR0_TX_READY) == 0);


	zc->zc_data = c;
	ZS_DELAY();

	splx(s);
}

/*****************************************************************/

void zscninit __P((struct consdev *));
int  zscngetc __P((dev_t));
void zscnputc __P((dev_t, int));
void zscnprobe __P((struct consdev *));
extern int	zsopen __P(( dev_t dev, int flags, int mode, struct proc *p));

void
zscnprobe(cp)
	struct consdev * cp;
{
  int     maj;
  for (maj = 0; maj < nchrdev; maj++) {
    if (cdevsw[maj].d_open == zsopen) {
      break;
    }
  }
  if (maj != nchrdev) {
#ifdef SERCONSOLE
    cp->cn_pri = CN_REMOTE;
#else
    cp->cn_pri = CN_NORMAL;		 /* Lower than CN_INTERNAL */
#endif
    zs_major = maj;
    zs_consunit = 0;
    zsaddr[0] = (void *)IIOV(NEXT_P_SCC);
    cp->cn_dev = makedev(maj, zs_consunit);
    zs_conschan = zs_get_chan_addr(0, zs_consunit);
  } else {
    cp->cn_pri = CN_DEAD;
  }
}


void
zscninit(cn)
	struct consdev *cn;
{
	zs_hwflags[0][zs_consunit] = ZS_HWFLAG_CONSOLE;

	{
		struct zs_chanstate xcs;
		struct zs_chanstate *cs;
		volatile struct zschan *zc;
		int    tconst, s;

		/* Setup temporary chanstate. */
		bzero((caddr_t)&xcs, sizeof(xcs));
		cs = &xcs;
		zc = zs_conschan;
		cs->cs_reg_csr  = &zc->zc_csr;
		cs->cs_reg_data = &zc->zc_data;
		cs->cs_channel = zs_consunit;
		cs->cs_brg_clk = PCLK / 16;

		bcopy(zs_init_reg, cs->cs_preg, 16);
		cs->cs_preg[5] |= ZSWR5_DTR | ZSWR5_RTS;
		cs->cs_preg[15] = ZSWR15_BREAK_IE;

		tconst = BPS_TO_TCONST(cs->cs_brg_clk,
				zs_defspeed[0][zs_consunit]);
		cs->cs_preg[12] = tconst;
		cs->cs_preg[13] = tconst >> 8;
		/* can't use zs_set_speed as we haven't set up the
		 * signal sources, and it's not worth it for now 
		 */

		cs->cs_preg[9] &= ~ZSWR9_MASTER_IE;
		/* no interrupts until later, after attach. */

		s = splhigh();
		zs_loadchannelregs(cs);
		splx(s);
	}

	printf("\nNetBSD/next68k console\n");
}

/*
 * Polled console input putchar.
 */
int
zscngetc(dev)
	dev_t dev;
{
	return (zs_getc(zs_conschan));
}

/*
 * Polled console output putchar.
 */
void
zscnputc(dev, c)
	dev_t dev;
	int c;
{
	zs_putc(zs_conschan, c);
}

/*****************************************************************/
