/*	$NetBSD: shpcic.c,v 1.6 2002/02/12 15:26:46 uch Exp $	*/

#define	SHPCICDEBUG

/*
 * Copyright (c) 1997 Marc Horowitz.  All rights reserved.
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
 *	This product includes software developed by Marc Horowitz.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/kthread.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>

#include <sh3/dev/shpcicreg.h>
#include <sh3/dev/shpcicvar.h>

#include "locators.h"

#ifdef SHPCICDEBUG
int	shpcic_debug = 0;
#define	DPRINTF(arg) if (shpcic_debug) printf arg;
#else
#define	DPRINTF(arg)
#endif

#define	PCIC_VENDOR_UNKNOWN		0
#define	PCIC_VENDOR_HITACHI		1

/*
 * Individual drivers will allocate their own memory and io regions. Memory
 * regions must be a multiple of 4k, aligned on a 4k boundary.
 */

#define	SHPCIC_MEM_ALIGN	SHPCIC_MEM_PAGESIZE

void	shpcic_attach_socket(struct shpcic_handle *);
void	shpcic_init_socket(struct shpcic_handle *);

int	shpcic_submatch(struct device *, struct cfdata *, void *);
int	shpcic_print (void *, const char *);
int	shpcic_intr_socket(struct shpcic_handle *);

void	shpcic_attach_card(struct shpcic_handle *);
void	shpcic_detach_card(struct shpcic_handle *, int);
void	shpcic_deactivate_card(struct shpcic_handle *);

void	shpcic_chip_do_mem_map(struct shpcic_handle *, int);
void	shpcic_chip_do_io_map(struct shpcic_handle *, int);

void	shpcic_create_event_thread(void *);
void	shpcic_event_thread(void *);

void	shpcic_queue_event(struct shpcic_handle *, int);

/* static void	shpcic_wait_ready(struct shpcic_handle *); */

int
shpcic_ident_ok(int ident)
{
	/* this is very empirical and heuristic */

	if ((ident == 0) || (ident == 0xff) || (ident & SHPCIC_IDENT_ZERO))
		return (0);

	if ((ident & SHPCIC_IDENT_IFTYPE_MASK) != SHPCIC_IDENT_IFTYPE_MEM_AND_IO) {
#ifdef DIAGNOSTIC
		printf("shpcic: does not support memory and I/O cards, "
		    "ignored (ident=%0x)\n", ident);
#endif
		return (0);
	}
	return (1);
}

int
shpcic_vendor(struct shpcic_handle *h)
{

	return (PCIC_VENDOR_HITACHI);
}

char *
shpcic_vendor_to_string(int vendor)
{

	switch (vendor) {
	case PCIC_VENDOR_HITACHI:
		return ("Hitachi SH");
	}

	return ("Unknown controller");
}

void
shpcic_attach(struct shpcic_softc *sc)
{
	int vendor, count, i;

	/* now check for each controller/socket */

	/*
	 * this could be done with a loop, but it would violate the
	 * abstraction
	 */

	count = 0;

#if 0
	DPRINTF(("shpcic ident regs:"));
#endif

	sc->handle[0].sc = sc;
	sc->handle[0].sock = C0SA;
	sc->handle[0].flags = SHPCIC_FLAG_SOCKETP;
	sc->handle[0].laststate = SHPCIC_LASTSTATE_EMPTY;
	count++;

	sc->handle[1].sc = sc;
	sc->handle[1].sock = C0SB;
	sc->handle[1].flags = 0;
	sc->handle[1].laststate = SHPCIC_LASTSTATE_EMPTY;
	count++;

	sc->handle[2].sc = sc;
	sc->handle[2].sock = C1SA;
	sc->handle[2].flags = 0;
	sc->handle[2].laststate = SHPCIC_LASTSTATE_EMPTY;

	sc->handle[3].sc = sc;
	sc->handle[3].sock = C1SB;
	sc->handle[3].flags = 0;
	sc->handle[3].laststate = SHPCIC_LASTSTATE_EMPTY;

	if (count == 0)
		panic("shpcic_attach: attach found no sockets");

	/* establish the interrupt */

	/* XXX block interrupts? */

	for (i = 0; i < SHPCIC_NSLOTS; i++) {
		/*
		 * this should work, but w/o it, setting tty flags hangs at
		 * boot time.
		 */
		if (sc->handle[i].flags & SHPCIC_FLAG_SOCKETP)
		{
			SIMPLEQ_INIT(&sc->handle[i].events);
#if 0
			shpcic_write(&sc->handle[i], SHPCIC_CSC_INTR, 0);
			shpcic_read(&sc->handle[i], SHPCIC_CSC);
#endif
		}
	}

	if ((sc->handle[0].flags & SHPCIC_FLAG_SOCKETP) ||
	    (sc->handle[1].flags & SHPCIC_FLAG_SOCKETP)) {
		vendor = shpcic_vendor(&sc->handle[0]);

		printf("%s: controller 0 (%s) has ", sc->dev.dv_xname,
		       shpcic_vendor_to_string(vendor));

		if ((sc->handle[0].flags & SHPCIC_FLAG_SOCKETP) &&
		    (sc->handle[1].flags & SHPCIC_FLAG_SOCKETP))
			printf("sockets A and B\n");
		else if (sc->handle[0].flags & SHPCIC_FLAG_SOCKETP)
			printf("socket A only\n");
		else
			printf("socket B only\n");

		if (sc->handle[0].flags & SHPCIC_FLAG_SOCKETP)
			sc->handle[0].vendor = vendor;
		if (sc->handle[1].flags & SHPCIC_FLAG_SOCKETP)
			sc->handle[1].vendor = vendor;
	}
}

