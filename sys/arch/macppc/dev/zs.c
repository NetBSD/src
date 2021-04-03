/*	$NetBSD: zs.c,v 1.50.66.1 2021/04/03 22:28:29 thorpej Exp $	*/

/*
 * Copyright (c) 1996, 1998 Bill Studenmund
 * Copyright (c) 1995 Gordon W. Ross
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Zilog Z8530 Dual UART driver (machine-dependent part)
 *
 * Runs two serial lines per chip using slave drivers.
 * Plain tty/async lines use the zs_async slave.
 * Sun keyboard/mouse uses the zs_kbd/zs_ms slaves.
 * Other ports use their own mice & keyboard slaves.
 *
 * Credits & history:
 *
 * With NetBSD 1.1, port-mac68k started using a port of the port-sparc
 * (port-sun3?) zs.c driver (which was in turn based on code in the
 * Berkeley 4.4 Lite release). Bill Studenmund did the port, with
 * help from Allen Briggs and Gordon Ross <gwr@NetBSD.org>. Noud de
 * Brouwer field-tested the driver at a local ISP.
 *
 * Bill Studenmund and Gordon Ross then ported the machine-independent
 * z8530 driver to work with port-mac68k. NetBSD 1.2 contained an
 * intermediate version (mac68k using a local, patched version of
 * the m.i. drivers), with NetBSD 1.3 containing a full version.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zs.c,v 1.50.66.1 2021/04/03 22:28:29 thorpej Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/intr.h>
#include <sys/cpu.h>
#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include <dev/cons.h>
#include <dev/ofw/openfirm.h>
#include <dev/ic/z8530reg.h>

#include <machine/z8530var.h>
#include <machine/autoconf.h>
#include <machine/pio.h>

/* Are these in a header file anywhere? */
/* Booter flags interface */
#define ZSMAC_RAW	0x01
#define ZSMAC_LOCALTALK	0x02

/*
 * Some warts needed by z8530tty.c -
 */
int zs_def_cflag = (CREAD | CS8 | HUPCL);

/*
 * abort detection on console will now timeout after iterating on a loop
 * the following # of times. Cheep hack. Also, abort detection is turned
 * off after a timeout (i.e. maybe there's not a terminal hooked up).
 */
#define ZSABORT_DELAY 3000000

struct zsdevice {
	/* Yes, they are backwards. */
	struct	zschan zs_chan_b;
	struct	zschan zs_chan_a;
};

static int zs_defspeed[2] = {
	38400,		/* ttyZ0 */
	38400,		/* ttyZ1 */
};

/* console stuff */
void	*zs_conschan = 0;
int	zs_conschannel = -1;
#ifdef	ZS_CONSOLE_ABORT
int	zs_cons_canabort = 1;
#else
int	zs_cons_canabort = 0;
#endif /* ZS_CONSOLE_ABORT*/

/* device to which the console is attached--if serial. */
/* Mac stuff */

static int zs_get_speed(struct zs_chanstate *);

/*
 * Even though zsparam will set up the clock multiples, etc., we
 * still set them here as: 1) mice & keyboards don't use zsparam,
 * and 2) the console stuff uses these defaults before device
 * attach.
 */

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
	((PCLK/32)/38400)-2,	/*12: BAUDLO (default=38400) */
	0,			/*13: BAUDHI (default=38400) */
	ZSWR14_BAUD_ENA,
	ZSWR15_BREAK_IE,
};

/****************************************************************
 * Autoconfig
 ****************************************************************/

/* Definition of the driver for autoconfig. */
static int	zsc_match(device_t, cfdata_t, void *);
static void	zsc_attach(device_t, device_t, void *);
static int	zsc_print(void *, const char *);

CFATTACH_DECL_NEW(zsc, sizeof(struct zsc_softc),
    zsc_match, zsc_attach, NULL, NULL);

extern struct cfdriver zsc_cd;

int zsc_attached;

int zshard(void *);
#ifdef ZS_TXDMA
static int zs_txdma_int(void *);
#endif

