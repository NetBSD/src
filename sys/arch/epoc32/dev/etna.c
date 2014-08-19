/*	$NetBSD: etna.c,v 1.2.2.3 2014/08/20 00:02:52 tls Exp $	*/
/*
 * Copyright (c) 2012 KIYOHARA Takashi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: etna.c,v 1.2.2.3 2014/08/20 00:02:52 tls Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kthread.h>
#include <sys/lwp.h>

#include <machine/epoc32.h>

#include <arm/pic/picvar.h>

#include <dev/pcmcia/pcmciachip.h>
#include <dev/pcmcia/pcmciavar.h>

#define ETNA_SIZE		0x10
#define ETNA_CFSPACE_SIZE	0x10000000
#define ETNA_ATTR_BASE		0x00000000
#define ETNA_MEM_BASE		0x04000000
#define ETNA_IO8_BASE		0x08000000
#define ETNA_IO16_BASE		0x0c000000

#define ETNA_INT_STATUS	0x6	/* Interrupt Status */
#define ETNA_INT_MASK	0x7	/* Interrupt Mask */
#define ETNA_INT_CLEAR	0x8	/* Interrupt Clear */
#define   INT_CARD	  (1 << 0)	/* Card Interrupt */
#define   INT_BUSY	  (1 << 2)	/* Socket Interrupt */
#define   INT_SOCK_1	  (1 << 4)
#define   INT_SOCK_2	  (1 << 5)
#define   INT_RESV	  (1 << 6)	/* Reserved ? */
#define ETNA_SKT_STATUS	0x9
#define   SKT_BUSY	  (1 << 0)	/* Socket BUSY */
#define   SKT_NOT_READY	  (1 << 1)	/* Socket Not Ready */
#define   SKT_CARD_OUT	  (1 << 2)	/* Socket Card Out */
#define ETNA_SKT_CONFIG	0xa
#define ETNA_SKT_CTRL	0xb
#define ETNA_WAKE_1	0xc
#define ETNA_SKT_ACTIVE	0xd
#define ETNA_WAKE_2	0xf

struct etna_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	device_t sc_pcmcia;
	bus_addr_t sc_pcmcia_space;	/* attr/mem/io spaces */
#define NMEM_HANDLE	8
	struct pcmcia_mem_handle *sc_pcmhp[NMEM_HANDLE];
#define NIO_HANDLE	8
	struct pcmcia_io_handle *sc_pcihp[NIO_HANDLE];

	int (*sc_card_handler)(void *);
	void *sc_card_arg;

	struct lwp *sc_event_thread;
};


static int etna_match(device_t, cfdata_t, void *);
static void etna_attach(device_t, device_t, void *);

static int etna_card_intr(void *);

static void etna_doattach(device_t);
static void etna_event_thread(void *);

static void etna_attach_card(struct etna_softc *);

static int etna_mem_alloc(pcmcia_chipset_handle_t, bus_size_t,
			  struct pcmcia_mem_handle *);
static void etna_mem_free(pcmcia_chipset_handle_t, struct pcmcia_mem_handle *);
static int etna_mem_map(pcmcia_chipset_handle_t, int, bus_addr_t, bus_size_t,
			struct pcmcia_mem_handle *, bus_size_t *, int *);
static void etna_mem_unmap(pcmcia_chipset_handle_t, int);

static int etna_io_alloc(pcmcia_chipset_handle_t, bus_addr_t, bus_size_t,
			 bus_size_t, struct pcmcia_io_handle *);
static void etna_io_free(pcmcia_chipset_handle_t, struct pcmcia_io_handle *);
static int etna_io_map(pcmcia_chipset_handle_t, int, bus_addr_t, bus_size_t,
		       struct pcmcia_io_handle *, int *);
static void etna_io_unmap(pcmcia_chipset_handle_t, int);

static void *etna_intr_establish(pcmcia_chipset_handle_t,
				 struct pcmcia_function *, int,
				 int (*)(void *), void *);
static void etna_intr_disestablish(pcmcia_chipset_handle_t, void *);

static void etna_socket_enable(pcmcia_chipset_handle_t);
static void etna_socket_disable(pcmcia_chipset_handle_t);
static void etna_socket_settype(pcmcia_chipset_handle_t, int);

CFATTACH_DECL_NEW(etna, sizeof(struct etna_softc),
    etna_match, etna_attach, NULL, NULL);

struct pcmcia_chip_functions etna_pcmcia_functions = {
	etna_mem_alloc,
	etna_mem_free,
	etna_mem_map,
	etna_mem_unmap,

	etna_io_alloc,
	etna_io_free,
	etna_io_map,
	etna_io_unmap,

	etna_intr_establish,
	etna_intr_disestablish,

	etna_socket_enable,
	etna_socket_disable,
	etna_socket_settype,
};

