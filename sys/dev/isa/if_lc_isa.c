/*	$NetBSD: if_lc_isa.c,v 1.1 1997/07/31 21:58:19 matt Exp $ */

/*-
 * Copyright (c) 1994, 1995, 1997 Matt Thomas <matt@3am-software.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * DEC EtherWORKS 3 Ethernet Controllers
 *
 * Written by Matt Thomas
 *
 *   This driver supports the LEMAC (DE203, DE204, and DE205) cards.
 */

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/select.h>
#include <sys/device.h>
#include <sys/queue.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>


#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ic/lemacreg.h>
#include <dev/ic/lemacvar.h>

#include <dev/isa/isavar.h>

/*
 * This keeps track of which ISAs have been through a lc probe sequence.
 * A simple static variable isn't enough, since it's conceivable that
 * a system might have more than one ISA bus.
 *
 * The "isa_bus" member is a pointer to the parent ISA bus device struct
 * which will unique per ISA bus.
 */

#define MAXCARDS_PER_ISABUS	4	/* if you have more than 4, you lose */
#define	MAXSLOTS_PER_BUS	16	/* 0x200-0x3FF in 0x20 incr. */

struct lemac_isabus {
    LIST_ENTRY(lemac_isabus) isa_link;
    struct device *isa_bus;
    struct lemac_card {
	bus_addr_t iobase;
	bus_addr_t maddr;
	bus_size_t msize;
	int irq;
	int available;
    } isa_cards[MAXCARDS_PER_ISABUS];
};

static LIST_HEAD(, lemac_isabus) lemac_isa_buses;
static int lemac_isa_buses_inited;

static void
lemac_isa_card_add(
    struct lemac_isabus *bus,
    bus_addr_t iobase,
    bus_addr_t maddr,
    bus_size_t msize,
    int irq)
{
    int idx;

    for (idx = 0; idx < MAXCARDS_PER_ISABUS; idx++) {
	if (bus->isa_cards[idx].available == 0) {
	    bus->isa_cards[idx].iobase = iobase;
	    bus->isa_cards[idx].maddr = maddr;
	    bus->isa_cards[idx].msize = msize;
	    bus->isa_cards[idx].irq = irq;
	    bus->isa_cards[idx].available = 1;
	    break;
	}
    }
}

/*
 * We'd like like to allow the irq, maddr, and iobase addresses to be
 * wildcarded. So, we probe all the cards the first time lemac_isa_probe()
 * is called. On subsequent calls we look for matching cards.
 */
static int
lemac_isa_probe(
    struct device *parent,
#ifdef __BROKEN_INDIRECT_CONFIG
    void *match,
#else
    struct cfdata *match,
#endif
    void *aux)
{
    struct isa_attach_args * const ia = aux;
    struct lemac_isabus *bus;
    int idx;

    if (lemac_isa_buses_inited == 0) {
	LIST_INIT(&lemac_isa_buses);
	lemac_isa_buses_inited = 1;
    }

    /*
     * Probe this bus if we haven't done so already.
     */
    for (bus = lemac_isa_buses.lh_first; bus != NULL;
					 bus = bus->isa_link.le_next) {
	if (bus->isa_bus == parent)
	    break;
    }

    if (bus == NULL) {
	bus_addr_t iobase;
	/*
	 * Mark this bus so we don't probe it again.
	 */
	bus = (struct lemac_isabus *)
	    malloc(sizeof(struct lemac_isabus), M_DEVBUF, M_NOWAIT);
	if (bus == NULL)
	    panic("lemac_isa_probe: can't allocate state storage for %s",
		  parent->dv_xname);

	bus->isa_bus = parent;
	LIST_INSERT_HEAD(&lemac_isa_buses, bus, isa_link);

	for (iobase = LEMAC_IOBASE_LOW; iobase < LEMAC_IOBASE_HIGH;
					iobase += LEMAC_IOSIZE) {
	    bus_space_handle_t ioh;
	    /*
	     * Map the LEMACs port-space for the probe sequence.
	     */
	    if (bus_space_map(ia->ia_iot, iobase, LEMAC_IOSIZE, 0, &ioh))
		continue;

	    /*
	     * Read the Ethernet address from the EEPROM.
	     * It must start with on the DEC OUIs and pass the
	     * DEC ethernet checksum test.
	     */

	    if (lemac_port_check(ia->ia_iot, ioh)) {
		int irq;
		bus_addr_t maddr;
		bus_addr_t msize;
		bus_space_handle_t memh;

		lemac_info_get(ia->ia_iot, ioh, &maddr, &msize, &irq);
		if (!bus_space_map(ia->ia_memt, maddr, msize, 0, &memh)) {
		    lemac_isa_card_add(bus, iobase, maddr, msize, irq);
		    bus_space_unmap(ia->ia_memt, memh, msize);
		}
	    }

	    bus_space_unmap(ia->ia_iot, ioh, LEMAC_IOSIZE);
	}
    }

    for (idx = 0; idx < MAXCARDS_PER_ISABUS; idx++) {
	if (bus->isa_cards[idx].available != 1)
	    continue;
	if (ia->ia_iobase != IOBASEUNK
	        && ia->ia_iobase != bus->isa_cards[idx].iobase)
	    continue;
	if (ia->ia_maddr != MADDRUNK
	        && ia->ia_maddr != bus->isa_cards[idx].maddr)
	    continue;
	if (ia->ia_irq != IRQUNK && ia->ia_irq != bus->isa_cards[idx].irq)
	    continue;
	break;
    }
    if (idx == MAXCARDS_PER_ISABUS)
	return 0;

    bus->isa_cards[idx].available++;
    ia->ia_iobase = bus->isa_cards[idx].iobase;
    ia->ia_irq    = bus->isa_cards[idx].irq;
    ia->ia_iosize = LEMAC_IOSIZE;
    ia->ia_maddr  = bus->isa_cards[idx].maddr;
    ia->ia_msize  = bus->isa_cards[idx].msize;
    return 1;
}

static void
lemac_isa_attach(
    struct device *parent,
    struct device *self,
    void *aux)
{
    lemac_softc_t *sc = (void *)self;
    struct isa_attach_args *ia = aux;


    /* Map i/o space. */
    sc->sc_iot = ia->ia_iot;
    if (bus_space_map(sc->sc_iot, ia->ia_iobase, ia->ia_iosize, 0, &sc->sc_ioh))
	panic("\n%s: can't map i/o space 0x%x-0x%x\n",
	      sc->sc_dv.dv_xname,
	      ia->ia_iobase, ia->ia_iobase + ia->ia_iosize - 1);

    if (ia->ia_msize && ia->ia_maddr) {
	sc->sc_memt = ia->ia_memt;
	/* Map memory space. */
	if (bus_space_map(sc->sc_memt, ia->ia_maddr, ia->ia_msize, 0, &sc->sc_memh))
	    panic("\n%s: can't map iomem space 0x%x-0x%x\n",
		  sc->sc_dv.dv_xname,
		  ia->ia_maddr, ia->ia_maddr + ia->ia_msize - 1);
    }

    sc->sc_ats = shutdownhook_establish(lemac_shutdown, sc);
    if (sc->sc_ats == NULL)
	printf("\n%s: warning: couldn't establish shutdown hook\n",
	       sc->sc_dv.dv_xname);

    lemac_ifattach(sc);

    sc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq, IST_EDGE,
				   IPL_NET, lemac_intr, sc);
}

struct cfattach lc_isa_ca = {
    sizeof(lemac_softc_t), lemac_isa_probe, lemac_isa_attach
};