void zscnprobe(struct consdev *);
void zscninit(struct consdev *);
int  zscngetc(dev_t);
void zscnputc(dev_t, int);
void zscnpollc(dev_t, int);

/*
 * Is the zs chip present?
 */
static int
zsc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "escc") != 0)
		return 0;

	if (zsc_attached)
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
zsc_attach(device_t parent, device_t self, void *aux)
{
	struct zsc_softc *zsc = device_private(self);
	struct confargs *ca = aux;
	struct zsc_attach_args zsc_args;
	volatile struct zschan *zc;
	struct xzs_chanstate *xcs;
	struct zs_chanstate *cs;
	struct zsdevice *zsd;
	int channel;
	int s, chip, theflags;
	int node, intr[2][3];
	u_int regs[6];
	char intr_xname[INTRDEVNAMEBUF];

	zsc_attached = 1;

	zsc->zsc_dev = self;

	chip = 0;
	ca->ca_reg[0] += ca->ca_baseaddr;
	zsd = mapiodev(ca->ca_reg[0], ca->ca_reg[1], false);

	node = OF_child(ca->ca_node);	/* ch-a */

	for (channel = 0; channel < 2; channel++) {
		if (OF_getprop(node, "AAPL,interrupts",
			       intr[channel], sizeof(intr[0])) == -1 &&
		    OF_getprop(node, "interrupts",
			       intr[channel], sizeof(intr[0])) == -1) {
			aprint_error(": cannot find interrupt property\n");
			return;
		}

		if (OF_getprop(node, "reg", regs, sizeof(regs)) < 24) {
			aprint_error(": cannot find reg property\n");
			return;
		}
		regs[2] += ca->ca_baseaddr;
		regs[4] += ca->ca_baseaddr;
#ifdef ZS_TXDMA
		zsc->zsc_txdmareg[channel] = mapiodev(regs[2], regs[3], false);
		zsc->zsc_txdmacmd[channel] =
			dbdma_alloc(sizeof(dbdma_command_t) * 3);
		memset(zsc->zsc_txdmacmd[channel], 0,
			sizeof(dbdma_command_t) * 3);
		dbdma_reset(zsc->zsc_txdmareg[channel]);
#endif
		node = OF_peer(node);	/* ch-b */
	}

	aprint_normal(" irq %d,%d\n", intr[0][0], intr[1][0]);

	/*
	 * Initialize software state for each channel.
	 */
	for (channel = 0; channel < 2; channel++) {
		zsc_args.channel = channel;
		zsc_args.hwflags = (channel == zs_conschannel ?
				    ZS_HWFLAG_CONSOLE : 0);
		xcs = &zsc->xzsc_xcs_store[channel];
		cs  = &xcs->xzs_cs;
		zsc->zsc_cs[channel] = cs;

		zs_lock_init(cs);
		cs->cs_channel = channel;
		cs->cs_private = NULL;
		cs->cs_ops = &zsops_null;

		zc = (channel == 0) ? &zsd->zs_chan_a : &zsd->zs_chan_b;

		cs->cs_reg_csr  = &zc->zc_csr;
		cs->cs_reg_data = &zc->zc_data;

		memcpy(cs->cs_creg, zs_init_reg, 16);
		memcpy(cs->cs_preg, zs_init_reg, 16);

		/* Current BAUD rate generator clock. */
		cs->cs_brg_clk = PCLK / 16;	/* RTxC is 230400*16, so use 230400 */
		if (zsc_args.hwflags & ZS_HWFLAG_CONSOLE)
			cs->cs_defspeed = zs_get_speed(cs);
		else
			cs->cs_defspeed = zs_defspeed[channel];
		cs->cs_defcflag = zs_def_cflag;

		/* Make these correspond to cs_defcflag (-crtscts) */
		cs->cs_rr0_dcd = ZSRR0_DCD;
		cs->cs_rr0_cts = 0;
		cs->cs_wr5_dtr = ZSWR5_DTR;
		cs->cs_wr5_rts = 0;

#ifdef __notyet__
		cs->cs_slave_type = ZS_SLAVE_NONE;
#endif

		/* Define BAUD rate stuff. */
		xcs->cs_clocks[0].clk = PCLK;
		xcs->cs_clocks[0].flags = ZSC_RTXBRG | ZSC_RTXDIV;
		xcs->cs_clocks[1].flags =
			ZSC_RTXBRG | ZSC_RTXDIV | ZSC_VARIABLE | ZSC_EXTERN;
		xcs->cs_clocks[2].flags = ZSC_TRXDIV | ZSC_VARIABLE;
		xcs->cs_clock_count = 3;
		if (channel == 0) {
			theflags = 0; /*mac68k_machine.modem_flags;*/
			/*xcs->cs_clocks[1].clk = mac68k_machine.modem_dcd_clk;*/
			/*xcs->cs_clocks[2].clk = mac68k_machine.modem_cts_clk;*/
			xcs->cs_clocks[1].clk = 0;
			xcs->cs_clocks[2].clk = 0;
		} else {
			theflags = 0; /*mac68k_machine.print_flags;*/
			xcs->cs_clocks[1].flags = ZSC_VARIABLE;
			/*
			 * Yes, we aren't defining ANY clock source enables for the
			 * printer's DCD clock in. The hardware won't let us
			 * use it. But a clock will freak out the chip, so we
			 * let you set it, telling us to bar interrupts on the line.
			 */
			/*xcs->cs_clocks[1].clk = mac68k_machine.print_dcd_clk;*/
			/*xcs->cs_clocks[2].clk = mac68k_machine.print_cts_clk;*/
			xcs->cs_clocks[1].clk = 0;
			xcs->cs_clocks[2].clk = 0;
		}
		if (xcs->cs_clocks[1].clk)
			zsc_args.hwflags |= ZS_HWFLAG_NO_DCD;
		if (xcs->cs_clocks[2].clk)
			zsc_args.hwflags |= ZS_HWFLAG_NO_CTS;

		/* Set defaults in our "extended" chanstate. */
		xcs->cs_csource = 0;
		xcs->cs_psource = 0;
		xcs->cs_cclk_flag = 0;  /* Nothing fancy by default */
		xcs->cs_pclk_flag = 0;

		if (theflags & ZSMAC_RAW) {
			zsc_args.hwflags |= ZS_HWFLAG_RAW;
			printf(" (raw defaults)");
		}

		/*
		 * XXX - This might be better done with a "stub" driver
		 * (to replace zstty) that ignores LocalTalk for now.
		 */
		if (theflags & ZSMAC_LOCALTALK) {
			printf(" shielding from LocalTalk");
			cs->cs_defspeed = 1;
			cs->cs_creg[ZSRR_BAUDLO] = cs->cs_preg[ZSRR_BAUDLO] = 0xff;
			cs->cs_creg[ZSRR_BAUDHI] = cs->cs_preg[ZSRR_BAUDHI] = 0xff;
			zs_write_reg(cs, ZSRR_BAUDLO, 0xff);
			zs_write_reg(cs, ZSRR_BAUDHI, 0xff);
			/*
			 * If we might have LocalTalk, then make sure we have the
			 * Baud rate low-enough to not do any damage.
			 */
		}

		/*
		 * We used to disable chip interrupts here, but we now
		 * do that in zscnprobe, just in case MacOS left the chip on.
		 */

		xcs->cs_chip = chip;

		/* Stash away a copy of the final H/W flags. */
		xcs->cs_hwflags = zsc_args.hwflags;

		/*
		 * Look for a child driver for this channel.
		 * The child attach will setup the hardware.
		 */
		if (!config_found(self, (void *)&zsc_args, zsc_print)) {
			/* No sub-driver.  Just reset it. */
			uint8_t reset = (channel == 0) ?
				ZSWR9_A_RESET : ZSWR9_B_RESET;
			s = splzs();
			zs_write_reg(cs, 9, reset);
			splx(s);
		}
	}

	/* XXX - Now safe to install interrupt handlers. */
	for (channel = 0; channel < 2; channel++) {
		snprintf(intr_xname, sizeof(intr_xname), "%s pio%d",
		    device_xname(self), channel);
		intr_establish_xname(intr[channel][0], IST_EDGE, IPL_TTY,
		    zshard, zsc, intr_xname);
#ifdef ZS_TXDMA
		snprintf(intr_xname, sizeof(intr_xname), "%s dma%d",
		    device_xname(self), channel);
		intr_establish_xname(intr[channel][1], IST_EDGE, IPL_TTY,
		    zs_txdma_int, (void *)channel, intr_xname);
#endif
	}

	zsc->zsc_si = softint_establish(SOFTINT_SERIAL,
		(void (*)(void *)) zsc_intr_soft, zsc);

	/*
	 * Set the master interrupt enable and interrupt vector.
	 * (common to both channels, do it on A)
	 */
	cs = zsc->zsc_cs[0];
	s = splzs();
	/* interrupt vector */
	zs_write_reg(cs, 2, zs_init_reg[2]);
	/* master interrupt control (enable) */
	zs_write_reg(cs, 9, zs_init_reg[9]);
	splx(s);
}

