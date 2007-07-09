/*	$NetBSD: pxa2x0_pcic.c,v 1.3 2007/07/09 20:52:06 ad Exp $	*/
/*	$OpenBSD: pxa2x0_pcic.c,v 1.17 2005/12/14 15:08:51 uwe Exp $	*/

/*
 * Copyright (c) 2005 Dale Rahn <drahn@openbsd.org>
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
__KERNEL_RCSID(0, "$NetBSD: pxa2x0_pcic.c,v 1.3 2007/07/09 20:52:06 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/malloc.h>

#include <uvm/uvm.h>

#include <machine/bus.h>
#include <machine/intr.h>
        
#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciachip.h>

#include <arm/xscale/pxa2x0cpu.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>
#include <arm/xscale/pxa2x0_pcic.h>

static int	pxapcic_print(void *, const char *);

static void	pxapcic_event_thread(void *);
static void	pxapcic_event_process(struct pxapcic_socket *);
static void	pxapcic_attach_card(struct pxapcic_socket *);
static void	pxapcic_detach_card(struct pxapcic_socket *, int);

static int	pxapcic_mem_alloc(pcmcia_chipset_handle_t, bus_size_t,
		    struct pcmcia_mem_handle *);
static void	pxapcic_mem_free(pcmcia_chipset_handle_t,
		    struct pcmcia_mem_handle *);
static int	pxapcic_mem_map(pcmcia_chipset_handle_t, int, bus_addr_t,
		   bus_size_t, struct pcmcia_mem_handle *, bus_size_t *, int *);
static void	pxapcic_mem_unmap(pcmcia_chipset_handle_t, int);

static int	pxapcic_io_alloc(pcmcia_chipset_handle_t, bus_addr_t,
		    bus_size_t, bus_size_t, struct pcmcia_io_handle *);
static void	pxapcic_io_free(pcmcia_chipset_handle_t,
		    struct pcmcia_io_handle *);
static int	pxapcic_io_map(pcmcia_chipset_handle_t, int,
		    bus_addr_t, bus_size_t, struct pcmcia_io_handle *, int *);
static void	pxapcic_io_unmap(pcmcia_chipset_handle_t, int);

static void	*pxapcic_intr_establish(pcmcia_chipset_handle_t,
		    struct pcmcia_function *, int, int (*)(void *), void *);
static void	pxapcic_intr_disestablish(pcmcia_chipset_handle_t, void *);

static void	pxapcic_socket_enable(pcmcia_chipset_handle_t);
static void	pxapcic_socket_disable(pcmcia_chipset_handle_t);
static void	pxapcic_socket_settype(pcmcia_chipset_handle_t, int);

/*
 * PCMCIA chipset methods
 */
static struct pcmcia_chip_functions pxapcic_pcmcia_functions = {
	pxapcic_mem_alloc,
	pxapcic_mem_free,
	pxapcic_mem_map,
	pxapcic_mem_unmap,

	pxapcic_io_alloc,
	pxapcic_io_free,
	pxapcic_io_map,
	pxapcic_io_unmap,

	pxapcic_intr_establish,
	pxapcic_intr_disestablish,

	pxapcic_socket_enable,
	pxapcic_socket_disable,
	pxapcic_socket_settype,
};

#define	PXAPCIC_ATTR_OFFSET	0x08000000
#define	PXAPCIC_COMMON_OFFSET	0x0C000000

/*
 * PCMCIA Helpers
 */
static int
pxapcic_mem_alloc(pcmcia_chipset_handle_t pch, bus_size_t size,
    struct pcmcia_mem_handle *pmh)
{
	struct pxapcic_socket *so = (struct pxapcic_socket *)pch;

	/* All we need is the bus space tag */
	memset(pmh, 0, sizeof(*pmh));
	pmh->memt = so->sc->sc_iot;

	return 0;
}

static void
pxapcic_mem_free(pcmcia_chipset_handle_t pch, struct pcmcia_mem_handle *pmh)
{

	/* Nothing to do */
}

