/*	$NetBSD: zs_ap.c,v 1.1 1999/12/22 05:55:25 tsubai Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/tty.h>

#include <machine/adrsmap.h>
#include <machine/cpu.h>
#include <machine/z8530var.h>

#include <dev/cons.h>
#include <dev/ic/z8530reg.h>

#include <newsmips/apbus/apbusvar.h>

#include "zsc.h"	/* NZSC */
#define NZS NZSC

/* Make life easier for the initialized arrays here. */
#if NZS < 2
#undef  NZS
#define NZS 2
#endif

#define	MODE_REGISTER	(-0x00080000)
#define	FIFO_CH0	0x00000000
#define	FIFO_CH1	0x00010000
#define	FIFO_CH2	0x00020000
#define	FIFO_CH3	0x00030000
#define	FIFO_DEVWIN0	0x00040000
#define	FIFO_DEVWIN1	0x00050000
#define	FIFO_DEVWIN2	0x00060000
#define	FIFO_DEVWIN3	0x00070000

#define	PORTB_XPORT	FIFO_CH0
#define	PORTB_RPORT	FIFO_CH1
#define	PORTA_XPORT	FIFO_CH2
#define	PORTA_RPORT	FIFO_CH3
#define	PORTB_OFFSET	FIFO_DEVWIN0
#define	PORTA_OFFSET	FIFO_DEVWIN1

extern int zs_def_cflag;
extern void (*zs_delay) __P((void));

/*
 * The news5000 provides a 9.8304 MHz clock to the ZS chips.
 */
#define PCLK	(9600 * 1024)	/* PCLK pin input clock rate */

#define ZS_DELAY()	DELAY(2)

/* The layout of this is hardware-dependent (padding, order). */
struct zschan {
	volatile u_char pad1[3];
	volatile u_char zc_csr;		/* ctrl,status, and indirect access */
	volatile u_char pad2[3];
	volatile u_char zc_data;	/* data */
};

static caddr_t zsaddr[NZS];

/* Flags from cninit() */
static int zs_hwflags[NZS][2];

/* Default speed for all channels */
static int zs_defspeed = 9600;

static u_char zs_init_reg[16] = {
	0,	/* 0: CMD (reset, etc.) */
	0,	/* 1: No interrupts yet. */
	0,	/* IVECT */
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
static void zs_ap_delay __P((void));
static void zshard __P((void *));
static void zssoft __P((void *));
static int zs_getc __P((void *));
static void zs_putc __P((void *, int));
int zs_get_speed __P((struct zs_chanstate *));

struct zschan *
zs_get_chan_addr(zs_unit, channel)
	int zs_unit, channel;
{
	caddr_t addr;
	struct zschan *zc;

	if (zs_unit >= NZS)
		return NULL;
	addr = zsaddr[zs_unit];
	if (addr == NULL)
		return NULL;
	if (channel == 0) {
		zc = (void *)(addr + PORTA_OFFSET);
	} else {
		zc = (void *)(addr + PORTB_OFFSET);
	}
	return (zc);
}

void
zs_ap_delay()
{
	ZS_DELAY();
}

/****************************************************************
 * Autoconfig
 ****************************************************************/

/* Definition of the driver for autoconfig. */
int zs_ap_match __P((struct device *, struct cfdata *, void *));
void zs_ap_attach __P((struct device *, struct device *, void *));
int zs_print __P((void *, const char *name));

struct cfattach zsc_ap_ca = {
	sizeof(struct zsc_softc), zs_ap_match, zs_ap_attach
};

extern struct cfdriver zsc_cd;

/*
 * Is the zs chip present?
 */
int
zs_ap_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct apbus_attach_args *apa = aux;

	if (strcmp("esccf", apa->apa_name) != 0)
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
zs_ap_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct zsc_softc *zsc = (void *)self;
	struct apbus_attach_args *apa = aux;
	struct zsc_attach_args zsc_args;
	volatile struct zschan *zc;
	struct zs_chanstate *cs;
	int s, zs_unit, channel;
	volatile u_int *txBfifo = (void *)(apa->apa_hwbase + FIFO_CH0);
	volatile u_int *rxBfifo = (void *)(apa->apa_hwbase + FIFO_CH1);
	volatile u_int *txAfifo = (void *)(apa->apa_hwbase + FIFO_CH2);
	volatile u_int *rxAfifo = (void *)(apa->apa_hwbase + FIFO_CH3);
	volatile u_int *devwin0 = (void *)(apa->apa_hwbase + FIFO_DEVWIN0);
	volatile u_int *devwin1 = (void *)(apa->apa_hwbase + FIFO_DEVWIN1);
	volatile u_int *devwin2 = (void *)(apa->apa_hwbase + FIFO_DEVWIN2);
	static int didintr;

	zs_unit = zsc->zsc_dev.dv_unit;
	zsaddr[zs_unit] = (caddr_t)apa->apa_hwbase;

	printf(" slot%d addr 0x%lx\n", apa->apa_slotno, apa->apa_hwbase);

	/* enable DMA external ready */
	txAfifo[3] = rxAfifo[3] = 8;
	txBfifo[3] = rxBfifo[3] = 8;

	/* assert DTR */			/* XXX */
	devwin0[2] = devwin1[2] = 0x04;

	/* select RS-232C (ch1 only) */
	devwin1[3] = 0x02;

	/* enable SCC interrupts */
	devwin2[1] = 0x01;

	zs_delay = zs_ap_delay;

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

		apbus_intr_establish(1, /* interrupt level ( 0 or 1 ) */
				     NEWS5000_INT1_SERIAL,
				     0, /* priority */
				     zshard, zsc,
				     apa->apa_name, apa->apa_ctlnum);
	}
	/* XXX; evcnt_attach() ? */