static int
zsc_print(void *aux, const char *name)
{
	struct zsc_attach_args *args = aux;

	if (name != NULL)
		aprint_normal("%s: ", name);

	if (args->channel != -1)
		aprint_normal(" channel %d", args->channel);

	return UNCONF;
}

int
zsmdioctl(struct zs_chanstate *cs, u_long cmd, void *data)
{
	switch (cmd) {
	default:
		return (EPASSTHROUGH);
	}
	return (0);
}

void
zsmd_setclock(struct zs_chanstate *cs)
{
#ifdef NOTYET
	struct xzs_chanstate *xcs = (void *)cs;

	if (cs->cs_channel != 0)
		return;

	/*
	 * If the new clock has the external bit set, then select the
	 * external source.
	 */
	via_set_modem((xcs->cs_pclk_flag & ZSC_EXTERN) ? 1 : 0);
#endif
}

int
zshard(void *arg)
{
	struct zsc_softc *zsc;
	int rval;

	zsc = arg;
	rval = zsc_intr_hard(zsc);
	if ((zsc->zsc_cs[0]->cs_softreq) || (zsc->zsc_cs[1]->cs_softreq))
		softint_schedule(zsc->zsc_si);

	return rval;
}

#ifdef ZS_TXDMA
int
zs_txdma_int(void *arg)
{
	int ch = (int)arg;
	struct zsc_softc *zsc;
	struct zs_chanstate *cs;

	zsc = device_lookup_private(&zsc_cd, ch);
	if (zsc == NULL)
		panic("zs_txdma_int");

	cs = zsc->zsc_cs[ch];
	zstty_txdma_int(cs);

	if (cs->cs_softreq)
		softint_schedule(zsc->zsc_si);

	return 1;
}

