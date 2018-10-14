/*	$NetBSD: zs_ap.c,v 1.29 2018/10/14 00:10:11 tsutsui Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: zs_ap.c,v 1.29 2018/10/14 00:10:11 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/intr.h>
#ifdef NEWS4000_ZS_AP_POLLING
#include <sys/callout.h>
#endif

#include <machine/adrsmap.h>
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

#define NEWS5000_PORTB_TXPORT	0x00000000
#define NEWS5000_PORTB_RXPORT	0x00010000
#define NEWS5000_PORTA_TXPORT	0x00020000
#define NEWS5000_PORTA_RXPORT	0x00030000
#define   NEWS5000_DMA_MODE_REG		3
#define     NEWS5000_DMA_ENABLE		0x01	/* DMA enable */
#define     NEWS5000_DMA_DIR_DM		0x00	/* device to memory */
#define     NEWS5000_DMA_DIR_MD		0x02	/* memory to device */
#define     NEWS5000_DMA_EXTRDY		0x08	/* DMA external ready */
#define NEWS5000_PORTB_OFFSET	0x00040000
#define NEWS5000_PORTA_OFFSET	0x00050000
#define   NEWS5000_PORT_CTL		2
#define     NEWS5000_PORTCTL_RI		0x01
#define     NEWS5000_PORTCTL_DSR	0x02
#define     NEWS5000_PORTCTL_DTR	0x04
#define   NEWS5000_PORT_SEL		3
#define     NEWS5000_PORTSEL_LOCALTALK	0x01
#define     NEWS5000_PORTSEL_RS232C	0x02
#define NEWS5000_ESCC_REG	0x00060000
#define   NEWS5000_ESCCREG_INTSTAT	0
#define     NEWS5000_INTSTAT_SCC	0x01
#define   NEWS5000_ESCCREG_INTMASK	1
#define     NEWS5000_INTMASK_SCC	0x01

#define NEWS4000_PORTB_TXPORT	0x00000000	/* XXX: not confirmed */
#define NEWS4000_PORTB_RXPORT	0x00010000	/* XXX: not confirmed */
#define NEWS4000_PORTA_TXPORT	0x00040000	/* XXX: not confirmed */
#define NEWS4000_PORTA_RXPORT	0x00050000	/* XXX: not confirmed */
#define   NEWS4000_DMA_MODE_REG		3
#define     NEWS4000_DMA_ENABLE		0x01	/* DMA enable */
#define     NEWS4000_DMA_DIR_DM		0x00	/* device to memory */
#define     NEWS4000_DMA_DIR_MD		0x02	/* memory to device */
#define     NEWS4000_DMA_EXTRDY		0x08	/* DMA external ready */
#define NEWS4000_PORTB_CTL	0x00020000	/* XXX: not confirmed */
#define NEWS4000_PORTA_CTL	0x00060000	/* XXX: not confirmed */
#define   NEWS4000_PORT_CTL		4
#define     NEWS4000_PORTCTL_RI		0x01
#define     NEWS4000_PORTCTL_DSR	0x02    
#define     NEWS4000_PORTCTL_DTR	0x04
#define   NEWS4000_PORT_SEL		5
#define     NEWS4000_PORTSEL_LOCALTALK	0x01
#define     NEWS4000_PORTSEL_RS232C	0x02
#define NEWS4000_ESCC_REG	0x00060000	/* XXX:  not confirmed */
#define   NEWS4000_ESCCREG_INTSTAT	0
#define     NEWS4000_INTSTAT_SCC	0x01
#define   NEWS4000_ESCCREG_INTMASK	1
#define     NEWS4000_INTMASK_SCC	0x01
#define NEWS4000_PORTB_OFFSET	0x00080000
#define NEWS4000_PORTA_OFFSET	0x00080008

extern int zs_def_cflag;
extern void (*zs_delay)(void);

/*
 * The news5000 provides a 9.8304 MHz clock to the ZS chips.
 */
#define PCLK	(9600 * 1024)	/* PCLK pin input clock rate */

#define ZS_DELAY()	DELAY(2)

/* The layout of this is hardware-dependent (padding, order). */
struct zschan {
	volatile uint8_t pad1[3];
	volatile uint8_t zc_csr;	/* ctrl,status, and indirect access */
	volatile uint8_t pad2[3];
	volatile uint8_t zc_data;	/* data */
};

static void *zsaddr[NZS];

/* Flags from cninit() */
static int zs_hwflags[NZS][2];

/* Default speed for all channels */
static int zs_defspeed = 9600;

