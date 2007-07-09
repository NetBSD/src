/*      $NetBSD: ipaq_pcic.c,v 1.16 2007/07/09 20:52:12 ad Exp $        */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA (ichiro@ichiro.org).
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ipaq_pcic.c,v 1.16 2007/07/09 20:52:12 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <dev/pcmcia/pcmciachip.h>
#include <dev/pcmcia/pcmciavar.h>

#include <hpcarm/dev/ipaq_saipvar.h>
#include <hpcarm/dev/ipaq_pcicreg.h>
#include <hpcarm/dev/ipaq_gpioreg.h>

#include <arm/sa11x0/sa11x0_gpioreg.h>
#include <arm/sa11x0/sa11x0_var.h>
#include <arm/sa11x0/sa11xx_pcicvar.h>

#include "ipaqpcic.h"

static	int	ipaqpcic_match(struct device *, struct cfdata *, void *);
static	void	ipaqpcic_attach(struct device *, struct device *, void *);
static	int	ipaqpcic_print(void *, const char *);

static	int	ipaqpcic_read(struct sapcic_socket *, int);
static	void	ipaqpcic_write(struct sapcic_socket *, int, int);
static	void	ipaqpcic_set_power(struct sapcic_socket *, int);
static	void	ipaqpcic_clear_intr(int);
static	void	*ipaqpcic_intr_establish(struct sapcic_socket *, int,
				       int (*)(void *), void *);
static	void	ipaqpcic_intr_disestablish(struct sapcic_socket *, void *);

struct ipaqpcic_softc {
	struct sapcic_softc	sc_pc;
	bus_space_handle_t	sc_ioh;
	struct ipaq_softc	*sc_parent;
	struct sapcic_socket	sc_socket[2];
};

static	void	ipaqpcic_init(struct ipaqpcic_softc *);

static struct sapcic_tag ipaqpcic_functions = {
	ipaqpcic_read,
	ipaqpcic_write,
	ipaqpcic_set_power,
	ipaqpcic_clear_intr,
	ipaqpcic_intr_establish,
	ipaqpcic_intr_disestablish
};

CFATTACH_DECL(ipaqpcic, sizeof(struct ipaqpcic_softc),
    ipaqpcic_match, ipaqpcic_attach, NULL, NULL);

static int
ipaqpcic_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return (1);
}

static void
ipaqpcic_attach(struct device *parent, struct device *self, void *aux)
{
	int i;
	struct pcmciabus_attach_args paa;
	struct ipaqpcic_softc *sc = (struct ipaqpcic_softc *)self;
	struct ipaq_softc *psc = (struct ipaq_softc *)parent;

	printf("\n");

	sc->sc_pc.sc_iot = psc->sc_iot;
	sc->sc_ioh = psc->sc_ioh;
	sc->sc_parent = (struct ipaq_softc *)parent;

	ipaqpcic_init(sc);

	for(i = 0; i < 2; i++) {
		sc->sc_socket[i].sc = (struct sapcic_softc *)sc;
		sc->sc_socket[i].socket = i;
		sc->sc_socket[i].pcictag_cookie = psc;
		sc->sc_socket[i].pcictag = &ipaqpcic_functions;
		sc->sc_socket[i].event_thread = NULL;
		sc->sc_socket[i].event = 0;
		sc->sc_socket[i].laststatus = SAPCIC_CARD_INVALID;
		sc->sc_socket[i].shutdown = 0;

		paa.paa_busname = "pcmcia";
		paa.pct = (pcmcia_chipset_tag_t)&sa11x0_pcmcia_functions;
		paa.pch = (pcmcia_chipset_handle_t)&sc->sc_socket[i];
		paa.iobase = 0;
		paa.iosize = 0x4000000;

		sc->sc_socket[i].pcmcia =
		    config_found_ia(&sc->sc_pc.sc_dev, "pcmciabus",
		    &paa, ipaqpcic_print);

		sa11x0_intr_establish((sa11x0_chipset_tag_t)psc,
			    i ? IRQ_CD1 : IRQ_CD0,
			    1, IPL_BIO, sapcic_intr,
			    &sc->sc_socket[i]);

		/* schedule kthread creation */
		sapcic_kthread_create(&sc->sc_socket[i]);

#if 0 /* XXX */
		/* establish_intr should be after creating the kthread */
		config_interrupt(&sc->sc_socket[i], ipaqpcic_config_intr);
#endif
	}
}

static int
ipaqpcic_print(void *aux, const char *name)
{
	return (UNCONF);
}

