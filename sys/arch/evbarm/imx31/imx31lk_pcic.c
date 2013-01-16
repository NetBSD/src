/*	$Id: imx31lk_pcic.c,v 1.4.2.2 2013/01/16 05:32:54 yamt Exp $	*/
/*	$NetBSD: imx31lk_pcic.c,v 1.4.2.2 2013/01/16 05:32:54 yamt Exp $	*/
/*	$OpenBSD: pxapcic.c,v 1.1 2005/07/01 23:51:55 uwe Exp $	*/

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
__KERNEL_RCSID(0, "$Id: imx31lk_pcic.c,v 1.4.2.2 2013/01/16 05:32:54 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <uvm/uvm.h>

#ifdef NOTYET
#include <arch/arm/imx/imx_emifs.h>
#include <arch/arm/imx/imx_gpio.h>
#endif
#include <arch/arm/imx/imx31var.h>
#include <arch/arm/imx/imx_pcic.h>


static int	imx31lk_pcic_match(device_t, cfdata_t, void *);
static void	imx31lk_pcic_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(imx31lk_pcic, sizeof(struct imx_pcic_softc),
	imx31lk_pcic_match, imx31lk_pcic_attach, NULL, NULL);

static void	imx31lk_pcic_socket_setup(struct imx_pcic_socket *);
static u_int	imx31lk_pcic_read(struct imx_pcic_socket *, int);
static void	imx31lk_pcic_write(struct imx_pcic_socket *, int, u_int);
static void	imx31lk_pcic_set_power(struct imx_pcic_socket *, int);
static void	imx31lk_pcic_clear_intr(struct imx_pcic_socket *);
static void 	*imx31lk_pcic_intr_establish(struct imx_pcic_socket *, int,
		    int (*)(void *), void *);
static void	imx31lk_pcic_intr_disestablish(struct imx_pcic_socket *, void *);

struct imx_pcic_tag imx31lk_pcic_functions = {
	imx31lk_pcic_read,
	imx31lk_pcic_write,
	imx31lk_pcic_set_power,
	imx31lk_pcic_clear_intr,
	imx31lk_pcic_intr_establish,
	imx31lk_pcic_intr_disestablish
};

static int
imx31lk_pcic_match(device_t parent, cfdata_t cf, void *aux)
{

	return 1;	/* XXX */
}

static void
imx31lk_pcic_attach(device_t parent, device_t self, void *aux)
{
	struct imx_pcic_softc *sc = device_private(self);
	struct aips_attach_args * const aipsa = aux;

	sc->sc_dev = self;

printf("\n");
printf("imx_iot %p\n", aipsa->aipsa_memt);
printf("imx_addr %lx\n", aipsa->aipsa_addr);
printf("imx_size %lx\n", aipsa->aipsa_size);
printf("imx_intr %d\n", aipsa->aipsa_intr);

	sc->sc_pa = aipsa->aipsa_addr;
	sc->sc_iot = aipsa->aipsa_memt;

	sc->sc_nslots = 1;
#ifdef NOTYET
	sc->sc_irqpin[0] = aa->emifs_intr;	/* XXX */
	sc->sc_irqcfpin[0] = -1;		/* XXX */
#endif

	sc->sc_flags |= PPF_REVERSE_ORDER;

	imx_pcic_attach_common(sc, &imx31lk_pcic_socket_setup);
}

static void
imx31lk_pcic_socket_setup(struct imx_pcic_socket *so)
{
	struct imx_pcic_softc *sc;
	bus_addr_t pa;
	bus_size_t size = 0x2000;		/* XXX */
	bus_space_tag_t iot;
	bus_space_handle_t imx31lkh;
	int error;

	sc = so->sc;
	iot = sc->sc_iot;

	if (so->socket != 0)
		panic("%s: CF slot %d not supported", device_xname(sc->sc_dev), so->socket);

	pa = sc->sc_pa;

	error = bus_space_map(iot, trunc_page(pa), round_page(size),
	    0, &imx31lkh);
	if (error) {
		panic("%s: failed to map memory %x for imx31lk",
		    device_xname(sc->sc_dev), (uint32_t)pa);
	}
	imx31lkh += pa - trunc_page(pa);
	
#ifdef NOTYET
	/* setup */
#endif

#ifdef NOTYET
	so->power_capability = PXAPCIC_POWER_3V;
	if (so->socket == 0)
		so->power_capability |= PXAPCIC_POWER_5V;
#endif

	so->pcictag_cookie = (void *)imx31lkh;
	so->pcictag = &imx31lk_pcic_functions;
}

static u_int
imx31lk_pcic_read(struct imx_pcic_socket *so, int reg)
{
#ifdef NOTYET
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
		panic("imx31lk_pcic_read: bogus register");
	}
	/*NOTREACHED*/
#else
	panic("imx31lk_pcic_read");
#endif
}

static void
imx31lk_pcic_write(struct imx_pcic_socket *so, int reg, u_int val)
{
#ifdef NOTYET
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
		panic("imx31lk_pcic_write: bogus register");
	}

	splx(s);
#else
	panic("imx31lk_pcic_write");
#endif
}

static void
imx31lk_pcic_set_power(struct imx_pcic_socket *so, int pwr)
{
#ifdef NOTYET
	bus_space_tag_t iot = so->sc->sc_iot;
	bus_space_handle_t ioh = (bus_space_handle_t)so->pcictag_cookie;
	uint16_t reg;
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
		panic("imx31lk_pcic_set_power: bogus power state");
	}

	splx(s);
#else
	panic("imx31lk_pcic_set_power");
#endif
}

static void
imx31lk_pcic_clear_intr(struct imx_pcic_socket *so)
{
#ifdef NOTYET
	bus_space_tag_t iot = so->sc->sc_iot;
	bus_space_handle_t ioh = (bus_space_handle_t)so->pcictag_cookie;

	bus_space_write_2(iot, ioh, SCOOP_IRM, 0x00ff);
	bus_space_write_2(iot, ioh, SCOOP_ISR, 0x0000);
	bus_space_write_2(iot, ioh, SCOOP_IRM, 0x0000);
#else
	panic("imx31lk_pcic_clear_intr");
#endif
}

static void *
imx31lk_pcic_intr_establish(struct imx_pcic_socket *so, int ipl,
    int (*func)(void *), void *arg)
{
#ifdef NOTYET
printf("%s: irqpin %d\n", __func__, so->irqpin);
	return (imx_gpio_intr_establish(so->irqpin, IST_EDGE_FALLING,
	    ipl, "pcic", func, arg));
#else
	return 0;		/* XXX */
#endif
}

static void
imx31lk_pcic_intr_disestablish(struct imx_pcic_socket *so, void *ih)
{

#ifdef NOTYET
	imx_gpio_intr_disestablish(ih);
#endif
}
