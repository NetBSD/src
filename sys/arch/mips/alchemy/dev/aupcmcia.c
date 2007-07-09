/* $NetBSD: aupcmcia.c,v 1.3 2007/07/09 20:52:22 ad Exp $ */

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Garrett D'Amore for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 

/* #include "opt_pci.h" */
/* #include "pci.h" */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: aupcmcia.c,v 1.3 2007/07/09 20:52:22 ad Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/kthread.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciachip.h>

#include <mips/alchemy/include/au_himem_space.h>
#include <mips/alchemy/include/aubusvar.h>
#include <mips/alchemy/include/aureg.h>
#include <mips/alchemy/include/auvar.h>

#include <mips/alchemy/dev/aupcmciareg.h>
#include <mips/alchemy/dev/aupcmciavar.h>

/*
 * Borrow PCMCIADEBUG for now.  Generally aupcmcia is the only PCMCIA
 * host on these machines anyway.
 */
#ifdef	PCMCIADEBUG
int	aupcm_debug = 1;
#define	DPRINTF(arg)	if (aupcm_debug) printf arg
#else
#define	DPRINTF(arg)
#endif	

/*
 * And for information about mappings, etc. use this one.
 */
#ifdef	AUPCMCIANOISY
#define	NOISY(arg)	printf arg
#else
#define	NOISY(arg)
#endif	

/*
 * Note, we use prefix "aupcm" instead of "aupcmcia", even though our
 * driver is the latter, mostly because my fingers have trouble typing
 * the former.  "aupcm" should be sufficiently unique to avoid
 * confusion.
 */

static int aupcm_mem_alloc(pcmcia_chipset_handle_t, bus_size_t,
    struct pcmcia_mem_handle *);
static void aupcm_mem_free(pcmcia_chipset_handle_t,
    struct pcmcia_mem_handle *);
static int aupcm_mem_map(pcmcia_chipset_handle_t, int, bus_addr_t,
    bus_size_t, struct pcmcia_mem_handle *, bus_size_t *, int *);
static void aupcm_mem_unmap(pcmcia_chipset_handle_t, int);

static int aupcm_io_alloc(pcmcia_chipset_handle_t, bus_addr_t, bus_size_t,
    bus_size_t, struct pcmcia_io_handle *);
static void aupcm_io_free(pcmcia_chipset_handle_t, struct pcmcia_io_handle *);
static int aupcm_io_map(pcmcia_chipset_handle_t, int, bus_addr_t,
    bus_size_t, struct pcmcia_io_handle *, int *);
static void aupcm_io_unmap(pcmcia_chipset_handle_t, int);
static void *aupcm_intr_establish(pcmcia_chipset_handle_t,
    struct pcmcia_function *, int, int (*)(void *), void *);
static void aupcm_intr_disestablish(pcmcia_chipset_handle_t, void *);

static void aupcm_slot_enable(pcmcia_chipset_handle_t);
static void aupcm_slot_disable(pcmcia_chipset_handle_t);
static void aupcm_slot_settype(pcmcia_chipset_handle_t, int);

static int aupcm_match(struct device *, struct cfdata *, void *);
static void aupcm_attach(struct device *, struct device *, void *);

static void aupcm_event_thread(void *);
static int aupcm_card_intr(void *);
static void aupcm_softintr(void *);
static int aupcm_print(void *, const char *);

struct aupcm_slot {
	struct aupcm_softc	*as_softc;
	int			as_slot;
	int			as_status;
	int			as_enabled;
	int			(*as_intr)(void *);
	int			as_card_irq;
	int			as_status_irq;
	void			*as_intrarg;
	void			*as_softint;
	void			*as_hardint;
	const char		*as_name;
	bus_addr_t		as_offset;
	struct mips_bus_space	as_iot;
	struct mips_bus_space	as_attrt;
	struct mips_bus_space	as_memt;
	void			*as_wins[AUPCMCIA_NWINS];

	struct device		*as_pcmcia;
};

/* this structure needs to be exposed... */
struct aupcm_softc {
	struct device		sc_dev;
	pcmcia_chipset_tag_t	sc_pct;