void
zs_dma_setup(struct zs_chanstate *cs, void *pa, int len)
{
	struct zsc_softc *zsc;
	dbdma_command_t *cmdp;
	int ch = cs->cs_channel;

	zsc = device_lookup_private(&zsc_cd, ch);
	cmdp = zsc->zsc_txdmacmd[ch];

	DBDMA_BUILD(cmdp, DBDMA_CMD_OUT_LAST, 0, len, kvtop(pa),
		DBDMA_INT_ALWAYS, DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);
	cmdp++;
	DBDMA_BUILD(cmdp, DBDMA_CMD_STOP, 0, 0, 0,
		DBDMA_INT_NEVER, DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);

	__asm volatile("eieio");

	dbdma_start(zsc->zsc_txdmareg[ch], zsc->zsc_txdmacmd[ch]);
}
#endif

/*
 * Compute the current baud rate given a ZS channel.
 * XXX Assume internal BRG.
 */
int
zs_get_speed(struct zs_chanstate *cs)
{
	int tconst;

	tconst = zs_read_reg(cs, 12);
	tconst |= zs_read_reg(cs, 13) << 8;
	return TCONST_TO_BPS(cs->cs_brg_clk, tconst);
}

#ifndef ZS_TOLERANCE
#define ZS_TOLERANCE 51
/* 5% in tenths of a %, plus 1 so that exactly 5% will be ok. */
#endif