#if 0
{
	u_int x;

	/* determine SCC/ESCC type */
	x = zs_read_reg(cs, 15);
	zs_write_reg(cs, 15, x | ZSWR15_ENABLE_ENHANCED);

	if (zs_read_reg(cs, 15) & ZSWR15_ENABLE_ENHANCED) { /* ESCC Z85230 */
		zs_write_reg(cs, 7, ZSWR7P_EXTEND_READ | ZSWR7P_TX_FIFO);
	}
}
#endif

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

static volatile int zssoftpending;

/*
 * Our ZS chips all share a common, autovectored interrupt,
 * so we have to look at all of them on each interrupt.
 */
static void
zshard(arg)
	void *arg;
{
	register struct zsc_softc *zsc;
	register int unit, rval, softreq;

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
		zssoftpending = 1;
		zssoft(arg);	/*isr_soft_request(ZSSOFT_PRI);*/
	}
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
		return;

	/*
	 * The soft intr. bit will be set by zshard only if
	 * the variable zssoftpending is zero.  The order of
	 * these next two statements prevents our clearing
	 * the soft intr bit just after zshard has set it.
	 */
	/*isr_soft_clear(ZSSOFT_PRI);*/
	/*zssoftpending = 0;*/

	/* Make sure we call the tty layer at spltty. */
	s = spltty();
	for (unit = 0; unit < zsc_cd.cd_ndevs; unit++) {
		zsc = zsc_cd.cd_devs[unit];
		if (zsc == NULL)
			continue;
		(void) zsc_intr_soft(zsc);
	}
	splx(s);
	zssoftpending = 0;
	return;
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

static void zscnprobe __P((struct consdev *));
static void zscninit __P((struct consdev *));
static int  zscngetc __P((dev_t));
static void zscnputc __P((dev_t, int));
static void zscnpollc __P((dev_t, int));

struct consdev consdev_zs_ap = {
	zscnprobe,
	zscninit,
	zscngetc,
	zscnputc,
	zscnpollc
};

void
zscnprobe(cn)
	struct consdev *cn;
{
}

void
zscninit(cn)
	struct consdev *cn;
{
	cn->cn_dev = makedev(zs_major, 0);
	cn->cn_pri = CN_REMOTE;
	zs_hwflags[0][0] = ZS_HWFLAG_CONSOLE;
}

int
zscngetc(dev)
	dev_t dev;
{
	return zs_getc((void *)NEWS5000_SCCPORT0A);
}

void
zscnputc(dev, c)
	dev_t dev;
	int c;
{
	zs_putc((void *)NEWS5000_SCCPORT0A, c);
}

void
zscnpollc(dev, on)
	dev_t dev;
	int on;
{
}
