/*	$NetBSD: mmeyepcmcia.c,v 1.20 2011/07/19 15:17:20 dyoung Exp $	*/

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

/*
 *  PCMCIA I/F for MMEYE
 *
 *  T.Horiuichi
 *  Brains Corp. 1998.8.25
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mmeyepcmcia.c,v 1.20 2011/07/19 15:17:20 dyoung Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/kthread.h>
#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/intr.h>
#include <machine/mmeye.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciachip.h>

#include <mmeye/dev/mmeyepcmciareg.h>

#include "locators.h"

#ifdef MMEYEPCMCIADEBUG
int	mmeyepcmcia_debug = 1;
#define	DPRINTF(arg) if (mmeyepcmcia_debug) printf arg;
#else
#define	DPRINTF(arg)
#endif

struct mmeyepcmcia_event {
	SIMPLEQ_ENTRY(mmeyepcmcia_event) pe_q;
	int pe_type;
};

/* pe_type */
#define MMEYEPCMCIA_EVENT_INSERTION	0
#define MMEYEPCMCIA_EVENT_REMOVAL	1

struct mmeyepcmcia_handle {
	struct mmeyepcmcia_softc *sc;
	int	flags;
	int	laststate;
	int	memalloc;
	struct {
		bus_addr_t		addr;
		bus_size_t		size;
		int			kind;
		bus_space_tag_t		memt;
		bus_space_handle_t	memh;
	} mem[MMEYEPCMCIA_MEM_WINS];
	int	ioalloc;
	struct {
		bus_addr_t		addr;
		bus_size_t		size;
		int			width;
		bus_space_tag_t		iot;
		bus_space_handle_t	ioh;
	} io[MMEYEPCMCIA_IO_WINS];
	int	ih_irq;
	struct device *pcmcia;

	int	shutdown;
	lwp_t	*event_thread;
	SIMPLEQ_HEAD(, mmeyepcmcia_event) events;
};

#define	MMEYEPCMCIA_FLAG_CARDP		0x0002
#define	MMEYEPCMCIA_FLAG_SOCKETP	0x0001

#define MMEYEPCMCIA_LASTSTATE_PRESENT	0x0002
#define MMEYEPCMCIA_LASTSTATE_HALF	0x0001
#define MMEYEPCMCIA_LASTSTATE_EMPTY	0x0000

/*
 * This is sort of arbitrary.  It merely needs to be "enough". It can be
 * overridden in the conf file, anyway.
 */

#define	MMEYEPCMCIA_MEM_PAGES	4

#define	MMEYEPCMCIA_NSLOTS	1

#define MMEYEPCMCIA_WINS	5
#define MMEYEPCMCIA_IOWINS	2

struct mmeyepcmcia_softc {
	struct device dev;

	bus_space_tag_t iot;		/* mmeyepcmcia registers */
	bus_space_handle_t ioh;
	int controller_irq;

	bus_space_tag_t memt;		/* PCMCIA spaces */
	bus_space_handle_t memh;
	int card_irq;

	pcmcia_chipset_tag_t pct;

	/* this needs to be large enough to hold PCIC_MEM_PAGES bits */
	int	subregionmask;
#define MMEYEPCMCIA_MAX_MEM_PAGES (8 * sizeof(int))

	/*
	 * used by io/mem window mapping functions.  These can actually overlap
	 * with another pcic, since the underlying extent mapper will deal
	 * with individual allocations.  This is here to deal with the fact
	 * that different busses have different real widths (different pc
	 * hardware seems to use 10 or 12 bits for the I/O bus).
	 */
	bus_addr_t iobase;
	bus_addr_t iosize;

	struct mmeyepcmcia_handle handle[MMEYEPCMCIA_NSLOTS];
};

static void	mmeyepcmcia_attach_sockets(struct mmeyepcmcia_softc *);
static int	mmeyepcmcia_intr(void *arg);

static inline int mmeyepcmcia_read(struct mmeyepcmcia_handle *, int);
static inline void mmeyepcmcia_write(struct mmeyepcmcia_handle *, int, int);