static void
ipaqpcic_init(struct ipaqpcic_softc *sc)
{
	int cr;

	/* All those are inputs */
	cr = bus_space_read_4(sc->sc_pc.sc_iot, sc->sc_parent->sc_gpioh, SAGPIO_PDR);
	cr &= ~(GPIO_H3600_PCMCIA_CD0 | GPIO_H3600_PCMCIA_CD1 | GPIO_H3600_PCMCIA_IRQ0 |
		GPIO_H3600_PCMCIA_IRQ1);
	bus_space_write_4(sc->sc_pc.sc_iot, sc->sc_parent->sc_gpioh, SAGPIO_PDR, cr);

	sc->sc_parent->ipaq_egpio |=
		EGPIO_H3600_OPT_NVRAM_ON | EGPIO_H3600_OPT_ON ;
	sc->sc_parent->ipaq_egpio &=
		~(EGPIO_H3600_CARD_RESET | EGPIO_H3600_OPT_RESET);
	bus_space_write_2(sc->sc_pc.sc_iot, sc->sc_parent->sc_egpioh,
			  0, sc->sc_parent->ipaq_egpio);
}

static int
ipaqpcic_read(struct sapcic_socket *so, int reg)
{
	int cr, bit;
	struct ipaqpcic_softc *sc = (struct ipaqpcic_softc *)so->sc;

	cr = bus_space_read_4(sc->sc_pc.sc_iot, sc->sc_parent->sc_gpioh, SAGPIO_PLR);

	switch (reg) {
	case SAPCIC_STATUS_CARD:
		bit = (so->socket ? GPIO_H3600_PCMCIA_CD0 :
				GPIO_H3600_PCMCIA_CD1) & cr;
		if (!bit)
			return SAPCIC_CARD_INVALID;
		else
			return SAPCIC_CARD_VALID;
	case SAPCIC_STATUS_VS1:
        case SAPCIC_STATUS_VS2:
	case SAPCIC_STATUS_READY:
		bit = (so->socket ? GPIO_H3600_PCMCIA_IRQ0:
				GPIO_H3600_PCMCIA_IRQ1);
		return (bit & cr);
	default:
		panic("ipaqpcic_read: bogus register");
	}
}

static void
ipaqpcic_write(struct sapcic_socket *so, int reg, int arg)
{
	int s;
	struct ipaqpcic_softc *sc = (struct ipaqpcic_softc *)so->sc;

	s = splhigh();
	switch (reg) {
	case SAPCIC_CONTROL_RESET:
		sc->sc_parent->ipaq_egpio |= EGPIO_H3600_CARD_RESET;
		break;
	case SAPCIC_CONTROL_LINEENABLE:
	case SAPCIC_CONTROL_WAITENABLE:
	case SAPCIC_CONTROL_POWERSELECT:
		break;

	default:
		splx(s);
		panic("ipaqpcic_write: bogus register");
	}
	bus_space_write_2(sc->sc_pc.sc_iot, sc->sc_parent->sc_egpioh, 0,
			  sc->sc_parent->ipaq_egpio);
	splx(s);
}
		
static void
ipaqpcic_set_power(struct sapcic_socket *so, int arg)
{
	int s;
	struct ipaqpcic_softc *sc = (struct ipaqpcic_softc *)so->sc;

	s = splbio();
	switch (arg) {
	case SAPCIC_POWER_OFF:
		sc->sc_parent->ipaq_egpio &=
			~(EGPIO_H3600_OPT_NVRAM_ON | EGPIO_H3600_OPT_ON);
		break;
	case SAPCIC_POWER_3V:
	case SAPCIC_POWER_5V:
		sc->sc_parent->ipaq_egpio |=
			EGPIO_H3600_OPT_NVRAM_ON | EGPIO_H3600_OPT_ON;
		break;
	default:
		panic("ipaqpcic_set_power: bogus arg");
	}
	bus_space_write_2(sc->sc_pc.sc_iot, sc->sc_parent->sc_egpioh,
			  0, sc->sc_parent->ipaq_egpio);
	splx(s);
}

static void
ipaqpcic_clear_intr(int arg)
{
}

static void *
ipaqpcic_intr_establish(struct sapcic_socket *so, int level,
			int (*ih_fun)(void *), void *ih_arg)
{
	int irq;

	irq = so->socket ? IRQ_IRQ0 : IRQ_IRQ1;
	return (sa11x0_intr_establish((sa11x0_chipset_tag_t)so->pcictag_cookie,
				    irq-16, 1, level, ih_fun, ih_arg));
}

static void
ipaqpcic_intr_disestablish(struct sapcic_socket *so, void *ih)
{
	sa11x0_intr_disestablish((sa11x0_chipset_tag_t)so->pcictag_cookie, ih);
}
