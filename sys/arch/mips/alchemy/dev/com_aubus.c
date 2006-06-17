/* $NetBSD: com_aubus.c,v 1.1.2.4 2006/06/17 05:38:21 gdamore Exp $ */

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: com_aubus.c,v 1.1.2.4 2006/06/17 05:38:21 gdamore Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/tty.h>

#include <machine/bus.h>
#include <dev/ic/comvar.h>

#include <mips/alchemy/include/aureg.h>
#include <mips/alchemy/include/auvar.h>
#include <mips/alchemy/include/aubusvar.h>
#include <mips/alchemy/dev/com_aubus_reg.h>

struct com_aubus_softc {
	struct com_softc sc_com;
	int sc_irq;
	void *sc_ih;
};

static int	com_aubus_probe(struct device *, struct cfdata *, void *);
static void	com_aubus_attach(struct device *, struct device *, void *);
static int	com_aubus_enable(struct com_softc *);
static void	com_aubus_disable(struct com_softc *);
static void	com_aubus_initmap(struct com_regs *);

CFATTACH_DECL(com_aubus, sizeof(struct com_aubus_softc),
    com_aubus_probe, com_aubus_attach, NULL, NULL);

#define CONMODE	((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */

#ifndef	COM_REGMAP
#error	COM_REGMAP not defined!
#endif

int
com_aubus_probe(struct device *parent, struct cfdata *cf, void *aux)
{
	struct aubus_attach_args *aa = aux;

	/* match only aucom devices */
	if (strcmp(aa->aa_name, cf->cf_name) == 0)
		return (1);

	return (0);
}

void
com_aubus_attach(struct device *parent, struct device *self, void *aux)
{
	struct com_aubus_softc *asc = (void *)self;
	struct com_softc *sc = &asc->sc_com;
	struct aubus_attach_args *aa = aux;
	int addr = aa->aa_addr;
	
	sc->sc_regs.cr_iot = aa->aa_st;
	sc->sc_regs.cr_iobase = addr;
	asc->sc_irq = aa->aa_irq[0];

	if (com_is_console(aa->aa_st, addr, &sc->sc_regs.cr_ioh) == 0 &&
	    bus_space_map(aa->aa_st, addr, AUCOM_NPORTS, 0,
		&sc->sc_regs.cr_ioh) != 0) {
		printf(": can't map i/o space\n");
		return;
	}
	com_aubus_initmap(&sc->sc_regs);

	/*
	 * The input to the clock divider is the internal pbus clock (1/4 the
	 * processor frequency).  The actual baud rate of the interface will
	 * be pbus_freq / CLKDIV.
	 */
	sc->sc_frequency = curcpu()->ci_cpu_freq / 4;

	sc->sc_hwflags = COM_HW_NO_TXPRELOAD;
	sc->sc_type = COM_TYPE_AU1x00;

	sc->enable = com_aubus_enable;
	sc->disable = com_aubus_disable;

	/* Enable UART so we can access it. */
	com_aubus_enable(sc);
	sc->enabled = 1;

	/* Attach MI com driver. */
	com_attach_subr(sc);

	/* Disable UART if it's not the console. (XXX kgdb?) */
	if (!ISSET(sc->sc_hwflags, COM_HW_CONSOLE)) {
		com_aubus_disable(sc);
		sc->enabled = 0;
	}
}

int
com_aubus_enable(struct com_softc *sc)
{
	struct com_aubus_softc *asc = (void *)sc; /* XXX mi prototype */

	/* Ignore requests to enable an already enabled console. */
	if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE) && (asc->sc_ih != NULL))
		return (0);

	/* Enable the UART module. */
	bus_space_write_1(sc->sc_regs.cr_iot, sc->sc_regs.cr_ioh, AUCOM_MODCTL,
	    UMC_ME | UMC_CE);

	/* Establish the interrupt. */
	asc->sc_ih = au_intr_establish(asc->sc_irq, 0, IPL_SERIAL, IST_LEVEL,
	    comintr, sc);
	if (asc->sc_ih == NULL) {
		printf("%s: unable to establish interrupt\n",
		    sc->sc_dev.dv_xname);
		return (1);
	}

	return (0);
}

void
com_aubus_disable(struct com_softc *sc)
{
	struct com_aubus_softc *asc = (void *)sc; /* XXX mi prototype */

	/* Ignore requests to disable the console. */
	if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE))
		return;

	/* Disestablish the interrupt. */
	au_intr_disestablish(asc->sc_ih);

	/* Disable the UART module. */
	bus_space_write_1(sc->sc_regs.cr_iot, sc->sc_regs.cr_ioh,
	    AUCOM_MODCTL, 0);
}

void
com_aubus_initmap(struct com_regs *regsp)
{
	regsp->cr_nports = AUCOM_NPORTS;
	regsp->cr_map[COM_REG_RXDATA] = AUCOM_RXDATA;
	regsp->cr_map[COM_REG_TXDATA] = AUCOM_TXDATA;
	regsp->cr_map[COM_REG_DLBL] = AUCOM_DLB;
	regsp->cr_map[COM_REG_DLBH] = AUCOM_DLB;
	regsp->cr_map[COM_REG_IER] = AUCOM_IER;
	regsp->cr_map[COM_REG_IIR] = AUCOM_IIR;
	regsp->cr_map[COM_REG_FIFO] = AUCOM_FIFO;
	regsp->cr_map[COM_REG_EFR] = 0;
	regsp->cr_map[COM_REG_LCR] = AUCOM_LCTL;
	regsp->cr_map[COM_REG_MCR] = AUCOM_MCR;
	regsp->cr_map[COM_REG_LSR] = AUCOM_LSR;
	regsp->cr_map[COM_REG_MSR] = AUCOM_MSR;
}

int
com_aubus_cnattach(bus_addr_t addr, int baud)
{
	struct com_regs		regs;
	uint32_t		sysfreq;

	regs.cr_iot = aubus_st;
	regs.cr_iobase = addr;
	regs.cr_nports = AUCOM_NPORTS;
	com_aubus_initmap(&regs);

	sysfreq = curcpu()->ci_cpu_freq / 4;

	return comcnattach1(&regs, baud, sysfreq, COM_TYPE_AU1x00, CONMODE);
}
