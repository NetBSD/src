/*	$NetBSD: shpcmcia.c,v 1.2 2011/07/26 22:52:48 dyoung Exp $	*/

/*-
 * Copyright (c) 2009 NONAKA Kimihiro <nonaka@netbsd.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: shpcmcia.c,v 1.2 2011/07/26 22:52:48 dyoung Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kthread.h>
#include <sys/kernel.h>
#include <sys/callout.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <dev/pcmcia/pcmciachip.h>
#include <dev/pcmcia/pcmciavar.h>

#include <machine/autoconf.h>

#include <sh3/devreg.h>
#include <sh3/bscreg.h>
#include <sh3/pfcreg.h>

#include <evbsh3/ap_ms104_sh4/ap_ms104_sh4reg.h>
#include <evbsh3/ap_ms104_sh4/ap_ms104_sh4var.h>

#ifdef SHPCMCIA_DEBUG
#define DPRINTF(s)	printf s
#else
#define DPRINTF(s)
#endif

static int	shpcmcia_chip_mem_alloc(pcmcia_chipset_handle_t,
		    bus_size_t, struct pcmcia_mem_handle *);
static void	shpcmcia_chip_mem_free(pcmcia_chipset_handle_t,
		    struct pcmcia_mem_handle *);
static int	shpcmcia_chip_mem_map(pcmcia_chipset_handle_t, int,
		    bus_addr_t, bus_size_t, struct pcmcia_mem_handle *,
		    bus_size_t *, int *);
static void	shpcmcia_chip_mem_unmap(pcmcia_chipset_handle_t, int);
static int	shpcmcia_chip_io_alloc(pcmcia_chipset_handle_t,
		    bus_addr_t, bus_size_t, bus_size_t,
		    struct pcmcia_io_handle *);
static void	shpcmcia_chip_io_free(pcmcia_chipset_handle_t,
		    struct pcmcia_io_handle *);
static int	shpcmcia_chip_io_map(pcmcia_chipset_handle_t, int,
		    bus_addr_t, bus_size_t, struct pcmcia_io_handle *, int *);
static void	shpcmcia_chip_io_unmap(pcmcia_chipset_handle_t, int);
static void	*shpcmcia_chip_intr_establish(pcmcia_chipset_handle_t,
		    struct pcmcia_function *, int, int (*)(void *), void *);
static void	shpcmcia_chip_intr_disestablish(pcmcia_chipset_handle_t,
		    void *);
static void	shpcmcia_chip_socket_enable(pcmcia_chipset_handle_t);
static void	shpcmcia_chip_socket_disable(pcmcia_chipset_handle_t);
static void	shpcmcia_chip_socket_settype(pcmcia_chipset_handle_t,
		    int);

static struct pcmcia_chip_functions shpcmcia_chip_functions = {
	/* memory space allocation */
	.mem_alloc		= shpcmcia_chip_mem_alloc,
	.mem_free		= shpcmcia_chip_mem_free,

	/* memory space window mapping */
	.mem_map		= shpcmcia_chip_mem_map,
	.mem_unmap		= shpcmcia_chip_mem_unmap,

	/* I/O space allocation */
	.io_alloc		= shpcmcia_chip_io_alloc,
	.io_free		= shpcmcia_chip_io_free,

	/* I/O space window mapping */
	.io_map			= shpcmcia_chip_io_map,
	.io_unmap		= shpcmcia_chip_io_unmap,

	/* interrupt glue */
	.intr_establish		= shpcmcia_chip_intr_establish,
	.intr_disestablish	= shpcmcia_chip_intr_disestablish,

	/* card enable/disable */
	.socket_enable		= shpcmcia_chip_socket_enable,
	.socket_disable		= shpcmcia_chip_socket_disable,
	.socket_settype		= shpcmcia_chip_socket_settype,

	/* card detection */
	.card_detect		= NULL,
};

/*
 * event thread
 */
struct shpcmcia_event {
	SIMPLEQ_ENTRY(shpcmcia_event) pe_q;
	int pe_type;
};