void
shpcic_attach_sockets(struct shpcic_softc *sc)
{
	int i;

	for (i = 0; i < SHPCIC_NSLOTS; i++)
		if (sc->handle[i].flags & SHPCIC_FLAG_SOCKETP)
			shpcic_attach_socket(&sc->handle[i]);
}

void
shpcic_attach_socket(struct shpcic_handle *h)
{
	struct pcmciabus_attach_args paa;

	/* initialize the rest of the handle */

	h->shutdown = 0;
	h->memalloc = 0;
	h->ioalloc = 0;
	h->ih_irq = 0;

	/* now, config one pcmcia device per socket */

	paa.paa_busname = "pcmcia";
	paa.pct = (pcmcia_chipset_tag_t) h->sc->pct;
	paa.pch = (pcmcia_chipset_handle_t) h;
	paa.iobase = h->sc->iobase;
	paa.iosize = h->sc->iosize;

	h->pcmcia = config_found_sm(&h->sc->dev, &paa, shpcic_print,
	    shpcic_submatch);

	/* if there's actually a pcmcia device attached, initialize the slot */

	if (h->pcmcia)
		shpcic_init_socket(h);
}

void
shpcic_create_event_thread(void *arg)
{
	struct shpcic_handle *h = arg;
	const char *cs;

	switch (h->sock) {
	case C0SA:
		cs = "0,0";
		break;
	case C0SB:
		cs = "0,1";
		break;
	case C1SA:
		cs = "1,0";
		break;
	case C1SB:
		cs = "1,1";
		break;
	default:
		panic("shpcic_create_event_thread: unknown pcic socket");
	}

	if (kthread_create1(shpcic_event_thread, h, &h->event_thread,
	    "%s,%s", h->sc->dev.dv_xname, cs)) {
		printf("%s: unable to create event thread for sock 0x%02x\n",
		    h->sc->dev.dv_xname, h->sock);
		panic("shpcic_create_event_thread");
	}
}

