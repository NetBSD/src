/*	$NetBSD: lpt_pcctwo.c,v 1.2 1999/02/14 17:54:28 scw Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Device Driver back-end for the PCCChip2's parallel printer port
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/syslog.h>

#include <machine/cpu.h>

#include <mvme68k/dev/lptvar.h>
#include <mvme68k/dev/pccvar.h>
#include <mvme68k/dev/pcctworeg.h>


static int	lpt_pcctwo_intr __P((void *));
static void	lpt_pcctwo_open __P((struct lpt_softc *, int));
static void	lpt_pcctwo_close __P((struct lpt_softc *));
static void	lpt_pcctwo_iprime __P((struct lpt_softc *));
static void	lpt_pcctwo_speed __P((struct lpt_softc *, int));
static int	lpt_pcctwo_notrdy __P((struct lpt_softc *, int));
static void	lpt_pcctwo_wr_data __P((struct lpt_softc *, u_char));

struct lpt_funcs lpt_pcctwo_funcs = {
	lpt_pcctwo_open,
	lpt_pcctwo_close,
	lpt_pcctwo_iprime,
	lpt_pcctwo_speed,
	lpt_pcctwo_notrdy,
	lpt_pcctwo_wr_data
};

/*
 * Autoconfig stuff
 */
static int  lpt_pcctwo_match  __P((struct device *, struct cfdata *, void *));
static void lpt_pcctwo_attach __P((struct device *, struct device *, void *));

struct cfattach lpt_pcctwo_ca = {
	sizeof(struct lpt_softc), lpt_pcctwo_match, lpt_pcctwo_attach
};

extern struct cfdriver lpt_cd;


/*ARGSUSED*/
static	int
lpt_pcctwo_match(parent, cf, args)
	struct device *parent;
	struct cfdata *cf;
	void *args;
{
	struct pcc_attach_args *pa = args;

	if (strcmp(pa->pa_name, lpt_cd.cd_name))
		return (0);

	pa->pa_ipl = cf->pcccf_ipl;

	return (1);
}

/*ARGSUSED*/
static void
lpt_pcctwo_attach(parent, self, args)
struct	device *parent, *self;
void	*args;
{
	struct lpt_softc *sc = (void *)self;
	struct pcc_attach_args *pa = args;
	struct pcctwo	*regs;

	/*
	 * Get pointer to regs
	 */
	regs = sc->sc_regs = (void *) PCCTWO_VADDR(pa->pa_offset);
	sc->sc_ipl = pa->pa_ipl & PCCTWO_ICR_LEVEL_MASK;
	sc->sc_laststatus = 0;
	sc->sc_funcs = &lpt_pcctwo_funcs;

	printf(": PCCchip2 Parallel Printer\n");

	/*
	 * Disable interrupts until device is opened
	 */
	regs->prt_ack_icr = 0;
	regs->prt_fault_icr = 0;
	regs->prt_sel_icr = 0;
	regs->prt_pe_icr = 0;
	regs->prt_busy_icr = 0;
	regs->prt_ctrl = 0;

	/*
	 * Main attachment code
	 */
	lpt_attach_subr(sc);

	/*
	 * Hook into the printer interrupt
	 */
	pcctwointr_establish(PCCTWOV_PRT_ACK, lpt_pcctwo_intr, sc->sc_ipl, sc);
}

/*
 * Handle printer interrupts
 */
int
lpt_pcctwo_intr(arg)
	void *arg;
{
	struct lpt_softc *sc = (struct lpt_softc *)arg;
	struct pcctwo *regs = sc->sc_regs;
	int i;

	/* is printer online and ready for output */
	if (lpt_pcctwo_notrdy(sc, 0) || lpt_pcctwo_notrdy(sc, 1))
		return 0;

	i = lpt_intr(sc);

	if ( regs->prt_input_sr & PCCTWO_PRT_IN_SR_PINT )
		regs->prt_ack_icr = sc->sc_icr | PCCTWO_ICR_ICLR;

	return i;
}

static void
lpt_pcctwo_open(sc, int_ena)
	struct lpt_softc *sc;
	int int_ena;
{
	struct pcctwo *regs = sc->sc_regs;
	int sps;

	regs->prt_ack_icr = PCCTWO_ICR_ICLR | PCCTWO_ICR_EDGE;
	regs->prt_ctrl |= PCCTWO_PRT_CTRL_DOEN;

	if ( int_ena == 0 ) {
		sps = splhigh();
		sc->sc_icr = sc->sc_ipl | PCCTWO_ICR_EDGE;
		regs->prt_ack_icr = sc->sc_icr;
		splx(sps);
	}
}

static void
lpt_pcctwo_close(sc)
	struct lpt_softc *sc;
{
	struct pcctwo *regs = sc->sc_regs;

	regs->prt_ack_icr = PCCTWO_ICR_ICLR | PCCTWO_ICR_EDGE;
	regs->prt_ctrl = 0;
}

static void
lpt_pcctwo_iprime(sc)
	struct lpt_softc *sc;
{
	struct pcctwo *regs = sc->sc_regs;

	regs->prt_ctrl |= PCCTWO_PRT_CTRL_INP;
	delay(100);
	regs->prt_ctrl &= ~PCCTWO_PRT_CTRL_INP;
	delay(100);
}

static void
lpt_pcctwo_speed(sc, speed)
	struct lpt_softc *sc;
	int speed;
{
	struct pcctwo *regs = sc->sc_regs;

	if ( speed == LPT_STROBE_FAST )
		regs->prt_ctrl |= PCCTWO_PRT_CTRL_FAST;
	else
		regs->prt_ctrl &= ~PCCTWO_PRT_CTRL_FAST;
}

static int
lpt_pcctwo_notrdy(sc, err)
	struct lpt_softc *sc;
	int err;
{
	struct pcctwo *regs = sc->sc_regs;
	u_char status;
	u_char new;

#define	LPS_INVERT	(PCCTWO_PRT_IN_SR_SEL)
#define	LPS_MASK	(PCCTWO_PRT_IN_SR_SEL | PCCTWO_PRT_IN_SR_FLT | \
			 PCCTWO_PRT_IN_SR_BSY | PCCTWO_PRT_IN_SR_PE)

	status = (regs->prt_input_sr ^ LPS_INVERT) & LPS_MASK;

	if ( err ) {
		new = status & ~sc->sc_laststatus;
		sc->sc_laststatus = status;

		if ( new & PCCTWO_PRT_IN_SR_SEL )
			log(LOG_NOTICE, "%s: offline\n",
				sc->sc_dev.dv_xname);
		else if ( new & PCCTWO_PRT_IN_SR_PE )
			log(LOG_NOTICE, "%s: out of paper\n",
				sc->sc_dev.dv_xname);
		else if ( new & PCCTWO_PRT_IN_SR_FLT )
			log(LOG_NOTICE, "%s: output error\n",
				sc->sc_dev.dv_xname);
	}

	return status;
}

static void
lpt_pcctwo_wr_data(sc, data)
	struct lpt_softc *sc;
	u_char data;
{
	struct pcctwo *regs = sc->sc_regs;

	regs->prt_data = (u_short) data;
}
