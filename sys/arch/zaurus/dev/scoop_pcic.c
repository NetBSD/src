/*	$NetBSD: scoop_pcic.c,v 1.3 2009/01/29 12:28:15 nonaka Exp $	*/
/*	$OpenBSD: scoop_pcic.c,v 1.1 2005/07/01 23:51:55 uwe Exp $	*/

/*
 * Copyright (c) 2005 Uwe Stuehler <uwe@bsdx.de>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: scoop_pcic.c,v 1.3 2009/01/29 12:28:15 nonaka Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm.h>

#include <arch/arm/xscale/pxa2x0var.h>
#include <arch/arm/xscale/pxa2x0_gpio.h>
#include <arch/arm/xscale/pxa2x0_pcic.h>

#include <zaurus/zaurus/zaurus_reg.h>
#include <zaurus/zaurus/zaurus_var.h>

#include <zaurus/dev/scoopreg.h>

static int	scoop_pcic_match(device_t, cfdata_t, void *);
static void	scoop_pcic_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(pxapcic_scoop, sizeof(struct pxapcic_softc),
    scoop_pcic_match, scoop_pcic_attach, NULL, NULL);

static void	scoop_pcic_socket_setup(struct pxapcic_socket *);
static u_int	scoop_pcic_read(struct pxapcic_socket *, int);
static void	scoop_pcic_write(struct pxapcic_socket *, int, u_int);
static void	scoop_pcic_set_power(struct pxapcic_socket *, int);
static void	scoop_pcic_clear_intr(struct pxapcic_socket *);
static void 	*scoop_pcic_intr_establish(struct pxapcic_socket *, int,
		    int (*)(void *), void *);
static void	scoop_pcic_intr_disestablish(struct pxapcic_socket *, void *);

static struct pxapcic_tag scoop_pcic_functions = {
	scoop_pcic_read,
	scoop_pcic_write,
	scoop_pcic_set_power,
	scoop_pcic_clear_intr,
	scoop_pcic_intr_establish,
	scoop_pcic_intr_disestablish
};

static int
scoop_pcic_match(device_t parent, cfdata_t cf, void *aux)
{

	return (ZAURUS_ISC860 || ZAURUS_ISC3000);
}

static void
scoop_pcic_attach(device_t parent, device_t self, void *aux)
{
	struct pxapcic_softc *sc = device_private(self);
	struct pxaip_attach_args *pxa = (struct pxaip_attach_args *)aux;

	sc->sc_dev = self;
	sc->sc_iot = pxa->pxa_iot;

	if (ZAURUS_ISC860) {
		sc->sc_nslots = 1;
		sc->sc_irqpin[0] = C860_CF0_IRQ;
		sc->sc_irqcfpin[0] = C860_CF0_IRQ_PIN;
	} else if (ZAURUS_ISC3000) {
		sc->sc_nslots = 2;
		sc->sc_irqpin[0] = C3000_CF0_IRQ;
		sc->sc_irqcfpin[0] = C3000_CF0_IRQ_PIN;
		sc->sc_irqpin[1] = C3000_CF1_IRQ;
		sc->sc_irqcfpin[1] = C3000_CF1_IRQ_PIN;
	}
	sc->sc_flags |= PPF_REVERSE_ORDER;

	pxapcic_attach_common(sc, &scoop_pcic_socket_setup);
}

static void
scoop_pcic_socket_setup(struct pxapcic_socket *so)
{
	struct pxapcic_softc *sc;
	bus_addr_t pa;
	bus_size_t size = SCOOP_SIZE;
	bus_space_tag_t iot;
	bus_space_handle_t scooph;
	int error;

	sc = so->sc;
	iot = sc->sc_iot;

	if (so->socket == 0) {
		pa = C3000_SCOOP0_BASE;
	} else if (so->socket == 1) {
		pa = C3000_SCOOP1_BASE;
	} else {
		panic("%s: invalid CF slot %d", device_xname(sc->sc_dev),
		    so->socket);
	}

	error = bus_space_map(iot, trunc_page(pa), round_page(size),
	    0, &scooph);
	if (error) {
		panic("%s: failed to map memory %x for scoop",
		    device_xname(sc->sc_dev), (uint32_t)pa);
	}
	scooph += pa - trunc_page(pa);
	
	bus_space_write_2(iot, scooph, SCOOP_IMR,
	    SCP_IMR_UNKN0 | SCP_IMR_UNKN1);
	
	/* setup */
	bus_space_write_2(iot, scooph, SCOOP_MCR, 0x0100);
	bus_space_write_2(iot, scooph, SCOOP_CDR, 0x0000);
	bus_space_write_2(iot, scooph, SCOOP_CPR, 0x0000);
	bus_space_write_2(iot, scooph, SCOOP_IMR, 0x0000);
	bus_space_write_2(iot, scooph, SCOOP_IRM, 0x00ff);
	bus_space_write_2(iot, scooph, SCOOP_ISR, 0x0000);
	bus_space_write_2(iot, scooph, SCOOP_IRM, 0x0000);
	
	/* C3000 */
	if (so->socket == 1) {
		bus_space_write_2(iot, scooph, SCOOP_CPR, 0x80c1);
		bus_space_write_2(iot, scooph, SCOOP_IMR, 0x00c4);
		bus_space_write_2(iot, scooph, SCOOP_MCR, 0x0111);
	} else {
		bus_space_write_2(iot, scooph, SCOOP_CPR,
		    SCP_CPR_PWR|SCP_CPR_5V);
	}

	bus_space_write_2(iot, scooph, SCOOP_IMR, 0x00ce);
	bus_space_write_2(iot, scooph, SCOOP_MCR, 0x0111);

	/* C3000 */
	so->power_capability = PXAPCIC_POWER_3V;
	if (so->socket == 0)
		so->power_capability |= PXAPCIC_POWER_5V;

	so->pcictag_cookie = (void *)scooph;
	so->pcictag = &scoop_pcic_functions;
}

