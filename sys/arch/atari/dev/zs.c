/*	$NetBSD: zs.c,v 1.74.12.1 2014/08/20 00:02:48 tls Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)zs.c	8.1 (Berkeley) 7/19/93
 */

/*-
 * Copyright (c) 1995 The NetBSD Foundation, Inc. (Atari modifications)
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Leo Weppelman.
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
 * Zilog Z8530 (ZSCC) driver.
 *
 * Runs two tty ports (modem2 and serial2) on zs0.
 *
 * This driver knows far too much about chip to usage mappings.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zs.c,v 1.74.12.1 2014/08/20 00:02:48 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/tty.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/kauth.h>

#include <machine/cpu.h>
#include <machine/iomap.h>
#include <machine/scu.h>
#include <machine/mfp.h>
#include <atari/dev/ym2149reg.h>

#include <dev/ic/z8530reg.h>
#include <atari/dev/zsvar.h>

#include "ioconf.h"

#include "zs.h"
#if NZS > 1
#error "This driver supports only 1 85C30!"
#endif

#if NZS > 0

#define PCLK	(8053976)	/* PCLK pin input clock rate */
#define PCLK_HD	(9600 * 1536)	/* PCLK on Hades pin input clock rate */

#define splzs	spl5

/*
 * Software state per found chip.
 */
struct zs_softc {
	device_t sc_dev;		/* base device */
	struct zs_chanstate *sc_cs[2];	/* chan A and B software state */

	struct zs_chanstate sc_cs_store[2];
	void *sc_sicookie;		/* for callback */
};

/*
 * Define the registers for a closed port
 */
static uint8_t zs_init_regs[16] = {
/*  0 */	0,
/*  1 */	0,
/*  2 */	0x60,
/*  3 */	0,
/*  4 */	0,
/*  5 */	0,
/*  6 */	0,
/*  7 */	0,
/*  8 */	0,
/*  9 */	ZSWR9_MASTER_IE | ZSWR9_VECTOR_INCL_STAT,
/* 10 */	ZSWR10_NRZ,
/* 11 */	ZSWR11_TXCLK_BAUD | ZSWR11_RXCLK_BAUD,
/* 12 */	0,
/* 13 */	0,
/* 14 */	ZSWR14_BAUD_FROM_PCLK | ZSWR14_BAUD_ENA,
/* 15 */	0
};

/*
 * Define the machine dependent clock frequencies
 * If BRgen feeds sender/receiver we always use a
 * divisor 16, therefor the division by 16 can as
 * well be done here.
 */
static const u_long zs_freqs_tt[] = {
	/*
	 * Atari TT, RTxCB is generated by TT-MFP timer C,
	 * which is set to 307.2 kHz during initialisation
	 * and never changed afterwards.
	 */
	PCLK/16,	/* BRgen, PCLK,  divisor 16	*/
	 229500,	/* BRgen, RTxCA, divisor 16	*/
	3672000,	/* RTxCA, from PCLK4		*/
	      0,	/* TRxCA, external		*/

	PCLK/16,	/* BRgen, PCLK,  divisor 16	*/
	  19200,	/* BRgen, RTxCB, divisor 16	*/
	 307200,	/* RTxCB, from TT-MFP TCO	*/
	2457600		/* TRxCB, from BCLK		*/
};

static const u_long zs_freqs_falcon[] = {
	/*
	 * Atari Falcon, XXX no specs available, this might be wrong
	 */
	PCLK/16,	/* BRgen, PCLK,  divisor 16	*/
	 229500,	/* BRgen, RTxCA, divisor 16	*/
	3672000,	/* RTxCA, ???			*/
	      0,	/* TRxCA, external		*/

	PCLK/16,	/* BRgen, PCLK,  divisor 16	*/
	 229500,	/* BRgen, RTxCB, divisor 16	*/
	3672000,	/* RTxCB, ???			*/
	2457600		/* TRxCB, ???			*/
};

static const u_long zs_freqs_hades[] = {
	/*
	 * XXX: Channel-A unchecked!!!!!
	 */
     PCLK_HD/16,	/* BRgen, PCLK,  divisor 16	*/
	 229500,	/* BRgen, RTxCA, divisor 16	*/
	3672000,	/* RTxCA, from PCLK4		*/
	      0,	/* TRxCA, external		*/

     PCLK_HD/16,	/* BRgen, PCLK,  divisor 16	*/
	 235550,	/* BRgen, RTxCB, divisor 16	*/
	3768800,	/* RTxCB, 3.7688MHz		*/
	3768800		/* TRxCB, 3.7688MHz		*/
};

static const u_long zs_freqs_generic[] = {
	/*
	 * other machines, assume only PCLK is available
	 */
	PCLK/16,	/* BRgen, PCLK,  divisor 16	*/
	      0,	/* BRgen, RTxCA, divisor 16	*/
	      0,	/* RTxCA, unknown		*/
	      0,	/* TRxCA, unknown		*/

	PCLK/16,	/* BRgen, PCLK,  divisor 16	*/
	      0,	/* BRgen, RTxCB, divisor 16	*/
	      0,	/* RTxCB, unknown		*/
	      0		/* TRxCB, unknown		*/
};
static const u_long *zs_frequencies;

/* Definition of the driver for autoconfig. */
static int	zsmatch(device_t, cfdata_t, void *);
static void	zsattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(zs, sizeof(struct zs_softc),
    zsmatch, zsattach, NULL, NULL);