static int	mmeyepcmcia_chip_mem_alloc(pcmcia_chipset_handle_t, bus_size_t,
		    struct pcmcia_mem_handle *);
static void	mmeyepcmcia_chip_mem_free(pcmcia_chipset_handle_t,
		    struct pcmcia_mem_handle *);
static int	mmeyepcmcia_chip_mem_map(pcmcia_chipset_handle_t, int,
		    bus_addr_t, bus_size_t, struct pcmcia_mem_handle *,
		    bus_size_t *, int *);
static void	mmeyepcmcia_chip_mem_unmap(pcmcia_chipset_handle_t, int);

static int	mmeyepcmcia_chip_io_alloc(pcmcia_chipset_handle_t, bus_addr_t,
		    bus_size_t, bus_size_t, struct pcmcia_io_handle *);
static void	mmeyepcmcia_chip_io_free(pcmcia_chipset_handle_t,
		    struct pcmcia_io_handle *);
static int	mmeyepcmcia_chip_io_map(pcmcia_chipset_handle_t, int,
		    bus_addr_t, bus_size_t, struct pcmcia_io_handle *, int *);
static void	mmeyepcmcia_chip_io_unmap(pcmcia_chipset_handle_t, int);

static void	mmeyepcmcia_chip_socket_enable(pcmcia_chipset_handle_t);
static void	mmeyepcmcia_chip_socket_disable(pcmcia_chipset_handle_t);
static void	mmeyepcmcia_chip_socket_settype(pcmcia_chipset_handle_t, int);

static inline int mmeyepcmcia_read(struct mmeyepcmcia_handle *, int);
static inline int
mmeyepcmcia_read(struct mmeyepcmcia_handle *h, int idx)
{
	static int prev_idx = 0;

	if (idx == -1){
		idx = prev_idx;
	}
	prev_idx = idx;
	return bus_space_read_stream_2(h->sc->iot, h->sc->ioh, idx);
}

static inline void mmeyepcmcia_write(struct mmeyepcmcia_handle *, int, int);
static inline void
mmeyepcmcia_write(struct mmeyepcmcia_handle *h, int idx, int data)
{
	static int prev_idx;
	if (idx == -1){
		idx = prev_idx;
	}
	prev_idx = idx;
	bus_space_write_stream_2(h->sc->iot, h->sc->ioh, idx, (data));
}

static void	*mmeyepcmcia_chip_intr_establish(pcmcia_chipset_handle_t,
		    struct pcmcia_function *, int, int (*) (void *), void *);
static void	mmeyepcmcia_chip_intr_disestablish(pcmcia_chipset_handle_t,
		    void *);
static void	*mmeyepcmcia_chip_intr_establish(pcmcia_chipset_handle_t,
		    struct pcmcia_function *, int, int (*) (void *), void *);
static void	mmeyepcmcia_chip_intr_disestablish(pcmcia_chipset_handle_t,
		    void *);

static void	mmeyepcmcia_attach_socket(struct mmeyepcmcia_handle *);
static void	mmeyepcmcia_init_socket(struct mmeyepcmcia_handle *);
static int	mmeyepcmcia_print (void *, const char *);
static int	mmeyepcmcia_intr_socket(struct mmeyepcmcia_handle *);
static void	mmeyepcmcia_attach_card(struct mmeyepcmcia_handle *);
static void	mmeyepcmcia_detach_card(struct mmeyepcmcia_handle *, int);
static void	mmeyepcmcia_deactivate_card(struct mmeyepcmcia_handle *);
static void	mmeyepcmcia_event_thread(void *);
static void	mmeyepcmcia_queue_event(struct mmeyepcmcia_handle *, int);

static int	mmeyepcmcia_match(struct device *, struct cfdata *, void *);
static void	mmeyepcmcia_attach(struct device *, struct device *, void *);

CFATTACH_DECL(mmeyepcmcia, sizeof(struct mmeyepcmcia_softc),
    mmeyepcmcia_match, mmeyepcmcia_attach, NULL, NULL);