static uint8_t zs_init_reg[16] = {
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

#ifdef NEWS4000_ZS_AP_POLLING
static struct callout zscallout;
#endif

static struct zschan * zs_get_chan_addr(int, int);
static void zs_ap_delay(void);
static int zshard_ap(void *);
static int zs_getc(void *);
static void zs_putc(void *, int);

struct zschan *
zs_get_chan_addr(int zs_unit, int channel)
{
	void *addr;
	struct zschan *zc = NULL;

	if (zs_unit >= NZS)
		return NULL;
	addr = zsaddr[zs_unit];
	if (addr == NULL)
		return NULL;
	if (systype == NEWS5000) {
		if (channel == 0) {
			zc = (void *)((uint8_t *)addr + NEWS5000_PORTA_OFFSET);
		} else {
			zc = (void *)((uint8_t *)addr + NEWS5000_PORTB_OFFSET);
		}
	}
	if (systype == NEWS4000) {
		if (channel == 0) {
			zc = (void *)((uint8_t *)addr + NEWS4000_PORTA_OFFSET);
		} else {
			zc = (void *)((uint8_t *)addr + NEWS4000_PORTB_OFFSET);
		}
	}
	return zc;
}

void
zs_ap_delay(void)
{

	ZS_DELAY();
}

/****************************************************************
 * Autoconfig
 ****************************************************************/

/* Definition of the driver for autoconfig. */
int zs_ap_match(device_t, cfdata_t, void *);
void zs_ap_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(zsc_ap, sizeof(struct zsc_softc),
    zs_ap_match, zs_ap_attach, NULL, NULL);

/*
 * Is the zs chip present?
 */
int
zs_ap_match(device_t parent, cfdata_t cf, void *aux)
{
	struct apbus_attach_args *apa = aux;

	if (strcmp("esccf", apa->apa_name) == 0 ||
	    strcmp("esccg", apa->apa_name) == 0)
		return 1;

	return 0;
}

/*
 * Attach a found zs.
 *
 * Match slave number to zs unit number, so that misconfiguration will
 * not set up the keyboard as ttya, etc.
 */
void
zs_ap_attach(device_t parent, device_t self, void *aux)
{
	struct zsc_softc *zsc = device_private(self);
	struct apbus_attach_args *apa = aux;
	struct zsc_attach_args zsc_args;
	volatile struct zschan *zc;
	struct zs_chanstate *cs;
	int s, zs_unit, channel;
	volatile uint32_t *txBfifo;
	volatile uint32_t *rxBfifo;
	volatile uint32_t *txAfifo;
	volatile uint32_t *rxAfifo;
	volatile uint32_t *portBctl;
	volatile uint32_t *portActl;
	volatile uint32_t *esccregs;

	zsc->zsc_dev = self;
	zs_unit = device_unit(self);
	zsaddr[zs_unit] = (void *)apa->apa_hwbase;

	aprint_normal(" slot%d addr 0x%lx\n", apa->apa_slotno, apa->apa_hwbase);

	/* XXX: appease gcc -Wuninitialized */
	txBfifo  = (void *)(apa->apa_hwbase);
	rxBfifo  = (void *)(apa->apa_hwbase);
	txAfifo  = (void *)(apa->apa_hwbase);
	rxAfifo  = (void *)(apa->apa_hwbase);
	portBctl = (void *)(apa->apa_hwbase);
	portActl = (void *)(apa->apa_hwbase);
	esccregs = (void *)(apa->apa_hwbase);

	if (systype == NEWS5000) {
		txBfifo  = (void *)(apa->apa_hwbase + NEWS5000_PORTB_TXPORT);
		rxBfifo  = (void *)(apa->apa_hwbase + NEWS5000_PORTB_RXPORT);
		txAfifo  = (void *)(apa->apa_hwbase + NEWS5000_PORTA_TXPORT);
		rxAfifo  = (void *)(apa->apa_hwbase + NEWS5000_PORTA_RXPORT);
		portBctl = (void *)(apa->apa_hwbase + NEWS5000_PORTB_OFFSET);
		portActl = (void *)(apa->apa_hwbase + NEWS5000_PORTA_OFFSET);
		esccregs = (void *)(apa->apa_hwbase + NEWS5000_ESCC_REG);
	}
	if (systype == NEWS4000) {
		txBfifo  = (void *)(apa->apa_hwbase + NEWS4000_PORTB_TXPORT);
		rxBfifo  = (void *)(apa->apa_hwbase + NEWS4000_PORTB_RXPORT);
		txAfifo  = (void *)(apa->apa_hwbase + NEWS4000_PORTA_TXPORT);
		rxAfifo  = (void *)(apa->apa_hwbase + NEWS4000_PORTA_RXPORT);
		portBctl = (void *)(apa->apa_hwbase + NEWS4000_PORTB_CTL);
		portActl = (void *)(apa->apa_hwbase + NEWS4000_PORTA_CTL);
		esccregs = (void *)(apa->apa_hwbase + NEWS4000_ESCC_REG);
	}

	if (systype == NEWS5000) {
		txAfifo[NEWS5000_DMA_MODE_REG] = NEWS5000_DMA_EXTRDY;
		rxAfifo[NEWS5000_DMA_MODE_REG] = NEWS5000_DMA_EXTRDY;
		txBfifo[NEWS5000_DMA_MODE_REG] = NEWS5000_DMA_EXTRDY;
		rxBfifo[NEWS5000_DMA_MODE_REG] = NEWS5000_DMA_EXTRDY;

		/* assert DTR */			/* XXX */
		portBctl[NEWS5000_PORT_CTL] = NEWS5000_PORTCTL_DTR;
		portActl[NEWS5000_PORT_CTL] = NEWS5000_PORTCTL_DTR;

		/* select RS-232C (ch1 only) */
		portActl[NEWS5000_PORT_SEL] = NEWS5000_PORTSEL_RS232C;

		/* enable SCC interrupts */
		esccregs[NEWS5000_ESCCREG_INTMASK] = NEWS5000_INTMASK_SCC;
	}

	if (systype == NEWS4000) {
		txAfifo[NEWS4000_DMA_MODE_REG] = NEWS4000_DMA_EXTRDY;
		rxAfifo[NEWS4000_DMA_MODE_REG] = NEWS4000_DMA_EXTRDY;
		txBfifo[NEWS4000_DMA_MODE_REG] = NEWS4000_DMA_EXTRDY;
		rxBfifo[NEWS4000_DMA_MODE_REG] = NEWS4000_DMA_EXTRDY;

#if 1	/* XXX: zsc on news4000 seems mangled by these ops */
		/* assert DTR */			/* XXX */
		portBctl[NEWS4000_PORT_CTL] = NEWS4000_PORTCTL_DTR;
		portActl[NEWS4000_PORT_CTL] = NEWS4000_PORTCTL_DTR;

		/* select RS-232C (ch1 only) */
		portActl[NEWS4000_PORT_SEL] = NEWS4000_PORTSEL_RS232C;
#endif

		/* enable SCC interrupts */
		esccregs[NEWS4000_ESCCREG_INTMASK] = NEWS4000_INTMASK_SCC;
	}

	zs_delay = zs_ap_delay;

	/*
	 * Initialize software state for each channel.
	 */
	for (channel = 0; channel < 2; channel++) {
		zsc_args.channel = channel;
		zsc_args.hwflags = zs_hwflags[zs_unit][channel];
		cs = &zsc->zsc_cs_store[channel];
		zsc->zsc_cs[channel] = cs;

		zs_lock_init(cs);
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
			zs_write_reg(cs, 9, reset);
			splx(s);
		}
	}

	/*
	 * Now safe to install interrupt handlers.
	 */
	zsc->zsc_si = softint_establish(SOFTINT_SERIAL,
	    (void (*)(void *))zsc_intr_soft, zsc);
	if (systype == NEWS5000) {
		apbus_intr_establish(1, /* interrupt level ( 0 or 1 ) */
				     NEWS5000_INT1_SCC,
				     0, /* priority */
				     zshard_ap, zsc,
				     device_xname(self), apa->apa_ctlnum);
	}
	if (systype == NEWS4000) {
		apbus_intr_establish(1, /* interrupt level ( 0 or 1 ) */
				     0x0200,
				     0, /* priority */
				     zshard_ap, zsc,
				     device_xname(self), apa->apa_ctlnum);
#ifdef NEWS4000_ZS_AP_POLLING
		/* XXX: no info how to enable zs ap interrupt for now */
		callout_init(&zscallout, 0);
		callout_reset(&zscallout, 1,
		    (void (*)(void *))zshard_ap, (void *)zsc);
#endif
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

static int
zshard_ap(void *arg)
{

	zshard(arg);
#ifdef NEWS4000_ZS_AP_POLLING
	if (systype == NEWS4000) {
		callout_schedule(&zscallout, 1);
	}
#endif
	return 1;
}

/*
 * Polled input char.
 */
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

	/*
	 * This is used by the kd driver to read scan codes,
	 * so don't translate '\r' ==> '\n' here...
	 */
	return c;
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
	ZS_DELAY();
	splx(s);
}

/*****************************************************************/

static void zscnprobe(struct consdev *);
static void zscninit(struct consdev *);
static int  zscngetc(dev_t);
static void zscnputc(dev_t, int);

struct consdev consdev_zs_ap = {
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
	extern const struct cdevsw zstty_cdevsw;

	cn->cn_dev = makedev(cdevsw_lookup_major(&zstty_cdevsw), 0);
	cn->cn_pri = CN_REMOTE;
	zs_hwflags[0][0] = ZS_HWFLAG_CONSOLE;
}

static int
zscngetc(dev_t dev)
{
	void *sccport0a;

	if (systype == NEWS5000)
		sccport0a = (void *)NEWS5000_SCCPORT0A;
	if (systype == NEWS4000)
		sccport0a = (void *)NEWS4000_SCCPORT0A;
 
	return zs_getc(sccport0a);
}

static void
zscnputc(dev_t dev, int c)
{
	void *sccport0a;

	if (systype == NEWS5000)
		sccport0a = (void *)NEWS5000_SCCPORT0A;
	if (systype == NEWS4000)
		sccport0a = (void *)NEWS4000_SCCPORT0A;

	zs_putc(sccport0a, c);
}