/* {b,c}devsw[] function prototypes */
dev_type_open(zsopen);
dev_type_close(zsclose);
dev_type_read(zsread);
dev_type_write(zswrite);
dev_type_ioctl(zsioctl);
dev_type_stop(zsstop);
dev_type_tty(zstty);
dev_type_poll(zspoll);

const struct cdevsw zs_cdevsw = {
	.d_open = zsopen,
	.d_close = zsclose,
	.d_read = zsread,
	.d_write = zswrite,
	.d_ioctl = zsioctl,
	.d_stop = zsstop,
	.d_tty = zstty,
	.d_poll = zspoll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};

/* Interrupt handlers. */
static int	zshard(void *);
static int	zssoft(void *);
static int	zsrint(struct zs_chanstate *, struct zschan *);
static int	zsxint(struct zs_chanstate *, struct zschan *);
static int	zssint(struct zs_chanstate *, struct zschan *);

/* Routines called from other code. */
static void	zsstart(struct tty *);

/* Routines purely local to this driver. */
static void	zsoverrun(int, long *, const char *);
static int	zsparam(struct tty *, struct termios *);
static int	zsbaudrate(int, int, int *, int *, int *, int *);
static int	zs_modem(struct zs_chanstate *, int, int);
static void	zs_loadchannelregs(struct zschan *, uint8_t *);
static void	zs_shutdown(struct zs_chanstate *);

static int
zsmatch(device_t parent, cfdata_t cf, void *aux)
{
	static int zs_matched = 0;

	if (strcmp("zs", aux) || zs_matched)
		return 0;
	zs_matched = 1;
	return 1;
}

/*
 * Attach a found zs.
 */
static void
zsattach(device_t parent, device_t self, void *aux)
{
	struct zs_softc *sc;
	struct zsdevice *zs;
	struct zschan *zc;
	struct zs_chanstate *cs;
	int channel;

	sc = device_private(self);
	sc->sc_dev = self;

	printf(": serial2 on channel a and modem2 on channel b\n");

	zs = (struct zsdevice *)AD_SCC;

	for (channel = 0; channel < 2; channel++) {
		cs = &sc->sc_cs_store[channel];
		sc->sc_cs[channel] = cs;

		cs->cs_unit = channel;
		cs->cs_zc = zc =
		    (channel == 0) ?  &zs->zs_chan_a : &zs->zs_chan_b;
		/*
		 * Get the command register into a known state.
		 */
		(void)zc->zc_csr;
		(void)zc->zc_csr;

		/*
		 * Do a hardware reset.
		 */
		if (channel == 0) {
			ZS_WRITE(zc, 9, ZSWR9_HARD_RESET);
			delay(50000);	/* enough ? */
			ZS_WRITE(zc, 9, 0);
		}

		/*
		 * Initialize channel
		 */
		zs_loadchannelregs(zc, zs_init_regs);
	}

	if (machineid & ATARI_TT) {
		/*
		 * ininitialise TT-MFP timer C: 307200Hz
		 * timer C and D share one control register:
		 *	bits 0-2 control timer D
		 *	bits 4-6 control timer C
		 */
		int cr = MFP2->mf_tcdcr & 7;
		MFP2->mf_tcdcr = cr;		/* stop timer C  */
		MFP2->mf_tcdr  = 1;		/* counter 1     */
		cr |= T_Q004 << 4;		/* divisor 4     */
		MFP2->mf_tcdcr = cr;		/* start timer C */
		/*
		 * enable scc related interrupts
		 */
		SCU->vme_mask |= SCU_SCC;

		zs_frequencies = zs_freqs_tt;
	} else if (machineid & ATARI_FALCON) {
		zs_frequencies = zs_freqs_falcon;
	} else if (machineid & ATARI_HADES) {
		zs_frequencies = zs_freqs_hades;
	} else {
		zs_frequencies = zs_freqs_generic;
	}

	if (intr_establish(36, USER_VEC, 0, (hw_ifun_t)zshard, sc) == NULL)
		aprint_error_dev(self,
		    "Can't establish interrupt (Rx chan B)\n");
	if (intr_establish(32, USER_VEC, 0, (hw_ifun_t)zshard, sc) == NULL)
		aprint_error_dev(self,
		    "Can't establish interrupt (Tx empty chan B)\n");
	if (intr_establish(34, USER_VEC, 0, (hw_ifun_t)zshard, sc) == NULL)
		aprint_error_dev(self,
		    "Can't establish interrupt (Ext./Status chan B)\n");
	if (intr_establish(38, USER_VEC, 0, (hw_ifun_t)zshard, sc) == NULL)
		aprint_error_dev(self,
		    "Can't establish interrupt (Special Rx cond. chan B)\n");
	if (intr_establish(44, USER_VEC, 0, (hw_ifun_t)zshard, sc) == NULL)
		aprint_error_dev(self,
		    "Can't establish interrupt (Rx chan A)\n");
	if (intr_establish(40, USER_VEC, 0, (hw_ifun_t)zshard, sc) == NULL)
		aprint_error_dev(self,
		    "Can't establish interrupt (Tx empty chan A)\n");
	if (intr_establish(42, USER_VEC, 0, (hw_ifun_t)zshard, sc) == NULL)
		aprint_error_dev(self,
		    "Can't establish interrupt (Ext./Status chan A)\n");
	if (intr_establish(46, USER_VEC, 0, (hw_ifun_t)zshard, sc) == NULL)
		aprint_error_dev(self,
		    "Can't establish interrupt (Special Rx cond. chan A)\n");

	sc->sc_sicookie = softint_establish(SOFTINT_SERIAL,
	    (void (*)(void *))zssoft, sc);
}