	void			(*sc_slot_enable)(int);
	void			(*sc_slot_disable)(int);
	int			(*sc_slot_status)(int);

	paddr_t			sc_base;

	int			sc_wake;
	lwp_t			*sc_thread;

	int			sc_nslots;
	struct aupcm_slot	sc_slots[AUPCMCIA_NSLOTS];
};

static struct pcmcia_chip_functions aupcm_functions = {
	aupcm_mem_alloc,
	aupcm_mem_free,
	aupcm_mem_map,
	aupcm_mem_unmap,

	aupcm_io_alloc,
	aupcm_io_free,
	aupcm_io_map,
	aupcm_io_unmap,

	aupcm_intr_establish,
	aupcm_intr_disestablish,

	aupcm_slot_enable,
	aupcm_slot_disable,
	aupcm_slot_settype,
};

static	struct mips_bus_space	aupcm_memt;

CFATTACH_DECL(aupcmcia, sizeof (struct aupcm_softc),
    aupcm_match, aupcm_attach, NULL, NULL);

int
aupcm_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct aubus_attach_args	*aa = aux;
	static int			found = 0;

	if (found)
		return 0;

	if (strcmp(aa->aa_name, "aupcmcia") != 0)
		return 0;

	found = 1;

	return 1;
}

void
aupcm_attach(struct device *parent, struct device *self, void *aux)
{
	/* struct aubus_attach_args	*aa = aux; */
	struct aupcm_softc		*sc = (struct aupcm_softc *)self;
	static int			done = 0;
	int				slot;
	struct aupcmcia_machdep		*md;

	/* initialize bus space */
	if (done) {
		/* there can be only one. */
		return;
	}

	done = 1;
	/*
	 * PCMCIA memory can live within pretty much the entire 32-bit
	 * space, modulo 64 MB wraps.  We don't have to support coexisting
	 * DMA.
	 */
	au_himem_space_init(&aupcm_memt, "pcmciamem",
	    PCMCIA_BASE, AUPCMCIA_ATTR_OFFSET, 0xffffffff,
	    AU_HIMEM_SPACE_LITTLE_ENDIAN);

	if ((md = aupcmcia_machdep()) == NULL) {
		printf("\n%s:unable to get machdep structure\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_nslots = md->am_nslots;
	sc->sc_slot_enable = md->am_slot_enable;
	sc->sc_slot_disable = md->am_slot_disable;
	sc->sc_slot_status = md->am_slot_status;

	printf(": Alchemy PCMCIA, %d slots\n", sc->sc_nslots);

	sc->sc_pct = (pcmcia_chipset_tag_t)&aupcm_functions;

	for (slot = 0; slot < sc->sc_nslots; slot++) {
		struct aupcm_slot		*sp;
		struct pcmciabus_attach_args	paa;

		sp = &sc->sc_slots[slot];
		sp->as_softc = sc;

		sp->as_slot = slot;
		sp->as_name = md->am_slot_name(slot);
		sp->as_offset = md->am_slot_offset(slot);
		sp->as_card_irq = md->am_slot_irq(slot, AUPCMCIA_IRQ_CARD);
		sp->as_status_irq = md->am_slot_irq(slot,
		    AUPCMCIA_IRQ_INSERT);

		au_himem_space_init(&sp->as_attrt, "pcmciaattr",
		    PCMCIA_BASE + sp->as_offset + AUPCMCIA_ATTR_OFFSET,
		    0, AUPCMCIA_MAP_SIZE, AU_HIMEM_SPACE_LITTLE_ENDIAN);

		au_himem_space_init(&sp->as_memt, "pcmciamem",
		    PCMCIA_BASE + sp->as_offset + AUPCMCIA_MEM_OFFSET,
		    0, AUPCMCIA_MAP_SIZE, AU_HIMEM_SPACE_LITTLE_ENDIAN);

		au_himem_space_init(&sp->as_iot, "pcmciaio",
		    PCMCIA_BASE + sp->as_offset + AUPCMCIA_IO_OFFSET,
		    0, AUPCMCIA_MAP_SIZE,
		    AU_HIMEM_SPACE_LITTLE_ENDIAN | AU_HIMEM_SPACE_IO);

		sp->as_status = 0;
		
		paa.paa_busname = "pcmcia";
		paa.pct = sc->sc_pct;
		paa.pch = (pcmcia_chipset_handle_t)sp;

		paa.iobase = 0;
		paa.iosize = AUPCMCIA_MAP_SIZE;

		sp->as_pcmcia = config_found(&sc->sc_dev, &paa, aupcm_print);

		/* if no pcmcia, make sure slot is powered down */
		if (sp->as_pcmcia == NULL) {
			aupcm_slot_disable(sp);
			continue;
		}

		/* this makes sure we probe the slot */
		sc->sc_wake |= (1 << slot);
	}

	/*
	 * XXX: this would be an excellent time time to establish a handler
	 * for the card insertion interrupt, but that's edge triggered, and
	 * au_icu.c won't support it right now.  We poll in the event thread
	 * for now.  Start by initializing it now.
	 */
	if (kthread_create(PRI_NONE, 0, NULL, aupcm_event_thread, sc,
	    &sc->sc_thread, "%s", sc->sc_dev.dv_xname) != 0)
		panic("%s: unable to create event kthread",
		    sc->sc_dev.dv_xname);
}

int
aupcm_print(void *aux, const char *pnp)
{
	struct pcmciabus_attach_args *paa = aux;
	struct aupcm_slot *sp = paa->pch;

	printf(" socket %d irq %d, %s", sp->as_slot, sp->as_card_irq,
	    sp->as_name);

	return (UNCONF);
}

void *
aupcm_intr_establish(pcmcia_chipset_handle_t pch,
    struct pcmcia_function *pf, int level, int (*handler)(void *), void *arg)
{
	struct aupcm_slot	*sp = (struct aupcm_slot *)pch;
	int			s;

	/*
	 * Hmm. perhaps this intr should be a list. well, PCMCIA
	 * devices generally only have one interrupt, and so should
	 * generally have only one handler.  So we leave it for now.
	 * (Other PCMCIA bus drivers do it this way.)
	 */
	sp->as_intr = handler;
	sp->as_intrarg = arg;

	/*
	 * XXX: pil must be a software interrupt level.  That
	 * automatically implies that it is lower than any other
	 * hardware interrupts.  So trying to figure out which level
	 * (IPL_SOFTNET, IPL_SOFTSERIAL, etc.) doesn't really do
	 * anything for us.
	 */
	sp->as_softint = softintr_establish(IPL_SOFT, aupcm_softintr, sp);

	/* set up hard interrupt handler for the card IRQs */
	s = splhigh();
	sp->as_hardint = au_intr_establish(sp->as_card_irq, 0,
	    IPL_TTY, IST_LEVEL_LOW, aupcm_card_intr, sp);
	/* if card is not powered up, then leave the IRQ masked */
	if (!sp->as_enabled) {
		au_intr_disable(sp->as_card_irq);
	}
	splx(s);

	return (sp->as_softint);
}

void
aupcm_intr_disestablish(pcmcia_chipset_handle_t pch, void *ih)
{
	struct aupcm_slot *sp = (struct aupcm_slot *)pch;

	KASSERT(sp->as_softint == ih);
	/* KASSERT(sp->as_hardint); */
	/* set up hard interrupt handler for the card IRQs */

	au_intr_disestablish(sp->as_hardint);
	sp->as_hardint = 0;

	softintr_disestablish(ih);
	sp->as_softint = 0;
	sp->as_intr = NULL;
	sp->as_intrarg = NULL;
}

/*
 * FYI: Hot detach of PCMCIA is supposedly safe because H/W doesn't
 * fault on accesses to missing hardware.
 */
void
aupcm_event_thread(void *arg)
{
	struct aupcm_softc	*sc = arg;
	struct aupcm_slot	*sp;
	int			s, i, attach, detach;

	for (;;) {
		s = splhigh();
		if (sc->sc_wake == 0) {
			splx(s);
			/*
			 * XXX: Currently, the au_icu.c lacks support
			 * for edge-triggered interrupts.  So we
			 * cannot really use the status change
			 * inerrupts.  For now we poll (once per sec).
			 * FYI, Linux does it this way, and they *do*
			 * have support for edge triggered interrupts.
			 * Go figure.
			 */
			tsleep(&sc->sc_wake, PWAIT, "aupcm_event", hz);
		}
		sc->sc_wake = 0;

		attach = detach = 0;
		for (i = 0; i < sc->sc_nslots; i++) {
			sp = &sc->sc_slots[i];

			if (sc->sc_slot_status(sp->as_slot) != 0) {
				if (!sp->as_status) {
					DPRINTF(("%s: card %d insertion\n",
						    sc->sc_dev.dv_xname, i));
					attach |= (1 << i);
					sp->as_status = 1;
				}
			} else {
				if (sp->as_status) {
					DPRINTF(("%s: card %d removal\n",
						    sc->sc_dev.dv_xname, i));
					detach |= (1 << i);
					sp->as_status = 0;
				}
			}
		}
		splx(s);

		for (i = 0; i < sc->sc_nslots; i++) {
			sp = &sc->sc_slots[i];

			if (detach & (1 << i)) {
				aupcm_slot_disable(sp);
				pcmcia_card_detach(sp->as_pcmcia,
				    DETACH_FORCE);
			} else if (attach & (1 << i)) {
				/*
				 * until the function is enabled, don't
				 * honor interrupts
				 */
				sp->as_enabled = 0;
				au_intr_disable(sp->as_card_irq);
				pcmcia_card_attach(sp->as_pcmcia);
			}
		}
	}
}

#if 0
void
aupcm_status_intr(void *arg)
{
	int s;
	struct aupcm_softc *sc = arg;

	s = splhigh();

	/* kick the status thread so it does its bit */
	sc->sc_wake = 1;
	wakeup(&sc->sc_wake);

	splx(s);
}
#endif

int
aupcm_card_intr(void *arg)
{
	struct aupcm_slot *sp = arg;

	/* disable the hard interrupt for now */
	au_intr_disable(sp->as_card_irq);

	if (sp->as_intr != NULL) {
		softintr_schedule(sp->as_softint);
	}

	return 1;
}

void
aupcm_softintr(void *arg)
{
	struct aupcm_slot	*sp = arg;
	int			s;

	sp->as_intr(sp->as_intrarg);

	s = splhigh();

	if (sp->as_intr && sp->as_enabled) {
		au_intr_enable(sp->as_card_irq);
	}

	splx(s);
}

void
aupcm_slot_enable(pcmcia_chipset_handle_t pch)
{
	struct aupcm_slot	*sp = (struct aupcm_slot *)pch;
	int			s;

	/* no interrupts while we reset the card, please */
	if (sp->as_intr)
		au_intr_disable(sp->as_card_irq);

	/*
	 * XXX: should probably lock to make sure slot_disable and
	 * enable not called together.  However, i believe that the
	 * event thread basically serializes them anyway.
	 */

	sp->as_softc->sc_slot_enable(sp->as_slot);
	/* card is powered up now, honor device interrupts */

	s = splhigh();
	sp->as_enabled = 1;
	if (sp->as_intr)
		au_intr_enable(sp->as_card_irq);
	splx(s);
}

void
aupcm_slot_disable(pcmcia_chipset_handle_t pch)
{
	struct aupcm_slot	*sp = (struct aupcm_slot *)pch;
	int			s;

	s = splhigh();
	au_intr_disable(sp->as_card_irq);
	sp->as_enabled = 0;
	splx(s);

	sp->as_softc->sc_slot_disable(sp->as_slot);
}

void
aupcm_slot_settype(pcmcia_chipset_handle_t pch, int type)
{
	/* we do nothing now : type == PCMCIA_IFTYPE_IO */
}

int
aupcm_mem_alloc(pcmcia_chipset_handle_t pch, bus_size_t size,
    struct pcmcia_mem_handle *pcmh)
{
	pcmh->memt = NULL;
	pcmh->size = pcmh->realsize = size;
	pcmh->addr = 0;
	pcmh->mhandle = 0;

	return 0;
}

void
aupcm_mem_free(pcmcia_chipset_handle_t pch, struct pcmcia_mem_handle *pcmh)
{
	/* nothing to do */
}

int
aupcm_mem_map(pcmcia_chipset_handle_t pch, int kind, bus_addr_t addr,
    bus_size_t size, struct pcmcia_mem_handle *pcmh, bus_size_t *offsetp,
    int *windowp)
{
	struct aupcm_slot	*sp = (struct aupcm_slot *)pch;
	int			win, err;
	int			s;

	s = splhigh();
	for (win = 0; win < AUPCMCIA_NWINS; win++) {
		if (sp->as_wins[win] == NULL) {
			sp->as_wins[win] = pcmh;
			break;
		}
	}
	splx(s);

	if (win >= AUPCMCIA_NWINS) {
		return ENOMEM;
	}

	if (kind & PCMCIA_MEM_ATTR) {
		pcmh->memt = &sp->as_attrt;
		NOISY(("mapping ATTR addr %x size %x\n", (uint32_t)addr,
		    (uint32_t)size));
	} else {
		pcmh->memt = &sp->as_memt;
		NOISY(("mapping MEMORY addr %x size %x\n", (uint32_t)addr,
			  (uint32_t)size));
	}

	if ((size + addr) > (64 * 1024 * 1024))
		return EINVAL;

	pcmh->size = size;

	err = bus_space_map(pcmh->memt, addr, size, 0, &pcmh->memh);
	if (err != 0) {
		sp->as_wins[win] = NULL;
		return err;
	}
	*offsetp = 0;
	*windowp = win;

	return 0;
}

void
aupcm_mem_unmap(pcmcia_chipset_handle_t pch, int win)
{
	struct aupcm_slot		*sp = (struct aupcm_slot *)pch;
	struct pcmcia_mem_handle	*pcmh;

	pcmh = (struct pcmcia_mem_handle *)sp->as_wins[win];
	sp->as_wins[win] = NULL;

	NOISY(("memory umap virtual %x\n", (uint32_t)pcmh->memh));
	bus_space_unmap(pcmh->memt, pcmh->memh, pcmh->size);
	pcmh->memt = NULL;
}

int
aupcm_io_alloc(pcmcia_chipset_handle_t pch, bus_addr_t start,
    bus_size_t size, bus_size_t align, struct pcmcia_io_handle *pih)
{
	struct aupcm_slot	*sp = (struct aupcm_slot *)pch;
	bus_space_handle_t	bush;
	int			err;

	pih->iot = &sp->as_iot;
	pih->size = size;
	pih->flags = 0;
	
	/*
	 * start from the initial offset - this gets us a slot
	 * specific address, while still leaving the addresses more or
	 * less zero-based which is required for x86-style device
	 * drivers.
	 */
	err = bus_space_alloc(pih->iot, start, 0x100000,
	    size, align, 0, 0, &pih->addr, &bush);
	NOISY(("start = %x, addr = %x, size = %x, bush = %x\n",
		  (uint32_t)start, (uint32_t)pih->addr, (uint32_t)size,
		  (uint32_t)bush));

	/* and we convert it back */
	if (err == 0) {
		pih->ihandle = (void *)bush;
	}

	return (err);
}

void
aupcm_io_free(pcmcia_chipset_handle_t pch, struct pcmcia_io_handle *pih)
{
	bus_space_free(pih->iot, (bus_space_handle_t)pih->ihandle,
	    pih->size);
}

int
aupcm_io_map(pcmcia_chipset_handle_t pch, int width, bus_addr_t offset,
    bus_size_t size, struct pcmcia_io_handle *pih, int *windowp)
{
	int			err;

	err = bus_space_subregion(pih->iot, (bus_space_handle_t)pih->ihandle,
	    offset, size, &pih->ioh);
	NOISY(("io map offset = %x, size = %x, ih = %x, hdl=%x\n",
		  (uint32_t)offset, (uint32_t)size,
		  (uint32_t)pih->ihandle, (uint32_t)pih->ioh));

	return err;
}

void
aupcm_io_unmap(pcmcia_chipset_handle_t pch, int win)
{
	/* We mustn't unmap/free subregion bus space! */
	NOISY(("io unmap\n"));
}