static struct pcmcia_chip_functions mmeyepcmcia_functions = {
	mmeyepcmcia_chip_mem_alloc,
	mmeyepcmcia_chip_mem_free,
	mmeyepcmcia_chip_mem_map,
	mmeyepcmcia_chip_mem_unmap,

	mmeyepcmcia_chip_io_alloc,
	mmeyepcmcia_chip_io_free,
	mmeyepcmcia_chip_io_map,
	mmeyepcmcia_chip_io_unmap,

	mmeyepcmcia_chip_intr_establish,
	mmeyepcmcia_chip_intr_disestablish,

	mmeyepcmcia_chip_socket_enable,
	mmeyepcmcia_chip_socket_disable,
	mmeyepcmcia_chip_socket_settype,
	NULL,
};

static int
mmeyepcmcia_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, match->cf_name) != 0)
		return 0;

	/* Disallow wildcarded values. */
	if (ma->ma_addr1 == MAINBUSCF_ADDR1_DEFAULT ||
	    ma->ma_addr2 == MAINBUSCF_ADDR2_DEFAULT ||
	    ma->ma_irq2 == MAINBUSCF_IRQ2_DEFAULT)
		return 0;

	return 1;
}

static void
mmeyepcmcia_attach(struct device *parent, struct device *self, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	struct mmeyepcmcia_softc *sc = (void *)self;

	aprint_naive("\n");

	sc->subregionmask = 1;	/* 1999.05.17 T.Horiuchi for R1.4 */

	sc->pct = (pcmcia_chipset_tag_t)&mmeyepcmcia_functions;
	sc->iot = 0;
	/* Map i/o space. */
	if (bus_space_map(sc->iot, ma->ma_addr1, MMEYEPCMCIA_IOSIZE,
	    0, &sc->ioh)) {
		aprint_error(": can't map i/o space\n");
		return;
	}
	sc->memt = 0;
	sc->iobase = ma->ma_addr2;
	sc->controller_irq = ma->ma_irq1;
	sc->card_irq = ma->ma_irq2;

	sc->handle[0].sc = sc;
	sc->handle[0].flags = MMEYEPCMCIA_FLAG_SOCKETP;
	sc->handle[0].laststate = MMEYEPCMCIA_LASTSTATE_EMPTY;

	SIMPLEQ_INIT(&sc->handle[0].events);

	if (sc->controller_irq != MAINBUSCF_IRQ1_DEFAULT) {
		aprint_normal(": using MMTA irq %d\n", sc->controller_irq);
		mmeye_intr_establish(sc->controller_irq,
		    IST_LEVEL, IPL_TTY, mmeyepcmcia_intr, sc);
	} else
		aprint_normal("\n");

	mmeyepcmcia_attach_sockets(sc);
}

static void *
mmeyepcmcia_chip_intr_establish(pcmcia_chipset_handle_t pch,
    struct pcmcia_function *pf, int ipl, int (*fct)(void *), void *arg)
{
	struct mmeyepcmcia_handle *h = (struct mmeyepcmcia_handle *) pch;
	int irq = h->sc->card_irq;
	void *ih;

	ih = mmeye_intr_establish(irq, IST_LEVEL, ipl, fct, arg);
	h->ih_irq = irq;

	printf("%s: card irq %d\n", h->pcmcia->dv_xname, irq);

	return ih;
}

static void
mmeyepcmcia_chip_intr_disestablish(pcmcia_chipset_handle_t pch, void *ih)
{
	struct mmeyepcmcia_handle *h = (struct mmeyepcmcia_handle *) pch;

	h->ih_irq = 0;
	mmeye_intr_disestablish(ih);
}


static void
mmeyepcmcia_attach_sockets(struct mmeyepcmcia_softc *sc)
{

	mmeyepcmcia_attach_socket(&sc->handle[0]);
}

static void
mmeyepcmcia_attach_socket(struct mmeyepcmcia_handle *h)
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

	h->pcmcia = config_found_ia(&h->sc->dev, "pcmciabus", &paa,
				    mmeyepcmcia_print);

	/* if there's actually a pcmcia device attached, initialize the slot */

	if (h->pcmcia)
		mmeyepcmcia_init_socket(h);
}