/*
 * Open a zs serial port.
 */
int
zsopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct tty *tp;
	struct zs_chanstate *cs;
	struct zs_softc *sc;
	int unit = ZS_UNIT(dev);
	int zs = unit >> 1;
	int error, s;

	sc = device_lookup_private(&zs_cd, zs);
	if (sc == NULL)
		return ENXIO;
	cs = sc->sc_cs[unit & 1];

	/*
	 * When port A (ser02) is selected on the TT, make sure
	 * the port is enabled.
	 */
	if ((machineid & ATARI_TT) && !(unit & 1))
		ym2149_ser2(1);

	if (cs->cs_rbuf == NULL) {
		cs->cs_rbuf = malloc(ZLRB_RING_SIZE * sizeof(int), M_DEVBUF,
		    M_WAITOK);
	}

	tp = cs->cs_ttyp;
	if (tp == NULL) {
		cs->cs_ttyp = tp = tty_alloc();
		tty_attach(tp);
		tp->t_dev   = dev;
		tp->t_oproc = zsstart;
		tp->t_param = zsparam;
	}

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
		return EBUSY;

	s  = spltty();

	/*
	 * Do the following iff this is a first open.
	 */
	if ((tp->t_state & TS_ISOPEN) == 0 && tp->t_wopen == 0) {
		if (tp->t_ispeed == 0) {
			tp->t_iflag = TTYDEF_IFLAG;
			tp->t_oflag = TTYDEF_OFLAG;
			tp->t_cflag = TTYDEF_CFLAG;
			tp->t_lflag = TTYDEF_LFLAG;
			tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		}
		ttychars(tp);
		ttsetwater(tp);

		(void)zsparam(tp, &tp->t_termios);

		/*
		 * Turn on DTR.  We must always do this, even if carrier is not
		 * present, because otherwise we'd have to use TIOCSDTR
		 * immediately after setting CLOCAL, which applications do not
		 * expect.  We always assert DTR while the device is open
		 * unless explicitly requested to deassert it.
		 */
		zs_modem(cs, ZSWR5_RTS|ZSWR5_DTR, DMSET);
		/* May never get a status intr. if DCD already on. -gwr */
		if (((cs->cs_rr0 = cs->cs_zc->zc_csr) & ZSRR0_DCD) != 0)
			tp->t_state |= TS_CARR_ON;
		if (cs->cs_softcar)
			tp->t_state |= TS_CARR_ON;
	}

	splx(s);

	error = ttyopen(tp, ZS_DIALOUT(dev), (flags & O_NONBLOCK));
	if (error)
		goto bad;
	
	error = tp->t_linesw->l_open(dev, tp);
	if (error)
		goto bad;
	return 0;

bad:
	if ((tp->t_state & TS_ISOPEN) == 0 && tp->t_wopen == 0) {
		/*
		 * We failed to open the device, and nobody else had it opened.
		 * Clean up the state as appropriate.
		 */
		zs_shutdown(cs);
	}
	return error;
}

/*
 * Close a zs serial port.
 */
int
zsclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct zs_chanstate *cs;
	struct tty *tp;
	struct zs_softc *sc;
	int unit = ZS_UNIT(dev);

	sc = device_lookup_private(&zs_cd, unit >> 1);
	cs = sc->sc_cs[unit & 1];
	tp = cs->cs_ttyp;

	tp->t_linesw->l_close(tp, flags);
	ttyclose(tp);

	if ((tp->t_state & TS_ISOPEN) == 0 && tp->t_wopen == 0) {
		/*
		 * Although we got a last close, the device may still be in
		 * use; e.g. if this was the dialout node, and there are still
		 * processes waiting for carrier on the non-dialout node.
		 */
		zs_shutdown(cs);
	}
	return 0;
}

/*
 * Read/write zs serial port.
 */
int
zsread(dev_t dev, struct uio *uio, int flags)
{
	struct zs_chanstate *cs;
	struct zs_softc *sc;
	struct tty *tp;
	int unit;

	unit = ZS_UNIT(dev);
	sc   = device_lookup_private(&zs_cd, unit >> 1);
	cs   = sc->sc_cs[unit & 1];
	tp   = cs->cs_ttyp;

	return (*tp->t_linesw->l_read)(tp, uio, flags);
}

int
zswrite(dev_t dev, struct uio *uio, int flags)
{
	struct zs_chanstate *cs;
	struct zs_softc *sc;
	struct tty *tp;
	int unit;

	unit = ZS_UNIT(dev);
	sc   = device_lookup_private(&zs_cd, unit >> 1);
	cs   = sc->sc_cs[unit & 1];
	tp   = cs->cs_ttyp;

	return (*tp->t_linesw->l_write)(tp, uio, flags);
}

int
zspoll(dev_t dev, int events, struct lwp *l)
{
	struct zs_chanstate *cs;
	struct zs_softc *sc;
	struct tty *tp;
	int unit;

	unit = ZS_UNIT(dev);
	sc   = device_lookup_private(&zs_cd, unit >> 1);
	cs   = sc->sc_cs[unit & 1];
	tp   = cs->cs_ttyp;
 
	return (*tp->t_linesw->l_poll)(tp, events, l);
}

struct tty *
zstty(dev_t dev)
{
	struct zs_chanstate *cs;
	struct zs_softc *sc;
	int unit;

	unit = ZS_UNIT(dev);
	sc   = device_lookup_private(&zs_cd, unit >> 1);
	cs   = sc->sc_cs[unit & 1];
	return cs->cs_ttyp;
}