/*
 * Search through the signal sources in the channel, and
 * pick the best one for the baud rate requested. Return
 * a -1 if not achievable in tolerance. Otherwise return 0
 * and fill in the values.
 *
 * This routine draws inspiration from the Atari port's zs.c
 * driver in NetBSD 1.1 which did the same type of source switching.
 * Tolerance code inspired by comspeed routine in isa/com.c.
 *
 * By Bill Studenmund, 1996-05-12
 */
int
zs_set_speed(struct zs_chanstate *cs, int bps)
{
	struct xzs_chanstate *xcs = (void *) cs;
	int i, tc, tc0 = 0, tc1, s, sf = 0;
	int src, rate0, rate1, err, tol;

	if (bps == 0)
		return (0);

	src = -1;		/* no valid source yet */
	tol = ZS_TOLERANCE;

	/*
	 * Step through all the sources and see which one matches
	 * the best. A source has to match BETTER than tol to be chosen.
	 * Thus if two sources give the same error, the first one will be
	 * chosen. Also, allow for the possability that one source might run
	 * both the BRG and the direct divider (i.e. RTxC).
	 */
	for (i = 0; i < xcs->cs_clock_count; i++) {
		if (xcs->cs_clocks[i].clk <= 0)
			continue;	/* skip non-existent or bad clocks */
		if (xcs->cs_clocks[i].flags & ZSC_BRG) {
			/* check out BRG at /16 */
			tc1 = BPS_TO_TCONST(xcs->cs_clocks[i].clk >> 4, bps);
			if (tc1 >= 0) {
				rate1 = TCONST_TO_BPS(xcs->cs_clocks[i].clk >> 4, tc1);
				err = abs(((rate1 - bps)*1000)/bps);
				if (err < tol) {
					tol = err;
					src = i;
					sf = xcs->cs_clocks[i].flags & ~ZSC_DIV;
					tc0 = tc1;
					rate0 = rate1;
				}
			}
		}
		if (xcs->cs_clocks[i].flags & ZSC_DIV) {
			/*
			 * Check out either /1, /16, /32, or /64
			 * Note: for /1, you'd better be using a synchronized
			 * clock!
			 */
			int b0 = xcs->cs_clocks[i].clk, e0 = abs(b0-bps);
			int b1 = b0 >> 4, e1 = abs(b1-bps);
			int b2 = b1 >> 1, e2 = abs(b2-bps);
			int b3 = b2 >> 1, e3 = abs(b3-bps);

			if (e0 < e1 && e0 < e2 && e0 < e3) {
				err = e0;
				rate1 = b0;
				tc1 = ZSWR4_CLK_X1;
			} else if (e0 > e1 && e1 < e2  && e1 < e3) {
				err = e1;
				rate1 = b1;
				tc1 = ZSWR4_CLK_X16;
			} else if (e0 > e2 && e1 > e2 && e2 < e3) {
				err = e2;
				rate1 = b2;
				tc1 = ZSWR4_CLK_X32;
			} else {
				err = e3;
				rate1 = b3;
				tc1 = ZSWR4_CLK_X64;
			}

			err = (err * 1000)/bps;
			if (err < tol) {
				tol = err;
				src = i;
				sf = xcs->cs_clocks[i].flags & ~ZSC_BRG;
				tc0 = tc1;
				rate0 = rate1;
			}
		}
	}
#ifdef ZSMACDEBUG
	zsprintf("Checking for rate %d. Found source #%d.\n",bps, src);
#endif
	if (src == -1)
		return (EINVAL); /* no can do */

	/*
	 * The M.I. layer likes to keep cs_brg_clk current, even though
	 * we are the only ones who should be touching the BRG's rate.
	 *
	 * Note: we are assuming that any ZSC_EXTERN signal source comes in
	 * on the RTxC pin. Correct for the mac68k obio zsc.
	 */
	if (sf & ZSC_EXTERN)
		cs->cs_brg_clk = xcs->cs_clocks[i].clk >> 4;
	else
		cs->cs_brg_clk = PCLK / 16;

	/*
	 * Now we have a source, so set it up.
	 */
	s = splzs();
	xcs->cs_psource = src;
	xcs->cs_pclk_flag = sf;
	bps = rate0;
	if (sf & ZSC_BRG) {
		cs->cs_preg[4] = ZSWR4_CLK_X16;
		cs->cs_preg[11]= ZSWR11_RXCLK_BAUD | ZSWR11_TXCLK_BAUD;
		if (sf & ZSC_PCLK) {
			cs->cs_preg[14] = ZSWR14_BAUD_ENA | ZSWR14_BAUD_FROM_PCLK;
		} else {
			cs->cs_preg[14] = ZSWR14_BAUD_ENA;
		}
		tc = tc0;
	} else {
		cs->cs_preg[4] = tc0;
		if (sf & ZSC_RTXDIV) {
			cs->cs_preg[11] = ZSWR11_RXCLK_RTXC | ZSWR11_TXCLK_RTXC;
		} else {
			cs->cs_preg[11] = ZSWR11_RXCLK_TRXC | ZSWR11_TXCLK_TRXC;
		}
		cs->cs_preg[14]= 0;
		tc = 0xffff;
	}
	/* Set the BAUD rate divisor. */
	cs->cs_preg[12] = tc;
	cs->cs_preg[13] = tc >> 8;
	splx(s);
	
#ifdef ZSMACDEBUG
	zsprintf("Rate is %7d, tc is %7d, source no. %2d, flags %4x\n", \
	    bps, tc, src, sf);
	zsprintf("Registers are: 4 %x, 11 %x, 14 %x\n\n",
		cs->cs_preg[4], cs->cs_preg[11], cs->cs_preg[14]);
#endif

	cs->cs_preg[5] |= ZSWR5_RTS;	/* Make sure the drivers are on! */

	/* Caller will stuff the pending registers. */
	return (0);
}