void
shpcic_event_thread(void *arg)
{
	struct shpcic_handle *h = arg;
	struct shpcic_event *pe;
	int s;

	while (h->shutdown == 0) {
		s = splhigh();
		if ((pe = SIMPLEQ_FIRST(&h->events)) == NULL) {
			splx(s);
			(void) tsleep(&h->events, PWAIT, "shpcicev", 0);
			continue;
		} else {
			splx(s);
			/* sleep .25s to be enqueued chatterling interrupts */
			(void) tsleep((caddr_t)shpcic_event_thread, PWAIT, "shpcicss", hz/4);
		}
		s = splhigh();
		SIMPLEQ_REMOVE_HEAD(&h->events, pe, pe_q);
		splx(s);

		switch (pe->pe_type) {
		case SHPCIC_EVENT_INSERTION:
			s = splhigh();
			while (1) {
				struct shpcic_event *pe1, *pe2;

				if ((pe1 = SIMPLEQ_FIRST(&h->events)) == NULL)
					break;
				if (pe1->pe_type != SHPCIC_EVENT_REMOVAL)
					break;
				if ((pe2 = SIMPLEQ_NEXT(pe1, pe_q)) == NULL)
					break;
				if (pe2->pe_type == SHPCIC_EVENT_INSERTION) {
					SIMPLEQ_REMOVE_HEAD(&h->events, pe1, pe_q);
					free(pe1, M_TEMP);
					SIMPLEQ_REMOVE_HEAD(&h->events, pe2, pe_q);
					free(pe2, M_TEMP);
				}
			}
			splx(s);

			DPRINTF(("%s: insertion event\n", h->sc->dev.dv_xname));
			shpcic_attach_card(h);
			break;

		case SHPCIC_EVENT_REMOVAL:
			s = splhigh();
			while (1) {
				struct shpcic_event *pe1, *pe2;

				if ((pe1 = SIMPLEQ_FIRST(&h->events)) == NULL)
					break;
				if (pe1->pe_type != SHPCIC_EVENT_INSERTION)
					break;
				if ((pe2 = SIMPLEQ_NEXT(pe1, pe_q)) == NULL)
					break;
				if (pe2->pe_type == SHPCIC_EVENT_REMOVAL) {
					SIMPLEQ_REMOVE_HEAD(&h->events, pe1, pe_q);
					free(pe1, M_TEMP);
					SIMPLEQ_REMOVE_HEAD(&h->events, pe2, pe_q);
					free(pe2, M_TEMP);
				}
			}
			splx(s);

			DPRINTF(("%s: removal event\n", h->sc->dev.dv_xname));
			shpcic_detach_card(h, DETACH_FORCE);
			break;

		default:
			panic("shpcic_event_thread: unknown event %d",
			    pe->pe_type);
		}
		free(pe, M_TEMP);
	}

	h->event_thread = NULL;

	/* In case parent is waiting for us to exit. */
	wakeup(h->sc);

	kthread_exit(0);
}

void
shpcic_init_socket(struct shpcic_handle *h)
{
	int reg;

	/*
	 * queue creation of a kernel thread to handle insert/removal events.
	 */
#ifdef DIAGNOSTIC
	if (h->event_thread != NULL)
		panic("shpcic_attach_socket: event thread");
#endif
	kthread_create(shpcic_create_event_thread, h);

	/* if there's a card there, then attach it. */

	reg = shpcic_read(h, SHPCIC_IF_STATUS);
	reg &= ~SHPCIC_IF_STATUS_BUSWIDTH; /* Set bus width to 16bit */

	if ((reg & SHPCIC_IF_STATUS_CARDDETECT_MASK) ==
	    SHPCIC_IF_STATUS_CARDDETECT_PRESENT) {
		int i;

		/* reset the card */
		shpcic_write(h, SHPCIC_IF_STATUS, reg|SHPCIC_IF_STATUS_RESET);
		delay(1000); /* wait 1000 uSec */
		shpcic_write(h, SHPCIC_IF_STATUS,
			     reg & ~SHPCIC_IF_STATUS_RESET);
		for (i = 0; i < 10000; i++)
			delay(1000); /* wait 1 mSec */

		shpcic_attach_card(h);
		h->laststate = SHPCIC_LASTSTATE_PRESENT;
	} else {
		h->laststate = SHPCIC_LASTSTATE_EMPTY;
	}
}