/*
 * ZS hardware interrupt.  Scan all ZS channels.  NB: we know here that
 * channels are kept in (A,B) pairs.
 *
 * Do just a little, then get out; set a software interrupt if more
 * work is needed.
 *
 * We deliberately ignore the vectoring Zilog gives us, and match up
 * only the number of `reset interrupt under service' operations, not
 * the order.
 */

int
zshard(void *arg)
{
	struct zs_softc *sc;
	struct zs_chanstate *cs0, *cs1;
	struct zschan *zc;
	int intflags, v, i;
	uint8_t rr3;

	sc = arg;
	intflags = 0;
	cs0 = sc->sc_cs[0];
	cs1 = sc->sc_cs[1];

	do {
		intflags &= ~4;
		rr3 = ZS_READ(cs0->cs_zc, 3);
		if (rr3 & (ZSRR3_IP_A_RX | ZSRR3_IP_A_TX | ZSRR3_IP_A_STAT)) {
			intflags |= 4 | 2;
			zc = cs0->cs_zc;
			i  = cs0->cs_rbput;
			if ((rr3 & ZSRR3_IP_A_RX) != 0 &&
			    (v = zsrint(cs0, zc)) != 0) {
				cs0->cs_rbuf[i++ & ZLRB_RING_MASK] = v;
				intflags |= 1;
			}
			if ((rr3 & ZSRR3_IP_A_TX) != 0 &&
			    (v = zsxint(cs0, zc)) != 0) {
				cs0->cs_rbuf[i++ & ZLRB_RING_MASK] = v;
				intflags |= 1;
			}
			if ((rr3 & ZSRR3_IP_A_STAT) != 0 &&
			    (v = zssint(cs0, zc)) != 0) {
				cs0->cs_rbuf[i++ & ZLRB_RING_MASK] = v;
				intflags |= 1;
			}
			cs0->cs_rbput = i;
		}
		if (rr3 & (ZSRR3_IP_B_RX | ZSRR3_IP_B_TX | ZSRR3_IP_B_STAT)) {
			intflags |= 4 | 2;
			zc = cs1->cs_zc;
			i  = cs1->cs_rbput;
			if ((rr3 & ZSRR3_IP_B_RX) != 0 &&
			    (v = zsrint(cs1, zc)) != 0) {
				cs1->cs_rbuf[i++ & ZLRB_RING_MASK] = v;
				intflags |= 1;
			}
			if ((rr3 & ZSRR3_IP_B_TX) != 0 &&
			    (v = zsxint(cs1, zc)) != 0) {
				cs1->cs_rbuf[i++ & ZLRB_RING_MASK] = v;
				intflags |= 1;
			}
			if ((rr3 & ZSRR3_IP_B_STAT) != 0 &&
			    (v = zssint(cs1, zc)) != 0) {
				cs1->cs_rbuf[i++ & ZLRB_RING_MASK] = v;
				intflags |= 1;
			}
			cs1->cs_rbput = i;
		}
	} while (intflags & 4);

	if (intflags & 1)
		softint_schedule(sc->sc_sicookie);

	return intflags & 2;
}

static int
zsrint(struct zs_chanstate *cs, struct zschan *zc)
{
	int c;

	/*
	 * First read the status, because read of the received char
	 * destroy the status of this char.
	 */
	c = ZS_READ(zc, 1);
	c |= (zc->zc_data << 8);

	/* clear receive error & interrupt condition */
	zc->zc_csr = ZSWR0_RESET_ERRORS;
	zc->zc_csr = ZSWR0_CLR_INTR;

	return ZRING_MAKE(ZRING_RINT, c);
}

static int
zsxint(struct zs_chanstate *cs, struct zschan *zc)
{
	int i = cs->cs_tbc;

	if (i == 0) {
		zc->zc_csr = ZSWR0_RESET_TXINT;
		zc->zc_csr = ZSWR0_CLR_INTR;
		return ZRING_MAKE(ZRING_XINT, 0);
	}
	cs->cs_tbc = i - 1;
	zc->zc_data = *cs->cs_tba++;
	zc->zc_csr = ZSWR0_CLR_INTR;
	return 0;
}

static int
zssint(struct zs_chanstate *cs, struct zschan *zc)
{
	int rr0;

	rr0 = zc->zc_csr;
	zc->zc_csr = ZSWR0_RESET_STATUS;
	zc->zc_csr = ZSWR0_CLR_INTR;
	/*
	 * The chip's hardware flow control is, as noted in zsreg.h,
	 * busted---if the DCD line goes low the chip shuts off the
	 * receiver (!).  If we want hardware CTS flow control but do
	 * not have it, and carrier is now on, turn HFC on; if we have
	 * HFC now but carrier has gone low, turn it off.
	 */
	if (rr0 & ZSRR0_DCD) {
		if (cs->cs_ttyp->t_cflag & CCTS_OFLOW &&
		    (cs->cs_creg[3] & ZSWR3_HFC) == 0) {
			cs->cs_creg[3] |= ZSWR3_HFC;
			ZS_WRITE(zc, 3, cs->cs_creg[3]);
		}
	} else {
		if (cs->cs_creg[3] & ZSWR3_HFC) {
			cs->cs_creg[3] &= ~ZSWR3_HFC;
			ZS_WRITE(zc, 3, cs->cs_creg[3]);
		}
	}
	return ZRING_MAKE(ZRING_SINT, rr0);
}

/*
 * Print out a ring or fifo overrun error message.
 */
