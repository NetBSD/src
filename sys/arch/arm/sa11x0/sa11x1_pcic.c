/*      $NetBSD: sa11x1_pcic.c,v 1.1.2.1 2001/08/03 04:11:03 lukem Exp $        */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro.
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
#ifdef hpcarm
#include <machine/platid.h>
#include <machine/platid_mask.h>
#endif

#include <dev/pcmcia/pcmciachip.h>
#include <dev/pcmcia/pcmciavar.h>
#include <arm/sa11x0/sa11x0_reg.h>
#include <arm/sa11x0/sa11x0_var.h>
#include <arm/sa11x0/sa1111_reg.h>
#include <arm/sa11x0/sa1111_var.h>
#include <arm/sa11x0/sa11x1_pcicreg.h>
#include <arm/sa11x0/sa11xx_pcicvar.h>

#include "sacpcic.h"

static	int	sacpcic_match(struct device *, struct cfdata *, void *);
static	void	sacpcic_attach(struct device *, struct device *, void *);
static	int	sacpcic_print(void *, const char *);
static	int	sacpcic_submatch(struct device *, struct cfdata *, void *);

static	int	sacpcic_read(struct sapcic_socket *, int);
static	void	sacpcic_write(struct sapcic_socket *, int, int);
static	void	sacpcic_set_power(struct sapcic_socket *, int);
static	void	sacpcic_clear_intr(int);
static	void	*sacpcic_intr_establish(struct sapcic_socket *, int,
				       int (*)(void *), void *);
static	void	sacpcic_intr_disestablish(struct sapcic_socket *, void *);

struct sacpcic_softc {
	struct sapcic_softc sc_pc;
	bus_space_handle_t sc_ioh;
	
	struct sapcic_socket sc_socket[2];
};

static struct sapcic_tag sacpcic_functions = {
	sacpcic_read,
	sacpcic_write,
	sacpcic_set_power,
	sacpcic_clear_intr,
	sacpcic_intr_establish,
	sacpcic_intr_disestablish
};

#ifdef hpcarm
static int j720_power_capability[] = {
	SAPCIC_POWER_5V | SAPCIC_POWER_3V, SAPCIC_POWER_3V
};

static struct platid_data sacpcic_platid_table[] = {
	{ &platid_mask_MACH_HP_JORNADA_720, j720_power_capability },
	{ &platid_mask_MACH_HP_JORNADA_720JP, j720_power_capability },
	{ NULL, NULL }
};
#endif

struct cfattach sacpcic_ca = {
	sizeof(struct sacpcic_softc), sacpcic_match, sacpcic_attach
};

static int
sacpcic_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return (1);
}

static void
sacpcic_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	int i;
#ifdef hpcarm
	int *ip;
#endif
	struct pcmciabus_attach_args paa;
	struct sacpcic_softc *sc = (struct sacpcic_softc *)self;
	struct sacc_softc *psc = (struct sacc_softc *)parent;
#ifdef hpcarm
	struct platid_data *p;
#endif

	printf("\n");

	sc->sc_pc.sc_iot = psc->sc_iot;
	sc->sc_ioh = psc->sc_ioh;
#ifdef hpcarm
	p = platid_search(&platid, sacpcic_platid_table);
#endif

	for(i = 0; i < 2; i++) {
		sc->sc_socket[i].sc = (struct sapcic_softc *)sc;
		sc->sc_socket[i].socket = i;
		sc->sc_socket[i].pcictag_cookie = psc;
		sc->sc_socket[i].pcictag = &sacpcic_functions;
		sc->sc_socket[i].event_thread = NULL;
		sc->sc_socket[i].event = 0;
		sc->sc_socket[i].laststatus = SAPCIC_CARD_INVALID;
		sc->sc_socket[i].shutdown = 0;

#ifdef hpcarm
		if (p == NULL) {
			sc->sc_socket[i].power_capability = SAPCIC_POWER_5V;
		} else {
			ip = (int *)p->data;
			sc->sc_socket[i].power_capability = ip[i];
		}
#else
		/* XXX */
		sc->sc_socket[i].power_capability = SAPCIC_POWER_5V;
#endif

		paa.paa_busname = "pcmcia";
		paa.pct = (pcmcia_chipset_tag_t)&sa11x0_pcmcia_functions;
		paa.pch = (pcmcia_chipset_handle_t)&sc->sc_socket[i];
		paa.iobase = 0;
		paa.iosize = 0x4000000;

		sc->sc_socket[i].pcmcia =
		    (struct device *)config_found_sm(&sc->sc_pc.sc_dev,
		    &paa, sacpcic_print, sacpcic_submatch);

		sacc_intr_establish((sacc_chipset_tag_t)psc,
				    i ? IRQ_S1_CDVALID : IRQ_S0_CDVALID,
				    IST_EDGE_RAISE, IPL_BIO, sapcic_intr,
				    &sc->sc_socket[i]);

		/* schedule kthread creation */
		kthread_create(sapcic_kthread_create, &sc->sc_socket[i]);

#if 0 /* XXX */
		/* establish_intr should be after creating the kthread */
		config_interrupt(&sc->sc_socket[i], sacpcic_config_intr);
#endif
	}
}