static void
mmeyepcmcia_event_thread(void *arg)
{
	struct mmeyepcmcia_handle *h = arg;
	struct mmeyepcmcia_event *pe;
	int s;

	while (h->shutdown == 0) {
		s = splhigh();
		if ((pe = SIMPLEQ_FIRST(&h->events)) == NULL) {
			splx(s);
			(void) tsleep(&h->events, PWAIT, "mmeyepcmciaev", 0);
			continue;
		} else {
			splx(s);
			/* sleep .25s to be enqueued chatterling interrupts */
			(void) tsleep((void *)mmeyepcmcia_event_thread, PWAIT,
			    "mmeyepcmciass", hz/4);
		}
		s = splhigh();
		SIMPLEQ_REMOVE_HEAD(&h->events, pe_q);
		splx(s);

		switch (pe->pe_type) {
		case MMEYEPCMCIA_EVENT_INSERTION:
			s = splhigh();
			while (1) {
				struct mmeyepcmcia_event *pe1, *pe2;

				if ((pe1 = SIMPLEQ_FIRST(&h->events)) == NULL)
					break;
				if (pe1->pe_type != MMEYEPCMCIA_EVENT_REMOVAL)
					break;
				if ((pe2 = SIMPLEQ_NEXT(pe1, pe_q)) == NULL)
					break;
				if (pe2->pe_type == MMEYEPCMCIA_EVENT_INSERTION) {
					SIMPLEQ_REMOVE_HEAD(&h->events, pe_q);
					free(pe1, M_TEMP);
					SIMPLEQ_REMOVE_HEAD(&h->events, pe_q);
					free(pe2, M_TEMP);
				}
			}
			splx(s);

			DPRINTF(("%s: insertion event\n", h->sc->dev.dv_xname));
			mmeyepcmcia_attach_card(h);
			break;

		case MMEYEPCMCIA_EVENT_REMOVAL:
			s = splhigh();
			while (1) {
				struct mmeyepcmcia_event *pe1, *pe2;

				if ((pe1 = SIMPLEQ_FIRST(&h->events)) == NULL)
					break;
				if (pe1->pe_type != MMEYEPCMCIA_EVENT_INSERTION)
					break;
				if ((pe2 = SIMPLEQ_NEXT(pe1, pe_q)) == NULL)
					break;
				if (pe2->pe_type == MMEYEPCMCIA_EVENT_REMOVAL) {
					SIMPLEQ_REMOVE_HEAD(&h->events, pe_q);
					free(pe1, M_TEMP);
					SIMPLEQ_REMOVE_HEAD(&h->events, pe_q);
					free(pe2, M_TEMP);
				}
			}
			splx(s);

			DPRINTF(("%s: removal event\n", h->sc->dev.dv_xname));
			mmeyepcmcia_detach_card(h, DETACH_FORCE);
			break;

		default:
			panic("mmeyepcmcia_event_thread: unknown event %d",
			    pe->pe_type);
		}
		free(pe, M_TEMP);
	}

	h->event_thread = NULL;

	/* In case parent is waiting for us to exit. */
	wakeup(h->sc);

	kthread_exit(0);
}