static int
pxapcic_mem_map(pcmcia_chipset_handle_t pch, int kind, bus_addr_t card_addr,
    bus_size_t size, struct pcmcia_mem_handle *pmh, bus_size_t *offsetp,
    int *windowp)
{
	struct pxapcic_socket *so = (struct pxapcic_socket *)pch;
	int error;
	bus_addr_t pa;
 
	pa = trunc_page(card_addr);
	*offsetp = card_addr - pa;
	size = round_page(card_addr + size) - pa;
	pmh->realsize = size;

	pa += PXA2X0_PCIC_SOCKET_BASE;
	pa += PXA2X0_PCIC_SOCKET_OFFSET * so->socket;

	switch (kind & ~PCMCIA_WIDTH_MEM_MASK) {
	case PCMCIA_MEM_ATTR:   
		pa += PXAPCIC_ATTR_OFFSET;
		break;
	case PCMCIA_MEM_COMMON:
		pa += PXAPCIC_COMMON_OFFSET;
		break;
	default:
		panic("pxapcic_mem_map: bogus kind");
	}

	error = bus_space_map(so->sc->sc_iot, pa, size, 0, &pmh->memh);
	if (error)
		return error;

	*windowp = (int)pmh->memh;
	return 0;
}

static void
pxapcic_mem_unmap(pcmcia_chipset_handle_t pch, int window)
{
	struct pxapcic_socket *so = (struct pxapcic_socket *)pch;

	bus_space_unmap(so->sc->sc_iot, (bus_addr_t)window, 4096); /* XXX */
}

static int
pxapcic_io_alloc(pcmcia_chipset_handle_t pch, bus_addr_t start,
    bus_size_t size, bus_size_t align, struct pcmcia_io_handle *pih)
{
	struct pxapcic_socket *so = (struct pxapcic_socket *)pch;
	bus_addr_t pa;
	int error;

	memset(pih, 0, sizeof(*pih));
	pih->iot = so->sc->sc_iot;
	pih->addr = start;
	pih->size = size;

	pa = pih->addr;
	pa += PXA2X0_PCIC_SOCKET_BASE;
	pa += PXA2X0_PCIC_SOCKET_OFFSET * so->socket;

	/* XXX Are we ignoring alignment constraints? */
	error = bus_space_map(so->sc->sc_iot, pa, size, 0, &pih->ioh);

	return error;
}

static void
pxapcic_io_free(pcmcia_chipset_handle_t pch, struct pcmcia_io_handle *pih)
{
	struct pxapcic_socket *so = (struct pxapcic_socket *)pch;

	bus_space_unmap(so->sc->sc_iot, pih->ioh, pih->size);
}
 
static int
pxapcic_io_map(pcmcia_chipset_handle_t pch, int width, bus_addr_t offset,
    bus_size_t size, struct pcmcia_io_handle *pih, int *windowp)
{

	return 0;
}

static void
pxapcic_io_unmap(pcmcia_chipset_handle_t pch, int window)
{

	/* Nothing to do */
}

static void *
pxapcic_intr_establish(pcmcia_chipset_handle_t pch,
    struct pcmcia_function *pf, int ipl, int (*fct)(void *), void *arg)
{
	struct pxapcic_socket *so = (struct pxapcic_socket *)pch;
	/* XXX need to check if something should be done here */

	return (*so->pcictag->intr_establish)(so, ipl, fct, arg);
}

static void
pxapcic_intr_disestablish(pcmcia_chipset_handle_t pch, void *ih)
{
	struct pxapcic_socket *so = (struct pxapcic_socket *)pch;

	(*so->pcictag->intr_disestablish)(so, ih);
}

static void
pxapcic_socket_enable(pcmcia_chipset_handle_t pch)
{
	struct pxapcic_socket *so = (struct pxapcic_socket *)pch;
	int i;

	/* Power down the card and socket before setting the voltage. */
	(*so->pcictag->write)(so, PXAPCIC_CARD_POWER, PXAPCIC_POWER_OFF);
	(*so->pcictag->set_power)(so, PXAPCIC_POWER_OFF);

	/*
	 * Wait 300ms until power fails (Tpf).  Then, wait 100ms since
	 * we are changing Vcc (Toff).   
	 */
	delay((300 + 100) * 1000);

	/* Power up the socket and card at appropriate voltage. */
	if (so->power_capability & PXAPCIC_POWER_5V) {
		(*so->pcictag->set_power)(so, PXAPCIC_POWER_5V);
		(*so->pcictag->write)(so, PXAPCIC_CARD_POWER,
		    PXAPCIC_POWER_5V);
	} else {
		(*so->pcictag->set_power)(so, PXAPCIC_POWER_3V);
		(*so->pcictag->write)(so, PXAPCIC_CARD_POWER,
		    PXAPCIC_POWER_3V);
	}

	/*
	 * Wait 100ms until power raise (Tpr) and 20ms to become
	 * stable (Tsu(Vcc)).
	 *
	 * Some machines require some more time to be settled
	 * (another 200ms is added here).
	 */
	delay((100 + 20 + 200) * 1000);

	/* Hold RESET at least 10us. */
	(*so->pcictag->write)(so, PXAPCIC_CARD_RESET, 1);
	delay(10);
	/* XXX wrong, but lets TE-CF100 cards work for some reason. */
	delay(3000);
	(*so->pcictag->write)(so, PXAPCIC_CARD_RESET, 0);

	/* Wait 20ms as per PC Card standard (r2.01) section 4.3.6. */
	delay(20 * 1000);

	/* Wait for the card to become ready. */
	for (i = 0; i < 10000; i++) {
		if ((*so->pcictag->read)(so, PXAPCIC_CARD_READY))
			break;
		delay(500);
	}
}

