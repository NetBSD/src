/* $NetBSD: zs_ioasic.c,v 1.12 2000/06/03 20:47:41 thorpej Exp $ */

/*-
 * Copyright (c) 1996, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross, Ken Hornstein, and by Jason R. Thorpe of the
 * Numerical Aerospace Simulation Facility, NASA Ames Research Center.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: zs_ioasic.c,v 1.12 2000/06/03 20:47:41 thorpej Exp $");

/*
 * Zilog Z8530 Dual UART driver (machine-dependent part).  This driver
 * handles Z8530 chips attached to the Alpha IOASIC.  Modified for
 * NetBSD/alpha by Ken Hornstein and Jason R. Thorpe.
 *
 * Runs two serial lines per chip using slave drivers.
 * Plain tty/async lines use the zstty slave.
 */

#include "opt_ddb.h"
#include "opt_dec_3000_300.h"
#include "opt_zs_ioasic_dma.h"
#include "zskbd.h"

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
#include <machine/intr.h>
#include <machine/z8530var.h>

#include <dev/cons.h>
#include <dev/ic/z8530reg.h>

#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicreg.h>
#include <dev/tc/ioasicvar.h>
#include <dev/dec/zskbdvar.h>

#include <alpha/tc/zs_ioasicvar.h>

#if 1
#define SPARSE
#endif

/*
 * Helpers for console support.
 */

int	zs_ioasic_cngetc __P((dev_t));
void	zs_ioasic_cnputc __P((dev_t, int));
void	zs_ioasic_cnpollc __P((dev_t, int));

struct consdev zs_ioasic_cons = {
	NULL, NULL, zs_ioasic_cngetc, zs_ioasic_cnputc,
	zs_ioasic_cnpollc, NULL, NODEV, CN_NORMAL,
};

tc_offset_t zs_ioasic_console_offset;
int zs_ioasic_console_channel;
int zs_ioasic_console;

int	zs_ioasic_isconsole __P((tc_offset_t, int));

struct zs_chanstate zs_ioasic_conschanstate_store;
struct zs_chanstate *zs_ioasic_conschanstate;

int	zs_getc __P((struct zs_chanstate *));
void	zs_putc __P((struct zs_chanstate *, int));
void	zs_ioasic_cninit __P((tc_addr_t, tc_offset_t, int));

/*
 * Some warts needed by z8530tty.c
 */
