/*	$NetBSD: zs.c,v 1.33.16.4 2008/01/21 09:37:40 yamt Exp $	*/

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
 *
 * Modified for NetBSD/mvme68k by Jason R. Thorpe <thorpej@NetBSD.org>
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zs.c,v 1.33.16.4 2008/01/21 09:37:40 yamt Exp $");

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
#include <sys/bus.h>
#include <sys/intr.h>

#include <dev/cons.h>
#include <dev/ic/z8530reg.h>
#include <machine/z8530var.h>

#include <mvme68k/dev/zsvar.h>

#include "ioconf.h"

/*
 * Some warts needed by z8530tty.c -
 * The default parity REALLY needs to be the same as the PROM uses,
 * or you can not see messages done with printf during boot-up...
 */
int zs_def_cflag = (CREAD | CS8 | HUPCL);

/* Flags from zscnprobe() */
static int zs_hwflags[NZSC][2];

/* Default speed for each channel */
static int zs_defspeed[NZSC][2] = {
	{ 9600, 	/* port 1 */
	  9600 },	/* port 2 */
	{ 9600, 	/* port 3 */
	  9600 },	/* port 4 */
};

static struct zs_chanstate zs_conschan_store;
static struct zs_chanstate *zs_conschan;

u_char zs_init_reg[16] = {
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
	0,			/*12: BAUDLO (default=9600) */
	0,			/*13: BAUDHI (default=9600) */
	ZSWR14_BAUD_ENA | ZSWR14_BAUD_FROM_PCLK,
	ZSWR15_BREAK_IE,
};


/****************************************************************
 * Autoconfig
 ****************************************************************/

/* Definition of the driver for autoconfig. */
static int	zsc_print(void *, const char *name);
int	zs_getc(void *);
void	zs_putc(void *, int);

#if 0
static int zs_get_speed(struct zs_chanstate *);
#endif

cons_decl(zsc_pcc);


/*
 * Configure children of an SCC.
 */