static u_int
scoop_pcic_read(struct pxapcic_socket *so, int reg)
{
	bus_space_tag_t iot = so->sc->sc_iot;
	bus_space_handle_t ioh = (bus_space_handle_t)so->pcictag_cookie;
	uint16_t csr;

	csr = bus_space_read_2(iot, ioh, SCOOP_CSR);

	switch (reg) {
	case PXAPCIC_CARD_STATUS:
		if (csr & SCP_CSR_MISSING)
			return (PXAPCIC_CARD_INVALID);
		else
			return (PXAPCIC_CARD_VALID);

	case PXAPCIC_CARD_READY:
		return ((bus_space_read_2(iot, ioh, SCOOP_CSR) &
		    SCP_CSR_READY) != 0);

	default:
		panic("scoop_pcic_read: bogus register");
	}
	/*NOTREACHED*/
}

static void
scoop_pcic_write(struct pxapcic_socket *so, int reg, u_int val)
{
	bus_space_tag_t iot = so->sc->sc_iot;
	bus_space_handle_t ioh = (bus_space_handle_t)so->pcictag_cookie;
	uint16_t newval;
	int s;

	s = splhigh();

	switch (reg) {
	case PXAPCIC_CARD_POWER:
		newval = bus_space_read_2(iot, ioh, SCOOP_CPR);
		newval &= ~(SCP_CPR_PWR | SCP_CPR_3V | SCP_CPR_5V);

		if (val == PXAPCIC_POWER_3V)
			newval |= (SCP_CPR_PWR | SCP_CPR_3V);
		else if (val == PXAPCIC_POWER_5V)
			newval |= (SCP_CPR_PWR | SCP_CPR_5V);

		bus_space_write_2(iot, ioh, SCOOP_CPR, newval);
		break;

	case PXAPCIC_CARD_RESET:
		bus_space_write_2(iot, ioh, SCOOP_CCR,
		    val ? SCP_CCR_RESET : 0);
		break;

	default:
		panic("scoop_pcic_write: bogus register");
	}

	splx(s);
}

static void
scoop_pcic_set_power(struct pxapcic_socket *so, int pwr)
{
	bus_space_tag_t iot = so->sc->sc_iot;
	bus_space_handle_t ioh = (bus_space_handle_t)so->pcictag_cookie;
	u_int16_t reg;
	int s;

	s = splhigh();

	switch (pwr) {
	case PXAPCIC_POWER_OFF:
#if 0
		/* XXX does this disable power to both sockets? */
		reg = bus_space_read_2(iot, ioh, SCOOP_GPWR);
		bus_space_write_2(iot, ioh, SCOOP_GPWR,
		    reg & ~(1 << SCOOP0_CF_POWER_C3000));
#endif
		break;

	case PXAPCIC_POWER_3V:
	case PXAPCIC_POWER_5V:
		/* XXX */
		if (so->socket == 0) {
			reg = bus_space_read_2(iot, ioh, SCOOP_GPWR);
			bus_space_write_2(iot, ioh, SCOOP_GPWR,
			    reg | (1 << SCOOP0_CF_POWER_C3000));
		}
		break;

	default:
		splx(s);
		panic("scoop_pcic_set_power: bogus power state");
	}

	splx(s);
}

static void
scoop_pcic_clear_intr(struct pxapcic_socket *so)
{
	bus_space_tag_t iot = so->sc->sc_iot;
	bus_space_handle_t ioh = (bus_space_handle_t)so->pcictag_cookie;

	bus_space_write_2(iot, ioh, SCOOP_IRM, 0x00ff);
	bus_space_write_2(iot, ioh, SCOOP_ISR, 0x0000);
	bus_space_write_2(iot, ioh, SCOOP_IRM, 0x0000);
}

static void *
scoop_pcic_intr_establish(struct pxapcic_socket *so, int ipl,
    int (*func)(void *), void *arg)
{

	return (pxa2x0_gpio_intr_establish(so->irqpin, IST_EDGE_FALLING,
	    ipl, func, arg));
}

static void
scoop_pcic_intr_disestablish(struct pxapcic_socket *so, void *ih)
{

	pxa2x0_gpio_intr_disestablish(ih);
}