/* ARGSUSED */
static int
etna_match(device_t parent, cfdata_t match, void *aux)
{
	struct external_attach_args *aa = aux;
	bus_space_handle_t ioh;
	int x, rv = 0;

	if (bus_space_map(aa->iot, aa->addr, ETNA_SIZE, 0, &ioh) != 0)
		return 0;
	if (bus_space_read_1(aa->iot, ioh, 0x0) != 0x0f ||
	    bus_space_read_1(aa->iot, ioh, 0x1) != 0x00 ||
	    bus_space_read_1(aa->iot, ioh, 0x2) != 0x02 ||
	    bus_space_read_1(aa->iot, ioh, 0x3) != 0x00 ||
	    bus_space_read_1(aa->iot, ioh, 0x4) != 0x00 ||
	    bus_space_read_1(aa->iot, ioh, 0xc) != 0x88)
		goto out;
	x = bus_space_read_1(aa->iot, ioh, ETNA_INT_MASK);
	bus_space_write_1(aa->iot, ioh, ETNA_INT_MASK, (~x & 0xff) | INT_RESV);
	if (bus_space_read_1(aa->iot, ioh, ETNA_INT_MASK) !=
						((~x & 0xff) | INT_RESV)) {
		bus_space_write_1(aa->iot, ioh, ETNA_INT_MASK, x);
		goto out;
	}
	bus_space_write_1(aa->iot, ioh, ETNA_INT_MASK, x);
	if (bus_space_read_1(aa->iot, ioh, ETNA_INT_MASK) != x)
		goto out;
	rv = 1;
out:
	bus_space_unmap(aa->iot, ioh, ETNA_SIZE);
	return rv;
}

/* ARGSUSED */
static void
etna_attach(device_t parent, device_t self, void *aux)
{
	struct etna_softc *sc = device_private(self);
	struct external_attach_args *aa = aux;
	struct pcmciabus_attach_args paa;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;
	if (bus_space_map(aa->iot, aa->addr, ETNA_SIZE, 0, &sc->sc_ioh) != 0) {
		aprint_error_dev(self, "can't map register\n");
		return;
	}
	sc->sc_iot = aa->iot;
	if (intr_establish(aa->irq, IPL_BIO, 0, etna_card_intr, sc) == NULL) {
		aprint_error_dev(self, "can't establish interrupt\n");
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, ETNA_SIZE);
		return;
	}
	sc->sc_pcmcia_space = aa->addr2;

	paa.paa_busname = "pcmcia";
	paa.pct = &etna_pcmcia_functions;
	paa.pch = sc;
	sc->sc_pcmcia = config_found_ia(self, "pcmciabus", &paa, NULL);

	config_interrupts(self, etna_doattach);
}

static int
etna_card_intr(void *arg)
{
	struct etna_softc *sc = arg;
	int status;

	status = bus_space_read_1(sc->sc_iot, sc->sc_ioh, ETNA_INT_STATUS);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, ETNA_INT_CLEAR, status);

	if (sc->sc_card_handler == NULL)
		return 0;
	return sc->sc_card_handler(sc->sc_card_arg);
}

static void
etna_doattach(device_t self)
{
	struct etna_softc *sc = device_private(self);
	int status;

	config_pending_incr(self);

	status = bus_space_read_1(sc->sc_iot, sc->sc_ioh, ETNA_SKT_STATUS);
	if ((status & SKT_CARD_OUT) != SKT_CARD_OUT)
		etna_attach_card(sc);

	if (kthread_create(PRI_NONE, 0, NULL, etna_event_thread, sc,
				   &sc->sc_event_thread, device_xname(self)))
		aprint_error_dev(self, "unable to create event thread\n");
}

static void
etna_event_thread(void *arg)
{
	struct etna_softc *sc = arg;

	config_pending_decr(sc->sc_dev);

//	while (1) {
//	}

	/* NOTREACHED */

	sc->sc_event_thread = NULL;
	kthread_exit(0);
}


static void
etna_attach_card(struct etna_softc *sc)
{

	pcmcia_card_attach(sc->sc_pcmcia);
}


/*
 * pcmcia chip functions
 */

/* ARGSUSED */
static int
etna_mem_alloc(pcmcia_chipset_handle_t pch, bus_size_t size,
	       struct pcmcia_mem_handle *pcmhp)
{
	struct etna_softc *sc = (struct etna_softc *)pch;

	memset(pcmhp, 0, sizeof(*pcmhp));
	pcmhp->memt = sc->sc_iot;
	return 0;
}

/* ARGSUSED */
static void
etna_mem_free(pcmcia_chipset_handle_t pch, struct pcmcia_mem_handle *pcmhp)
{
	/* Nothing */
}