int
shpcic_submatch(struct device *parent, struct cfdata *cf, void *aux)
{

	struct pcmciabus_attach_args *paa = aux;
	struct shpcic_handle *h = (struct shpcic_handle *) paa->pch;

	switch (h->sock) {
	case C0SA:
#ifdef TODO
		if (cf->cf_loc[PCMCIABUSCF_CONTROLLER] !=
		    PCMCIABUSCF_CONTROLLER_DEFAULT &&
		    cf->cf_loc[PCMCIABUSCF_CONTROLLER] != 0)
			return 0;
		if (cf->cf_loc[PCMCIABUSCF_SOCKET] !=
		    PCMCIABUSCF_SOCKET_DEFAULT &&
		    cf->cf_loc[PCMCIABUSCF_SOCKET] != 0)
			return 0;
#endif

		break;
	case C0SB:
#ifdef TODO
		if (cf->cf_loc[PCMCIABUSCF_CONTROLLER] !=
		    PCMCIABUSCF_CONTROLLER_DEFAULT &&
		    cf->cf_loc[PCMCIABUSCF_CONTROLLER] != 0)
			return 0;
		if (cf->cf_loc[PCMCIABUSCF_SOCKET] !=
		    PCMCIABUSCF_SOCKET_DEFAULT &&
		    cf->cf_loc[PCMCIABUSCF_SOCKET] != 1)
			return 0;
#endif

		break;
	case C1SA:
		if (cf->cf_loc[PCMCIABUSCF_CONTROLLER] !=
		    PCMCIABUSCF_CONTROLLER_DEFAULT &&
		    cf->cf_loc[PCMCIABUSCF_CONTROLLER] != 1)
			return 0;
		if (cf->cf_loc[PCMCIABUSCF_SOCKET] !=
		    PCMCIABUSCF_SOCKET_DEFAULT &&
		    cf->cf_loc[PCMCIABUSCF_SOCKET] != 0)
			return 0;

		break;
	case C1SB:
		if (cf->cf_loc[PCMCIABUSCF_CONTROLLER] !=
		    PCMCIABUSCF_CONTROLLER_DEFAULT &&
		    cf->cf_loc[PCMCIABUSCF_CONTROLLER] != 1)
			return 0;
		if (cf->cf_loc[PCMCIABUSCF_SOCKET] !=
		    PCMCIABUSCF_SOCKET_DEFAULT &&
		    cf->cf_loc[PCMCIABUSCF_SOCKET] != 1)
			return 0;

		break;
	default:
		panic("unknown pcic socket");
	}

	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

int
shpcic_print(void *arg, const char *pnp)
{
	struct pcmciabus_attach_args *paa = arg;
	struct shpcic_handle *h = (struct shpcic_handle *) paa->pch;

	/* Only "pcmcia"s can attach to "pcic"s... easy. */
	if (pnp)
		printf("pcmcia at %s", pnp);

	switch (h->sock) {
	case C0SA:
		printf(" controller 0 socket 0");
		break;
	case C0SB:
		printf(" controller 0 socket 1");
		break;
	case C1SA:
		printf(" controller 1 socket 0");
		break;
	case C1SB:
		printf(" controller 1 socket 1");
		break;
	default:
		panic("unknown pcic socket");
	}

	return (UNCONF);
}

int
shpcic_intr(void *arg)
{
	struct shpcic_softc *sc = arg;
	int i, ret = 0;

	DPRINTF(("%s: intr\n", sc->dev.dv_xname));

	for (i = 0; i < SHPCIC_NSLOTS; i++)
		if (sc->handle[i].flags & SHPCIC_FLAG_SOCKETP)
			ret += shpcic_intr_socket(&sc->handle[i]);

	return (ret ? 1 : 0);
}

int
shpcic_intr_socket(struct shpcic_handle *h)
{
	int cscreg;

	cscreg = shpcic_read(h, SHPCIC_CSC);

	cscreg &= (SHPCIC_CSC_GPI |
		   SHPCIC_CSC_CD |
		   SHPCIC_CSC_READY |
		   SHPCIC_CSC_BATTWARN |
		   SHPCIC_CSC_BATTDEAD);

	if (cscreg & SHPCIC_CSC_GPI) {
		DPRINTF(("%s: %02x GPI\n", h->sc->dev.dv_xname, h->sock));
	}
	if (cscreg & SHPCIC_CSC_CD) {
		int statreg;

		statreg = shpcic_read(h, SHPCIC_IF_STATUS);

		DPRINTF(("%s: %02x CD %x\n", h->sc->dev.dv_xname, h->sock,
		    statreg));

		if ((statreg & SHPCIC_IF_STATUS_CARDDETECT_MASK) ==
		    SHPCIC_IF_STATUS_CARDDETECT_PRESENT) {
			if (h->laststate != SHPCIC_LASTSTATE_PRESENT) {
				DPRINTF(("%s: enqueing INSERTION event\n",
						 h->sc->dev.dv_xname));
				shpcic_queue_event(h, SHPCIC_EVENT_INSERTION);
			}
			h->laststate = SHPCIC_LASTSTATE_PRESENT;
		} else {
			if (h->laststate == SHPCIC_LASTSTATE_PRESENT) {
				/* Deactivate the card now. */
				DPRINTF(("%s: deactivating card\n",
						 h->sc->dev.dv_xname));
				shpcic_deactivate_card(h);

				DPRINTF(("%s: enqueing REMOVAL event\n",
						 h->sc->dev.dv_xname));
				shpcic_queue_event(h, SHPCIC_EVENT_REMOVAL);
			}
			h->laststate = ((statreg & SHPCIC_IF_STATUS_CARDDETECT_MASK) == 0)
				? SHPCIC_LASTSTATE_EMPTY : SHPCIC_LASTSTATE_HALF;
		}
	}
	if (cscreg & SHPCIC_CSC_READY) {
		DPRINTF(("%s: %02x READY\n", h->sc->dev.dv_xname, h->sock));
		/* shouldn't happen */
	}
	if (cscreg & SHPCIC_CSC_BATTWARN) {
		DPRINTF(("%s: %02x BATTWARN\n", h->sc->dev.dv_xname, h->sock));
	}
	if (cscreg & SHPCIC_CSC_BATTDEAD) {
		DPRINTF(("%s: %02x BATTDEAD\n", h->sc->dev.dv_xname, h->sock));
	}
	return (cscreg ? 1 : 0);
}

void
shpcic_queue_event(struct shpcic_handle *h, int event)
{
	struct shpcic_event *pe;
	int s;

	pe = malloc(sizeof(*pe), M_TEMP, M_NOWAIT);
	if (pe == NULL)
		panic("shpcic_queue_event: can't allocate event");

	pe->pe_type = event;
	s = splhigh();
	SIMPLEQ_INSERT_TAIL(&h->events, pe, pe_q);
	splx(s);
	wakeup(&h->events);
}

void
shpcic_attach_card(struct shpcic_handle *h)
{

	if (!(h->flags & SHPCIC_FLAG_CARDP)) {
		/* call the MI attach function */
		pcmcia_card_attach(h->pcmcia);

		h->flags |= SHPCIC_FLAG_CARDP;
	} else {
		DPRINTF(("shpcic_attach_card: already attached"));
	}
}

void
shpcic_detach_card(struct shpcic_handle *h, int flags)
{

	if (h->flags & SHPCIC_FLAG_CARDP) {
		h->flags &= ~SHPCIC_FLAG_CARDP;

		/* call the MI detach function */
		pcmcia_card_detach(h->pcmcia, flags);
	} else {
		DPRINTF(("shpcic_detach_card: already detached"));
	}
}

void
shpcic_deactivate_card(struct shpcic_handle *h)
{

	/* call the MI deactivate function */
	pcmcia_card_deactivate(h->pcmcia);

#if 0
	/* power down the socket */
	shpcic_write(h, SHPCIC_PWRCTL, 0);

	/* reset the socket */
	shpcic_write(h, SHPCIC_INTR, 0);
#endif
}

int
shpcic_chip_mem_alloc(pcmcia_chipset_handle_t pch, bus_size_t size,
    struct pcmcia_mem_handle *pcmhp)
{
	struct shpcic_handle *h = (struct shpcic_handle *) pch;
	bus_space_handle_t memh = 0;
	bus_addr_t addr;
	bus_size_t sizepg;
	int i, mask, mhandle;

	/* out of sc->memh, allocate as many pages as necessary */

	/* convert size to PCIC pages */
	sizepg = (size + (SHPCIC_MEM_ALIGN - 1)) / SHPCIC_MEM_ALIGN;
	if (sizepg > SHPCIC_MAX_MEM_PAGES)
		return (1);

	mask = (1 << sizepg) - 1;

	addr = 0;		/* XXX gcc -Wuninitialized */
	mhandle = 0;		/* XXX gcc -Wuninitialized */

	for (i = 0; i <= SHPCIC_MAX_MEM_PAGES - sizepg; i++) {
		if ((h->sc->subregionmask & (mask << i)) == (mask << i)) {
#if 0
			if (bus_space_subregion(h->sc->memt, h->sc->memh,
			    i * SHPCIC_MEM_PAGESIZE,
			    sizepg * SHPCIC_MEM_PAGESIZE, &memh))
				return (1);
#endif
			memh = h->sc->memh;
			mhandle = mask << i;
			addr = h->sc->membase + (i * SHPCIC_MEM_PAGESIZE);
			h->sc->subregionmask &= ~(mhandle);
			pcmhp->memt = h->sc->memt;
			pcmhp->memh = memh;
			pcmhp->addr = addr;
			pcmhp->size = size;
			pcmhp->mhandle = mhandle;
			pcmhp->realsize = sizepg * SHPCIC_MEM_PAGESIZE;
			return (0);
		}
	}

	return (1);
}

void
shpcic_chip_mem_free(pcmcia_chipset_handle_t pch,
    struct pcmcia_mem_handle *pcmhp)
{
	struct shpcic_handle *h = (struct shpcic_handle *) pch;

	h->sc->subregionmask |= pcmhp->mhandle;
}

int
shpcic_chip_mem_map(pcmcia_chipset_handle_t pch, int kind,
    bus_addr_t card_addr, bus_size_t size, struct pcmcia_mem_handle *pcmhp,
    bus_size_t *offsetp, int *windowp)
{
	struct shpcic_handle *h = (struct shpcic_handle *) pch;
	bus_addr_t busaddr;
	long card_offset;
	int i, win;

	win = -1;
	for (i = 0; i < SHPCIC_WINS;
	    i++) {
		if ((h->memalloc & (1 << i)) == 0) {
			win = i;
			h->memalloc |= (1 << i);
			break;
		}
	}

	if (win == -1)
		return (1);

	*windowp = win;

	/* XXX this is pretty gross */

	if (h->sc->memt != pcmhp->memt)
		panic("shpcic_chip_mem_map memt is bogus");

	busaddr = pcmhp->addr;

	/*
	 * compute the address offset to the pcmcia address space for the
	 * pcic.  this is intentionally signed.  The masks and shifts below
	 * will cause TRT to happen in the pcic registers.  Deal with making
	 * sure the address is aligned, and return the alignment offset.
	 */

	*offsetp = 0;
	card_addr -= *offsetp;

	DPRINTF(("shpcic_chip_mem_map window %d bus %lx+%lx+%lx at card addr "
	    "%lx\n", win, (u_long) busaddr, (u_long) * offsetp, (u_long) size,
	    (u_long) card_addr));

	/*
	 * include the offset in the size, and decrement size by one, since
	 * the hw wants start/stop
	 */
	size += *offsetp - 1;

	card_offset = (((long) card_addr) - ((long) busaddr));

	h->mem[win].addr = busaddr;
	h->mem[win].size = size;
	h->mem[win].offset = card_offset;
	h->mem[win].kind = kind;

	if (kind == PCMCIA_MEM_ATTR) {
		pcmhp->memh = h->sc->memh + card_addr;
	} else {
		pcmhp->memh = h->sc->memh + card_addr + SHPCIC_ATTRMEM_SIZE;
	}
#if 0
	shpcic_chip_do_mem_map(h, win);
#endif

	return (0);
}

void
shpcic_chip_mem_unmap(pcmcia_chipset_handle_t pch, int window)
{
	struct shpcic_handle *h = (struct shpcic_handle *) pch;

	if (window >= SHPCIC_WINS)
		panic("shpcic_chip_mem_unmap: window out of range");

	h->memalloc &= ~(1 << window);
}

int
shpcic_chip_io_alloc(pcmcia_chipset_handle_t pch, bus_addr_t start,
    bus_size_t size, bus_size_t align, struct pcmcia_io_handle *pcihp)
{
	struct shpcic_handle *h = (struct shpcic_handle *) pch;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_addr_t ioaddr;
	int flags = 0;

	/*
	 * Allocate some arbitrary I/O space.
	 */

	iot = h->sc->iot;

	if (start) {
		ioaddr = start;
		if (bus_space_map(iot, start, size, 0, &ioh))
			return (1);
		DPRINTF(("shpcic_chip_io_alloc map port %lx+%lx\n",
		    (u_long) ioaddr, (u_long) size));
	} else {
		flags |= PCMCIA_IO_ALLOCATED;
		if (bus_space_alloc(iot, h->sc->iobase,
		    h->sc->iobase + h->sc->iosize, size, align, 0, 0,
		    &ioaddr, &ioh))
			return (1);
		DPRINTF(("shpcic_chip_io_alloc alloc port %lx+%lx\n",
		    (u_long) ioaddr, (u_long) size));
	}

	pcihp->iot = iot;
	pcihp->ioh = ioh + h->sc->memh + SHPCIC_ATTRMEM_SIZE;;
	pcihp->addr = ioaddr;
	pcihp->size = size;
	pcihp->flags = flags;

	return (0);
}

void
shpcic_chip_io_free(pcmcia_chipset_handle_t pch,
    struct pcmcia_io_handle *pcihp)
{
	bus_space_tag_t iot = pcihp->iot;
	bus_space_handle_t ioh = pcihp->ioh;
	bus_size_t size = pcihp->size;

	if (pcihp->flags & PCMCIA_IO_ALLOCATED)
		bus_space_free(iot, ioh, size);
	else
		bus_space_unmap(iot, ioh, size);
}

int
shpcic_chip_io_map(pcmcia_chipset_handle_t pch, int width, bus_addr_t offset,
    bus_size_t size, struct pcmcia_io_handle *pcihp, int *windowp)
{
	struct shpcic_handle *h = (struct shpcic_handle *) pch;
	bus_addr_t ioaddr = pcihp->addr + offset;
	int i, win;
#ifdef SHPCICDEBUG
	static char *width_names[] = { "auto", "io8", "io16" };
#endif
	int reg;

	/* XXX Sanity check offset/size. */

#ifdef MMEYE
	/* I/O width is hardwired to 16bit mode on mmeye. */
	width = PCMCIA_WIDTH_IO16;
#endif

	win = -1;
	for (i = 0; i < SHPCIC_IOWINS; i++) {
		if ((h->ioalloc & (1 << i)) == 0) {
			win = i;
			h->ioalloc |= (1 << i);
			break;
		}
	}

	if (win == -1)
		return (1);

	*windowp = win;

	/* XXX this is pretty gross */

	if (h->sc->iot != pcihp->iot)
		panic("shpcic_chip_io_map iot is bogus");

	DPRINTF(("shpcic_chip_io_map window %d %s port %lx+%lx\n",
		 win, width_names[width], (u_long) ioaddr, (u_long) size));

	/* XXX wtf is this doing here? */

	printf(" port 0x%lx", (u_long) ioaddr);
	if (size > 1)
		printf("-0x%lx", (u_long) ioaddr + (u_long) size - 1);

	h->io[win].addr = ioaddr;
	h->io[win].size = size;
	h->io[win].width = width;

	pcihp->ioh = h->sc->memh + SHPCIC_ATTRMEM_SIZE;

	if (width == PCMCIA_WIDTH_IO8) { /* IO8 */
		reg = shpcic_read(h, SHPCIC_IF_STATUS);
		reg |= SHPCIC_IF_STATUS_BUSWIDTH; /* Set bus width to 8bit */
		shpcic_write(h, SHPCIC_IF_STATUS, reg);
	}

#if 0
	shpcic_chip_do_io_map(h, win);
#endif

	return (0);
}

void
shpcic_chip_io_unmap(pcmcia_chipset_handle_t pch, int window)
{
	struct shpcic_handle *h = (struct shpcic_handle *) pch;

	if (window >= SHPCIC_IOWINS)
		panic("shpcic_chip_io_unmap: window out of range");

	h->ioalloc &= ~(1 << window);
}

#if 0
static void
shpcic_wait_ready(struct shpcic_handle *h)
{
	int i;

	for (i = 0; i < 10000; i++) {
		if (shpcic_read(h, SHPCIC_IF_STATUS) & SHPCIC_IF_STATUS_READY)
			return;
		delay(500);
#ifdef SHPCICDEBUG
		if (shpcic_debug) {
			if ((i>5000) && (i%100 == 99))
				printf(".");
		}
#endif
	}

#ifdef DIAGNOSTIC
	printf("shpcic_wait_ready: ready never happened, status = %02x\n",
	    shpcic_read(h, SHPCIC_IF_STATUS));
#endif
}
#endif

void
shpcic_chip_socket_enable(pcmcia_chipset_handle_t pch)
{
#if 0
	struct shpcic_handle *h = (struct shpcic_handle *) pch;
	int cardtype, reg, win;

	/* this bit is mostly stolen from shpcic_attach_card */

	/* power down the socket to reset it, clear the card reset pin */

	shpcic_write(h, SHPCIC_PWRCTL, 0);

	/*
	 * wait 300ms until power fails (Tpf).  Then, wait 100ms since
	 * we are changing Vcc (Toff).
	 */
	delay((300 + 100) * 1000);

#ifdef VADEM_POWER_HACK
	bus_space_write_1(h->sc->iot, h->sc->ioh, SHPCIC_REG_INDEX, 0x0e);
	bus_space_write_1(h->sc->iot, h->sc->ioh, SHPCIC_REG_INDEX, 0x37);
	printf("prcr = %02x\n", shpcic_read(h, 0x02));
	printf("cvsr = %02x\n", shpcic_read(h, 0x2f));
	printf("DANGER WILL ROBINSON!  Changing voltage select!\n");
	shpcic_write(h, 0x2f, shpcic_read(h, 0x2f) & ~0x03);
	printf("cvsr = %02x\n", shpcic_read(h, 0x2f));
#endif

	/* power up the socket */

	shpcic_write(h, SHPCIC_PWRCTL, SHPCIC_PWRCTL_DISABLE_RESETDRV
			   | SHPCIC_PWRCTL_PWR_ENABLE);

	/*
	 * wait 100ms until power raise (Tpr) and 20ms to become
	 * stable (Tsu(Vcc)).
	 *
	 * some machines require some more time to be settled
	 * (300ms is added here).
	 */
	delay((100 + 20 + 300) * 1000);

	shpcic_write(h, SHPCIC_PWRCTL, SHPCIC_PWRCTL_DISABLE_RESETDRV | SHPCIC_PWRCTL_OE
			   | SHPCIC_PWRCTL_PWR_ENABLE);
	shpcic_write(h, SHPCIC_INTR, 0);

	/*
	 * hold RESET at least 10us.
	 */
	delay(10);

	/* clear the reset flag */

	shpcic_write(h, SHPCIC_INTR, SHPCIC_INTR_RESET);

	/* wait 20ms as per pc card standard (r2.01) section 4.3.6 */

	delay(20000);

	/* wait for the chip to finish initializing */

#ifdef DIAGNOSTIC
	reg = shpcic_read(h, SHPCIC_IF_STATUS);
	if (!(reg & SHPCIC_IF_STATUS_POWERACTIVE)) {
		printf("shpcic_chip_socket_enable: status %x", reg);
	}
#endif

	shpcic_wait_ready(h);

	/* zero out the address windows */

	shpcic_write(h, SHPCIC_ADDRWIN_ENABLE, 0);

	/* set the card type */

	cardtype = pcmcia_card_gettype(h->pcmcia);

	reg = shpcic_read(h, SHPCIC_INTR);
	reg &= ~(SHPCIC_INTR_CARDTYPE_MASK | SHPCIC_INTR_IRQ_MASK | SHPCIC_INTR_ENABLE);
	reg |= ((cardtype == PCMCIA_IFTYPE_IO) ?
		SHPCIC_INTR_CARDTYPE_IO :
		SHPCIC_INTR_CARDTYPE_MEM);
	reg |= h->ih_irq;
	shpcic_write(h, SHPCIC_INTR, reg);

	DPRINTF(("%s: shpcic_chip_socket_enable %02x cardtype %s %02x\n",
	    h->sc->dev.dv_xname, h->sock,
	    ((cardtype == PCMCIA_IFTYPE_IO) ? "io" : "mem"), reg));

	/* reinstall all the memory and io mappings */

	for (win = 0; win < SHPCIC_MEM_WINS; win++)
		if (h->memalloc & (1 << win))
			shpcic_chip_do_mem_map(h, win);

	for (win = 0; win < SHPCIC_IO_WINS; win++)
		if (h->ioalloc & (1 << win))
			shpcic_chip_do_io_map(h, win);
#endif
}

void
shpcic_chip_socket_disable(pcmcia_chipset_handle_t pch)
{
#if 0
	struct shpcic_handle *h = (struct shpcic_handle *) pch;

	DPRINTF(("shpcic_chip_socket_disable\n"));

	/* power down the socket */

	shpcic_write(h, SHPCIC_PWRCTL, 0);

	/*
	 * wait 300ms until power fails (Tpf).
	 */
	delay(300 * 1000);
#endif
}