int
zs_set_modes(struct zs_chanstate *cs, int cflag)
{
	struct xzs_chanstate *xcs = (void*)cs;
	int s;

	/*
	 * Make sure we don't enable hfc on a signal line we're ignoring.
	 * As we enable CTS interrupts only if we have CRTSCTS or CDTRCTS,
	 * this code also effectivly turns off ZSWR15_CTS_IE.
	 *
	 * Also, disable DCD interrupts if we've been told to ignore
	 * the DCD pin. Happens on mac68k because the input line for
	 * DCD can also be used as a clock input.  (Just set CLOCAL.)
	 *
	 * If someone tries to turn an invalid flow mode on, Just Say No
	 * (Suggested by gwr)
	 */
	if ((cflag & CDTRCTS) && (cflag & (CRTSCTS | MDMBUF)))
		return (EINVAL);
	if (xcs->cs_hwflags & ZS_HWFLAG_NO_DCD) {
		if (cflag & MDMBUF)
			return (EINVAL);
		cflag |= CLOCAL;
	}
	if ((xcs->cs_hwflags & ZS_HWFLAG_NO_CTS) && (cflag & (CRTSCTS | CDTRCTS)))
		return (EINVAL);

	/*
	 * Output hardware flow control on the chip is horrendous:
	 * if carrier detect drops, the receiver is disabled, and if
	 * CTS drops, the transmitter is stoped IN MID CHARACTER!
	 * Therefore, NEVER set the HFC bit, and instead use the
	 * status interrupt to detect CTS changes.
	 */
	s = splzs();
	if ((cflag & (CLOCAL | MDMBUF)) != 0)
		cs->cs_rr0_dcd = 0;
	else
		cs->cs_rr0_dcd = ZSRR0_DCD;
	/*
	 * The mac hardware only has one output, DTR (HSKo in Mac
	 * parlance). In HFC mode, we use it for the functions
	 * typically served by RTS and DTR on other ports, so we
	 * have to fake the upper layer out some.
	 *
	 * CRTSCTS we use CTS as an input which tells us when to shut up.
	 * We make no effort to shut up the other side of the connection.
	 * DTR is used to hang up the modem.
	 *
	 * In CDTRCTS, we use CTS to tell us to stop, but we use DTR to
	 * shut up the other side.
	 */
	if ((cflag & CRTSCTS) != 0) {
		cs->cs_wr5_dtr = ZSWR5_DTR;
		cs->cs_wr5_rts = 0;
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
		cs->cs_wr5_dtr = ZSWR5_DTR;
		cs->cs_wr5_rts = 0;
		cs->cs_rr0_cts = 0;
	}
	splx(s);

	/* Caller will stuff the pending registers. */
	return (0);
}


