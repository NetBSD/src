/*	$Id: imx_pcic.c,v 1.1.20.1 2008/06/02 13:21:53 mjf Exp $	*/

/*
 * IMX CF interface to pcic/pcmcia
 * derived from pxa2x0_pcic
 * Sun Apr  1 21:42:37 PDT 2007
 */

/*	$NetBSD: imx_pcic.c,v 1.1.20.1 2008/06/02 13:21:53 mjf Exp $	*/
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
__KERNEL_RCSID(0, "$Id: imx_pcic.c,v 1.1.20.1 2008/06/02 13:21:53 mjf Exp $");

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

#ifdef NOTYET
#include <arm/imx/imx_gpio.h>
#endif
#include <arm/imx/imx_pcic.h>

static int	imx_pcic_print(void *, const char *);

static void	imx_pcic_event_thread(void *);
#ifdef NOTYET
static void	imx_pcic_event_process(struct imx_pcic_socket *);
static void	imx_pcic_attach_card(struct imx_pcic_socket *);
#endif
#ifdef NOTYET
static void	imx_pcic_detach_card(struct imx_pcic_socket *, int);
#endif

static int	imx_pcic_mem_alloc(pcmcia_chipset_handle_t, bus_size_t,
		    struct pcmcia_mem_handle *);
static void	imx_pcic_mem_free(pcmcia_chipset_handle_t,
		    struct pcmcia_mem_handle *);
static int	imx_pcic_mem_map(pcmcia_chipset_handle_t, int, bus_addr_t,
		   bus_size_t, struct pcmcia_mem_handle *, bus_size_t *, int *);
static void	imx_pcic_mem_unmap(pcmcia_chipset_handle_t, int);

static int	imx_pcic_io_alloc(pcmcia_chipset_handle_t, bus_addr_t,
		    bus_size_t, bus_size_t, struct pcmcia_io_handle *);
static void	imx_pcic_io_free(pcmcia_chipset_handle_t,
		    struct pcmcia_io_handle *);
static int	imx_pcic_io_map(pcmcia_chipset_handle_t, int,
		    bus_addr_t, bus_size_t, struct pcmcia_io_handle *, int *);
static void	imx_pcic_io_unmap(pcmcia_chipset_handle_t, int);

static void	*imx_pcic_intr_establish(pcmcia_chipset_handle_t,
		    struct pcmcia_function *, int, int (*)(void *), void *);
static void	imx_pcic_intr_disestablish(pcmcia_chipset_handle_t, void *);

static void	imx_pcic_socket_enable(pcmcia_chipset_handle_t);
static void	imx_pcic_socket_disable(pcmcia_chipset_handle_t);
static void	imx_pcic_socket_settype(pcmcia_chipset_handle_t, int);

/*
 * PCMCIA chipset methods
 */
static struct pcmcia_chip_functions imx_pcic_pcmcia_functions = {
	imx_pcic_mem_alloc,
	imx_pcic_mem_free,
	imx_pcic_mem_map,
	imx_pcic_mem_unmap,

	imx_pcic_io_alloc,
	imx_pcic_io_free,
	imx_pcic_io_map,
	imx_pcic_io_unmap,

	imx_pcic_intr_establish,
	imx_pcic_intr_disestablish,

	imx_pcic_socket_enable,
	imx_pcic_socket_disable,
	imx_pcic_socket_settype,
};

#define IMX_MEMCTL_BASE	0x08000000		/* XXX */
#define IMX_MEMCTL_SIZE	0x00000010		/* XXX */
#define IMX_PCIC_SOCKET_BASE	0x08009000		/* XXX */
#define IMX_PCIC_SOCKET_OFFSET	0x00000000		/* XXX */
#define	IMX_PCIC_ATTR_OFFSET	0x00000800		/* XXX 5912 */
#define	IMX_PCIC_COMMON_OFFSET	0x00000000		/* XXX 5912 */



/*
 * PCMCIA Helpers
 */
static int
imx_pcic_mem_alloc(pcmcia_chipset_handle_t pch, bus_size_t size,
    struct pcmcia_mem_handle *pmh)
{
	struct imx_pcic_socket *so = (struct imx_pcic_socket *)pch;

	/* All we need is the bus space tag */
	memset(pmh, 0, sizeof(*pmh));
	pmh->memt = so->sc->sc_iot;