static int
sacpcic_print(aux, name)
	void *aux;
	const char *name;
{
	return (UNCONF);
}

static int
sacpcic_submatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return (*cf->cf_attach->ca_match)(parent, cf, aux);
}


static int
sacpcic_read(so, reg)
	struct sapcic_socket *so;
	int reg;
{
	int cr, bit;
	struct sacpcic_softc *sc = (struct sacpcic_softc *)so->sc;

	cr = bus_space_read_4(sc->sc_pc.sc_iot, sc->sc_ioh, SACPCIC_SR);

	switch (reg) {
	case SAPCIC_STATUS_CARD:
		bit = (so->socket ? SR_S1_CARDDETECT : SR_S0_CARDDETECT) & cr;
		if (bit)
			return SAPCIC_CARD_INVALID;
		else
			return SAPCIC_CARD_VALID;

	case SAPCIC_STATUS_VS1:
		bit = (so->socket ? SR_S1_VS1 : SR_S0_VS1);
		return (bit & cr);

	case SAPCIC_STATUS_VS2:
		bit = (so->socket ? SR_S1_VS2 : SR_S0_VS2);
		return (bit & cr);

	case SAPCIC_STATUS_READY:
		bit = (so->socket ? SR_S1_READY : SR_S0_READY);
		return (bit & cr);

	default:
		panic("sacpcic_read: bogus register\n");
	}
}

static void
sacpcic_write(so, reg, arg)
	struct sapcic_socket *so;
	int reg;
	int arg;
{
	int s, oldvalue, newvalue, mask;
	struct sacpcic_softc *sc = (struct sacpcic_softc *)so->sc;

	s = splhigh();
	oldvalue = bus_space_read_4(sc->sc_pc.sc_iot, sc->sc_ioh, SACPCIC_CR);

	switch (reg) {
	case SAPCIC_CONTROL_RESET:
		mask = so->socket ? CR_S1_RST : CR_S0_RST;

		newvalue = (oldvalue & ~mask) | (arg ? mask : 0);
		break;

	case SAPCIC_CONTROL_LINEENABLE:
		mask = so->socket ? CR_S1_FLT : CR_S0_FLT;

		newvalue = (oldvalue & ~mask) | (arg ? mask : 0);
		break;

	case SAPCIC_CONTROL_WAITENABLE:
		mask = so->socket ? CR_S1_PWAITEN : CR_S0_PWAITEN;

		newvalue = (oldvalue & ~mask) | (arg ? mask : 0);
		break;

	case SAPCIC_CONTROL_POWERSELECT:
		mask = so->socket ? CR_S1_PSE : CR_S0_PSE;
		newvalue = oldvalue & ~mask;

		switch (arg) {
		case SAPCIC_POWER_3V:
			break;
		case SAPCIC_POWER_5V:
			newvalue |= mask;
			break;
		default:
			splx(s);
			panic("sacpcic_write: bogus arg");
		}
		break;

	default:
		splx(s);
		panic("sacpcic_write: bogus register");
	}
	bus_space_write_4(sc->sc_pc.sc_iot, sc->sc_ioh, SACPCIC_CR, newvalue);
	splx(s);
}
		
static void
sacpcic_set_power(so, arg)
	struct sapcic_socket *so;
	int arg;
{
	/* XXX this should go to dev/jornada720.c */
	int newval, oldval, s;
	struct sacc_softc *sc = so->pcictag_cookie;

	/* XXX this isn't well confirmed. DANGER DANGER */
	switch (arg) {
	case SAPCIC_POWER_OFF:
		newval = 0;
		break;
	case SAPCIC_POWER_3V:
		newval = 2;
		break;
	case SAPCIC_POWER_5V:
		newval = 1;
		break;
	default:
		panic("sacpcic_set_power: bogus arg\n");
	}

	s = splbio();
	oldval = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
				  SACCGPIOA_DVR);
	switch (so->socket) {
	case 0:
		newval = newval | (oldval & 0xc);
		break;
	case 1:
		newval = (newval << 2) | (oldval & 3);
		break;
	default:
		splx(s);
		panic("sacpcic_set_power\n");
	}
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACCGPIOA_DVR, newval);
	splx(s);
}

static void
sacpcic_clear_intr(arg)
{
	/* sacc_intr_dispatch takes care of intr status */
}

static void *
sacpcic_intr_establish(so, level, ih_fun, ih_arg)
	struct sapcic_socket *so;
	int level;
	int (*ih_fun)(void *);
	void *ih_arg;
{
	int irq;

	irq = so->socket ? IRQ_S1_READY : IRQ_S0_READY;
	return (sacc_intr_establish((sacc_chipset_tag_t)so->pcictag_cookie, irq,
				    IST_EDGE_FALL, level, ih_fun, ih_arg));
}

static void
sacpcic_intr_disestablish(so, ih)
	struct sapcic_socket *so;
	void *ih;
{
	sacc_intr_disestablish((sacc_chipset_tag_t)so->pcictag_cookie, ih);
}