/* pe_type */
#define SHPCMCIA_EVENT_INSERT	0
#define SHPCMCIA_EVENT_REMOVE	1

struct shpcmcia_softc;
struct shpcmcia_handle {
	struct shpcmcia_softc *sc;

	int	flags;
#define	SHPCMCIA_FLAG_SOCKETP		0x0001
#define	SHPCMCIA_FLAG_CARDP		0x0002
	int	laststate;
#define SHPCMCIA_LASTSTATE_EMPTY	0x0000
#define SHPCMCIA_LASTSTATE_PRESENT	0x0002


	int	memalloc;
	struct {
		bus_addr_t	addr;
		bus_size_t	size;
		long		offset;
		int		kind;
#define	SHPCMCIA_MEM_WINS		5
	} mem[SHPCMCIA_MEM_WINS];

	int	ioalloc;
	struct {
		bus_addr_t	addr;
		bus_size_t	size;
		int		width;
#define	SHPCMCIA_IO_WINS		2
	} io[SHPCMCIA_IO_WINS];

	struct device *pcmcia;

	int	shutdown;
	lwp_t	*event_thread;
	SIMPLEQ_HEAD(, shpcmcia_event) events;
};

struct shpcmcia_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_memh;
	bus_space_tag_t sc_attt;
	bus_space_handle_t sc_atth;

	pcmcia_chipset_tag_t sc_pct;

	void *sc_ih;
#if 0
	void *sc_detect_ih;
#else
	callout_t sc_detect_ch;
#endif

	bus_addr_t sc_membase;
#define SHPCMCIA_MAX_MEM_PAGES (8 * sizeof(int))

	bus_addr_t sc_iobase;
	bus_size_t sc_iosize;

#define	SHPCMCIA_NSLOTS	1
	struct shpcmcia_handle sc_handle[SHPCMCIA_NSLOTS];
};

static int	shpcmcia_probe(device_t, cfdata_t, void *);
static void	shpcmcia_attach(device_t, device_t, void *);
static int shpcmcia_print(void *, const char *);

CFATTACH_DECL_NEW(shpcmcia, sizeof(struct shpcmcia_softc),
    shpcmcia_probe, shpcmcia_attach, NULL, NULL);

#if 0
static int shpcmcia_card_detect_intr(void *arg);
#else
static void shpcmcia_card_detect_poll(void *arg);
#endif

static void shpcmcia_init_socket(struct shpcmcia_handle *);
static void shpcmcia_attach_socket(struct shpcmcia_handle *);
static void shpcmcia_attach_sockets(struct shpcmcia_softc *);

static void shpcmcia_event_thread(void *);
static void shpcmcia_queue_event(struct shpcmcia_handle *, int);

static void shpcmcia_attach_card(struct shpcmcia_handle *);
static void shpcmcia_detach_card(struct shpcmcia_handle *, int );
static void shpcmcia_deactivate_card(struct shpcmcia_handle *);

static int
shpcmcia_probe(device_t parent, cfdata_t cfp, void *aux)
{
	struct mainbus_attach_args *maa = aux;

	if (strcmp(maa->ma_name, "shpcmcia") != 0)
		return 0;
	return 1;
}