static void
zsoverrun(int unit, long *ptime, const char *what)
{
	time_t cur_sec = time_second;

	if (*ptime != cur_sec) {
		*ptime = cur_sec;
		log(LOG_WARNING, "zs%d%c: %s overrun\n", unit >> 1,
		    (unit & 1) + 'a', what);
	}
}

/*
 * ZS software interrupt.  Scan all channels for deferred interrupts.
 */
int
zssoft(void *arg)
{
	struct zs_softc *sc;
	struct zs_chanstate *cs;
	struct zschan *zc;
	struct linesw *line;
	struct tty *tp;
	int chan, get, n, c, cc, s;
	int retval = 0;

	sc = arg;
	s = spltty();
	for (chan = 0; chan < 2; chan++) {
		cs = sc->sc_cs[chan];
		get = cs->cs_rbget;
again:
		n = cs->cs_rbput;	/* atomic			*/
		if (get == n)		/* nothing more on this line	*/
			continue;
		retval = 1;
		zc     = cs->cs_zc;
		tp     = cs->cs_ttyp;
		line   = tp->t_linesw;
		/*
		 * Compute the number of interrupts in the receive ring.
		 * If the count is overlarge, we lost some events, and
		 * must advance to the first valid one.  It may get
		 * overwritten if more data are arriving, but this is
		 * too expensive to check and gains nothing (we already
		 * lost out; all we can do at this point is trade one
		 * kind of loss for another).
		 */
		n -= get;
		if (n > ZLRB_RING_SIZE) {
			zsoverrun(chan, &cs->cs_rotime, "ring");
			get += n - ZLRB_RING_SIZE;
			n    = ZLRB_RING_SIZE;
		}
		while (--n >= 0) {
			/* race to keep ahead of incoming interrupts */
			c = cs->cs_rbuf[get++ & ZLRB_RING_MASK];
			switch (ZRING_TYPE(c)) {

			case ZRING_RINT:
				c = ZRING_VALUE(c);
				if ((c & ZSRR1_DO) != 0)
					zsoverrun(chan, &cs->cs_fotime, "fifo");
				cc = c >> 8;
				if ((c & ZSRR1_FE) != 0)
					cc |= TTY_FE;
				if ((c & ZSRR1_PE) != 0)
					cc |= TTY_PE;
				line->l_rint(cc, tp);
				break;

			case ZRING_XINT:
				/*
				 * Transmit done: change registers and resume,
				 * or clear BUSY.
				 */
				if (cs->cs_heldchange) {
					int sps;

					sps = splzs();
					c = zc->zc_csr;
					if ((c & ZSRR0_DCD) == 0)
						cs->cs_preg[3] &= ~ZSWR3_HFC;
					memcpy((void *)cs->cs_creg,
					    (void *)cs->cs_preg, 16);
					zs_loadchannelregs(zc, cs->cs_creg);
					splx(sps);
					cs->cs_heldchange = 0;
					if (cs->cs_heldtbc &&
					    (tp->t_state & TS_TTSTOP) == 0) {
						cs->cs_tbc = cs->cs_heldtbc - 1;
						zc->zc_data = *cs->cs_tba++;
						goto again;
					}
				}
				tp->t_state &= ~TS_BUSY;
				if ((tp->t_state & TS_FLUSH) != 0)
					tp->t_state &= ~TS_FLUSH;
				else
					ndflush(&tp->t_outq,
					    cs->cs_tba - tp->t_outq.c_cf);
				line->l_start(tp);
				break;

			case ZRING_SINT:
				/*
				 * Status line change.  HFC bit is run in
				 * hardware interrupt, to avoid locking
				 * at splzs here.
				 */
				c = ZRING_VALUE(c);
				if (((c ^ cs->cs_rr0) & ZSRR0_DCD) != 0) {
					cc = (c & ZSRR0_DCD) != 0;
					if (line->l_modem(tp, cc) == 0)
						zs_modem(cs,
						    ZSWR5_RTS | ZSWR5_DTR,
						    cc ? DMBIS : DMBIC);
				}
				cs->cs_rr0 = c;
				break;

			default:
				log(LOG_ERR, "zs%d%c: bad ZRING_TYPE (%x)\n",
				    chan >> 1, (chan & 1) + 'a', c);
				break;
			}
		}
		cs->cs_rbget = get;
		goto again;
	}
	splx(s);
	return retval;
}