static void
mmeyepcmcia_init_socket(struct mmeyepcmcia_handle *h)
{
	int reg;

	/*
	 * queue creation of a kernel thread to handle insert/removal events.
	 */
#ifdef DIAGNOSTIC
	if (h->event_thread != NULL)
		panic("mmeyepcmcia_attach_socket: event thread");
#endif

	/* if there's a card there, then attach it. */

	reg = mmeyepcmcia_read(h, MMEYEPCMCIA_IF_STATUS);
	reg &= ~MMEYEPCMCIA_IF_STATUS_BUSWIDTH; /* Set bus width to 16bit */

	if ((reg & MMEYEPCMCIA_IF_STATUS_CARDDETECT_MASK) ==
	    MMEYEPCMCIA_IF_STATUS_CARDDETECT_PRESENT) {
		int i;

		/* reset the card */
		mmeyepcmcia_write(h, MMEYEPCMCIA_IF_STATUS, reg|MMEYEPCMCIA_IF_STATUS_RESET);
		delay(1000); /* wait 1000 uSec */
		mmeyepcmcia_write(h, MMEYEPCMCIA_IF_STATUS,
			     reg & ~MMEYEPCMCIA_IF_STATUS_RESET);
		for (i = 0; i < 10000; i++)
			delay(1000); /* wait 1 mSec */

		mmeyepcmcia_attach_card(h);
		h->laststate = MMEYEPCMCIA_LASTSTATE_PRESENT;
	} else {
		h->laststate = MMEYEPCMCIA_LASTSTATE_EMPTY;
	}

	if (kthread_create(PRI_NONE, 0, NULL, mmeyepcmcia_event_thread, h,
	    &h->event_thread, "%s", h->sc->dev.dv_xname)) {
		printf("%s: unable to create event thread\n",
		    h->sc->dev.dv_xname);
		panic("mmeyepcmcia_create_event_thread");
	}
}

static int
mmeyepcmcia_print(void *arg, const char *pnp)
{

	if (pnp)
		aprint_normal("pcmcia at %s", pnp);

	return UNCONF;
}

static int
mmeyepcmcia_intr(void *arg)
{
	struct mmeyepcmcia_softc *sc = arg;

	DPRINTF(("%s: intr\n", sc->dev.dv_xname));

	mmeyepcmcia_intr_socket(&sc->handle[0]);

	return 0;
}

static int
mmeyepcmcia_intr_socket(struct mmeyepcmcia_handle *h)
{
	int cscreg;

	cscreg = mmeyepcmcia_read(h, MMEYEPCMCIA_CSC);

	cscreg &= (MMEYEPCMCIA_CSC_GPI |
		   MMEYEPCMCIA_CSC_CD |
		   MMEYEPCMCIA_CSC_READY |
		   MMEYEPCMCIA_CSC_BATTWARN |
		   MMEYEPCMCIA_CSC_BATTDEAD);

	if (cscreg & MMEYEPCMCIA_CSC_GPI) {
		DPRINTF(("%s: %02x GPI\n", h->sc->dev.dv_xname, h->sock));
	}
	if (cscreg & MMEYEPCMCIA_CSC_CD) {
		int statreg;

		statreg = mmeyepcmcia_read(h, MMEYEPCMCIA_IF_STATUS);

		DPRINTF(("%s: %02x CD %x\n", h->sc->dev.dv_xname, h->sock,
		    statreg));

		if ((statreg & MMEYEPCMCIA_IF_STATUS_CARDDETECT_MASK) ==
		    MMEYEPCMCIA_IF_STATUS_CARDDETECT_PRESENT) {
			if (h->laststate != MMEYEPCMCIA_LASTSTATE_PRESENT) {
				DPRINTF(("%s: enqueing INSERTION event\n",
						 h->sc->dev.dv_xname));
				mmeyepcmcia_queue_event(h, MMEYEPCMCIA_EVENT_INSERTION);
			}
			h->laststate = MMEYEPCMCIA_LASTSTATE_PRESENT;
		} else {
			if (h->laststate == MMEYEPCMCIA_LASTSTATE_PRESENT) {
				/* Deactivate the card now. */
				DPRINTF(("%s: deactivating card\n",
						 h->sc->dev.dv_xname));
				mmeyepcmcia_deactivate_card(h);

				DPRINTF(("%s: enqueing REMOVAL event\n",
						 h->sc->dev.dv_xname));
				mmeyepcmcia_queue_event(h, MMEYEPCMCIA_EVENT_REMOVAL);
			}
			h->laststate = ((statreg & MMEYEPCMCIA_IF_STATUS_CARDDETECT_MASK) == 0)
				? MMEYEPCMCIA_LASTSTATE_EMPTY : MMEYEPCMCIA_LASTSTATE_HALF;
		}
	}
	if (cscreg & MMEYEPCMCIA_CSC_READY) {
		DPRINTF(("%s: %02x READY\n", h->sc->dev.dv_xname, h->sock));
		/* shouldn't happen */
	}
	if (cscreg & MMEYEPCMCIA_CSC_BATTWARN) {
		DPRINTF(("%s: %02x BATTWARN\n", h->sc->dev.dv_xname, h->sock));
	}
	if (cscreg & MMEYEPCMCIA_CSC_BATTDEAD) {
		DPRINTF(("%s: %02x BATTDEAD\n", h->sc->dev.dv_xname, h->sock));
	}
	return cscreg ? 1 : 0;
}

