/*	$NetBSD: zs.c,v 1.72 2003/07/15 03:36:16 lukem Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zs.c,v 1.72 2003/07/15 03:36:16 lukem Exp $");

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

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/mon.h>
#include <machine/z8530var.h>

#include <sun3/sun3/machdep.h>
#ifdef	_SUN3X_
#include <sun3/sun3x/obio.h>
#else
#include <sun3/sun3/obio.h>
#endif
#include <sun3/dev/zs_cons.h>

#include <dev/cons.h>
#include <dev/ic/z8530reg.h>

#include "kbd.h"	/* NKBD */
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

/*
 * The Sun3 provides a 4.9152 MHz clock to the ZS chips.
 */
#define PCLK	(9600 * 512)	/* PCLK pin input clock rate */

/*
 * Define interrupt levels.
 */
#define ZSHARD_PRI	6	/* Wired on the CPU board... */
#define ZSSOFT_PRI	3	/* Want tty pri (4) but this is OK. */

#define ZS_DELAY()			delay(2)

/* The layout of this is hardware-dependent (padding, order). */
struct zschan {
	volatile u_char	zc_csr;		/* ctrl,status, and indirect access */
	u_char		zc_xxx0;
	volatile u_char	zc_data;	/* data */
	u_char		zc_xxx1;
};
struct zsdevice {
	/* Yes, they are backwards. */
	struct	zschan zs_chan_b;
	struct	zschan zs_chan_a;
};


/* Default OBIO addresses. */
static int zs_physaddr[NZS] = {
	OBIO_ZS_KBD_MS,
	OBIO_ZS_TTY_AB };

/* Saved PROM mappings */
static struct zsdevice *zsaddr[NZS];

/* Flags from cninit() */
static int zs_hwflags[NZS][2];

/* Default speed for each channel */
static int zs_defspeed[NZS][2] = {
	{ 1200, 	/* keyboard */
	  1200 },	/* mouse */
	{ 9600, 	/* ttya */
	  9600 },	/* ttyb */
};

static u_char zs_init_reg[16] = {
	0,	/* 0: CMD (reset, etc.) */
	0,	/* 1: No interrupts yet. */
	0x18 + ZSHARD_PRI,	/* IVECT */
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


/* Find PROM mappings (for console support). */
void
zs_init()
{
	int i;

	for (i = 0; i < NZS; i++) {
		zsaddr[i] = (struct zsdevice *)
			obio_find_mapping(zs_physaddr[i], sizeof(struct zschan));
	}
}

struct zschan *
zs_get_chan_addr(zs_unit, channel)
	int zs_unit, channel;
{
	struct zsdevice *addr;
	struct zschan *zc;

	if (zs_unit >= NZS)
		return NULL;
	addr = zsaddr[zs_unit];
	if (addr == NULL)
		return NULL;
	if (channel == 0) {
		zc = &addr->zs_chan_a;
	} else {
		zc = &addr->zs_chan_b;
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

CFATTACH_DECL(zsc, sizeof(struct zsc_softc),
    zs_match, zs_attach, NULL, NULL);

extern struct cfdriver zsc_cd;

static int zshard __P((void *));
static int zssoft __P((void *));
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
	struct confargs *ca = aux;
	int unit;
	void *va;

	/*
	 * This driver only supports its wired-in mappings,
	 * because the console support depends on those.
	 */
	if (ca->ca_paddr == zs_physaddr[0]) {
		unit = 0;
	} else if (ca->ca_paddr == zs_physaddr[1]) {
		unit = 1;
	} else {
		return (0);
	}

	/* Make sure zs_init() found mappings. */
	va = zsaddr[unit];
	if (va == NULL)
		return (0);

	/* This returns -1 on a fault (bus error). */
	if (peek_byte(va) == -1)
		return (0);

	/* Default interrupt priority (always splbio==2) */
	if (ca->ca_intpri == -1)
		ca->ca_intpri = ZSHARD_PRI;

	return (1);
}

/*
 * Attach a found zs.
 *
 * Match slave number to zs unit number, so that misconfiguration will
 * not set up the keyboard as ttya, etc.
 */
static void
zs_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct zsc_softc *zsc = (void *) self;
	struct confargs *ca = aux;
	struct zsc_attach_args zsc_args;
	volatile struct zschan *zc;
	struct zs_chanstate *cs;
	int s, zs_unit, channel;
	static int didintr;

	zs_unit = zsc->zsc_dev.dv_unit;

	printf(": (softpri %d)\n", ZSSOFT_PRI);

	/* Use the mapping setup by the Sun PROM. */
	if (zsaddr[zs_unit] == NULL)
		panic("zs_attach: zs%d not mapped", zs_unit);

	/*
	 * Initialize software state for each channel.
	 */
	for (channel = 0; channel < 2; channel++) {
		zsc_args.channel = channel;
		zsc_args.hwflags = zs_hwflags[zs_unit][channel];
		cs = &zsc->zsc_cs_store[channel];
		zsc->zsc_cs[channel] = cs;

		simple_lock_init(&cs->cs_lock);
		cs->cs_channel = channel;
		cs->cs_private = NULL;
		cs->cs_ops = &zsops_null;
		cs->cs_brg_clk = PCLK / 16;

		zc = zs_get_chan_addr(zs_unit, channel);
		cs->cs_reg_csr  = &zc->zc_csr;
		cs->cs_reg_data = &zc->zc_data;

		memcpy(cs->cs_creg, zs_init_reg, 16);
		memcpy(cs->cs_preg, zs_init_reg, 16);

		/* XXX: Get these from the EEPROM instead? */
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
			s = splhigh();
			zs_write_reg(cs,  9, reset);
			splx(s);
		}
	}

	/*
	 * Now safe to install interrupt handlers.  Note the arguments
	 * to the interrupt handlers aren't used.  Note, we only do this
	 * once since both SCCs interrupt at the same level and vector.
	 */
	if (!didintr) {
		didintr = 1;
		isr_add_autovect(zssoft, NULL, ZSSOFT_PRI);
		isr_add_autovect(zshard, NULL, ca->ca_intpri);
	}
	/* XXX; evcnt_attach() ? */

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

	/*
	 * XXX: L1A hack - We would like to be able to break into
	 * the debugger during the rest of autoconfiguration, so
	 * lower interrupts just enough to let zs interrupts in.
	 * This is done after both zs devices are attached.
	 */
	if (zs_unit == 1) {
		(void)spl5(); /* splzs - 1 */
	}
}