int zs_def_cflag = (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8;
int zs_major = 15;

/*
 * The Alpha provides a 7.372 MHz clock to the ZS chips.
 */
#define	PCLK	(9600 * 768)	/* PCLK pin input clock rate */

/* The layout of this is hardware-dependent (padding, order). */
struct zshan {
	volatile u_int	zc_csr;		/* ctrl,status, and indirect access */
#ifdef SPARSE
	u_int		zc_pad0;
#endif
	volatile u_int	zc_data;	/* data */
#ifdef SPARSE
	u_int		sc_pad1;
#endif
};

struct zsdevice {
	/* Yes, they are backwards. */
	struct	zshan zs_chan_b;
	struct	zshan zs_chan_a;
};

static u_char zs_ioasic_init_reg[16] = {
	0,	/* 0: CMD (reset, etc.) */
	0,	/* 1: No interrupts yet. */
	0xf0,	/* 2: IVECT */
	ZSWR3_RX_8 | ZSWR3_RX_ENABLE,
	ZSWR4_CLK_X16 | ZSWR4_ONESB,
	ZSWR5_TX_8 | ZSWR5_TX_ENABLE,
	0,	/* 6: TXSYNC/SYNCLO */
	0,	/* 7: RXSYNC/SYNCHI */
	0,	/* 8: alias for data port */
	ZSWR9_MASTER_IE | ZSWR9_VECTOR_INCL_STAT,
	0,	/*10: Misc. TX/RX control bits */
	ZSWR11_TXCLK_BAUD | ZSWR11_RXCLK_BAUD,
	22,	/*12: BAUDLO (default=9600) */
	0,	/*13: BAUDHI (default=9600) */
	ZSWR14_BAUD_ENA | ZSWR14_BAUD_FROM_PCLK,
	ZSWR15_BREAK_IE,
};

struct zshan *zs_ioasic_get_chan_addr __P((tc_addr_t, int));

struct zshan *
zs_ioasic_get_chan_addr(zsaddr, channel)
	tc_addr_t zsaddr;
	int channel;
{
	struct zsdevice *addr;
	struct zshan *zc;

	addr = (struct zsdevice *) zsaddr;
#ifdef SPARSE
	addr = (struct zsdevice *) TC_DENSE_TO_SPARSE((tc_addr_t) addr);
#endif

	if (channel == 0)
		zc = &addr->zs_chan_a;
	else
		zc = &addr->zs_chan_b;

	return (zc);
}


/****************************************************************
 * Autoconfig
 ****************************************************************/

/* Definition of the driver for autoconfig. */
int	zs_ioasic_match __P((struct device *, struct cfdata *, void *));
void	zs_ioasic_attach __P((struct device *, struct device *, void *));
int	zs_ioasic_print __P((void *, const char *name));

struct cfattach zsc_ioasic_ca = {
	sizeof(struct zsc_softc), zs_ioasic_match, zs_ioasic_attach
};

/* Interrupt handlers. */
int	zs_ioasic_hardintr __P((void *));
void	zs_ioasic_softintr __P((void *));

/* Misc. */
void	zs_ioasic_enable __P((int));

extern struct cfdriver ioasic_cd;
extern struct cfdriver zsc_cd;

/*
 * Is the zs chip present?
 */
int
zs_ioasic_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct ioasicdev_attach_args *d = aux;
	void *zs_addr;

	if (parent->dv_cfdata->cf_driver != &ioasic_cd)
		return (0);

	/*
	 * Make sure that we're looking for the right kind of device.
	 */
	if (strncmp(d->iada_modname, "z8530   ", TC_ROM_LLEN) != 0 &&
	    strncmp(d->iada_modname, "scc", TC_ROM_LLEN) != 0)
		return (0);

	/*
	 * Check user-specified offset against the ioasic offset.
	 * Allow it to be wildcarded.
	 */
	if (cf->cf_loc[IOASICCF_OFFSET] != IOASICCF_OFFSET_DEFAULT &&
	    cf->cf_loc[IOASICCF_OFFSET] != d->iada_offset)
		return (0);

	/*
	 * Find out the device address, and check it for validity.
	 */
	zs_addr = (void *) d->iada_addr;
#ifdef SPARSE
	zs_addr = (void *) TC_DENSE_TO_SPARSE((tc_addr_t) zs_addr);
#endif
	if (tc_badaddr(zs_addr))
		return (0);

	return (1);
}

/*
 * Attach a found zs.
 */