static void
mmeyepcmcia_queue_event(struct mmeyepcmcia_handle *h, int event)
{
	struct mmeyepcmcia_event *pe;
	int s;

	pe = malloc(sizeof(*pe), M_TEMP, M_NOWAIT);
	if (pe == NULL)
		panic("mmeyepcmcia_queue_event: can't allocate event");

	pe->pe_type = event;
	s = splhigh();
	SIMPLEQ_INSERT_TAIL(&h->events, pe, pe_q);
	splx(s);
	wakeup(&h->events);
}

static void
mmeyepcmcia_attach_card(struct mmeyepcmcia_handle *h)
{

	if (!(h->flags & MMEYEPCMCIA_FLAG_CARDP)) {
		/* call the MI attach function */
		pcmcia_card_attach(h->pcmcia);

		h->flags |= MMEYEPCMCIA_FLAG_CARDP;
	} else {
		DPRINTF(("mmeyepcmcia_attach_card: already attached"));
	}
}

static void
mmeyepcmcia_detach_card(struct mmeyepcmcia_handle *h, int flags)
{

	if (h->flags & MMEYEPCMCIA_FLAG_CARDP) {
		h->flags &= ~MMEYEPCMCIA_FLAG_CARDP;

		/* call the MI detach function */
		pcmcia_card_detach(h->pcmcia, flags);
	} else {
		DPRINTF(("mmeyepcmcia_detach_card: already detached"));
	}
}

static void
mmeyepcmcia_deactivate_card(struct mmeyepcmcia_handle *h)
{

	/* call the MI deactivate function */
	pcmcia_card_deactivate(h->pcmcia);

	/* Power down and reset XXX notyet */
}

static int
mmeyepcmcia_chip_mem_alloc(pcmcia_chipset_handle_t pch, bus_size_t size,
    struct pcmcia_mem_handle *pcmhp)
{
	struct mmeyepcmcia_handle *h = (struct mmeyepcmcia_handle *) pch;
	bus_addr_t addr;
	bus_size_t sizepg;
	int i, mask, mhandle;

	/* out of sc->memh, allocate as many pages as necessary */
#define	MMEYEPCMCIA_MEM_ALIGN	MMEYEPCMCIA_MEM_PAGESIZE
	/* convert size to PCIC pages */
	sizepg = (size + (MMEYEPCMCIA_MEM_ALIGN - 1)) / MMEYEPCMCIA_MEM_ALIGN;
	if (sizepg > MMEYEPCMCIA_MAX_MEM_PAGES)
		return 1;

	mask = (1 << sizepg) - 1;

	addr = 0;		/* XXX gcc -Wuninitialized */
	mhandle = 0;		/* XXX gcc -Wuninitialized */

	for (i = 0; i <= MMEYEPCMCIA_MAX_MEM_PAGES - sizepg; i++) {
		if ((h->sc->subregionmask & (mask << i)) == (mask << i)) {
			mhandle = mask << i;
			addr = h->sc->iobase + (i * MMEYEPCMCIA_MEM_PAGESIZE);
			h->sc->subregionmask &= ~(mhandle);
			pcmhp->memt = h->sc->memt;
			pcmhp->memh = 0;
			pcmhp->addr = addr;
			pcmhp->size = size;
			pcmhp->mhandle = mhandle;
			pcmhp->realsize = sizepg * MMEYEPCMCIA_MEM_PAGESIZE;
			return 0;
		}
	}