static int
etna_mem_map(pcmcia_chipset_handle_t pch, int kind, bus_addr_t card_addr,
	     bus_size_t size, struct pcmcia_mem_handle *pcmhp,
	     bus_size_t *offsetp, int *windowp)
{
	struct etna_softc *sc = (struct etna_softc *)pch;
	bus_addr_t addr;
	int rv, i;

	for (i = 0; i < NMEM_HANDLE; i++)
		if (sc->sc_pcmhp[i] == NULL)
			break;
	if (i == NMEM_HANDLE) {
		aprint_error_dev(sc->sc_dev, "no mem window\n");
		return ENOMEM;
	}

	addr = trunc_page(card_addr);
	size = round_page(card_addr + size) - addr;
	*offsetp = card_addr - addr;
	addr += sc->sc_pcmcia_space;
	switch (kind & ~PCMCIA_WIDTH_MEM_MASK) {
	case PCMCIA_MEM_ATTR:
		addr += ETNA_ATTR_BASE;
		break;
	case PCMCIA_MEM_COMMON:
		addr += ETNA_MEM_BASE;
		break;
	default:
		panic("etna_mem_map: bogus kind\n");
	}
	rv = bus_space_map(pcmhp->memt, addr, size, 0, &pcmhp->memh);
	if (rv)
		return rv;
	pcmhp->realsize = size;
	sc->sc_pcmhp[i] = pcmhp;
	*windowp = i;
	return 0;
}

static void
etna_mem_unmap(pcmcia_chipset_handle_t pch, int window)
{
	struct etna_softc *sc = (struct etna_softc *)pch;
	struct pcmcia_mem_handle *pcmhp = sc->sc_pcmhp[window];

	bus_space_unmap(pcmhp->memt, pcmhp->memh, pcmhp->realsize);
	sc->sc_pcmhp[window] = NULL;
}

/* ARGSUSED */
static int
etna_io_alloc(pcmcia_chipset_handle_t pch, bus_addr_t start, bus_size_t size,
	      bus_size_t align, struct pcmcia_io_handle *pcihp)
{
	struct etna_softc *sc = (struct etna_softc *)pch;
	extern char epoc32_model[];

	/*
	 * XXXXX: Series 5 can't allocate I/O map???
	 */
	if (strcmp(epoc32_model, "SERIES5 R1") == 0)
		return -1;

	memset(pcihp, 0, sizeof(*pcihp));
	pcihp->iot = sc->sc_iot;
	pcihp->addr = start;
	pcihp->size = size;

	return 0;
}

/* ARGSUSED */
static void
etna_io_free(pcmcia_chipset_handle_t pch, struct pcmcia_io_handle *pcihp)
{
	/* Nothing */
}

/* ARGSUSED */
static int
etna_io_map(pcmcia_chipset_handle_t pch, int width, bus_addr_t offset,
	    bus_size_t size, struct pcmcia_io_handle *pcihp, int *windowp)
{
	struct etna_softc *sc = (struct etna_softc *)pch;
	bus_addr_t addr;
	int rv, i;

	for (i = 0; i < NIO_HANDLE; i++)
		if (sc->sc_pcihp[i] == NULL)
			break;
	if (i == NIO_HANDLE) {
		aprint_error_dev(sc->sc_dev, "no io window\n");
		return ENOMEM;
	}

	addr = trunc_page(pcihp->addr);
	size = round_page(pcihp->addr + size) - addr;
	addr += sc->sc_pcmcia_space;
	switch (width) {
	case PCMCIA_WIDTH_AUTO:
	case PCMCIA_WIDTH_IO16:
		addr += ETNA_IO16_BASE;
		break;
	case PCMCIA_WIDTH_IO8:
		addr += ETNA_IO8_BASE;
		break;
	default:
		panic("etna_io_map: bogus width\n");
	}
	rv = bus_space_map(pcihp->iot, addr, size, 0, &pcihp->ioh);
	if (rv)
		return rv;
	sc->sc_pcihp[i] = pcihp;
	*windowp = i;
	return 0;
}

static void
etna_io_unmap(pcmcia_chipset_handle_t pch, int window)
{
	struct etna_softc *sc = (struct etna_softc *)pch;
	struct pcmcia_io_handle *pcihp = sc->sc_pcihp[window];

	bus_space_unmap(pcihp->iot, pcihp->ioh, pcihp->size);
	sc->sc_pcihp[window] = NULL;
}

/* ARGSUSED */
static void *
etna_intr_establish(pcmcia_chipset_handle_t pch, struct pcmcia_function *pf,
		    int ipl, int (*fct)(void *), void *arg)
{
	struct etna_softc *sc = (struct etna_softc *)pch;

	sc->sc_card_handler = fct;
	sc->sc_card_arg = arg;

	/* Enable card interrupt */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, ETNA_INT_MASK, INT_CARD);

	return sc->sc_card_handler;	/* XXXX: Is it OK? */
}

static void
etna_intr_disestablish(pcmcia_chipset_handle_t pch, void *ih)
{
	struct etna_softc *sc = (struct etna_softc *)pch;

	KASSERT(ih == sc->sc_card_handler);

	/* Disable card interrupt */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, ETNA_INT_MASK, 0x00);

	sc->sc_card_handler = NULL;
	sc->sc_card_arg = NULL;
}

static void
etna_socket_enable(pcmcia_chipset_handle_t pch)
{
	/* XXXX: type depend */
}

static void
etna_socket_disable(pcmcia_chipset_handle_t pch)
{
	/* XXXX: type depend */
}

static void
etna_socket_settype(pcmcia_chipset_handle_t pch, int type)
{
	/* Nothing */
}