static void
shpcmcia_attach(device_t parent, device_t self, void *aux)
{
	struct shpcmcia_softc *sc = device_private(self);
#if 0
	uint32_t reg;
#endif

	sc->sc_dev = self;

	aprint_naive("\n");
	aprint_normal("\n");

#if 0
	/* setup bus controller */
	/* max wait */
	reg = _reg_read_4(SH4_WCR1);
	reg |= 0x00700000;
	_reg_write_4(SH4_WCR1, reg);
	reg = _reg_read_4(SH4_WCR2);
	reg |= 0xfff00000;
	_reg_write_4(SH4_WCR2, reg);
	reg = _reg_read_4(SH4_WCR3);
	reg |= 0x07700000;
	_reg_write_4(SH4_WCR3, reg);
	reg = _reg_read_4(SH4_PCR);
	reg |= 0xffffffff;
	_reg_write_4(SH4_PCR, reg);
#endif

	sc->sc_pct = (pcmcia_chipset_tag_t)&shpcmcia_chip_functions;
	sc->sc_iot = &ap_ms104_sh4_bus_io;
	sc->sc_memt = &ap_ms104_sh4_bus_mem;
	sc->sc_attt = &ap_ms104_sh4_bus_att;

	if (bus_space_map(sc->sc_attt, 0x14000000, 4 * 1024, 0, &sc->sc_atth))
		panic("%s: couldn't map attribute\n", device_xname(sc->sc_dev));
	if (bus_space_map(sc->sc_iot, 0x15000000, 64 * 1024, 0,
	    &sc->sc_ioh))
		panic("%s: couldn't map io memory\n", device_xname(sc->sc_dev));
	if (bus_space_map(sc->sc_memt, 0x16000000, 32 * 1024 * 1024, 0,
	    &sc->sc_memh))
		panic("%s: couldn't map memory\n", device_xname(sc->sc_dev));

	sc->sc_iobase = sc->sc_ioh;
	sc->sc_iosize = 64 * 1024;
	sc->sc_membase = sc->sc_memh;

	sc->sc_handle[0].sc = sc;
	sc->sc_handle[0].flags = SHPCMCIA_FLAG_SOCKETP;
	sc->sc_handle[0].laststate = SHPCMCIA_LASTSTATE_EMPTY;
	SIMPLEQ_INIT(&sc->sc_handle[0].events);

#if 0
	sc->sc_detect_ih = gpio_intr_establish(GPIO_PIN_CARD_CD,
	    shpcmcia_card_detect_intr, sc);
	if (sc->sc_detect_ih == NULL) {
		aprint_error_dev(self, "couldn't establish detect interrupt\n");
	}
#else
	callout_init(&sc->sc_detect_ch, 0);
	callout_reset(&sc->sc_detect_ch, hz, shpcmcia_card_detect_poll, sc);
#endif

	shpcmcia_attach_sockets(sc);
}

static void
shpcmcia_attach_sockets(struct shpcmcia_softc *sc)
{

	shpcmcia_attach_socket(&sc->sc_handle[0]);
}

static void
shpcmcia_attach_socket(struct shpcmcia_handle *h)
{
	struct pcmciabus_attach_args paa;

	/* initialize the rest of the handle */
	h->shutdown = 0;
	h->memalloc = 0;
	h->ioalloc = 0;

	/* now, config one pcmcia device per socket */
	paa.paa_busname = "pcmcia";
	paa.pct = (pcmcia_chipset_tag_t)h->sc->sc_pct;
	paa.pch = (pcmcia_chipset_handle_t)h;

	h->pcmcia = config_found_ia(h->sc->sc_dev, "pcmciabus", &paa,
				    shpcmcia_print);

	/* if there's actually a pcmcia device attached, initialize the slot */
	if (h->pcmcia)
		shpcmcia_init_socket(h);
}

/*ARGSUSED*/
static int
shpcmcia_print(void *arg, const char *pnp)
{

	if (pnp)
		aprint_normal("pcmcia at %s", pnp);
	return UNCONF;
}

static void
shpcmcia_init_socket(struct shpcmcia_handle *h)
{
	uint16_t reg;

	/*
	 * queue creation of a kernel thread to handle insert/removal events.
	 */
#ifdef DIAGNOSTIC
	if (h->event_thread != NULL)
		panic("shpcmcia_attach_socket: event thread");
#endif

	/* if there's a card there, then attach it. */
	reg = _reg_read_2(SH4_PDTRA);
	if (!(reg & (1 << GPIO_PIN_CARD_CD))) {
		shpcmcia_attach_card(h);
		h->laststate = SHPCMCIA_LASTSTATE_PRESENT;
	} else {
		h->laststate = SHPCMCIA_LASTSTATE_EMPTY;
	}

	if (kthread_create(PRI_NONE, 0, NULL, shpcmcia_event_thread, h,
	    &h->event_thread, "%s", device_xname(h->sc->sc_dev))) {
		aprint_error_dev(h->sc->sc_dev,
		    "unable to create event thread\n");
		panic("shpcmcia_create_event_thread");
	}
}