	return 1;
}

static void
mmeyepcmcia_chip_mem_free(pcmcia_chipset_handle_t pch,
    struct pcmcia_mem_handle *pcmhp)
{
	struct mmeyepcmcia_handle *h = (struct mmeyepcmcia_handle *) pch;

	h->sc->subregionmask |= pcmhp->mhandle;
}

static int
mmeyepcmcia_chip_mem_map(pcmcia_chipset_handle_t pch, int kind,
    bus_addr_t card_addr, bus_size_t size, struct pcmcia_mem_handle *pcmhp,
    bus_size_t *offsetp, int *windowp)
{
	struct mmeyepcmcia_handle *h = (struct mmeyepcmcia_handle *) pch;
	bus_addr_t busaddr;
	int i, win;

	win = -1;
	for (i = 0; i < MMEYEPCMCIA_WINS; i++) {
		if ((h->memalloc & (1 << i)) == 0) {
			win = i;
			h->memalloc |= (1 << i);
			break;
		}
	}

	if (win == -1)
		return 1;

	*windowp = win;

	/* XXX this is pretty gross */

	busaddr = pcmhp->addr + card_addr;

#if defined(SH7750R)
	switch (kind) {
	case PCMCIA_MEM_ATTR:
	case PCMCIA_MEM_ATTR | PCMCIA_WIDTH_MEM16:
		pcmhp->memt = SH3_BUS_SPACE_PCMCIA_ATT;
		break;

	case PCMCIA_MEM_ATTR | PCMCIA_WIDTH_MEM8:
		pcmhp->memt = SH3_BUS_SPACE_PCMCIA_ATT8;
		break;

	case PCMCIA_MEM_COMMON:
	case PCMCIA_MEM_COMMON | PCMCIA_WIDTH_MEM16:
		pcmhp->memt = SH3_BUS_SPACE_PCMCIA_MEM;
		break;

	case PCMCIA_MEM_COMMON | PCMCIA_WIDTH_MEM8:
		pcmhp->memt = SH3_BUS_SPACE_PCMCIA_MEM8;
		break;

	default:
		panic("mmeyepcmcia_chip_mem_map kind is bogus: 0x%x", kind);
	}
#else
	if (!bus_space_is_equal(h->sc->memt, pcmhp->memt))
		panic("mmeyepcmcia_chip_mem_map memt is bogus");
	if (kind != PCMCIA_MEM_ATTR)
		busaddr += MMEYEPCMCIA_ATTRMEM_SIZE;
#endif
	if (bus_space_map(pcmhp->memt, busaddr, pcmhp->size, 0, &pcmhp->memh))
		return 1;

	/*
	 * compute the address offset to the pcmcia address space for the
	 * pcic.  this is intentionally signed.  The masks and shifts below
	 * will cause TRT to happen in the pcic registers.  Deal with making
	 * sure the address is aligned, and return the alignment offset.
	 */

	*offsetp = 0;

	DPRINTF(("mmeyepcmcia_chip_mem_map window %d bus %lx+%lx+%lx at card addr "
	    "%lx\n", win, (u_long) busaddr, (u_long) * offsetp, (u_long) size,
	    (u_long) card_addr));

	/*
	 * include the offset in the size, and decrement size by one, since
	 * the hw wants start/stop
	 */
	h->mem[win].addr = busaddr;
	h->mem[win].size = size - 1;
	h->mem[win].kind = kind;
	h->mem[win].memt = pcmhp->memt;
	h->mem[win].memh = pcmhp->memh;

	return 0;
}

static void
mmeyepcmcia_chip_mem_unmap(pcmcia_chipset_handle_t pch, int window)
{
	struct mmeyepcmcia_handle *h = (struct mmeyepcmcia_handle *) pch;

	if (window >= MMEYEPCMCIA_WINS)
		panic("mmeyepcmcia_chip_mem_unmap: window out of range");

	h->memalloc &= ~(1 << window);

	bus_space_unmap(h->mem[window].memt, h->mem[window].memh,
	    h->mem[window].size);
}