void
zs_config(struct zsc_softc *zsc, struct zsdevice *zs, int vector, int pclk)
{
	struct zsc_attach_args zsc_args;
	volatile struct zschan *zc;
	struct zs_chanstate *cs;
	int zsc_unit, channel, s;

	zsc_unit = device_unit(&zsc->zsc_dev);
	printf(": Zilog 8530 SCC at vector 0x%x\n", vector);

	/*
	 * Initialize software state for each channel.
	 */
	for (channel = 0; channel < 2; channel++) {
		zsc_args.channel = channel;
		zsc_args.hwflags = zs_hwflags[zsc_unit][channel];
		cs = &zsc->zsc_cs_store[channel];
		zsc->zsc_cs[channel] = cs;
		zs_lock_init(cs);

		/*
		 * If we're the console, copy the channel state, and
		 * adjust the console channel pointer.
		 */
		if (zsc_args.hwflags & ZS_HWFLAG_CONSOLE) {
			memcpy(cs, zs_conschan, sizeof(struct zs_chanstate));
			zs_conschan = cs;
		} else {
			zc = (channel == 0) ? &zs->zs_chan_a : &zs->zs_chan_b;
			cs->cs_reg_csr  = zc->zc_csr;
			cs->cs_reg_data = zc->zc_data;
			memcpy(cs->cs_creg, zs_init_reg, 16);
			memcpy(cs->cs_preg, zs_init_reg, 16);
			cs->cs_defspeed = zs_defspeed[zsc_unit][channel];
		}

		cs->cs_brg_clk = pclk / 16;
		cs->cs_creg[2] = cs->cs_preg[2] = vector;
		zs_set_speed(cs, cs->cs_defspeed);
		cs->cs_creg[12] = cs->cs_preg[12];
		cs->cs_creg[13] = cs->cs_preg[13];
		cs->cs_defcflag = zs_def_cflag;

		/* Make these correspond to cs_defcflag (-crtscts) */
		cs->cs_rr0_dcd = ZSRR0_DCD;
		cs->cs_rr0_cts = 0;
		cs->cs_wr5_dtr = ZSWR5_DTR | ZSWR5_RTS;
		cs->cs_wr5_rts = 0;

		cs->cs_channel = channel;
		cs->cs_private = NULL;
		cs->cs_ops = &zsops_null;

		/*
		 * Clear the master interrupt enable.
		 * The INTENA is common to both channels,
		 * so just do it on the A channel.
		 * Write the interrupt vector while we're at it.
		 */
		if (channel == 0) {
			zs_write_reg(cs, 9, 0);
			zs_write_reg(cs, 2, vector);
		}

		/*
		 * Look for a child driver for this channel.
		 * The child attach will setup the hardware.
		 */
		if (!config_found(&zsc->zsc_dev, (void *)&zsc_args,
		    zsc_print)) {
			/* No sub-driver.  Just reset it. */
			u_char reset = (channel == 0) ?
				ZSWR9_A_RESET : ZSWR9_B_RESET;
			s = splzs();
			zs_write_reg(cs,  9, reset);
			splx(s);
		}
	}

	/*
	 * Allocate a software interrupt cookie.
	 */
	zsc->zsc_softintr_cookie = softint_establish(SOFTINT_SERIAL,
	    (void (*)(void *)) zsc_intr_soft, zsc);
#ifdef DEBUG
	assert(zsc->zsc_softintr_cookie);
#endif
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

#if defined(MVME162) || defined(MVME172)
/*
 * Our ZS chips each have their own interrupt vector.
 */
int
zshard_unshared(void *arg)
{
	struct zsc_softc *zsc = arg;
	int rval;

	rval = zsc_intr_hard(zsc);

	if (rval) {
		if ((zsc->zsc_cs[0]->cs_softreq) ||
		    (zsc->zsc_cs[1]->cs_softreq))
			softint_schedule(zsc->zsc_softintr_cookie);
		zsc->zsc_evcnt.ev_count++;
	}

	return rval;
}
#endif

#ifdef MVME147
/*
 * Our ZS chips all share a common, PCC-vectored interrupt,
 * so we have to look at all of them on each interrupt.
 */
int
zshard_shared(void *arg)
{
	struct zsc_softc *zsc;
	int unit, rval;

	rval = 0;
	for (unit = 0; unit < zsc_cd.cd_ndevs; unit++) {
		zsc = zsc_cd.cd_devs[unit];
		if (zsc != NULL && zsc_intr_hard(zsc)) {
			if ((zsc->zsc_cs[0]->cs_softreq) ||
			    (zsc->zsc_cs[1]->cs_softreq))
				softint_schedule(zsc->zsc_softintr_cookie);
			zsc->zsc_evcnt.ev_count++;
			rval++;
		}
	}
	return rval;
}
#endif


#if 0
/*
 * Compute the current baud rate given a ZSCC channel.
 */
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

	/* Allow 2% tolerance WRT the required bps */
	if (((abs(real_bps - bps) * 1000) / bps) > 20)
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

u_char zs_read_csr(struct zs_chanstate *cs)
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

/****************************************************************
 * Console support functions (MVME specific!)
 ****************************************************************/

/*
 * Polled input char.
 */
int
zs_getc(void *arg)
{
	struct zs_chanstate *cs = arg;
	int s, c, rr0, stat;

	s = splhigh();
 top:
	/* Wait for a character to arrive. */
	do {
		rr0 = *cs->cs_reg_csr;
		ZS_DELAY();
	} while ((rr0 & ZSRR0_RX_READY) == 0);

	/* Read error register. */
	stat = zs_read_reg(cs, 1) & (ZSRR1_FE | ZSRR1_DO | ZSRR1_PE);
	if (stat) {
		zs_write_csr(cs, ZSM_RESET_ERR);
		goto top;
	}

	/* Read character. */
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

/*
 * Common parts of console init.
 */
void
zs_cnconfig(int zsc_unit, int channel, struct zsdevice *zs, int pclk)
{
	struct zs_chanstate *cs;
	struct zschan *zc;

	zc = (channel == 0) ? &zs->zs_chan_a : &zs->zs_chan_b;

	/*
	 * Pointer to channel state.  Later, the console channel
	 * state is copied into the softc, and the console channel
	 * pointer adjusted to point to the new copy.
	 */
	zs_conschan = cs = &zs_conschan_store;
	zs_hwflags[zsc_unit][channel] = ZS_HWFLAG_CONSOLE;

	/* Setup temporary chanstate. */
	cs->cs_brg_clk = pclk / 16;
	cs->cs_reg_csr  = zc->zc_csr;
	cs->cs_reg_data = zc->zc_data;

	/* Initialize the pending registers. */
	memcpy(cs->cs_preg, zs_init_reg, 16);
	cs->cs_preg[5] |= (ZSWR5_DTR | ZSWR5_RTS);

#if 0
	/* XXX: Preserve BAUD rate from boot loader. */
	/* XXX: Also, why reset the chip here? -gwr */
	cs->cs_defspeed = zs_get_speed(cs);
#else
	cs->cs_defspeed = 9600;	/* XXX */
#endif
	zs_set_speed(cs, cs->cs_defspeed);
	cs->cs_creg[12] = cs->cs_preg[12];
	cs->cs_creg[13] = cs->cs_preg[13];

	/* Clear the master interrupt enable. */
	zs_write_reg(cs, 9, 0);

	/* Reset the whole SCC chip. */
	zs_write_reg(cs, 9, ZSWR9_HARD_RESET);

	/* Copy "pending" to "current" and H/W. */
	zs_loadchannelregs(cs);
}

/*
 * Polled console input putchar.
 */
int
zsc_pcccngetc(dev_t dev)
{
	struct zs_chanstate *cs = zs_conschan;
	int c;

	c = zs_getc(cs);
	return c;
}

/*
 * Polled console output putchar.
 */
void
zsc_pcccnputc(dev_t dev, int c)
{
	struct zs_chanstate *cs = zs_conschan;

	zs_putc(cs, c);
}

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

	mvme68k_abort("SERIAL LINE ABORT");
}