	return 0;
}

static void
imx_pcic_mem_free(pcmcia_chipset_handle_t pch, struct pcmcia_mem_handle *pmh)
{

	/* Nothing to do */
}

static int
imx_pcic_mem_map(pcmcia_chipset_handle_t pch, int kind, bus_addr_t card_addr,
    bus_size_t size, struct pcmcia_mem_handle *pmh, bus_size_t *offsetp,
    int *windowp)
{
	struct imx_pcic_socket *so = (struct imx_pcic_socket *)pch;
	int error;
	bus_addr_t pa;
 
printf("%s: card_addr %lx\n", __FUNCTION__, card_addr);
	pa = trunc_page(card_addr);
	*offsetp = card_addr - pa;
printf("%s: offset %lx\n", __FUNCTION__, *offsetp);
	size = round_page(card_addr + size) - pa;
	pmh->realsize = size;

	pa += IMX_PCIC_SOCKET_BASE;
	pa += IMX_PCIC_SOCKET_OFFSET * so->socket;
printf("%s: pa %lx\n", __FUNCTION__, pa);
printf("%s: kind %x\n", __FUNCTION__, kind);

	switch (kind & ~PCMCIA_WIDTH_MEM_MASK) {
	case PCMCIA_MEM_ATTR:   
		pa += IMX_PCIC_ATTR_OFFSET;
		break;
	case PCMCIA_MEM_COMMON:
		pa += IMX_PCIC_COMMON_OFFSET;
		break;
	default:
		panic("imx_pcic_mem_map: bogus kind");
	}

printf("%s: pa %lx\n", __FUNCTION__, pa);
Debugger();
	error = bus_space_map(so->sc->sc_iot, pa, size, 0, &pmh->memh);
	if (error)
		return error;

	*windowp = (int)pmh->memh;
	return 0;
}

static void
imx_pcic_mem_unmap(pcmcia_chipset_handle_t pch, int window)
{
	struct imx_pcic_socket *so = (struct imx_pcic_socket *)pch;

	bus_space_unmap(so->sc->sc_iot, (bus_addr_t)window, 4096); /* XXX */
}

static int
imx_pcic_io_alloc(pcmcia_chipset_handle_t pch, bus_addr_t start,
    bus_size_t size, bus_size_t align, struct pcmcia_io_handle *pih)
{
	struct imx_pcic_socket *so = (struct imx_pcic_socket *)pch;
	bus_addr_t pa;
	int error;

	memset(pih, 0, sizeof(*pih));
	pih->iot = so->sc->sc_iot;
	pih->addr = start;
	pih->size = size;

	pa = pih->addr;
	pa += IMX_PCIC_SOCKET_BASE;
	pa += IMX_PCIC_SOCKET_OFFSET * so->socket;

	/* XXX Are we ignoring alignment constraints? */
	error = bus_space_map(so->sc->sc_iot, pa, size, 0, &pih->ioh);

	return error;
}

static void
imx_pcic_io_free(pcmcia_chipset_handle_t pch, struct pcmcia_io_handle *pih)
{
	struct imx_pcic_socket *so = (struct imx_pcic_socket *)pch;

	bus_space_unmap(so->sc->sc_iot, pih->ioh, pih->size);
}
 
static int
imx_pcic_io_map(pcmcia_chipset_handle_t pch, int width, bus_addr_t offset,
    bus_size_t size, struct pcmcia_io_handle *pih, int *windowp)
{

	return 0;
}

static void
imx_pcic_io_unmap(pcmcia_chipset_handle_t pch, int window)
{

	/* Nothing to do */
}

static void *
imx_pcic_intr_establish(pcmcia_chipset_handle_t pch,
    struct pcmcia_function *pf, int ipl, int (*fct)(void *), void *arg)
{
	struct imx_pcic_socket *so = (struct imx_pcic_socket *)pch;
	/* XXX need to check if something should be done here */

	return (*so->pcictag->intr_establish)(so, ipl, fct, arg);
}

static void
imx_pcic_intr_disestablish(pcmcia_chipset_handle_t pch, void *ih)
{
	struct imx_pcic_socket *so = (struct imx_pcic_socket *)pch;

	(*so->pcictag->intr_disestablish)(so, ih);
}

