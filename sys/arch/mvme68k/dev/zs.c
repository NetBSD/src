/*	$NetBSD: zs.c,v 1.6 1996/08/26 06:39:03 thorpej Exp $	*/

/*
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 4. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Gordon Ross
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
 *
 * Modified for NetBSD/mvme68k by Jason R. Thorpe <thorpej@NetBSD.ORG>
 */

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

#include <dev/cons.h>
#include <dev/ic/z8530reg.h>
#include <machine/z8530var.h>

#include <machine/cpu.h>

#include <mvme68k/dev/zsvar.h>

static u_long zs_sir;	/* software interrupt cookie */

/* Flags from zscnprobe() */
static int zs_hwflags[NZS][2];

/* Default speed for each channel */
static int zs_defspeed[NZS][2] = {
	{ 9600, 	/* port 1 */
	  9600 },	/* port 2 */
	{ 9600, 	/* port 3 */
	  9600 },	/* port 4 */
};

static struct zs_chanstate zs_conschan_store;
static struct zs_chanstate *zs_conschan;

u_char zs_init_reg[16] = {
	0,	/* 0: CMD (reset, etc.) */
	ZSWR1_RIE | ZSWR1_TIE | ZSWR1_SIE,
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
	14,	/*12: BAUDLO (default=9600) */
	0,	/*13: BAUDHI (default=9600) */
	ZSWR14_BAUD_FROM_PCLK | ZSWR14_BAUD_ENA,
	ZSWR15_BREAK_IE | ZSWR15_DCD_IE,
};


/****************************************************************
 * Autoconfig
 ****************************************************************/

/* Definition of the driver for autoconfig. */
static int	zsc_print __P((void *, char *name));

struct cfdriver zsc_cd = {
	NULL, "zsc", DV_DULL
};


/*
 * Configure children of an SCC.
 */
void
zs_config(zsc, chan_addr)
	struct zsc_softc *zsc;
	struct zschan *(*chan_addr) __P((int, int));
{
	struct zsc_attach_args zsc_args;
	volatile struct zschan *zc;
	struct zs_chanstate *cs;
	int zsc_unit, channel, s;
	u_char reset;

	zsc_unit = zsc->zsc_dev.dv_unit;
	printf(": Zilog 8530 SCC\n");

	/*
	 * Initialize software state for each channel.
	 */
	for (channel = 0; channel < 2; channel++) {
		cs = &zsc->zsc_cs[channel];

		/*
		 * If we're the console, copy the channel state, and
		 * adjust the console channel pointer.
		 */
		if (zs_hwflags[zsc_unit][channel] & ZS_HWFLAG_CONSOLE) {
			bcopy(zs_conschan, cs, sizeof(struct zs_chanstate));
			zs_conschan = cs;
		} else {
			zc = (*chan_addr)(zsc_unit, channel);
			cs->cs_reg_csr  = &zc->zc_csr;
			cs->cs_reg_data = &zc->zc_data;

			/* Define BAUD rate clock for the MI code. */
			cs->cs_brg_clk = PCLK / 16;

			cs->cs_defspeed = zs_defspeed[zsc_unit][channel];

			bcopy(zs_init_reg, cs->cs_creg, 16);
			bcopy(zs_init_reg, cs->cs_preg, 16);
		}

		cs->cs_channel = channel;
		cs->cs_private = NULL;
		cs->cs_ops = &zsops_null;

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
		zsc_args.channel = channel;
		zsc_args.hwflags = zs_hwflags[zsc_unit][channel];
		if (config_found(&zsc->zsc_dev, (void *)&zsc_args,
		    zsc_print) == NULL) {
			/* No sub-driver.  Just reset it. */
			reset = (channel == 0) ?
				ZSWR9_A_RESET : ZSWR9_B_RESET;
			s = splzs();
			zs_write_reg(cs,  9, reset);
			splx(s);
		}
	}

	/*
	 * Allocate a software interrupt cookie.  Note that the argument
	 * "zsc" is never actually used in the software interrupt
	 * handler.
	 */
	if (zs_sir == 0)
		zs_sir = allocate_sir(zssoft, zsc);
}

static int
zsc_print(aux, name)
	void *aux;
	char *name;
{
	struct zsc_attach_args *args = aux;

	if (name != NULL)
		printf("%s: ", name);

	if (args->channel != -1)
		printf(" channel %d", args->channel);

	return UNCONF;
}

int
zshard(arg)
	void *arg;
{
	struct zsc_softc *zsc;
	int unit, rval;

	rval = 0;
	for (unit = 0; unit < zsc_cd.cd_ndevs; ++unit) {
		zsc = zsc_cd.cd_devs[unit];
		if (zsc != NULL) {
			rval |= zsc_intr_hard(zsc);
		}
	}
	return (rval);
}