static int
mmeyepcmcia_chip_io_alloc(pcmcia_chipset_handle_t pch, bus_addr_t start,
    bus_size_t size, bus_size_t align, struct pcmcia_io_handle *pcihp)
{
	struct mmeyepcmcia_handle *h = (struct mmeyepcmcia_handle *) pch;

	/*
	 * Allocate some arbitrary I/O space.
	 */

	DPRINTF(("mmeyepcmcia_chip_io_alloc alloc port %lx+%lx\n",
	    (u_long) ioaddr, (u_long) size));

	pcihp->iot = h->sc->memt;
	pcihp->ioh = 0;
	pcihp->addr = h->sc->iobase + start;
	pcihp->size = size;

	return 0;
}

static void
mmeyepcmcia_chip_io_free(pcmcia_chipset_handle_t pch,
    struct pcmcia_io_handle *pcihp)
{
}

static int
mmeyepcmcia_chip_io_map(pcmcia_chipset_handle_t pch, int width,
    bus_addr_t offset, bus_size_t size, struct pcmcia_io_handle *pcihp,
    int *windowp)
{
	struct mmeyepcmcia_handle *h = (struct mmeyepcmcia_handle *) pch;
	bus_addr_t busaddr;
	int i, win;
#ifdef MMEYEPCMCIADEBUG
	static char *width_names[] = { "auto", "io8", "io16" };
#endif

	/* I/O width is hardwired to 16bit mode on mmeye. */
	width = PCMCIA_WIDTH_IO16;

	win = -1;
	for (i = 0; i < MMEYEPCMCIA_IOWINS; i++) {
		if ((h->ioalloc & (1 << i)) == 0) {
			win = i;
			h->ioalloc |= (1 << i);
			break;
		}
	}

	if (win == -1)
		return 1;

	*windowp = win;

	/* XXX this is pretty gross */

	busaddr = pcihp->addr + offset;

#if defined(SH7750R)
	switch (width) {
	case PCMCIA_WIDTH_IO8:
		pcihp->iot = SH3_BUS_SPACE_PCMCIA_IO8;
		break;

	case PCMCIA_WIDTH_IO16:
		pcihp->iot = SH3_BUS_SPACE_PCMCIA_IO;
		break;

	case PCMCIA_WIDTH_AUTO:
	default:
		panic("mmeyepcmcia_chip_io_map width is bogus: 0x%x", width);
	}
#else
	if (!bus_space_is_equal(h->sc->iot, pcihp->iot))
		panic("mmeyepcmcia_chip_io_map iot is bogus");
	busaddr += MMEYEPCMCIA_ATTRMEM_SIZE;
#endif
	if (bus_space_map(pcihp->iot, busaddr, pcihp->size, 0, &pcihp->ioh))
		return 1;

	DPRINTF(("mmeyepcmcia_chip_io_map window %d %s port %lx+%lx\n",
		 win, width_names[width], (u_long) offset, (u_long) size));

	/* XXX wtf is this doing here? */

	h->io[win].addr = busaddr;
	h->io[win].size = size;
	h->io[win].width = width;
	h->io[win].iot = pcihp->iot;
	h->io[win].ioh = pcihp->ioh;

	return 0;
}

static void
mmeyepcmcia_chip_io_unmap(pcmcia_chipset_handle_t pch, int window)
{
	struct mmeyepcmcia_handle *h = (struct mmeyepcmcia_handle *) pch;

	if (window >= MMEYEPCMCIA_IOWINS)
		panic("mmeyepcmcia_chip_io_unmap: window out of range");

	h->ioalloc &= ~(1 << window);

	bus_space_unmap(h->io[window].iot, h->io[window].ioh,
	    h->io[window].size);
}

static void
mmeyepcmcia_chip_socket_enable(pcmcia_chipset_handle_t pch)
{
}

static void
mmeyepcmcia_chip_socket_disable(pcmcia_chipset_handle_t pch)
{
}

static void
mmeyepcmcia_chip_socket_settype(pcmcia_chipset_handle_t pch, int type)
{
}