/*
 * Read or write the chip with suitable delays.
 * MacII hardware has the delay built in.
 * No need for extra delay. :-) However, some clock-chirped
 * macs, or zsc's on serial add-on boards might need it.
 */
#define	ZS_DELAY()

uint8_t
zs_read_reg(struct zs_chanstate *cs, uint8_t reg)
{
	uint8_t val;

	out8(cs->cs_reg_csr, reg);
	ZS_DELAY();
	val = in8(cs->cs_reg_csr);
	ZS_DELAY();
	return val;
}

void
zs_write_reg(struct zs_chanstate *cs, uint8_t reg, uint8_t val)
{
	out8(cs->cs_reg_csr, reg);
	ZS_DELAY();
	out8(cs->cs_reg_csr, val);
	ZS_DELAY();
}

uint8_t
zs_read_csr(struct zs_chanstate *cs)
{
	uint8_t val;

	val = in8(cs->cs_reg_csr);
	ZS_DELAY();
	/* make up for the fact CTS is wired backwards */
	val ^= ZSRR0_CTS;
	return val;
}

void
zs_write_csr(struct zs_chanstate *cs, uint8_t val)
{
	/* Note, the csr does not write CTS... */
	out8(cs->cs_reg_csr, val);
	ZS_DELAY();
}

uint8_t
zs_read_data(struct zs_chanstate *cs)
{
	uint8_t val;

	val = in8(cs->cs_reg_data);
	ZS_DELAY();
	return val;
}

void
zs_write_data(struct zs_chanstate *cs, uint8_t val)
{
	out8(cs->cs_reg_data, val);
	ZS_DELAY();
}

/****************************************************************
 * Console support functions (powermac specific!)
 * Note: this code is allowed to know about the layout of
 * the chip registers, and uses that to keep things simple.
 * XXX - I think I like the mvme167 code better. -gwr
 * XXX - Well :-P  :-)  -wrs
 ****************************************************************/

#define zscnpollc	nullcnpollc
cons_decl(zs);

static int stdin, stdout;

/*
 * Console functions.
 */

/*
 * zscnprobe is the routine which gets called as the kernel is trying to
 * figure out where the console should be. Each io driver which might
 * be the console (as defined in mac68k/conf.c) gets probed. The probe
 * fills in the consdev structure. Important parts are the device #,
 * and the console priority. Values are CN_DEAD (don't touch me),
 * CN_NORMAL (I'm here, but elsewhere might be better), CN_INTERNAL
 * (the video, better than CN_NORMAL), and CN_REMOTE (pick me!)
 *
 * As the mac's a bit different, we do extra work here. We mainly check
 * to see if we have serial echo going on. Also chould check for default
 * speeds.
 */

/*
 * Polled input char.
 */