static void
imx_pcic_socket_enable(pcmcia_chipset_handle_t pch)
{
#ifdef NOTYET
	struct imx_pcic_socket *so = (struct imx_pcic_socket *)pch;
	int i;

	/* Power down the card and socket before setting the voltage. */
	(*so->pcictag->write)(so, IMX_PCIC_CARD_POWER, IMX_PCIC_POWER_OFF);
	(*so->pcictag->set_power)(so, IMX_PCIC_POWER_OFF);

	/*
	 * Wait 300ms until power fails (Tpf).  Then, wait 100ms since
	 * we are changing Vcc (Toff).   
	 */
	delay((300 + 100) * 1000);

	/* Power up the socket and card at appropriate voltage. */
	if (so->power_capability & IMX_PCIC_POWER_5V) {
		(*so->pcictag->set_power)(so, IMX_PCIC_POWER_5V);
		(*so->pcictag->write)(so, IMX_PCIC_CARD_POWER,
		    IMX_PCIC_POWER_5V);
	} else {
		(*so->pcictag->set_power)(so, IMX_PCIC_POWER_3V);
		(*so->pcictag->write)(so, IMX_PCIC_CARD_POWER,
		    IMX_PCIC_POWER_3V);
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
	(*so->pcictag->write)(so, IMX_PCIC_CARD_RESET, 1);
	delay(10);
	/* XXX wrong, but lets TE-CF100 cards work for some reason. */
	delay(3000);
	(*so->pcictag->write)(so, IMX_PCIC_CARD_RESET, 0);

	/* Wait 20ms as per PC Card standard (r2.01) section 4.3.6. */
	delay(20 * 1000);

	/* Wait for the card to become ready. */
	for (i = 0; i < 10000; i++) {
		if ((*so->pcictag->read)(so, IMX_PCIC_CARD_READY))
			break;
		delay(500);
	}
#else
printf("%s: (stubbed)\n", __FUNCTION__);
#endif	/* NOTYET */
}

static void
imx_pcic_socket_disable(pcmcia_chipset_handle_t pch)
{
#ifdef NOTYET
	struct imx_pcic_socket *so = (struct imx_pcic_socket *)pch;

#ifdef PCICDEBUG
	printf("imx_pcic_socket_disable: socket %d\n", so->socket);
#endif

	/* Power down the card and socket. */
	(*so->pcictag->write)(so, IMX_PCIC_CARD_POWER, IMX_PCIC_POWER_OFF);
	(*so->pcictag->set_power)(so, IMX_PCIC_POWER_OFF);
#endif	/* NOTYET */
}

static void
imx_pcic_socket_settype(pcmcia_chipset_handle_t pch, int type)
{

#ifdef PCICDEBUG
	printf("imx_pcic_socket_settype: type=%d",type);

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
imx_pcic_print(void *aux, const char *name)
{

	return UNCONF;
}

void
imx_pcic_attach_common(struct imx_pcic_softc *sc,
    void (*socket_setup_hook)(struct imx_pcic_socket *))
{
	struct pcmciabus_attach_args paa;
	struct imx_pcic_socket *so;
	int s[IMX_PCIC_NSLOT];
	int i;

	printf(": %d slot%s\n", sc->sc_nslots, sc->sc_nslots < 2 ? "" : "s");

	if (sc->sc_nslots == 0) {
		aprint_error("%s: can't attach\n", sc->sc_dev.dv_xname);
		return;
	}

	if (bus_space_map(sc->sc_iot, IMX_MEMCTL_BASE, IMX_MEMCTL_SIZE,
	    0, &sc->sc_memctl_ioh)) {
		aprint_error("%s: failed to map MEMCTL\n", sc->sc_dev.dv_xname);
		return;
	}

#if 0
	/* Clear CIT (card present) and set NOS correctly. */
	bus_space_write_4(sc->sc_iot, sc->sc_memctl_ioh, MEMCTL_MECR,
	    (sc->sc_nslots == 2) ? MECR_NOS : 0);
#endif

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
		paa.pct = (pcmcia_chipset_tag_t)&imx_pcic_pcmcia_functions;
		paa.pch = (pcmcia_chipset_handle_t)so;
printf("%s: sc_pa %lx\n", __FUNCTION__, sc->sc_pa);
		paa.iobase = sc->sc_pa;
		paa.iosize = 0x2000;

		so->pcmcia = config_found_ia(&sc->sc_dev, "pcmciabus", &paa,
		    imx_pcic_print);

#ifdef NOTYET
		imx_gpio_set_direction(sc->sc_irqpin[s[i]], GPIO_IN);
		imx_gpio_set_direction(sc->sc_irqcfpin[s[i]], GPIO_IN);

		/* Card slot interrupt */
		so->irq = imx_gpio_intr_establish(sc->sc_irqcfpin[s[i]],
		    IST_EDGE_BOTH, IPL_BIO /* XXX */, "pcic",
		    imx_pcic_intr, so);

		/* GPIO pin for interrupt */
		so->irqpin = sc->sc_irqpin[s[i]];
#else
		so->irqpin = sc->sc_irqpin[s[i]];
printf("%s: slot %d, irqpin %d\n",  __FUNCTION__, s[i], sc->sc_irqpin[s[i]]);
#endif	/* NOTYET */

		if (kthread_create(PRI_NONE, 0, NULL,
		    imx_pcic_event_thread, so, &so->event_thread,
		    "%s,%d", sc->sc_dev.dv_xname, so->socket) != 0) {
			printf("%s: unable to create event thread for %d\n",
				sc->sc_dev.dv_xname, so->socket);
		}
	}
}

/*
 * Card slot interrupt handling
 */
int
imx_pcic_intr(void *arg)
{
	struct imx_pcic_socket *so = (struct imx_pcic_socket *)arg;

	(*so->pcictag->clear_intr)(so);
        wakeup(so);

        return 1;
}

static void   
imx_pcic_event_thread(void *arg)
{
#ifdef NOTYET
	struct imx_pcic_socket *sock = (struct imx_pcic_socket *)arg;
	u_int cs;
	int present;

	while (sock->sc->sc_shutdown == 0) {
		(void) tsleep(sock, PWAIT, "imx_pcicev", 0);

		/* sleep .25s to avoid chattering interrupts */
		(void) tsleep((void *)sock, PWAIT, "imx_pcicss", hz/4);

		cs = (*sock->pcictag->read)(sock, IMX_PCIC_CARD_STATUS);
		present = sock->flags & IMX_PCIC_FLAG_CARDP;
		if ((cs == IMX_PCIC_CARD_VALID) == (present == 1)) {
			continue;	/* state unchanged */
		}

		/* XXX Do both? */
		imx_pcic_event_process(sock);
	}
	sock->event_thread = NULL;

	/* In case parent is waiting for us to exit. */
	wakeup(sock->sc);
	kthread_exit(0);
#endif	/* NOTYET */
}

#ifdef NOTYET
static void
imx_pcic_event_process(struct imx_pcic_socket *sock)
{
	u_int cs;

	cs = (*sock->pcictag->read)(sock, IMX_PCIC_CARD_STATUS);
	if (cs == IMX_PCIC_CARD_VALID) {
		if (!(sock->flags & IMX_PCIC_FLAG_CARDP)) {
			imx_pcic_attach_card(sock);
		}
	} else {
		if ((sock->flags & IMX_PCIC_FLAG_CARDP)) {
			imx_pcic_detach_card(sock, DETACH_FORCE);
		}
	}
}

static void
imx_pcic_attach_card(struct imx_pcic_socket *h)
{

	if (h->flags & IMX_PCIC_FLAG_CARDP)
		panic("pcic_attach_card: already attached"); 
	h->flags |= IMX_PCIC_FLAG_CARDP;


	/* call the MI attach function */
	pcmcia_card_attach(h->pcmcia);
}
#endif	/* NOTYET */

#ifdef NOTYET
static void
imx_pcic_detach_card(struct imx_pcic_socket *h, int flags)
{
	struct imx_pcic_softc *sc = h->sc;
	int i;

	if (h->flags & IMX_PCIC_FLAG_CARDP) {
		h->flags &= ~IMX_PCIC_FLAG_CARDP;

		/* call the MI detach function */
		pcmcia_card_detach(h->pcmcia, flags);
	}

	/* Clear CIT if no other card is present. */
	for (i = 0; i < sc->sc_nslots; i++) {
		if (sc->sc_socket[i].flags & IMX_PCIC_FLAG_CARDP) {
			return;
		}
	}
}
#endif	/* NOTYET */
