/*	$NetBSD: clmpcc_pcctwo.c,v 1.3.4.1 2000/07/22 15:48:06 scw Exp $ */

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
 * Cirrus Logic CD2401 4-channel serial chip. PCCchip2 Front-end.
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

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ic/clmpccvar.h>

#include <mvme68k/dev/mainbus.h>
#include <mvme68k/dev/pcctwovar.h>
#include <mvme68k/dev/pcctworeg.h>


/* Definition of the driver for autoconfig. */
int clmpcc_pcctwo_match __P((struct device *, struct cfdata *, void *));
void clmpcc_pcctwo_attach __P((struct device *, struct device *, void *));
void clmpcc_pcctwo_iackhook __P((struct clmpcc_softc *, int));
void clmpcc_pcctwo_consiackhook __P((struct clmpcc_softc *, int));

struct cfattach clmpcc_pcctwo_ca = {
	sizeof(struct clmpcc_softc), clmpcc_pcctwo_match, clmpcc_pcctwo_attach
};

extern struct cfdriver clmpcc_cd;

/*
 * For clmpccopen() and clmpcccn*()
 */
cdev_decl(clmpcc);
cons_decl(clmpcc);


/*
 * Is the CD2401 chip present?
 */
int
clmpcc_pcctwo_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pcctwo_attach_args *pa;

	pa = aux;

	if (strcmp(pa->pa_name, clmpcc_cd.cd_name))
		return (0);

	pa->pa_ipl = cf->pcctwocf_ipl;

	return (1);
}
/*
 * Attach a found CD2401.
 */
void
clmpcc_pcctwo_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct clmpcc_softc *sc;
	struct pcctwo_attach_args *pa;
	int level = pa->pa_ipl;

	sc = (struct clmpcc_softc *)self;
	pa = aux;
	level = pa->pa_ipl;
	sc->sc_iot = pa->pa_bust;
	bus_space_map(pa->pa_bust, pa->pa_offset, 0x100, 0, &sc->sc_ioh);

	sc->sc_clk = 20000000;
	sc->sc_byteswap = CLMPCC_BYTESWAP_LOW;
	sc->sc_swaprtsdtr = 1;
	sc->sc_iackhook = clmpcc_pcctwo_iackhook;
	sc->sc_vector_base = PCCTWO_SCC_VECBASE;
	sc->sc_rpilr = 0x03;
	sc->sc_tpilr = 0x02;
	sc->sc_mpilr = 0x01;

	/* Do common parts of CD2401 configuration. */
	clmpcc_attach(sc);

	/* Hook the interrupts */
	pcctwointr_establish(PCCTWOV_SCC_RX, clmpcc_rxintr, level, sc);
	pcctwointr_establish(PCCTWOV_SCC_RX_EXCEP, clmpcc_rxintr, level, sc);
	pcctwointr_establish(PCCTWOV_SCC_TX, clmpcc_txintr, level, sc);
	pcctwointr_establish(PCCTWOV_SCC_MODEM, clmpcc_mdintr, level, sc);

	/* Enable the interrupts */
	pcc2_reg_write(sys_pcctwo, PCC2REG_SCC_MODEM_ICSR,
	    level | PCCTWO_ICR_IEN);
	pcc2_reg_write(sys_pcctwo, PCC2REG_SCC_RX_ICSR, level | PCCTWO_ICR_IEN);
	pcc2_reg_write(sys_pcctwo, PCC2REG_SCC_TX_ICSR, level | PCCTWO_ICR_IEN);
}

void
clmpcc_pcctwo_iackhook(sc, which)
	struct clmpcc_softc *sc;
	int which;
{
	bus_size_t offset;
	volatile u_char foo;

	switch (which) {
	case CLMPCC_IACK_MODEM:
		offset = PCC2REG_SCC_MODEM_PIACK;
		break;

	case CLMPCC_IACK_RX:
		offset = PCC2REG_SCC_RX_PIACK;
		break;

	case CLMPCC_IACK_TX:
		offset = PCC2REG_SCC_TX_PIACK;
		break;
#ifdef DEBUG
	default:
		printf("%s: Invalid IACK number '%d'\n",
		    sc->sc_dev.dv_xname, which);
		panic("clmpcc_pcctwo_iackhook");
#endif
	}

	foo = pcc2_reg_read(sys_pcctwo, offset);
}
/*
 * This routine is only used prior to clmpcc_attach() being called
 */
void
clmpcc_pcctwo_consiackhook(sc, which)
	struct clmpcc_softc *sc;
	int which;
{
	bus_space_tag_t bust;
	bus_space_handle_t bush;
	bus_size_t offset;
	volatile u_char foo;

	/*
	 * We need to fake the tag and handle since 'sys_pcctwo' will
	 * be NULL during early system startup...
	 */
	bust = MVME68K_INTIO_BUS_SPACE;
	bush = (bus_space_tag_t) & (intiobase[MAINBUS_PCCTWO_OFFSET +
		PCCTWO_REG_OFF]);

	switch (which) {
	case CLMPCC_IACK_MODEM:
		offset = PCC2REG_SCC_MODEM_PIACK;
		break;

	case CLMPCC_IACK_RX:
		offset = PCC2REG_SCC_RX_PIACK;
		break;

	case CLMPCC_IACK_TX:
		offset = PCC2REG_SCC_TX_PIACK;
		break;
#ifdef DEBUG
	default:
		printf("%s: Invalid IACK number '%d'\n",
		    sc->sc_dev.dv_xname, which);
		panic("clmpcc_pcctwo_consiackhook");
#endif
	}

	foo = bus_space_read_1(bust, bush, offset);
}


/****************************************************************
 * Console support functions (MVME PCCchip2 specific!)
 ****************************************************************/

/*
 * Check for CD2401 console.
 */
void
clmpcccnprobe(cp)
	struct consdev *cp;
{
	int maj;

	if (machineid == MVME_147) {
		cp->cn_pri = CN_DEAD;
		return;
	}

	/*
	 * Locate the major number
	 */
	for (maj = 0; maj < nchrdev; maj++) {
		if (cdevsw[maj].d_open == clmpccopen)
			break;
	}

	/* Initialize required fields. */
	cp->cn_dev = makedev(maj, 0);
	cp->cn_pri = CN_NORMAL;
}

void
clmpcccninit(cp)
	struct consdev *cp;
{
	static struct clmpcc_softc cons_sc;

	cons_sc.sc_iot = MVME68K_INTIO_BUS_SPACE;
	bus_space_map(cons_sc.sc_iot, MAINBUS_PCCTWO_OFFSET + PCCTWO_SCC_OFF,
	    PCC2REG_SIZE, 0, &cons_sc.sc_ioh);
	cons_sc.sc_clk = 20000000;
	cons_sc.sc_byteswap = CLMPCC_BYTESWAP_LOW;
	cons_sc.sc_swaprtsdtr = 1;
	cons_sc.sc_iackhook = clmpcc_pcctwo_consiackhook;
	cons_sc.sc_vector_base = PCCTWO_SCC_VECBASE;
	cons_sc.sc_rpilr = 0x03;
	cons_sc.sc_tpilr = 0x02;
	cons_sc.sc_mpilr = 0x01;

	clmpcc_cnattach(&cons_sc, 0, 9600);
}