int
zs_getc(void *v)
{
	volatile struct zschan *zc = v;
	int s, c, rr0;

	s = splhigh();
	/* Wait for a character to arrive. */
	do {
		rr0 = in8(&zc->zc_csr);
		ZS_DELAY();
	} while ((rr0 & ZSRR0_RX_READY) == 0);

	c = in8(&zc->zc_data);
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
zs_putc(void *v, int c)
{
	volatile struct zschan *zc = v;
	int s, rr0;
	long wait = 0;

	s = splhigh();
	/* Wait for transmitter to become ready. */
	do {
		rr0 = in8(&zc->zc_csr);
		ZS_DELAY();
	} while (((rr0 & ZSRR0_TX_READY) == 0) && (wait++ < 1000000));

	if ((rr0 & ZSRR0_TX_READY) != 0) {
		out8(&zc->zc_data, c);
		ZS_DELAY();
	}
	splx(s);
}


/*
 * Polled console input putchar.
 */
int
zscngetc(dev_t dev)
{
	volatile struct zschan *zc = zs_conschan;
	int c;

	if (zc) {
		c = zs_getc(__UNVOLATILE(zc));
	} else {
		char ch = 0;
		OF_read(stdin, &ch, 1);
		c = ch;
	}
	return c;
}

/*
 * Polled console output putchar.
 */
void
zscnputc(dev_t dev, int c)
{
	volatile struct zschan *zc = zs_conschan;

	if (zc) {
		zs_putc(__UNVOLATILE(zc), c);
	} else {
		char ch = c;
		OF_write(stdout, &ch, 1);
	}
}

/*
 * Handle user request to enter kernel debugger.
 */
void
zs_abort(struct zs_chanstate *cs)
{
	volatile struct zschan *zc = zs_conschan;
	int rr0;
	long wait = 0;

	if (zs_cons_canabort == 0)
		return;

	/* Wait for end of break to avoid PROM abort. */
	do {
		rr0 = in8(&zc->zc_csr);
		ZS_DELAY();
	} while ((rr0 & ZSRR0_BREAK) && (wait++ < ZSABORT_DELAY));

	if (wait > ZSABORT_DELAY) {
		zs_cons_canabort = 0;
	/* If we time out, turn off the abort ability! */
	}

#if defined(KGDB)
	kgdb_connect(1);
#elif defined(DDB)
	Debugger();
#endif
}

extern int ofccngetc(dev_t);
extern void ofccnputc(dev_t, int);

struct consdev consdev_zs = {
	zscnprobe,
	zscninit,
	zscngetc,
	zscnputc,
	zscnpollc,
};

void
zscnprobe(struct consdev *cp)
{
	int chosen, pkg;
	char name[16];
	
	if ((chosen = OF_finddevice("/chosen")) == -1)
		return;

	if (OF_getprop(chosen, "stdin", &stdin, sizeof(stdin)) == -1)
		return;
	if (OF_getprop(chosen, "stdout", &stdout, sizeof(stdout)) == -1)
		return;
	
	if ((pkg = OF_instance_to_package(stdin)) == -1)
		return;

	memset(name, 0, sizeof(name));
	if (OF_getprop(pkg, "device_type", name, sizeof(name)) == -1)
		return;

	if (strcmp(name, "serial") != 0)
		return;

	memset(name, 0, sizeof(name));
	if (OF_getprop(pkg, "name", name, sizeof(name)) == -1)
		return;

	cp->cn_pri = CN_REMOTE;
}

void
zscninit(struct consdev *cp)
{
	int escc, escc_ch, obio, zs_offset;
	u_int32_t reg[5];
	char name[16];

	if ((escc_ch = OF_instance_to_package(stdin)) == -1)
		return;

	memset(name, 0, sizeof(name));
	if (OF_getprop(escc_ch, "name", name, sizeof(name)) == -1)
		return;

	zs_conschannel = strcmp(name, "ch-b") == 0;

	if (OF_getprop(escc_ch, "reg", reg, sizeof(reg)) < 4)
		return;
	zs_offset = reg[0];

	escc = OF_parent(escc_ch);
	obio = OF_parent(escc);

	if (OF_getprop(obio, "assigned-addresses", reg, sizeof(reg)) < 12)
		return;
	zs_conschan = (void *)(reg[2] + zs_offset);
}