void
zs_ioasic_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct zsc_softc *zs = (void *) self;
	struct zsc_attach_args zs_args;
	struct zs_chanstate *cs;
	struct ioasicdev_attach_args *d = aux;
	volatile struct zshan *zc;
	tc_addr_t zs_addr;
	int s, channel;

	printf("\n");

	/*
	 * Initialize software state for each channel.
	 */
	for (channel = 0; channel < 2; channel++) {
		zs_args.channel = channel;
		zs_args.hwflags = 0;

		cs = &zs->zsc_cs_store[channel];
		zs->zsc_cs[channel] = cs;

		/*
		 * If we're the console, copy the channel state, and
		 * adjust the console channel pointer.
		 */
		if (zs_ioasic_isconsole(d->iada_offset, channel)) {
			bcopy(zs_ioasic_conschanstate, cs,
			    sizeof(struct zs_chanstate));
			zs_ioasic_conschanstate = cs;
			zs_args.hwflags |= ZS_HWFLAG_CONSOLE;
		} else {
			zs_addr = d->iada_addr;
			zc = zs_ioasic_get_chan_addr(zs_addr, channel);
			cs->cs_reg_csr  = (volatile u_char *)&zc->zc_csr;
			cs->cs_reg_data = (volatile u_char *)&zc->zc_data;

			bcopy(zs_ioasic_init_reg, cs->cs_creg, 16);
			bcopy(zs_ioasic_init_reg, cs->cs_preg, 16);

			cs->cs_defcflag = zs_def_cflag;
			cs->cs_defspeed = 9600;		/* XXX */
			(void) zs_set_modes(cs, cs->cs_defcflag);
		}

		cs->cs_channel = channel;
		cs->cs_ops = &zsops_null;
		cs->cs_brg_clk = PCLK / 16;

		/*
		 * DCD and CTS interrupts are only meaningful on
		 * SCC 0/B.
		 *
		 * XXX This is sorta gross.
		 */
		if (d->iada_offset == 0x00100000 && channel == 1)
			(u_long)cs->cs_private = ZIP_FLAGS_DCDCTS;
		else
			cs->cs_private = NULL;

		/*
		 * Clear the master interrupt enable.
		 * The INTENA is common to both channels,
		 * so just do it on the A channel.
		 */
		if (channel == 0) {
			zs_write_reg(cs, 9, 0);
		}

#ifdef notyet /* XXX thorpej */
		/*
		 * Set up the flow/modem control channel pointer to
		 * deal with the weird wiring on the TC Alpha and
		 * DECstation.
		 */
		if (channel == 1)
			cs->cs_ctl_chan = zs->zsc_cs[0];
		else
			cs->cs_ctl_chan = NULL;
#endif

		/*
		 * Look for a child driver for this channel.
		 * The child attach will setup the hardware.
		 */
		if (config_found(self, (void *)&zs_args, zs_ioasic_print)
		    == NULL) {
			/* No sub-driver.  Just reset it. */
			u_char reset = (channel == 0) ?
				ZSWR9_A_RESET : ZSWR9_B_RESET;
			s = splhigh();
			zs_write_reg(cs, 9, reset);
			splx(s);
		}
	}

	/*
	 * Set up the ioasic interrupt handler.
	 */
	ioasic_intr_establish(parent, d->iada_cookie, TC_IPL_TTY,
	    zs_ioasic_hardintr, zs);
	zs->zsc_sih = softintr_establish(IPL_SOFTSERIAL,
	    zs_ioasic_softintr, zs);
	if (zs->zsc_sih == NULL)
		panic("zs_ioasic_attach: unable to register softintr");

	/*
	 * Set the master interrupt enable and interrupt vector.  The
	 * Sun does this only on one channel.  The old Alpha SCC driver
	 * did it on both.  We'll do it on both.
	 */
	s = splhigh();
	/* interrupt vector */
	zs_write_reg(zs->zsc_cs[0], 2, zs_ioasic_init_reg[2]);
	zs_write_reg(zs->zsc_cs[1], 2, zs_ioasic_init_reg[2]);

	/* master interrupt control (enable) */
	zs_write_reg(zs->zsc_cs[0], 9, zs_ioasic_init_reg[9]);
	zs_write_reg(zs->zsc_cs[1], 9, zs_ioasic_init_reg[9]);

	/* ioasic interrupt enable */
	zs_ioasic_enable(1);
	splx(s);
}

int
zs_ioasic_print(aux, name)
	void *aux;
	const char *name;
{
	struct zsc_attach_args *args = aux;

	if (name != NULL)
		printf("%s:", name);

	if (args->channel != -1)
		printf(" channel %d", args->channel);

	return (UNCONF);
}

/*
 * Enable the SCC interrupts in the ioasic.
 */
void
zs_ioasic_enable(onoff)
	int onoff;
{

	if (onoff) {
		*(volatile u_int *)(ioasic_base + IOASIC_IMSK) |=
		    IOASIC_INTR_SCC_1 | IOASIC_INTR_SCC_0;
#if !defined(DEC_3000_300) && defined(ZS_IOASIC_DMA)
		*(volatile u_int *)(ioasic_base + IOASIC_CSR) |=
		    IOASIC_CSR_DMAEN_T1 | IOASIC_CSR_DMAEN_R1 |
		    IOASIC_CSR_DMAEN_T2 | IOASIC_CSR_DMAEN_R2;
#endif
	} else {
		*(volatile u_int *)(ioasic_base + IOASIC_IMSK) &= 
		    ~(IOASIC_INTR_SCC_1 | IOASIC_INTR_SCC_0);
#if !defined(DEC_3000_300) && defined(ZS_IOASIC_DMA)
		*(volatile u_int *)(ioasic_base + IOASIC_CSR) &=
		    ~(IOASIC_CSR_DMAEN_T1 | IOASIC_CSR_DMAEN_R1 |
		    IOASIC_CSR_DMAEN_T2 | IOASIC_CSR_DMAEN_R2);
#endif
	}
	tc_mb();
}

/*
 * Hardware interrupt handler.
 */
int
zs_ioasic_hardintr(arg)
	void *arg;
{
	struct zsc_softc *zsc = arg;

	/*
	 * Call the upper-level MI hardware interrupt handler.
	 */
	zsc_intr_hard(zsc);

	/*
	 * Check to see if we need to schedule any software-level
	 * processing interrupts.
	 */
	if (zsc->zsc_cs[0]->cs_softreq | zsc->zsc_cs[1]->cs_softreq)
		softintr_schedule(zsc->zsc_sih);

	return (1);
}