static void
pxapcic_socket_disable(pcmcia_chipset_handle_t pch)
{
	struct pxapcic_socket *so = (struct pxapcic_socket *)pch;

#ifdef PCICDEBUG
	printf("pxapcic_socket_disable: socket %d\n", so->socket);
#endif

	/* Power down the card and socket. */
	(*so->pcictag->write)(so, PXAPCIC_CARD_POWER, PXAPCIC_POWER_OFF);
	(*so->pcictag->set_power)(so, PXAPCIC_POWER_OFF);
}

static void
pxapcic_socket_settype(pcmcia_chipset_handle_t pch, int type)
{

#ifdef PCICDEBUG
	printf("pxapcic_socket_settype: type=%d",type);

	switch (type) {
	case PCMCIA_IFTYPE_MEMORY:
		printf("(Memory)\n");
		break;
	case PCMCIA_IFTYPE_IO:
		printf("(I/O)\n");
		break;
	default:
		printf("(unknown)\n");
		break;
	}
#endif
}

/*
 * Attachment and initialization
 */
static int
pxapcic_print(void *aux, const char *name)
{

	return UNCONF;
}

void
pxapcic_attach_common(struct pxapcic_softc *sc,
    void (*socket_setup_hook)(struct pxapcic_socket *))
{
	struct pcmciabus_attach_args paa;
	struct pxapcic_socket *so;
	int s[PXAPCIC_NSLOT];
	u_int cs;
	int i;

	printf(": %d slot%s\n", sc->sc_nslots, sc->sc_nslots < 2 ? "" : "s");

	if (sc->sc_nslots == 0) {
		aprint_error("%s: can't attach\n", sc->sc_dev.dv_xname);
		return;
	}

	if (bus_space_map(sc->sc_iot, PXA2X0_MEMCTL_BASE, PXA2X0_MEMCTL_SIZE,
	    0, &sc->sc_memctl_ioh)) {
		aprint_error("%s: failed to map MEMCTL\n", sc->sc_dev.dv_xname);
		return;
	}

	/* Clear CIT (card present) and set NOS correctly. */
	bus_space_write_4(sc->sc_iot, sc->sc_memctl_ioh, MEMCTL_MECR,
	    (sc->sc_nslots == 2) ? MECR_NOS : 0);

	if (sc->sc_flags & PPF_REVERSE_ORDER) {
		for (i = 0; i < sc->sc_nslots; i++) {
			s[i] = sc->sc_nslots - 1 - i;
		}
	} else {
		for (i = 0; i < sc->sc_nslots; i++) {
			s[i] = i;
		}
	}

	for (i = 0; i < sc->sc_nslots; i++) {
		so = &sc->sc_socket[s[i]];
		so->sc = sc;
		so->socket = s[i];
		so->flags = 0;

		(*socket_setup_hook)(so);

		paa.paa_busname = "pcmcia";
		paa.pct = (pcmcia_chipset_tag_t)&pxapcic_pcmcia_functions;
		paa.pch = (pcmcia_chipset_handle_t)so;
		paa.iobase = 0;
		paa.iosize = 0x4000000;

		so->pcmcia = config_found_ia(&sc->sc_dev, "pcmciabus", &paa,
		    pxapcic_print);

		pxa2x0_gpio_set_function(sc->sc_irqpin[s[i]], GPIO_IN);
		pxa2x0_gpio_set_function(sc->sc_irqcfpin[s[i]], GPIO_IN);

		/* Card slot interrupt */
		so->irq = pxa2x0_gpio_intr_establish(sc->sc_irqcfpin[s[i]],
		    IST_EDGE_BOTH, IPL_BIO /* XXX */, pxapcic_intr, so);

		/* GPIO pin for interrupt */
		so->irqpin = sc->sc_irqpin[s[i]];

		/* If there's a card there, attach it. */
		cs = (*so->pcictag->read)(so, PXAPCIC_CARD_STATUS);
		if (cs == PXAPCIC_CARD_VALID)
			pxapcic_attach_card(so);

		if (kthread_create(PRI_NONE, 0, NULL, pxapcic_event_thread,
		    so, &so->event_thread, "%s,%d", sc->sc_dev.dv_xname,
		    so->socket)) {
			printf("%s: unable to create event thread for %d\n",
			     sc->sc_dev.dv_xname, so->socket);
		}
	}
}