int
zsioctl(dev_t dev, u_long cmd, void * data, int flag, struct lwp *l)
{
	int unit = ZS_UNIT(dev);
	struct zs_softc *sc = device_lookup_private(&zs_cd, unit >> 1);
	struct zs_chanstate *cs = sc->sc_cs[unit & 1];
	struct tty *tp = cs->cs_ttyp;
	int error, s;

	error = tp->t_linesw->l_ioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	error = ttioctl(tp, cmd, data, flag, l);
	if (error !=EPASSTHROUGH)
		return error;

	switch (cmd) {
	case TIOCSBRK:
		s = splzs();
		cs->cs_preg[5] |= ZSWR5_BREAK;
		cs->cs_creg[5] |= ZSWR5_BREAK;
		ZS_WRITE(cs->cs_zc, 5, cs->cs_creg[5]);
		splx(s);
		break;
	case TIOCCBRK:
		s = splzs();
		cs->cs_preg[5] &= ~ZSWR5_BREAK;
		cs->cs_creg[5] &= ~ZSWR5_BREAK;
		ZS_WRITE(cs->cs_zc, 5, cs->cs_creg[5]);
		splx(s);
		break;
	case TIOCGFLAGS: {
		int bits = 0;

		if (cs->cs_softcar)
			bits |= TIOCFLAG_SOFTCAR;
		if ((cs->cs_creg[15] & ZSWR15_DCD_IE) != 0)
			bits |= TIOCFLAG_CLOCAL;
		if ((cs->cs_creg[3] & ZSWR3_HFC) != 0)
			bits |= TIOCFLAG_CRTSCTS;
		*(int *)data = bits;
		break;
	}
	case TIOCSFLAGS: {
		int userbits = 0;

		error = kauth_authorize_device_tty(l->l_cred,
		    KAUTH_DEVICE_TTY_PRIVSET, tp);
		if (error != 0)
			return EPERM;

		userbits = *(int *)data;

		/*
		 * can have `local' or `softcar', and `rtscts' or `mdmbuf'
		 # defaulting to software flow control.
		 */
		if ((userbits & TIOCFLAG_SOFTCAR) != 0 &&
		    (userbits & TIOCFLAG_CLOCAL) != 0)
			return EINVAL;
		if ((userbits & TIOCFLAG_MDMBUF) != 0)
			/* don't support this (yet?) */
			return ENODEV;

		s = splzs();
		if ((userbits & TIOCFLAG_SOFTCAR) != 0) {
			cs->cs_softcar = 1;	/* turn on softcar */
			cs->cs_preg[15] &= ~ZSWR15_DCD_IE; /* turn off dcd */
			cs->cs_creg[15] &= ~ZSWR15_DCD_IE;
			ZS_WRITE(cs->cs_zc, 15, cs->cs_creg[15]);
		} else if ((userbits & TIOCFLAG_CLOCAL) != 0) {
			cs->cs_softcar = 0; 	/* turn off softcar */
			cs->cs_preg[15] |= ZSWR15_DCD_IE; /* turn on dcd */
			cs->cs_creg[15] |= ZSWR15_DCD_IE;
			ZS_WRITE(cs->cs_zc, 15, cs->cs_creg[15]);
			tp->t_termios.c_cflag |= CLOCAL;
		}
		if ((userbits & TIOCFLAG_CRTSCTS) != 0) {
			cs->cs_preg[15] |= ZSWR15_CTS_IE;
			cs->cs_creg[15] |= ZSWR15_CTS_IE;
			ZS_WRITE(cs->cs_zc, 15, cs->cs_creg[15]);
			cs->cs_preg[3] |= ZSWR3_HFC;
			cs->cs_creg[3] |= ZSWR3_HFC;
			ZS_WRITE(cs->cs_zc, 3, cs->cs_creg[3]);
			tp->t_termios.c_cflag |= CRTSCTS;
		} else {
			/* no mdmbuf, so we must want software flow control */
			cs->cs_preg[15] &= ~ZSWR15_CTS_IE;
			cs->cs_creg[15] &= ~ZSWR15_CTS_IE;
			ZS_WRITE(cs->cs_zc, 15, cs->cs_creg[15]);
			cs->cs_preg[3] &= ~ZSWR3_HFC;
			cs->cs_creg[3] &= ~ZSWR3_HFC;
			ZS_WRITE(cs->cs_zc, 3, cs->cs_creg[3]);
			tp->t_termios.c_cflag &= ~CRTSCTS;
		}
		splx(s);
		break;
	}
	case TIOCSDTR:
		zs_modem(cs, ZSWR5_DTR, DMBIS);
		break;
	case TIOCCDTR:
		zs_modem(cs, ZSWR5_DTR, DMBIC);
		break;
	case TIOCMGET:
		zs_modem(cs, 0, DMGET);
		break;
	case TIOCMSET:
	case TIOCMBIS:
	case TIOCMBIC:
	default:
		return EPASSTHROUGH;
	}
	return 0;
}

/*
 * Start or restart transmission.
 */
static void
zsstart(struct tty *tp)
{
	struct zs_chanstate *cs;
	int s, nch;
	int unit = ZS_UNIT(tp->t_dev);
	struct zs_softc *sc = device_lookup_private(&zs_cd, unit >> 1);

	cs = sc->sc_cs[unit & 1];
	s  = spltty();

	/*
	 * If currently active or delaying, no need to do anything.
	 */
	if ((tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP)) != 0)
		goto out;

	/*
	 * If there are sleepers, and output has drained below low
	 * water mark, awaken.
	 */
	ttypull(tp);

	nch = ndqb(&tp->t_outq, 0);	/* XXX */
	if (nch) {
		char *p = tp->t_outq.c_cf;

		/* mark busy, enable tx done interrupts, & send first byte */
		tp->t_state |= TS_BUSY;
		(void)splzs();
		cs->cs_preg[1] |= ZSWR1_TIE;
		cs->cs_creg[1] |= ZSWR1_TIE;
		ZS_WRITE(cs->cs_zc, 1, cs->cs_creg[1]);
		cs->cs_zc->zc_data = *p;
		cs->cs_tba = p + 1;
		cs->cs_tbc = nch - 1;
	} else {
		/*
		 * Nothing to send, turn off transmit done interrupts.
		 * This is useful if something is doing polled output.
		 */
		(void)splzs();
		cs->cs_preg[1] &= ~ZSWR1_TIE;
		cs->cs_creg[1] &= ~ZSWR1_TIE;
		ZS_WRITE(cs->cs_zc, 1, cs->cs_creg[1]);
	}
out:
	splx(s);
}

/*
 * Stop output, e.g., for ^S or output flush.
 */
