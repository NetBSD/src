/*	$NetBSD: zs_hb.c,v 1.17 2003/07/15 02:59:30 lukem Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: zs_hb.c,v 1.17 2003/07/15 02:59:30 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/tty.h>
#include <sys/conf.h>

#include <machine/adrsmap.h>
#include <machine/cpu.h>
#include <machine/z8530var.h>

#include <dev/cons.h>
#include <dev/ic/z8530reg.h>

#include <newsmips/dev/hbvar.h>

#include "zsc.h"	/* NZSC */
#define NZS NZSC

/* Make life easier for the initialized arrays here. */
#if NZS < 2
#undef  NZS
#define NZS 2
#endif

#define ZSCFLAG_EX	0x01	/* expansion board */

/*
 * The news3400 provides a 4.9152 MHz clock to the ZS chips.
 */
#define PCLK	(9600 * 512)	/* PCLK pin input clock rate */
#define PCLK_EX	(9600 * 384)

/*
 * Define interrupt levels.
 */
#define ZSHARD_PRI 64

#define ZS_DELAY() {(void)*(volatile char *)INTEN1; delay(2);}

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

extern int zs_def_cflag;

static struct zsdevice *zsaddr[NZS];

/* Flags from cninit() */
static int zs_hwflags[NZS][2];

/* Default speed for all channels */
static int zs_defspeed = 9600;

static u_char zs_init_reg[16] = {
	0,	/* 0: CMD (reset, etc.) */
	0,	/* 1: No interrupts yet. */
	ZSHARD_PRI,	/* IVECT */
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

static struct zschan * zs_get_chan_addr __P((int, int));
static void zs_hb_delay __P((void));
static int zshard_hb __P((void *));
static int zs_getc __P((void *));
static void zs_putc __P((void *, int));

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

static void
zs_hb_delay()
{

	ZS_DELAY();
}

/****************************************************************
 * Autoconfig
 ****************************************************************/

/* Definition of the driver for autoconfig. */
int zs_hb_match __P((struct device *, struct cfdata *, void *));
void zs_hb_attach __P((struct device *, struct device *, void *));

CFATTACH_DECL(zsc_hb, sizeof(struct zsc_softc),
    zs_hb_match, zs_hb_attach, NULL, NULL);

/*
 * Is the zs chip present?
 */
int
zs_hb_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct hb_attach_args *ha = aux;

	if (strcmp(ha->ha_name, "zsc"))
		return 0;

	/* This returns -1 on a fault (bus error). */
	if (hb_badaddr((char *)ha->ha_addr, 1))
		return 0;

	return 1;
}

/*
 * Attach a found zs.
 *
 * Match slave number to zs unit number, so that misconfiguration will
 * not set up the keyboard as ttya, etc.
 */
void
zs_hb_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct zsc_softc *zsc = (void *)self;
	struct hb_attach_args *ha = aux;
	struct zsc_attach_args zsc_args;
	volatile struct zschan *zc;
	struct zs_chanstate *cs;
	int s, zs_unit, channel, intlevel;
	static int didintr;

	zs_unit = zsc->zsc_dev.dv_unit;
	intlevel = ha->ha_level;
	zsaddr[zs_unit] = (void *)ha->ha_addr;

	if (intlevel == -1) {
#if 0
		printf(": interrupt level not configured\n");
		return;
#else
		printf(": interrupt level not configured; using");
		intlevel = 1;
#endif
	}

	printf(" level %d\n", intlevel);

	zs_delay = zs_hb_delay;

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
		if ((zsc->zsc_dev.dv_cfdata->cf_flags & ZSCFLAG_EX) == 0)
			cs->cs_brg_clk = PCLK / 16;
		else
			cs->cs_brg_clk = PCLK_EX / 16;

		zc = zs_get_chan_addr(zs_unit, channel);
		cs->cs_reg_csr  = &zc->zc_csr;
		cs->cs_reg_data = &zc->zc_data;

		bcopy(zs_init_reg, cs->cs_creg, 16);
		bcopy(zs_init_reg, cs->cs_preg, 16);

		/* XXX: Get these from the EEPROM instead? */
		/* XXX: See the mvme167 code.  Better. */
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
			u_char reset = (channel == 0) ?
				ZSWR9_A_RESET : ZSWR9_B_RESET;
			s = splhigh();
			zs_write_reg(cs, 9, reset);
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

		zsc->zsc_si = softintr_establish(IPL_SOFTSERIAL, zssoft, zsc);
		hb_intr_establish(intlevel, INTST1_SCC, IPL_SERIAL,
		    zshard_hb, NULL);
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
}

static int
zshard_hb(arg)
	void *arg;
{
	int rv;

	(void) *(volatile u_char *)SCCVECT;
	rv = zshard(arg);

	/* XXX news3400 sometimes losts zs interrupt */
	if (rv)
		zshard(arg);

	return rv;
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

static void zscnprobe __P((struct consdev *));
static void zscninit __P((struct consdev *));
static int  zscngetc __P((dev_t));
static void zscnputc __P((dev_t, int));

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
zscnprobe(cn)
	struct consdev *cn;
{
}

static void
zscninit(cn)
	struct consdev *cn;
{
	extern const struct cdevsw zstty_cdevsw;

	cn->cn_dev = makedev(cdevsw_lookup_major(&zstty_cdevsw), 0);
	cn->cn_pri = CN_REMOTE;
	zs_hwflags[0][0] = ZS_HWFLAG_CONSOLE;
}

static int
zscngetc(dev)
	dev_t dev;
{

	return zs_getc((void *)SCCPORT0A);
}

static void
zscnputc(dev, c)
	dev_t dev;
	int c;
{

	zs_putc((void *)SCCPORT0A, c);
}
