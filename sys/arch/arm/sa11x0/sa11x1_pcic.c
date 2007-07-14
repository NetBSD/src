/*      $NetBSD: sa11x1_pcic.c,v 1.15 2007/07/14 21:46:36 ad Exp $        */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sa11x1_pcic.c,v 1.15 2007/07/14 21:46:36 ad Exp $");

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
#include <arm/sa11x0/sa11x0_reg.h>
#include <arm/sa11x0/sa11x0_var.h>
#include <arm/sa11x0/sa1111_reg.h>
#include <arm/sa11x0/sa1111_var.h>
#include <arm/sa11x0/sa11x1_pcicreg.h>
#include <arm/sa11x0/sa11xx_pcicvar.h>
#include <arm/sa11x0/sa11x1_pcicvar.h>

#include "sacpcic.h"

static int	sacpcic_print(void *, const char *);

void
sacpcic_attach_common(struct sacc_softc *psc, struct sacpcic_softc *sc,
    void *aux, void (* socket_setup_hook)(struct sapcic_socket *))
{
	int i;
	struct pcmciabus_attach_args paa;

	printf("\n");

	sc->sc_pc.sc_iot = psc->sc_iot;
	sc->sc_ioh = psc->sc_ioh;

	for (i = 0; i < 2; i++) {
		sc->sc_socket[i].sc = (struct sapcic_softc *)sc;
		sc->sc_socket[i].socket = i;
		sc->sc_socket[i].pcictag_cookie = psc;
		sc->sc_socket[i].pcictag = NULL;
		sc->sc_socket[i].event_thread = NULL;
		sc->sc_socket[i].event = 0;
		sc->sc_socket[i].laststatus = SAPCIC_CARD_INVALID;
		sc->sc_socket[i].shutdown = 0;

		socket_setup_hook(&sc->sc_socket[i]);

		paa.paa_busname = "pcmcia";
		paa.pct = (pcmcia_chipset_tag_t)&sa11x0_pcmcia_functions;
		paa.pch = (pcmcia_chipset_handle_t)&sc->sc_socket[i];
		paa.iobase = 0;
		paa.iosize = 0x4000000;

		sc->sc_socket[i].pcmcia =
		    config_found_ia(&sc->sc_pc.sc_dev, "pcmciabus", &paa,
				    sacpcic_print);

		sacc_intr_establish((sacc_chipset_tag_t)psc,
				    i ? IRQ_S1_CDVALID : IRQ_S0_CDVALID,
				    IST_EDGE_RAISE, IPL_BIO, sapcic_intr,
				    &sc->sc_socket[i]);

		/* create kthread */
		sapcic_kthread_create(&sc->sc_socket[i]);
#if 0 /* XXX */
		/* establish_intr should be after creating the kthread */
		config_interrupt(&sc->sc_socket[i], sapcic_config_intr);
#endif
	}
}

int
sacpcic_print(void *aux, const char *name)
{

	return UNCONF;
}

int
sacpcic_read(struct sapcic_socket *so, int reg)
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
		panic("sacpcic_read: bogus register");
	}
}

void
sacpcic_write(struct sapcic_socket *so, int reg, int arg)
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
		
void
sacpcic_clear_intr(int arg)
{
	/* sacc_intr_dispatch takes care of intr status */
}

void *
sacpcic_intr_establish(struct sapcic_socket *so, int level,
    int (*ih_fun)(void *), void *ih_arg)
{
	int irq;

	irq = so->socket ? IRQ_S1_READY : IRQ_S0_READY;
	return sacc_intr_establish((sacc_chipset_tag_t)so->pcictag_cookie, irq,
				    IST_EDGE_FALL, level, ih_fun, ih_arg);
}

void
sacpcic_intr_disestablish(struct sapcic_socket *so, void *ih)
{
	sacc_intr_disestablish((sacc_chipset_tag_t)so->pcictag_cookie, ih);
}