void
zsstop(struct tty *tp, int flag)
{
	struct zs_chanstate *cs;
	int s, unit = ZS_UNIT(tp->t_dev);
	struct zs_softc *sc = device_lookup_private(&zs_cd, unit >> 1);

	cs = sc->sc_cs[unit & 1];
	s  = splzs();
	if ((tp->t_state & TS_BUSY) != 0) {
		/*
		 * Device is transmitting; must stop it.
		 */
		cs->cs_tbc = 0;
		if ((tp->t_state & TS_TTSTOP) == 0)
			tp->t_state |= TS_FLUSH;
	}
	splx(s);
}

static void
zs_shutdown(struct zs_chanstate *cs)
{
	struct tty *tp = cs->cs_ttyp;
	int s;

	s = splzs();

	/*
	 * Hang up if necessary.  Wait a bit, so the other side has time to
	 * notice even if we immediately open the port again.
	 */
	if ((tp->t_cflag & HUPCL) != 0) {
		zs_modem(cs, 0, DMSET);
		(void)tsleep((void *)cs, TTIPRI, ttclos, hz);
	}

	/* Clear any break condition set with TIOCSBRK. */
	if ((cs->cs_creg[5] & ZSWR5_BREAK) != 0) {
		cs->cs_preg[5] &= ~ZSWR5_BREAK;
		cs->cs_creg[5] &= ~ZSWR5_BREAK;
		ZS_WRITE(cs->cs_zc, 5, cs->cs_creg[5]);
	}

	/*
	 * Drop all lines and cancel interrupts
	 */
	zs_loadchannelregs(cs->cs_zc, zs_init_regs);
	splx(s);
}

/*
 * Set ZS tty parameters from termios.
 *
 * This routine makes use of the fact that only registers
 * 1, 3, 4, 5, 9, 10, 11, 12, 13, 14, and 15 are written.
 */
static int
zsparam(struct tty *tp, struct termios *t)
{
	int unit = ZS_UNIT(tp->t_dev);
	struct zs_softc *sc = device_lookup_private(&zs_cd, unit >> 1);
	struct zs_chanstate *cs = sc->sc_cs[unit & 1];
	int cdiv = 0;	/* XXX gcc4 -Wuninitialized */
	int clkm = 0;	/* XXX gcc4 -Wuninitialized */
	int brgm = 0;	/* XXX gcc4 -Wuninitialized */
	int tcon = 0;	/* XXX gcc4 -Wuninitialized */
	int tmp, tmp5, cflag, s;

	tmp  = t->c_ospeed;
	tmp5 = t->c_ispeed;
	if (tmp < 0 || (tmp5 && tmp5 != tmp))
		return EINVAL;
	if (tmp == 0) {
		/* stty 0 => drop DTR and RTS */
		zs_modem(cs, 0, DMSET);
		return 0;
	}
	tmp = zsbaudrate(unit, tmp, &cdiv, &clkm, &brgm, &tcon);
	if (tmp < 0)
		return EINVAL;
	tp->t_ispeed = tp->t_ospeed = tmp;

	cflag = tp->t_cflag = t->c_cflag;
	if ((cflag & CSTOPB) != 0)
		cdiv |= ZSWR4_TWOSB;
	else
		cdiv |= ZSWR4_ONESB;
	if ((cflag & PARODD) == 0)
		cdiv |= ZSWR4_EVENP;
	if ((cflag & PARENB) != 0)
		cdiv |= ZSWR4_PARENB;

	switch (cflag & CSIZE) {
	case CS5:
		tmp  = ZSWR3_RX_5;
		tmp5 = ZSWR5_TX_5;
		break;
	case CS6:
		tmp  = ZSWR3_RX_6;
		tmp5 = ZSWR5_TX_6;
		break;
	case CS7:
		tmp  = ZSWR3_RX_7;
		tmp5 = ZSWR5_TX_7;
		break;
	case CS8:
	default:
		tmp  = ZSWR3_RX_8;
		tmp5 = ZSWR5_TX_8;
		break;
	}
	tmp  |= ZSWR3_RX_ENABLE;
	tmp5 |= ZSWR5_TX_ENABLE | ZSWR5_DTR | ZSWR5_RTS;

	/*
	 * Block interrupts so that state will not
	 * be altered until we are done setting it up.
	 */
	s = splzs();
	cs->cs_preg[4]  = cdiv;
	cs->cs_preg[11] = clkm;
	cs->cs_preg[12] = tcon;
	cs->cs_preg[13] = tcon >> 8;
	cs->cs_preg[14] = brgm;
	cs->cs_preg[1]  = ZSWR1_RIE | ZSWR1_TIE | ZSWR1_SIE;
	cs->cs_preg[9]  = ZSWR9_MASTER_IE | ZSWR9_VECTOR_INCL_STAT;
	cs->cs_preg[10] = ZSWR10_NRZ;
	cs->cs_preg[15] = ZSWR15_BREAK_IE | ZSWR15_DCD_IE;

	/*
	 * Output hardware flow control on the chip is horrendous: if
	 * carrier detect drops, the receiver is disabled.  Hence we
	 * can only do this when the carrier is on.
	 */
	if ((cflag & CCTS_OFLOW) != 0 &&
	    (cs->cs_zc->zc_csr & ZSRR0_DCD) != 0)
		tmp |= ZSWR3_HFC;
	cs->cs_preg[3] = tmp;
	cs->cs_preg[5] = tmp5;

	/*
	 * If nothing is being transmitted, set up new current values,
	 * else mark them as pending.
	 */
	if (cs->cs_heldchange == 0) {
		if ((cs->cs_ttyp->t_state & TS_BUSY) != 0) {
			cs->cs_heldtbc = cs->cs_tbc;
			cs->cs_tbc = 0;
			cs->cs_heldchange = 1;
		} else {
			memcpy((void *)cs->cs_creg, (void *)cs->cs_preg, 16);
			zs_loadchannelregs(cs->cs_zc, cs->cs_creg);
		}
	}
	splx(s);
	return 0;
}