static int
zs_print(aux, name)
	void *aux;
	const char *name;
{
	struct zsc_attach_args *args = aux;

	if (name != NULL)
		aprint_normal("%s: ", name);

	if (args->channel != -1)
		aprint_normal(" channel %d", args->channel);

	return UNCONF;
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
	struct zsc_softc *zsc;
	int unit, rval, softreq;

	rval = softreq = 0;
	for (unit = 0; unit < zsc_cd.cd_ndevs; unit++) {
		zsc = zsc_cd.cd_devs[unit];
		if (zsc == NULL)
			continue;
		rval |= zsc_intr_hard(zsc);
		softreq |= zsc->zsc_cs[0]->cs_softreq;
		softreq |= zsc->zsc_cs[1]->cs_softreq;
	}

	/* We are at splzs here, so no need to lock. */
	if (softreq && (zssoftpending == 0)) {
		zssoftpending = ZSSOFT_PRI;
		isr_soft_request(ZSSOFT_PRI);
	}
	return (rval);
}

/*
 * Similar scheme as for zshard (look at all of them)
 */
static int
zssoft(arg)
	void *arg;
{
	struct zsc_softc *zsc;
	int s, unit;

	/* This is not the only ISR on this IPL. */
	if (zssoftpending == 0)
		return (0);

	/*
	 * The soft intr. bit will be set by zshard only if
	 * the variable zssoftpending is zero.  The order of
	 * these next two statements prevents our clearing
	 * the soft intr bit just after zshard has set it.
	 */
	isr_soft_clear(ZSSOFT_PRI);
	zssoftpending = 0;

	/* Make sure we call the tty layer at spltty. */
	s = spltty();
	for (unit = 0; unit < zsc_cd.cd_ndevs; unit++) {
		zsc = zsc_cd.cd_devs[unit];
		if (zsc == NULL)
			continue;
		(void) zsc_intr_soft(zsc);
	}
	splx(s);
	return (1);
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
	return val;
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

u_char zs_read_csr(cs)
	struct zs_chanstate *cs;
{
	u_char val;

	val = *cs->cs_reg_csr;
	ZS_DELAY();
	return val;
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
	u_char val;

	val = *cs->cs_reg_data;
	ZS_DELAY();
	return val;
}

void  zs_write_data(cs, val)
	struct zs_chanstate *cs;
	u_char val;
{
	*cs->cs_reg_data = val;
	ZS_DELAY();
}

/****************************************************************
 * Console support functions (Sun3 specific!)
 * Note: this code is allowed to know about the layout of
 * the chip registers, and uses that to keep things simple.
 * XXX - I think I like the mvme167 code better. -gwr
 ****************************************************************/

void *zs_conschan;

/*
 * Handle user request to enter kernel debugger.
 */
void
zs_abort(cs)
	struct zs_chanstate *cs;
{
	volatile struct zschan *zc = zs_conschan;
	int rr0;

	/* Wait for end of break to avoid PROM abort. */
	/* XXX - Limit the wait? */
	do {
		rr0 = zc->zc_csr;
		ZS_DELAY();
	} while (rr0 & ZSRR0_BREAK);

	/* This is always available on the Sun3. */
	Debugger();
}

/*
 * Polled input char.
 */
int
zs_getc(arg)
	void *arg;
{
	volatile struct zschan *zc = arg;
	int s, c, rr0;

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
	volatile struct zschan *zc = arg;
	int s, rr0;

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

static void zscninit __P((struct consdev *));
static int  zscngetc __P((dev_t));
static void zscnputc __P((dev_t, int));

/*
 * Console table shared by ttya, ttyb
 */
struct consdev consdev_tty = {
	nullcnprobe,
	zscninit,
	zscngetc,
	zscnputc,
	nullcnpollc,
	NULL,
};

static void
zscninit(cn)
	struct consdev *cn;
{
}

/*
 * Polled console input putchar.
 */
static int
zscngetc(dev)
	dev_t dev;
{
	return (zs_getc(zs_conschan));
}

/*
 * Polled console output putchar.
 */
static void
zscnputc(dev, c)
	dev_t dev;
	int c;
{
	zs_putc(zs_conschan, c);
}

/*****************************************************************/

static void prom_cninit __P((struct consdev *));
static int  prom_cngetc __P((dev_t));
static void prom_cnputc __P((dev_t, int));

/*
 * The console is set to this one initially,
 * which lets us use the PROM until consinit()
 * is called to select a real console.
 */
struct consdev consdev_prom = {
	nullcnprobe,
	prom_cninit,
	prom_cngetc,
	prom_cnputc,
	nullcnpollc,
};

/*
 * The console table pointer is statically initialized
 * to point to the PROM (output only) table, so that
 * early calls to printf will work.
 */
struct consdev *cn_tab = &consdev_prom;

void
nullcnprobe(cn)
	struct consdev *cn;
{
}

static void
prom_cninit(cn)
	struct consdev *cn;
{
}

/*
 * PROM console input putchar.
 * (dummy - this is output only)
 */
static int
prom_cngetc(dev)
	dev_t dev;
{
	return (0);
}

/*
 * PROM console output putchar.
 */
static void
prom_cnputc(dev, c)
	dev_t dev;
	int c;
{
	(*romVectorPtr->putChar)(c & 0x7f);
}

/*****************************************************************/

extern struct consdev consdev_kd;

static struct {
	int zs_unit, channel;
} zstty_conf[NZS*2] = {
	/* XXX: knowledge from the config file here... */
	{ 1, 0 },	/* ttya */
	{ 1, 1 },	/* ttyb */
	{ 0, 0 },	/* ttyc */
	{ 0, 1 },	/* ttyd */
};

static char *prom_inSrc_name[] = {
	"keyboard/display",
	"ttya", "ttyb",
	"ttyc", "ttyd" };

/*
 * This function replaces sys/dev/cninit.c
 * Determine which device is the console using
 * the PROM "input source" and "output sink".
 */
void
cninit()
{
	struct sunromvec *v;
	struct zschan *zc;
	struct consdev *cn;
	int channel, zs_unit, zstty_unit;
	u_char inSource, outSink;
	extern const struct cdevsw zstty_cdevsw;

	/* Get the zs driver ready for console duty. */
	zs_init();

	v = romVectorPtr;
	inSource = *v->inSource;
	outSink  = *v->outSink;
	if (inSource != outSink) {
		mon_printf("cninit: mismatched PROM output selector\n");
	}

	switch (inSource) {
	default:
		mon_printf("cninit: invalid inSource=%d\n", inSource);
		sunmon_abort();
		inSource = 0;
		/* fall through */

	case 0:	/* keyboard/display */
#if NKBD > 0
		zs_unit = 0;
		channel = 0;
		cn = &consdev_kd;
		/* Set cn_dev, cn_pri in kd.c */
		break;
#else	/* NKBD */
		mon_printf("cninit: kdb/display not configured\n");
		sunmon_abort();
		inSource = 1;
		/* fall through */
#endif	/* NKBD */

	case 1:	/* ttya */
	case 2:	/* ttyb */
	case 3:	/* ttyc (rewired keyboard connector) */
	case 4:	/* ttyd (rewired mouse connector)   */
		zstty_unit = inSource - 1;
		zs_unit = zstty_conf[zstty_unit].zs_unit;
		channel = zstty_conf[zstty_unit].channel;
		cn = &consdev_tty;
		cn->cn_dev = makedev(cdevsw_lookup_major(&zstty_cdevsw),
				     zstty_unit);
		cn->cn_pri = CN_REMOTE;
		break;

	}
	/* Now that inSource has been validated, print it. */
	mon_printf("console is %s\n", prom_inSrc_name[inSource]);

	zc = zs_get_chan_addr(zs_unit, channel);
	if (zc == NULL) {
		mon_printf("cninit: zs not mapped.\n");
		return;
	}
	zs_conschan = zc;
	zs_hwflags[zs_unit][channel] = ZS_HWFLAG_CONSOLE;
	cn_tab = cn;
	(*cn->cn_init)(cn);
#ifdef	KGDB
	zs_kgdb_init();
#endif
}
