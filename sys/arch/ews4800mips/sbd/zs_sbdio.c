/*	$NetBSD: zs_sbdio.c,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

/*-
 * Copyright (c) 1996, 2005 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zs_sbdio.c,v 1.1 2005/12/29 15:20:09 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/tty.h>
#include <sys/conf.h>

#include <dev/cons.h>
#include <dev/ic/z8530reg.h>

#include <machine/sbdiovar.h>
#include <machine/z8530var.h>

#define ZS_DEFSPEED	9600
#define PCLK		(9600 * 512)		/* 4.915200MHz */

/* The layout of this is hardware-dependent (padding, order). */
struct zschan {
	volatile uint8_t zc_csr;	/* ctrl, status, and indirect access */
	uint8_t padding1[3];
	volatile uint8_t zc_data;	/* data */
	uint8_t padding2[3];
} __attribute__((__packed__));

struct zsdevice {
	/* Yes, they are backwards. */
	struct	zschan zs_chan_b;
	struct	zschan zs_chan_a;
} __attribute__((__packed__));

static uint8_t zs_init_reg[16] = {
	0,				/*  0: CMD (reset, etc.) */
	0,				/*  1: No interrupts yet. */
	0,				/*  2: IVECT EWS-UX don't set this. */
	ZSWR3_RX_8 | ZSWR3_RX_ENABLE,
	ZSWR4_CLK_X16 | ZSWR4_ONESB | ZSWR4_EVENP,
	ZSWR5_TX_8 | ZSWR5_TX_ENABLE,
	0,				/*  6: TXSYNC/SYNCLO */
	0,				/*  7: RXSYNC/SYNCHI */
	0,				/*  8: alias for data port */
	ZSWR9_MASTER_IE,
	0,				/* 10: Misc. TX/RX control bits */
	ZSWR11_TXCLK_BAUD | ZSWR11_RXCLK_BAUD,
	BPS_TO_TCONST((PCLK/16), ZS_DEFSPEED), /* 12: BAUDLO (default=9600) */
	0,				/*13: BAUDHI (default=9600) */
	ZSWR14_BAUD_ENA | ZSWR14_BAUD_FROM_PCLK,
	ZSWR15_BREAK_IE,
};

int zs_sbdio_match(struct device *, struct cfdata *, void *);
void zs_sbdio_attach(struct device *, struct device *, void *);

CFATTACH_DECL(zsc_sbdio, sizeof(struct zsc_softc),
    zs_sbdio_match, zs_sbdio_attach, NULL, NULL);

int
zs_sbdio_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct sbdio_attach_args *sa = aux;

	return strcmp(sa->sa_name, "zsc") ? 0 : 1;
}

void
zs_sbdio_attach(struct device *parent, struct device *self, void *aux)
{
	struct sbdio_attach_args *sa = aux;
	struct zsc_softc *zsc = (void *)self;
	struct zsc_attach_args zsc_args;
	struct zschan *zc;
	struct zs_chanstate *cs;
	struct zsdevice *zs_addr;
	int s, zs_unit, channel;

	printf(" at %p irq %d\n", (void *)sa->sa_addr1, sa->sa_irq);

	zs_unit = zsc->zsc_dev.dv_unit;
	zs_addr = (void *)MIPS_PHYS_TO_KSEG1(sa->sa_addr1);
	zsc->zsc_flags = sa->sa_flags;

	/*
	 * Initialize software state for each channel.
	 */
	for (channel = 0; channel < 2; channel++) {
		zsc_args.channel = channel;
		zsc_args.hwflags = 0;
		cs = &zsc->zsc_cs_store[channel];
		zsc->zsc_cs[channel] = cs;

		simple_lock_init(&cs->cs_lock);
		cs->cs_channel = channel;
		cs->cs_private = NULL;
		cs->cs_ops = &zsops_null;

		if (channel == 0)
			zc = &zs_addr->zs_chan_a;
		else
			zc = &zs_addr->zs_chan_b;

		if (zc == zs_consaddr) {
			memcpy(cs, zs_conscs, sizeof(struct zs_chanstate));
			zs_conscs = cs;
			zsc_args.hwflags = ZS_HWFLAG_CONSOLE;
		} else {
			cs->cs_reg_csr  = &zc->zc_csr;
			cs->cs_reg_data = &zc->zc_data;
			memcpy(cs->cs_creg, zs_init_reg, 16);
			memcpy(cs->cs_preg, zs_init_reg, 16);
			cs->cs_defspeed = ZS_DEFSPEED;
			zsc_args.hwflags = 0;
		}

		cs->cs_brg_clk = PCLK / 16;
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

	zsc->zsc_si = softintr_establish(IPL_SOFTSERIAL, zssoft, zsc);
	intr_establish(sa->sa_irq, zshard, self);

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

/*
 * console stuff
 */

static void zs_sbdio_cnprobe(struct consdev *);
static void zs_sbdio_cninit(struct consdev *);

struct consdev consdev_zs_sbdio = {
	zs_sbdio_cnprobe,
	zs_sbdio_cninit,
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
zs_sbdio_cnprobe(struct consdev *cn)
{

	/* not used */
}

static void
zs_sbdio_cninit(struct consdev *cn)
{
	struct zs_chanstate *cs;
	struct zschan *zc;

	zc = zs_consaddr;
	cs = zs_conscs;

	/* Setup temporary chanstate. */
	cs->cs_reg_csr  = &zc->zc_csr;
	cs->cs_reg_data = &zc->zc_data;

	/* Initialize the pending registers. */
	memcpy(cs->cs_preg, zs_init_reg, 16);
	cs->cs_preg[5] |= ZSWR5_DTR | ZSWR5_RTS;

	cs->cs_brg_clk = PCLK / 16;
	cs->cs_defspeed = ZS_DEFSPEED;
	zs_set_speed(cs, ZS_DEFSPEED);

	/* Clear the master interrupt enable. */
	zs_write_reg(cs, 9, 0);

	/* Reset the whole SCC chip. */
	zs_write_reg(cs, 9, ZSWR9_HARD_RESET);

	/* Copy "pending" to "current" and H/W */
	zs_loadchannelregs(cs);
}