/*
 * Software-level interrupt (character processing, lower priority).
 */
void
zs_ioasic_softintr(arg)
	void *arg;
{
	struct zsc_softc *zsc = arg;
	int s;

	s = spltty();
	(void) zsc_intr_soft(zsc);
	splx(s);
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

#ifdef DIAGNOSTIC
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
	u_long privflags = (u_long)cs->cs_private;
	int s;

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

	if ((privflags & ZIP_FLAGS_DCDCTS) == 0) {
		cs->cs_rr0_dcd &= ~(ZSRR0_CTS|ZSRR0_DCD);
		cs->cs_rr0_cts &= ~(ZSRR0_CTS|ZSRR0_DCD);
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

	*((volatile unsigned int *) cs->cs_reg_csr) =
	    ((volatile unsigned int) reg) << 8;
	tc_mb();
	DELAY(5);

	val = ((*(volatile unsigned int *) cs->cs_reg_csr) >> 8) & 0xff;
	tc_mb();
	DELAY(5);

	return (val);
}

void
zs_write_reg(cs, reg, val)
	struct zs_chanstate *cs;
	u_char reg, val;
{

	*((volatile unsigned int *) cs->cs_reg_csr) =
	    ((volatile unsigned int) reg) << 8;
	tc_mb();
	DELAY(5);

	*((volatile unsigned int *) cs->cs_reg_csr) =
	    ((volatile unsigned int) val) << 8;
	tc_mb();
	DELAY(5);
}

u_char
zs_read_csr(cs)
	struct zs_chanstate *cs;
{
	register u_char val;

	val = (*((volatile unsigned int *) cs->cs_reg_csr) >> 8) & 0xff;
	tc_mb();
	DELAY(5);

	return (val);
}

void
zs_write_csr(cs, val)
	struct zs_chanstate *cs;
	u_char val;
{

	*((volatile unsigned int *) cs->cs_reg_csr) =
	    ((volatile unsigned int) val) << 8;
	tc_mb();
	DELAY(5);
}

u_char
zs_read_data(cs)
	struct zs_chanstate *cs;
{
	register u_char val;

	val = (*((volatile unsigned int *) cs->cs_reg_data) >> 8) & 0xff;
	tc_mb();
	DELAY(5);

	return (val);
}

void
zs_write_data(cs, val)
	struct zs_chanstate *cs;
	u_char val;
{

	*((volatile unsigned int *) cs->cs_reg_data) =
	    ((volatile unsigned int) val) << 8;
	tc_mb();
	DELAY(5);
}

/****************************************************************
 * Console support functions (Alpha TC specific!)
 ****************************************************************/

/*
 * Handle user request to enter kernel debugger.
 */
void
zs_abort(cs)
	struct zs_chanstate *cs;
{
	int rr0;

	/* Wait for end of break. */
	/* XXX - Limit the wait? */
	do {
		rr0 = zs_read_csr(cs);
	} while (rr0 & ZSRR0_BREAK);

#if defined(KGDB)
	zskgdb(cs);
#elif defined(DDB)
	Debugger();
#else
	printf("zs_abort: ignoring break on console\n");
#endif
}

/*
 * Polled input char.
 */
int
zs_getc(cs)
	struct zs_chanstate *cs;
{
	int s, c, rr0;

	s = splhigh();
	/* Wait for a character to arrive. */
	do {
		rr0 = zs_read_csr(cs);
	} while ((rr0 & ZSRR0_RX_READY) == 0);

	c = zs_read_data(cs);
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
zs_putc(cs, c)
	struct zs_chanstate *cs;
	int c;
{
	register int s, rr0;

	s = splhigh();
	/* Wait for transmitter to become ready. */
	do {
		rr0 = zs_read_csr(cs);
	} while ((rr0 & ZSRR0_TX_READY) == 0);

	zs_write_data(cs, c);

	/* Wait for the character to be transmitted. */
	do {
		rr0 = zs_read_csr(cs);
	} while ((rr0 & ZSRR0_TX_READY) == 0);
	splx(s);
}

/*****************************************************************/

/*
 * zs_ioasic_cninit --
 *	Initialize the serial channel for console use--either the
 * primary keyboard or as the serial console.
 */
void
zs_ioasic_cninit(ioasic_addr, zs_offset, channel)
	tc_addr_t ioasic_addr;
	tc_offset_t zs_offset;
	int channel;
{
	struct zs_chanstate *cs;
	tc_addr_t zs_addr;
	struct zshan *zc;

	/*
	 * Initialize the console finder helpers.
	 */
	zs_ioasic_console_offset = zs_offset;
	zs_ioasic_console_channel = channel;
	zs_ioasic_console = 1;

	/*
	 * Pointer to channel state.  Later, the console channel
	 * state is copied into the softc, and the console channel
	 * pointer adjusted to point to the new copy.
	 */
	zs_ioasic_conschanstate = cs = &zs_ioasic_conschanstate_store;

	/*
	 * Compute the physical address of the chip, "map" it via
	 * K0SEG, and then get the address of the actual channel.
	 */
	zs_addr = ALPHA_PHYS_TO_K0SEG(ioasic_addr + zs_offset);
	zc = zs_ioasic_get_chan_addr(zs_addr, channel);

	/* Setup temporary chanstate. */
	cs->cs_reg_csr  = (volatile u_char *)&zc->zc_csr;
	cs->cs_reg_data = (volatile u_char *)&zc->zc_data;

	/* Initialize the pending registers. */
	bcopy(zs_ioasic_init_reg, cs->cs_preg, 16);
	cs->cs_preg[5] |= (ZSWR5_DTR | ZSWR5_RTS);

	/*
	 * DCD and CTS interrupts are only meaningful on
	 * SCC 0/B.
	 *
	 * XXX This is sorta gross.
	 */
	if (zs_offset == 0x00100000 && channel == 1)
		(u_long)cs->cs_private = ZIP_FLAGS_DCDCTS;
	else
		cs->cs_private = NULL;

	/* Clear the master interrupt enable. */
	zs_write_reg(cs, 9, 0);

	/* Reset the whole SCC chip. */
	zs_write_reg(cs, 9, ZSWR9_HARD_RESET);

	/* Copy "pending" to "current" and H/W. */
	zs_loadchannelregs(cs);
}

/*
 * zs_ioasic_cnattach --
 *	Initialize and attach a serial console.
 */
int
zs_ioasic_cnattach(ioasic_addr, zs_offset, channel, rate, cflag)
	tc_addr_t ioasic_addr;
	tc_offset_t zs_offset;
	int channel, rate, cflag;
{
	zs_ioasic_cninit(ioasic_addr, zs_offset, channel);

	zs_ioasic_conschanstate->cs_defspeed = rate;
	zs_ioasic_conschanstate->cs_defcflag = cflag;

	/* Point the console at the SCC. */
	cn_tab = &zs_ioasic_cons;

	return (0);
}

/*
 * zs_ioasic_lk201_cnattach --
 *	Initialize and attach the primary keyboard.
 */
int
zs_ioasic_lk201_cnattach(ioasic_addr, zs_offset, channel)
	tc_addr_t ioasic_addr;
	tc_offset_t zs_offset;
	int channel;
{
#if (NZSKBD > 0)
	zs_ioasic_cninit(ioasic_addr, zs_offset, channel);
	zs_ioasic_conschanstate->cs_defspeed = 4800;
	zs_ioasic_conschanstate->cs_defcflag =
	     (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8;
	zs_ioasic_conschanstate->cs_brg_clk = PCLK / 16;
	return (zskbd_cnattach(zs_ioasic_conschanstate));
#else
	return (ENXIO);
#endif
}

int
zs_ioasic_isconsole(offset, channel)
	tc_offset_t offset;
	int channel;
{

	if (zs_ioasic_console &&
	    offset == zs_ioasic_console_offset &&
	    channel == zs_ioasic_console_channel)
		return (1);

	return (0);
}

/*
 * Polled console input putchar.
 */
int
zs_ioasic_cngetc(dev)
	dev_t dev;
{

	return (zs_getc(zs_ioasic_conschanstate));
}

/*
 * Polled console output putchar.
 */
void
zs_ioasic_cnputc(dev, c)
	dev_t dev;
	int c;
{

	zs_putc(zs_ioasic_conschanstate, c);
}

/*
 * Set polling/no polling on console.
 */
void
zs_ioasic_cnpollc(dev, onoff)
	dev_t dev;
	int onoff;
{

	/* XXX ??? */
}
