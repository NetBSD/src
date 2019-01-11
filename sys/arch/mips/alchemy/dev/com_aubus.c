/* $NetBSD: com_aubus.c,v 1.9 2019/01/11 23:10:40 thorpej Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: com_aubus.c,v 1.9 2019/01/11 23:10:40 thorpej Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/lwp.h>
#include <sys/systm.h>
#include <sys/tty.h>

#include <sys/bus.h>
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

static int	com_aubus_probe(device_t, cfdata_t , void *);
static void	com_aubus_attach(device_t, device_t, void *);
static int	com_aubus_enable(struct com_softc *);
static void	com_aubus_disable(struct com_softc *);
static void	com_aubus_init_regs(struct com_regs *, bus_space_tag_t,
				    bus_space_handle_t, bus_addr_t);

CFATTACH_DECL_NEW(com_aubus, sizeof(struct com_aubus_softc),
    com_aubus_probe, com_aubus_attach, NULL, NULL);

#define CONMODE	((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */

int
com_aubus_probe(device_t parent, cfdata_t cf, void *aux)
{
	struct aubus_attach_args *aa = aux;

	/* match only aucom devices */
	if (strcmp(aa->aa_name, cf->cf_name) == 0)
		return (1);

	return (0);
}

void
com_aubus_attach(device_t parent, device_t self, void *aux)
{
	struct com_aubus_softc *asc = device_private(self);
	struct com_softc *sc = &asc->sc_com;
	struct aubus_attach_args *aa = aux;
	bus_space_handle_t bsh;
	int addr = aa->aa_addr;

	sc->sc_dev = self;
	asc->sc_irq = aa->aa_irq[0];

	if (com_is_console(aa->aa_st, addr, &bsh) == 0 &&
	    bus_space_map(aa->aa_st, addr, AUCOM_NPORTS, 0,
		&sc->sc_regs.cr_ioh) != 0) {
		aprint_error(": can't map i/o space\n");
		return;
	}

	com_aubus_init_regs(&sc->sc_regs, aa->aa_st, bsh, addr);

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
		aprint_error_dev(sc->sc_dev,
		    "unable to establish interrupt\n");
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

static const bus_size_t com_aubus_regmap[COM_REGMAP_NENTRIES] = {
	[COM_REG_RXDATA]	=	AUCOM_RXDATA,
	[COM_REG_TXDATA]	=	AUCOM_TXDATA,
	[COM_REG_DLBL]		=	AUCOM_DLB,
	[COM_REG_DLBH]		=	AUCOM_DLB,
	[COM_REG_IER]		=	AUCOM_IER,
	[COM_REG_IIR]		=	AUCOM_IIR,
	[COM_REG_FIFO]		=	AUCOM_FIFO,
	[COM_REG_TCR]		=	AUCOM_FIFO,
	[COM_REG_LCR]		=	AUCOM_LCTL,
	[COM_REG_MCR]		=	AUCOM_MCR,
	[COM_REG_LSR]		=	AUCOM_LSR,
	[COM_REG_MSR]		=	AUCOM_MSR,
};

void
com_aubus_init_regs(struct com_regs *regsp, bus_space_tag_t bst,
		    bus_space_handle_t bsh, bus_addr_t addr)
{

	com_init_regs(regsp, bst, bsh, addr);

	memcpy(regsp->cr_map, com_aubus_regmap, sizeof(regsp->cr_map));
	regsp->cr_nports = AUCOM_NPORTS;
}

int
com_aubus_cnattach(bus_addr_t addr, int baud)
{
	struct com_regs		regs;
	uint32_t		sysfreq;

	com_aubus_init_regs(&regs, aubus_st, (bus_space_handle_t)0/*XXX*/,
			    addr);

	sysfreq = curcpu()->ci_cpu_freq / 4;

	return comcnattach1(&regs, baud, sysfreq, COM_TYPE_AU1x00, CONMODE);
}