/*
 * Card slot interrupt handling
 */
int
pxapcic_intr(void *arg)
{
	struct pxapcic_socket *so = (struct pxapcic_socket *)arg;

	(*so->pcictag->clear_intr)(so);
        wakeup(so);

        return 1;
}

static void   
pxapcic_event_thread(void *arg)
{
	struct pxapcic_socket *sock = (struct pxapcic_socket *)arg;
	u_int cs;
	int present;

	while (sock->sc->sc_shutdown == 0) {
		(void) tsleep(sock, PWAIT, "pxapcicev", 0);

		/* sleep .25s to avoid chattering interrupts */
		(void) tsleep((void *)sock, PWAIT, "pxapcicss", hz/4);

		cs = (*sock->pcictag->read)(sock, PXAPCIC_CARD_STATUS);
		present = sock->flags & PXAPCIC_FLAG_CARDP;
		if ((cs == PXAPCIC_CARD_VALID) == (present == 1)) {
			continue;	/* state unchanged */
		}

		/* XXX Do both? */
		pxapcic_event_process(sock);
	}
	sock->event_thread = NULL;

	/* In case parent is waiting for us to exit. */
	wakeup(sock->sc);
	kthread_exit(0);
}

static void
pxapcic_event_process(struct pxapcic_socket *sock)
{
	u_int cs;

	cs = (*sock->pcictag->read)(sock, PXAPCIC_CARD_STATUS);
	if (cs == PXAPCIC_CARD_VALID) {
		if (!(sock->flags & PXAPCIC_FLAG_CARDP)) {
			pxapcic_attach_card(sock);
		}
	} else {
		if ((sock->flags & PXAPCIC_FLAG_CARDP)) {
			pxapcic_detach_card(sock, DETACH_FORCE);
		}
	}
}

static void
pxapcic_attach_card(struct pxapcic_socket *h)
{
	struct pxapcic_softc *sc = h->sc;
	uint32_t reg;

	if (h->flags & PXAPCIC_FLAG_CARDP)
		panic("pcic_attach_card: already attached"); 
	h->flags |= PXAPCIC_FLAG_CARDP;

	/* Set CIT if any card is present. */
	reg = bus_space_read_4(sc->sc_iot, sc->sc_memctl_ioh, MEMCTL_MECR);
	bus_space_write_4(sc->sc_iot, sc->sc_memctl_ioh, MEMCTL_MECR,
	    reg | MECR_CIT);

	/* call the MI attach function */
	pcmcia_card_attach(h->pcmcia);
}

static void
pxapcic_detach_card(struct pxapcic_socket *h, int flags)
{
	struct pxapcic_softc *sc = h->sc;
	uint32_t reg;
	int i;

	if (h->flags & PXAPCIC_FLAG_CARDP) {
		h->flags &= ~PXAPCIC_FLAG_CARDP;

		/* call the MI detach function */
		pcmcia_card_detach(h->pcmcia, flags);
	}

	/* Clear CIT if no other card is present. */
	for (i = 0; i < sc->sc_nslots; i++) {
		if (sc->sc_socket[i].flags & PXAPCIC_FLAG_CARDP) {
			return;
		}
	}

	reg = bus_space_read_4(sc->sc_iot, sc->sc_memctl_ioh, MEMCTL_MECR);
	bus_space_write_4(sc->sc_iot, sc->sc_memctl_ioh, MEMCTL_MECR,
	    reg & ~MECR_CIT);
}