/*
 * event thread
 */
static void
shpcmcia_event_thread(void *arg)
{
	struct shpcmcia_handle *h = (struct shpcmcia_handle *)arg;
	struct shpcmcia_event *pe;
	int s;

	while (h->shutdown == 0) {
		s = splhigh();
		if ((pe = SIMPLEQ_FIRST(&h->events)) == NULL) {
			splx(s);
			(void) tsleep(&h->events, PWAIT, "waitev", 0);
			continue;
		} else {
			splx(s);
			/* sleep .25s to be enqueued chatterling interrupts */
			(void) tsleep((void *)shpcmcia_event_thread,
			    PWAIT, "waitss", hz / 4);
		}
		s = splhigh();
		SIMPLEQ_REMOVE_HEAD(&h->events, pe_q);
		splx(s);

		switch (pe->pe_type) {
		case SHPCMCIA_EVENT_INSERT:
			s = splhigh();
			for (;;) {
				struct shpcmcia_event *pe1, *pe2;

				if ((pe1 = SIMPLEQ_FIRST(&h->events)) == NULL)
					break;
				if (pe1->pe_type != SHPCMCIA_EVENT_REMOVE)
					break;
				if ((pe2 = SIMPLEQ_NEXT(pe1, pe_q)) == NULL)
					break;
				if (pe2->pe_type == SHPCMCIA_EVENT_INSERT) {
					SIMPLEQ_REMOVE_HEAD(&h->events, pe_q);
					free(pe1, M_TEMP);
					SIMPLEQ_REMOVE_HEAD(&h->events, pe_q);
					free(pe2, M_TEMP);
				}
			}
			splx(s);

			DPRINTF(("%s: insertion event\n",
			    device_xname(h->sc->sc_dev)));
			shpcmcia_attach_card(h);
			break;

		case SHPCMCIA_EVENT_REMOVE:
			s = splhigh();
			for (;;) {
				struct shpcmcia_event *pe1, *pe2;

				if ((pe1 = SIMPLEQ_FIRST(&h->events)) == NULL)
					break;
				if (pe1->pe_type != SHPCMCIA_EVENT_INSERT)
					break;
				if ((pe2 = SIMPLEQ_NEXT(pe1, pe_q)) == NULL)
					break;
				if (pe2->pe_type == SHPCMCIA_EVENT_REMOVE) {
					SIMPLEQ_REMOVE_HEAD(&h->events, pe_q);
					free(pe1, M_TEMP);
					SIMPLEQ_REMOVE_HEAD(&h->events, pe_q);
					free(pe2, M_TEMP);
				}
			}
			splx(s);

			DPRINTF(("%s: removal event\n",
			    device_xname(h->sc->sc_dev)));
			shpcmcia_detach_card(h, DETACH_FORCE);
			break;

		default:
			panic("shpcmcia_event_thread: unknown event %d",
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
shpcmcia_queue_event(struct shpcmcia_handle *h, int event)
{
	struct shpcmcia_event *pe;
	int s;

	pe = malloc(sizeof(*pe), M_TEMP, M_NOWAIT);
	if (pe == NULL)
		panic("shpcmcia_queue_event: can't allocate event");

	pe->pe_type = event;
	s = splhigh();
	SIMPLEQ_INSERT_TAIL(&h->events, pe, pe_q);
	splx(s);
	wakeup(&h->events);
}

static void
shpcmcia_attach_card(struct shpcmcia_handle *h)
{

	DPRINTF(("%s\n", __func__));

	if (!(h->flags & SHPCMCIA_FLAG_CARDP)) {
		/* call the MI attach function */
		pcmcia_card_attach(h->pcmcia);

		h->flags |= SHPCMCIA_FLAG_CARDP;
	} else {
		DPRINTF(("shpcmcia_attach_card: already attached"));
	}
}

static void
shpcmcia_detach_card(struct shpcmcia_handle *h, int flags)
{

	DPRINTF(("%s\n", __func__));

	if (h->flags & SHPCMCIA_FLAG_CARDP) {
		h->flags &= ~SHPCMCIA_FLAG_CARDP;

		/* call the MI detach function */
		pcmcia_card_detach(h->pcmcia, flags);
	} else {
		DPRINTF(("shpcmcia_detach_card: already detached"));
	}
}

static void
shpcmcia_deactivate_card(struct shpcmcia_handle *h)
{

	DPRINTF(("%s\n", __func__));

	/* call the MI deactivate function */
	pcmcia_card_deactivate(h->pcmcia);

	shpcmcia_chip_socket_disable(h);
}

#if 0
/*
 * interrupt
 */
static int
shpcmcia_card_detect_intr(void *arg)
{
	struct shpcmcia_softc *sc = (struct shpcmcia_softc *)arg;
	struct shpcmcia_handle *h = &sc->sc_handle[0];
	uint16_t reg;

	DPRINTF(("%s\n", __func__));

	reg = _reg_read_2(SH4_PDTRA);
	if (reg & (1 << GPIO_PIN_CARD_CD)) {
		/* remove */
		if (h->laststate == SHPCMCIA_LASTSTATE_PRESENT) {
			/* Deactivate the card now. */
			shpcmcia_deactivate_card(h);
			shpcmcia_queue_event(h, SHPCMCIA_EVENT_REMOVE);
		}
		h->laststate = SHPCMCIA_LASTSTATE_EMPTY;
	} else {
		/* insert */
		if (h->laststate != SHPCMCIA_LASTSTATE_PRESENT) {
			shpcmcia_queue_event(h, SHPCMCIA_EVENT_INSERT);
		}
		h->laststate = SHPCMCIA_LASTSTATE_PRESENT;
	}
	return 1;
}
#else
/*
 * card polling
 */
static void
shpcmcia_card_detect_poll(void *arg)
{
	struct shpcmcia_softc *sc = (struct shpcmcia_softc *)arg;
	struct shpcmcia_handle *h = &sc->sc_handle[0];
	uint16_t reg;

	DPRINTF(("%s\n", __func__));

	reg = _reg_read_2(SH4_PDTRA);
	if (reg & (1 << GPIO_PIN_CARD_CD)) {
		/* remove */
		if (h->laststate == SHPCMCIA_LASTSTATE_PRESENT) {
			/* Deactivate the card now. */
			shpcmcia_deactivate_card(h);
			shpcmcia_queue_event(h, SHPCMCIA_EVENT_REMOVE);
		}
		h->laststate = SHPCMCIA_LASTSTATE_EMPTY;
	} else {
		/* insert */
		if (h->laststate != SHPCMCIA_LASTSTATE_PRESENT) {
			shpcmcia_queue_event(h, SHPCMCIA_EVENT_INSERT);
		}
		h->laststate = SHPCMCIA_LASTSTATE_PRESENT;
	}

	callout_schedule(&sc->sc_detect_ch, hz);
}
#endif

/*
 * pcmcia chip functions
 */
/* Memory space functions. */
static int
shpcmcia_chip_mem_alloc(pcmcia_chipset_handle_t pch, bus_size_t size,
    struct pcmcia_mem_handle *pmhp)
{
	struct shpcmcia_handle *h = (struct shpcmcia_handle *)pch;
	struct shpcmcia_softc *sc = h->sc;

	DPRINTF(("%s: size=%d\n", __func__, (unsigned)size));

	memset(pmhp, 0, sizeof(*pmhp));
	pmhp->memt = sc->sc_memt;
	pmhp->memh = sc->sc_memh;
	pmhp->addr = 0;
	pmhp->size = size;
	pmhp->realsize = size;

	return 0;
}

/*ARGSUSED*/
static void
shpcmcia_chip_mem_free(pcmcia_chipset_handle_t pch,
    struct pcmcia_mem_handle *pmhp)
{

	DPRINTF(("%s\n", __func__));
}

static int
shpcmcia_chip_mem_map(pcmcia_chipset_handle_t pch, int kind,
    bus_addr_t card_addr, bus_size_t size, struct pcmcia_mem_handle *pmhp,
    bus_size_t *offsetp, int *windowp)
{
	struct shpcmcia_handle *h = (struct shpcmcia_handle *)pch;
	struct shpcmcia_softc *sc = h->sc;
	int win;
	int i;
	int s;

	DPRINTF(("%s: kind=%#x, card_addr=%#x, size=%d\n",
	    __func__, kind, (unsigned)card_addr, (unsigned)size));

	s = splbio();
	win = -1;
	for (i = 0; i < SHPCMCIA_MEM_WINS; i++) {
		if ((h->memalloc & (1 << i)) == 0) {
			win = i;
			h->memalloc |= (1 << i);
			break;
		}
	}
	splx(s);
	if (win == -1)
		return 1;

	*windowp = win;
	*offsetp = 0;

	h->mem[win].addr = pmhp->addr;
	h->mem[win].size = size;
	h->mem[win].offset = (((long)card_addr) - ((long)pmhp->addr));
	h->mem[win].kind = kind;

	switch (kind) {
	case PCMCIA_MEM_ATTR:
		DPRINTF(("%s:PCMCIA_MEM_ATTR\n",device_xname(sc->sc_dev)));
		pmhp->memh = sc->sc_atth + card_addr;
		break;

	default:
		pmhp->memh = sc->sc_memh + card_addr;
		break;
	}

	return 0;
}

/*ARGSUSED*/
static void
shpcmcia_chip_mem_unmap(pcmcia_chipset_handle_t pch, int window)
{
	struct shpcmcia_handle *h = (struct shpcmcia_handle *)pch;
	int s;

	DPRINTF(("%s\n", __func__));

	s = splbio();
	h->memalloc &= ~(1 << window);
	splx(s);
}

/* I/O space functions. */
static int
shpcmcia_chip_io_alloc(pcmcia_chipset_handle_t pch,
    bus_addr_t start, bus_size_t size, bus_size_t align,
    struct pcmcia_io_handle *pihp)
{
	struct shpcmcia_handle *h = (struct shpcmcia_handle *)pch;
	struct shpcmcia_softc *sc = h->sc;

	DPRINTF(("%s\n", __func__));

	memset(pihp, 0, sizeof(*pihp));
	pihp->iot = sc->sc_iot;
	pihp->ioh = sc->sc_ioh;
	pihp->addr = start;
	pihp->size = size;
	pihp->flags |= PCMCIA_IO_ALLOCATED;

	return 0;
}

/*ARGSUSED*/
static void
shpcmcia_chip_io_free(pcmcia_chipset_handle_t pch,
    struct pcmcia_io_handle *pih)
{

	DPRINTF(("%s\n", __func__));
}

/*ARGSUSED*/
static int
shpcmcia_chip_io_map(pcmcia_chipset_handle_t pch, int width,
    bus_addr_t card_addr, bus_size_t size, struct pcmcia_io_handle *pihp,
    int *windowp)
{
	struct shpcmcia_handle *h = (struct shpcmcia_handle *)pch;
	struct shpcmcia_softc *sc = h->sc;
	bus_addr_t ioaddr = pihp->addr + card_addr;
	int win;
	int i;
	int s;

	DPRINTF(("%s\n", __func__));

	s = splbio();
	win = -1;
	for (i = 0; i < SHPCMCIA_IO_WINS; i++) {
		if ((h->ioalloc & (1 << i)) == 0) {
			win = i;
			h->ioalloc |= (1 << i);
			break;
		}
	}
	splx(s);
	if (win == -1)
		return 1;

	*windowp = win;

	/* XXX: IOS16 */

	aprint_normal_dev(sc->sc_dev, "port 0x%0lx", (u_long)ioaddr);
	if (size > 1)
		aprint_normal("-0x%lx", (u_long) ioaddr + (u_long) size - 1);
	aprint_normal("\n");

	h->io[win].addr = ioaddr;
	h->io[win].size = size;
	h->io[win].width = width;

	return 0;
}

/*ARGSUSED*/
static void
shpcmcia_chip_io_unmap(pcmcia_chipset_handle_t pch, int window)
{
	struct shpcmcia_handle *h = (struct shpcmcia_handle *)pch;
	int s;

	DPRINTF(("%s\n", __func__));

	s = splbio();
	h->ioalloc &= ~(1 << window);
	splx(s);
}

/* Interrupt functions. */
static void *
shpcmcia_chip_intr_establish(pcmcia_chipset_handle_t pch,
    struct pcmcia_function *pf, int ipl, int (*ih_func)(void *), void *ih_arg)
{
	struct shpcmcia_handle *h = (struct shpcmcia_handle *)pch;
	struct shpcmcia_softc *sc = h->sc;
	int s;

	KASSERT(sc->sc_ih == NULL);
	DPRINTF(("%s\n", __func__));

	s = splhigh();
	sc->sc_ih = extintr_establish(EXTINTR_INTR_CFIREQ, IST_LEVEL, ipl,
	    ih_func, ih_arg);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish card interrupt\n");
	}
	splx(s);

	return sc->sc_ih;
}

static void
shpcmcia_chip_intr_disestablish(pcmcia_chipset_handle_t pch, void *cookie)
{
	struct shpcmcia_handle *h = (struct shpcmcia_handle *)pch;
	struct shpcmcia_softc *sc = h->sc;
	int s;

	KASSERT(sc->sc_ih != NULL);
	DPRINTF(("%s\n", __func__));

	s = splhigh();
	extintr_disestablish(sc->sc_ih);
	sc->sc_ih = NULL;
	splx(s);
}

/* Socket functions. */
static void
shpcmcia_chip_socket_enable(pcmcia_chipset_handle_t pch)
{
	uint16_t reg;

	DPRINTF(("%s\n", __func__));

	/* power on the card */
	reg = _reg_read_2(SH4_PDTRA);
	reg &= ~(1 << GPIO_PIN_CARD_PON);
	_reg_write_2(SH4_PDTRA, reg);

	/* wait for card ready */
	while (_reg_read_1(EXTINTR_STAT1) & MASK1_INT12)
		continue;

	/* enable bus buffer */
	reg = _reg_read_2(SH4_PDTRA);
	reg &= ~(1 << GPIO_PIN_CARD_ENABLE);
	_reg_write_2(SH4_PDTRA, reg);

	/* reset the card */
	reg = _reg_read_2(SH4_PDTRA);
	reg &= ~(1 << GPIO_PIN_CARD_RESET);
	_reg_write_2(SH4_PDTRA, reg);
	delay(100 * 1000);
}

/*ARGSUSED*/
static void
shpcmcia_chip_socket_disable(pcmcia_chipset_handle_t pch)
{
	uint16_t reg;

	DPRINTF(("%s\n", __func__));

	/* reset the card */
	reg = _reg_read_2(SH4_PDTRA);
	reg |= (1 << GPIO_PIN_CARD_RESET);
	_reg_write_2(SH4_PDTRA, reg);

	/* power off the card */
	reg = _reg_read_2(SH4_PDTRA);
	reg |= (1 << GPIO_PIN_CARD_PON);
	_reg_write_2(SH4_PDTRA, reg);

	/* disable bus buffer */
	reg = _reg_read_2(SH4_PDTRA);
	reg |= (1 << GPIO_PIN_CARD_ENABLE);
	_reg_write_2(SH4_PDTRA, reg);
}

/*ARGSUSED*/
static void
shpcmcia_chip_socket_settype(pcmcia_chipset_handle_t pch, int type)
{

	DPRINTF(("%s\n", __func__));
}