int zssoftpending;

void
zsc_req_softint(zsc)
	struct zsc_softc *zsc;
{	
	if (zssoftpending == 0) {
		/* We are at splzs here, so no need to lock. */
		zssoftpending = 1;
		setsoftint(zs_sir);
	}
}

int
zssoft(arg)
	void *arg;
{
	struct zsc_softc *zsc;
	int unit;

	/* This is not the only ISR on this IPL. */
	if (zssoftpending == 0)
		return (0);

	/*
	 * The soft intr. bit will be set by zshard only if
	 * the variable zssoftpending is zero.
	 */
	zssoftpending = 0;

	for (unit = 0; unit < zsc_cd.cd_ndevs; ++unit) {
		zsc = zsc_cd.cd_devs[unit];
		if (zsc != NULL) {
			(void) zsc_intr_soft(zsc);
		}
	}
	return (1);
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
	register u_char v;

	v = *cs->cs_reg_csr;
	ZS_DELAY();
	return v;
}

u_char zs_read_data(cs)
	struct zs_chanstate *cs;
{
	register u_char v;

	v = *cs->cs_reg_data;
	ZS_DELAY();
	return v;
}

void  zs_write_csr(cs, val)
	struct zs_chanstate *cs;
	u_char val;
{
	*cs->cs_reg_csr = val;
	ZS_DELAY();
}

void  zs_write_data(cs, val)
	struct zs_chanstate *cs;
	u_char val;
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
zs_getc(arg)
	void *arg;
{
	register struct zs_chanstate *cs = arg;
	register int s, c, rr0, stat;

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
	register struct zs_chanstate *cs = arg;
	register int s, rr0;

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
zs_cnconfig(zsc_unit, channel, zcp)
	int zsc_unit, channel;
	struct zschan *zcp;
{
	volatile struct zschan *zc = (volatile struct zschan *)zcp;
	struct zs_chanstate *cs;

	/*
	 * Pointer to channel state.  Later, the console channel
	 * state is copied into the softc, and the console channel
	 * pointer adjusted to point to the new copy.
	 */
	zs_conschan = cs = &zs_conschan_store;

	zs_hwflags[zsc_unit][channel] = ZS_HWFLAG_CONSOLE;

	cs->cs_reg_csr  = &zc->zc_csr;
	cs->cs_reg_data = &zc->zc_data;

	cs->cs_channel = channel;
	cs->cs_private = NULL;
	cs->cs_ops = &zsops_null;

	/* Define BAUD rate clock for the MI code. */
	cs->cs_brg_clk = PCLK / 16;

	cs->cs_defspeed = zs_defspeed[zsc_unit][channel];

	bcopy(zs_init_reg, cs->cs_creg, 16);
	bcopy(zs_init_reg, cs->cs_preg, 16);

	/*
	 * Clear the master interrupt enable.
	 * The INTENA is common to both channels,
	 * so just do it on the A channel.
	 */
	if (channel == 0) {
		zs_write_reg(cs, 9, 0);
	}

	/* Reset the SCC chip. */
	zs_write_reg(cs, 9, ZSWR9_HARD_RESET);

	/* Initialize a few important registers. */
	zs_write_reg(cs, 10, cs->cs_creg[10]);
	zs_write_reg(cs, 11, cs->cs_creg[11]);
	zs_write_reg(cs, 14, cs->cs_creg[14]);

	/* Assert DTR and RTS. */
	cs->cs_creg[5] |= (ZSWR5_DTR | ZSWR5_RTS);
	cs->cs_preg[5] |= (ZSWR5_DTR | ZSWR5_RTS);
	zs_write_reg(cs, 5, cs->cs_creg[5]);
}

/*
 * Polled console input putchar.
 */
int
zscngetc(dev)
	dev_t dev;
{
	register volatile struct zs_chanstate *cs = zs_conschan;
	register int c;

	c = zs_getc(cs);
	return (c);
}

/*
 * Polled console output putchar.
 */
void
zscnputc(dev, c)
	dev_t dev;
	int c;
{
	register volatile struct zs_chanstate *cs = zs_conschan;

	zs_putc(cs, c);
}

/*
 * Handle user request to enter kernel debugger.
 */
void
zs_abort()
{
	register volatile struct zs_chanstate *cs = zs_conschan;
	int rr0;

	/* Wait for end of break to avoid PROM abort. */
	/* XXX - Limit the wait? */
	do {
		rr0 = *cs->cs_reg_csr;
		ZS_DELAY();
	} while (rr0 & ZSRR0_BREAK);

	mvme68k_abort("SERIAL LINE ABORT");
}