/*
 * search for the best matching baudrate
 */
static int
zsbaudrate(int unit, int wanted, int *divisor, int *clockmode, int *brgenmode,
    int *timeconst)
{
	int bestdiff, bestbps, source;

	bestdiff = bestbps = 0;
	unit = (unit & 1) << 2;
	for (source = 0; source < 4; ++source) {
		u_long freq = zs_frequencies[unit + source];
		int diff, bps, div, clkm, brgm, tcon;

		bps = div = clkm = brgm = tcon = 0;
		switch (source) {
		case 0:	/* BRgen, PCLK */
			brgm = ZSWR14_BAUD_ENA|ZSWR14_BAUD_FROM_PCLK;
			break;
		case 1:	/* BRgen, RTxC */
			brgm = ZSWR14_BAUD_ENA;
			break;
		case 2: /* RTxC */
			clkm = ZSWR11_RXCLK_RTXC|ZSWR11_TXCLK_RTXC;
			break;
		case 3: /* TRxC */
			clkm = ZSWR11_RXCLK_TRXC|ZSWR11_TXCLK_TRXC;
			break;
		}
		switch (source) {
		case 0:
		case 1:
			div  = ZSWR4_CLK_X16;
			clkm = ZSWR11_RXCLK_BAUD|ZSWR11_TXCLK_BAUD;
			tcon = BPS_TO_TCONST(freq, wanted);
			if (tcon < 0)
				tcon = 0;
			bps  = TCONST_TO_BPS(freq, tcon);
			break;
		case 2:
		case 3:
		    {
			int b1 = freq / 16, d1 = abs(b1 - wanted);
			int b2 = freq / 32, d2 = abs(b2 - wanted);
			int b3 = freq / 64, d3 = abs(b3 - wanted);

			if (d1 < d2 && d1 < d3) {
				div = ZSWR4_CLK_X16;
				bps = b1;
			} else if (d2 < d3 && d2 < d1) {
				div = ZSWR4_CLK_X32;
				bps = b2;
			} else {
				div = ZSWR4_CLK_X64;
				bps = b3;
			}
			brgm = tcon = 0;
			break;
		    }
		}
		diff = abs(bps - wanted);
		if (!source || diff < bestdiff) {
			*divisor   = div;
			*clockmode = clkm;
			*brgenmode = brgm;
			*timeconst = tcon;
			bestbps    = bps;
			bestdiff   = diff;
			if (diff == 0)
				break;
		}
	}
	/* Allow deviations upto 5% */
	if (20 * bestdiff > wanted)
		return -1;
	return bestbps;
}

/*
 * Raise or lower modem control (DTR/RTS) signals.  If a character is
 * in transmission, the change is deferred.
 */
static int
zs_modem(struct zs_chanstate *cs, int bits, int how)
{
	int s, mbits;

	bits  &= ZSWR5_DTR | ZSWR5_RTS;

	s = splzs();
	mbits  = cs->cs_preg[5] &  (ZSWR5_DTR | ZSWR5_RTS);

	switch (how) {
	case DMSET:
		mbits  = bits;
		break;
	case DMBIS:
		mbits |= bits;
		break;
	case DMBIC:
		mbits &= ~bits;
		break;
	case DMGET:
		splx(s);
		return mbits;
	}

	cs->cs_preg[5] = (cs->cs_preg[5] & ~(ZSWR5_DTR | ZSWR5_RTS)) | mbits;
	if (cs->cs_heldchange == 0) {
		if ((cs->cs_ttyp->t_state & TS_BUSY) != 0) {
			cs->cs_heldtbc = cs->cs_tbc;
			cs->cs_tbc = 0;
			cs->cs_heldchange = 1;
		} else {
			ZS_WRITE(cs->cs_zc, 5, cs->cs_creg[5]);
		}
	}
	splx(s);
	return 0;
}

/*
 * Write the given register set to the given zs channel in the proper order.
 * The channel must not be transmitting at the time.  The receiver will
 * be disabled for the time it takes to write all the registers.
 */
static void
zs_loadchannelregs(struct zschan *zc, uint8_t *reg)
{
	int i;

	zc->zc_csr = ZSM_RESET_ERR;	/* reset error condition */
	i = zc->zc_data;		/* drain fifo */
	i = zc->zc_data;
	i = zc->zc_data;
	ZS_WRITE(zc,  4, reg[4]);
	ZS_WRITE(zc, 10, reg[10]);
	ZS_WRITE(zc,  3, reg[3] & ~ZSWR3_RX_ENABLE);
	ZS_WRITE(zc,  5, reg[5] & ~ZSWR5_TX_ENABLE);
	ZS_WRITE(zc,  1, reg[1]);
	ZS_WRITE(zc,  9, reg[9]);
	ZS_WRITE(zc, 11, reg[11]);
	ZS_WRITE(zc, 12, reg[12]);
	ZS_WRITE(zc, 13, reg[13]);
	ZS_WRITE(zc, 14, reg[14]);
	ZS_WRITE(zc, 15, reg[15]);
	ZS_WRITE(zc,  3, reg[3]);
	ZS_WRITE(zc,  5, reg[5]);
	__USE(i);
}
#endif /* NZS > 1 */
