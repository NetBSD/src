/*	$NetBSD: plumpcmcia.c,v 1.22.8.1 2008/01/02 21:47:58 bouyer Exp $ */

/*
 * Copyright (c) 1999, 2000 UCHIYAMA Yasushi. All rights reserved.
 * Copyright (c) 1997 Marc Horowitz. All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: plumpcmcia.c,v 1.22.8.1 2008/01/02 21:47:58 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kthread.h>

#include <machine/bus.h>
#include <machine/config_hook.h>
#include <machine/bus_space_hpcmips.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciachip.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/dev/plumvar.h>
#include <hpcmips/dev/plumicuvar.h>
#include <hpcmips/dev/plumpowervar.h>
#include <hpcmips/dev/plumpcmciareg.h>

#ifdef	PLUMPCMCIA_DEBUG
#define DPRINTF_ENABLE
#define DPRINTF_DEBUG	plumpcmcia_debug
#endif
#include <machine/debug.h>

int	plumpcmcia_match(struct device *, struct cfdata *, void *);
void	plumpcmcia_attach(struct device *, struct device *, void *);
int	plumpcmcia_print(void *, const char *);

int	plumpcmcia_power(void *, int, long, void *);

struct plumpcmcia_softc;

struct plumpcmcia_handle {
	/* parent */
	struct device	*ph_parent;
	/* child */
	struct device	*ph_pcmcia;

	/* PCMCIA controller register space */
	bus_space_tag_t ph_regt;
	bus_space_handle_t ph_regh;

	/* I/O port space */
	int ph_ioarea;	/* not PCMCIA window */
	struct {
		bus_addr_t	pi_addr;
		bus_size_t	pi_size;
		int		pi_width;
	} ph_io[PLUM_PCMCIA_IO_WINS];
	int ph_ioalloc;
	bus_space_tag_t ph_iot;
	bus_space_handle_t ph_ioh;
	bus_addr_t ph_iobase;
	bus_size_t ph_iosize;

	/* I/O Memory space */
	int ph_memarea;	/* not PCMCIA window */
	struct {
		bus_addr_t	pm_addr;
		bus_size_t	pm_size;
		int32_t		pm_offset;
		int		pm_kind;
	} ph_mem[PLUM_PCMCIA_MEM_WINS];
	int ph_memalloc;
	bus_space_tag_t ph_memt;
	bus_space_handle_t ph_memh;
	bus_addr_t ph_membase;
	bus_size_t ph_memsize;

	/* Card interrupt handler */
	int ph_plum_irq;
	void *ph_card_ih;
};

enum plumpcmcia_event_type {
	PLUM_PCMCIA_EVENT_INSERT,
	PLUM_PCMCIA_EVENT_REMOVE,
};

struct plumpcmcia_event {
	int __queued;
	enum plumpcmcia_event_type pe_type;
	struct plumpcmcia_handle *pe_ph;
	SIMPLEQ_ENTRY(plumpcmcia_event) pe_link;
};

struct plumpcmcia_softc {
	struct device	sc_dev;
	plum_chipset_tag_t sc_pc;

	/* Register space */
	bus_space_tag_t sc_regt;
	bus_space_handle_t sc_regh;

	/* power management hook */
	void *sc_powerhook;

	/* CSC event */
	lwp_t *sc_event_thread;
	SIMPLEQ_HEAD (, plumpcmcia_event) sc_event_head;

	/* for each slot */
	struct plumpcmcia_handle sc_ph[PLUMPCMCIA_NSLOTS];
};

static void plumpcmcia_attach_socket(struct plumpcmcia_handle *);
static int plumpcmcia_chip_mem_alloc(pcmcia_chipset_handle_t, bus_size_t,
    struct pcmcia_mem_handle *);
static void plumpcmcia_chip_mem_free(pcmcia_chipset_handle_t,
    struct pcmcia_mem_handle *);
static int plumpcmcia_chip_mem_map(pcmcia_chipset_handle_t, int, bus_addr_t,
    bus_size_t, struct pcmcia_mem_handle *,  bus_size_t *, int *);
static void plumpcmcia_chip_mem_unmap(pcmcia_chipset_handle_t, int);
static int plumpcmcia_chip_io_alloc(pcmcia_chipset_handle_t, bus_addr_t,
    bus_size_t, bus_size_t, struct pcmcia_io_handle *);
static void plumpcmcia_chip_io_free(pcmcia_chipset_handle_t,
    struct pcmcia_io_handle *);
static int plumpcmcia_chip_io_map(pcmcia_chipset_handle_t, int, bus_addr_t,
    bus_size_t, struct pcmcia_io_handle *, int *);
static void plumpcmcia_chip_io_unmap(pcmcia_chipset_handle_t, int);
static void plumpcmcia_chip_socket_enable(pcmcia_chipset_handle_t);
static void plumpcmcia_chip_socket_disable(pcmcia_chipset_handle_t);
static void plumpcmcia_chip_socket_settype(pcmcia_chipset_handle_t, int);
static void *plumpcmcia_chip_intr_establish(pcmcia_chipset_handle_t,
    struct pcmcia_function *, int, int (*)(void *), void *);
static void plumpcmcia_chip_intr_disestablish(pcmcia_chipset_handle_t, void *);
static void plumpcmcia_wait_ready(	struct plumpcmcia_handle *);
static void plumpcmcia_chip_do_mem_map(struct plumpcmcia_handle *, int);
static void plumpcmcia_chip_do_io_map(struct plumpcmcia_handle *, int);

static struct pcmcia_chip_functions plumpcmcia_functions = {
	plumpcmcia_chip_mem_alloc,
	plumpcmcia_chip_mem_free,
	plumpcmcia_chip_mem_map,
	plumpcmcia_chip_mem_unmap,
	plumpcmcia_chip_io_alloc,
	plumpcmcia_chip_io_free,
	plumpcmcia_chip_io_map,
	plumpcmcia_chip_io_unmap,
	plumpcmcia_chip_intr_establish,
	plumpcmcia_chip_intr_disestablish,
	plumpcmcia_chip_socket_enable,
	plumpcmcia_chip_socket_disable,
	plumpcmcia_chip_socket_settype,
};

/* CSC */
#define PLUM_PCMCIA_EVENT_QUEUE_MAX		5
static struct plumpcmcia_event __event_queue_pool[PLUM_PCMCIA_EVENT_QUEUE_MAX];
static struct plumpcmcia_event *plumpcmcia_event_alloc(void);
static void plumpcmcia_event_free(struct plumpcmcia_event *);
static void plum_csc_intr_setup(struct plumpcmcia_softc *,
    struct plumpcmcia_handle *, int);
static int plum_csc_intr(void *);
static void plumpcmcia_event_thread(void *);

#ifdef PLUMPCMCIA_DEBUG
/* debug */
#define __DEBUG_FUNC	__attribute__((__unused__))
static void __ioareadump(plumreg_t) __DEBUG_FUNC;
static void __memareadump(plumreg_t) __DEBUG_FUNC;
static void plumpcmcia_dump(struct plumpcmcia_softc *) __DEBUG_FUNC;
#endif /* PLUMPCMCIA_DEBUG */

CFATTACH_DECL(plumpcmcia, sizeof(struct plumpcmcia_softc),
    plumpcmcia_match, plumpcmcia_attach, NULL, NULL);

int
plumpcmcia_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return (1);
}

void
plumpcmcia_attach(struct device *parent, struct device *self, void *aux)
{
	struct plum_attach_args *pa = aux;
	struct plumpcmcia_softc *sc = (void*)self;
	struct plumpcmcia_handle *ph;
	int error;

	sc->sc_pc	= pa->pa_pc;
	sc->sc_regt	= pa->pa_regt;

	/* map register area */
	if (bus_space_map(sc->sc_regt, PLUM_PCMCIA_REGBASE, 
	    PLUM_PCMCIA_REGSIZE, 0, &sc->sc_regh)) {
		printf(": register map failed\n");
	}

	/* power control */
	plumpcmcia_power(sc, 0, 0, (void *)PWR_RESUME);
	/* Add a hard power hook to power saving */
#if notyet
	sc->sc_powerhook = config_hook(CONFIG_HOOK_PMEVENT,
	    CONFIG_HOOK_PMEVENT_HARDPOWER,
	    CONFIG_HOOK_SHARE,
	    plumpcmcia_power, sc);
	if (sc->sc_powerhook == 0)
		printf(": WARNING unable to establish hard power hook");
#endif
	printf("\n");

	/* Slot0/1 CSC event queue */
	SIMPLEQ_INIT (&sc->sc_event_head);
	error = kthread_create(PRI_NONE, 0, NULL, plumpcmcia_event_thread,
	    sc, &sc->sc_event_thread, "%s", sc->sc_dev.dv_xname);
	KASSERT(error == 0);

	/* Slot 0 */
	ph = &sc->sc_ph[0];
	ph->ph_plum_irq	= PLUM_INT_C1IO;
	ph->ph_memarea	= PLUM_PCMCIA_MEMWINCTRL_MAP_AREA1;
	ph->ph_membase	= PLUM_PCMCIA_MEMBASE1;
	ph->ph_memsize	= PLUM_PCMCIA_MEMSIZE1;
	ph->ph_ioarea	= PLUM_PCMCIA_IOWINADDRCTRL_AREA1;
	ph->ph_iobase	= PLUM_PCMCIA_IOBASE1;
	ph->ph_iosize	= PLUM_PCMCIA_IOSIZE1;
	ph->ph_regt = sc->sc_regt;
	bus_space_subregion(sc->sc_regt, sc->sc_regh, 
	    PLUM_PCMCIA_REGSPACE_SLOT0,
	    PLUM_PCMCIA_REGSPACE_SIZE,
	    &ph->ph_regh);
	ph->ph_iot	= pa->pa_iot;
	ph->ph_memt	= pa->pa_iot;
	ph->ph_parent = (void*)sc;

	plum_csc_intr_setup(sc, ph, PLUM_INT_C1SC);
	plum_power_establish(sc->sc_pc, PLUM_PWR_PCC1);
	plumpcmcia_attach_socket(ph);

	/* Slot 1 */
	ph = &sc->sc_ph[1];
	ph->ph_plum_irq	= PLUM_INT_C2IO;
	ph->ph_memarea	= PLUM_PCMCIA_MEMWINCTRL_MAP_AREA2;
	ph->ph_membase	= PLUM_PCMCIA_MEMBASE2;
	ph->ph_memsize	= PLUM_PCMCIA_MEMSIZE2;
	ph->ph_ioarea	= PLUM_PCMCIA_IOWINADDRCTRL_AREA2;
	ph->ph_iobase	= PLUM_PCMCIA_IOBASE2;
	ph->ph_iosize	= PLUM_PCMCIA_IOSIZE2;
	ph->ph_regt = sc->sc_regt;
	bus_space_subregion(sc->sc_regt, sc->sc_regh, 
	    PLUM_PCMCIA_REGSPACE_SLOT1,
	    PLUM_PCMCIA_REGSPACE_SIZE,
	    &ph->ph_regh);
	ph->ph_iot	= pa->pa_iot;
	ph->ph_memt	= pa->pa_iot;
	ph->ph_parent = (void*)sc;

	plum_csc_intr_setup(sc, ph, PLUM_INT_C2SC);
	plum_power_establish(sc->sc_pc, PLUM_PWR_PCC2);
	plumpcmcia_attach_socket(ph);
}

int
plumpcmcia_print(void *arg, const char *pnp)
{
	if (pnp) {
		aprint_normal("pcmcia at %s", pnp);
	}

	return (UNCONF);
}

static void
plumpcmcia_attach_socket(struct plumpcmcia_handle *ph)
{
	struct pcmciabus_attach_args paa;
	struct plumpcmcia_softc *sc = (void*)ph->ph_parent;

	paa.paa_busname = "pcmcia";
	paa.pct = (pcmcia_chipset_tag_t)&plumpcmcia_functions;
	paa.pch = (pcmcia_chipset_handle_t)ph;
	paa.iobase = 0;
	paa.iosize = ph->ph_iosize;

	if ((ph->ph_pcmcia = config_found_ia((void*)sc, "pcmciabus", &paa,
	    plumpcmcia_print))) {
		/* Enable slot */
		plum_conf_write(ph->ph_regt, ph->ph_regh,
		    PLUM_PCMCIA_SLOTCTRL,
		    PLUM_PCMCIA_SLOTCTRL_ENABLE);
		/* Support 3.3V card & enable Voltage Sense Status */
		plum_conf_write(ph->ph_regt, ph->ph_regh,
		    PLUM_PCMCIA_FUNCCTRL,
		    PLUM_PCMCIA_FUNCCTRL_VSSEN |
		    PLUM_PCMCIA_FUNCCTRL_3VSUPPORT);
		pcmcia_card_attach(ph->ph_pcmcia);		
	}
}

static void *
plumpcmcia_chip_intr_establish(pcmcia_chipset_handle_t pch,
    struct pcmcia_function *pf, int ipl,
    int (*ih_fun)(void *), void *ih_arg)
{
	struct plumpcmcia_handle *ph = (void*)pch;
	struct plumpcmcia_softc *sc = (void*)ph->ph_parent;

	if (!(ph->ph_card_ih = 
	    plum_intr_establish(sc->sc_pc, ph->ph_plum_irq,
		IST_EDGE, IPL_BIO, ih_fun, ih_arg))) {
		printf("plumpcmcia_chip_intr_establish: can't establish\n");
		return (0);
	}

	return (ph->ph_card_ih);
}

static void 
plumpcmcia_chip_intr_disestablish(pcmcia_chipset_handle_t pch, void *ih)
{
	struct plumpcmcia_handle *ph = (void*)pch;
	struct plumpcmcia_softc *sc = (void*)ph->ph_parent;

	plum_intr_disestablish(sc->sc_pc, ih);
}

static int
plumpcmcia_chip_mem_alloc(pcmcia_chipset_handle_t pch, bus_size_t size,
    struct pcmcia_mem_handle *pcmhp)
{
	struct plumpcmcia_handle *ph = (void*)pch;
	bus_size_t realsize;

	/* convert size to PCIC pages */
	realsize = ((size + (PLUM_PCMCIA_MEM_PAGESIZE - 1)) / 
	    PLUM_PCMCIA_MEM_PAGESIZE) * PLUM_PCMCIA_MEM_PAGESIZE;

	if (bus_space_alloc(ph->ph_memt, ph->ph_membase,
	    ph->ph_membase + ph->ph_memsize, 
	    realsize, PLUM_PCMCIA_MEM_PAGESIZE, 
	    0, 0, 0, &pcmhp->memh)) {
		return (1);
	}

	pcmhp->memt = ph->ph_memt;
	/* Address offset from MEM area base */
	pcmhp->addr = pcmhp->memh - ph->ph_membase - 
	    ((struct bus_space_tag_hpcmips*)ph->ph_memt)->base;
	pcmhp->size = size;
	pcmhp->realsize = realsize;

	DPRINTF(("plumpcmcia_chip_mem_alloc: size %#x->%#x addr %#x->%#x\n", 
	    (unsigned)size, (unsigned)realsize, (unsigned)pcmhp->addr,
	    (unsigned)pcmhp->memh));

	return (0);
}

static void
plumpcmcia_chip_mem_free(pcmcia_chipset_handle_t pch,
    struct pcmcia_mem_handle *pcmhp)
{

	bus_space_free(pcmhp->memt, pcmhp->memh, pcmhp->size);
}

static int
plumpcmcia_chip_mem_map(pcmcia_chipset_handle_t pch, int kind,
    bus_addr_t card_addr, bus_size_t size,
    struct pcmcia_mem_handle *pcmhp,
    bus_size_t *offsetp, int *windowp)
{
	struct plumpcmcia_handle *ph = (void*)pch;
	bus_addr_t busaddr;
	int32_t card_offset;
	int i, win;

	for (win = -1, i = 0; i < PLUM_PCMCIA_MEM_WINS; i++) {
		if ((ph->ph_memalloc & (1 << i)) == 0) {
			win = i;
			ph->ph_memalloc |= (1 << i);
			break;
		}
	}
	if (win == -1) {
		DPRINTF(("plumpcmcia_chip_mem_map: no window\n"));
		return (1);
	}

	busaddr = pcmhp->addr;

	*offsetp = card_addr % PLUM_PCMCIA_MEM_PAGESIZE;
	card_addr -= *offsetp;
	size += *offsetp - 1;
	*windowp = win;
	card_offset = (((int32_t)card_addr) - ((int32_t)busaddr));

	DPRINTF(("plumpcmcia_chip_mem_map window %d bus %#x(kv:%#x)+%#x"
	    " size %#x at card addr %#x offset %#x\n", win,
	    (unsigned)busaddr, (unsigned)pcmhp->memh, (unsigned)*offsetp,
	    (unsigned)size, (unsigned)card_addr, (unsigned)card_offset));

	ph->ph_mem[win].pm_addr = busaddr;
	ph->ph_mem[win].pm_size = size;
	ph->ph_mem[win].pm_offset = card_offset;
	ph->ph_mem[win].pm_kind = kind;
	ph->ph_memalloc |= (1 << win);

	plumpcmcia_chip_do_mem_map(ph, win);

	return (0);
}

static void
plumpcmcia_chip_do_mem_map(struct plumpcmcia_handle *ph, int win)
{
	bus_space_tag_t regt = ph->ph_regt;
	bus_space_handle_t regh = ph->ph_regh;
	plumreg_t reg, addr, offset, size;
	
	if (win < 0 || win > 4) {
		panic("plumpcmcia_chip_do_mem_map: bogus window %d", win);
	}

	addr = (ph->ph_mem[win].pm_addr) >> PLUM_PCMCIA_MEM_SHIFT;
	size = (ph->ph_mem[win].pm_size) >> PLUM_PCMCIA_MEM_SHIFT;
	offset = (ph->ph_mem[win].pm_offset) >> PLUM_PCMCIA_MEM_SHIFT;
	
	/* Attribute memory or not */
	reg = ph->ph_mem[win].pm_kind == PCMCIA_MEM_ATTR ?
	    PLUM_PCMCIA_MEMWINCTRL_REGACTIVE : 0;

	/* Notify I/O area to select for PCMCIA controller */
	reg = PLUM_PCMCIA_MEMWINCTRL_MAP_SET(reg, ph->ph_memarea);

	/* Zero wait & 16bit access */
	reg |= (PLUM_PCMCIA_MEMWINCTRL_ZERO_WS |
	    PLUM_PCMCIA_MEMWINCTRL_DATASIZE16);
	plum_conf_write(regt, regh, PLUM_PCMCIA_MEMWINCTRL(win), reg);

	/* Map Host <-> PC-Card address */

	/* host-side */
	plum_conf_write(regt, regh, PLUM_PCMCIA_MEMWINSTARTADDR(win), 
	    addr);
	plum_conf_write(regt, regh, PLUM_PCMCIA_MEMWINSTOPADDR(win), 
	    addr + size);

	/* card-side */
	plum_conf_write(regt, regh, PLUM_PCMCIA_MEMWINOFSADDR(win), offset);

	/* Enable memory window */
	reg = plum_conf_read(regt, regh, PLUM_PCMCIA_WINEN);
	reg |= PLUM_PCMCIA_WINEN_MEM(win);
	plum_conf_write(regt, regh, PLUM_PCMCIA_WINEN, reg);
	
	DPRINTF(("plumpcmcia_chip_do_mem_map: window:%d %#x(%#x)+%#x\n",
	    win, offset, addr, size));

	delay(100);
}

static void
plumpcmcia_chip_mem_unmap(pcmcia_chipset_handle_t pch, int window)
{
	struct plumpcmcia_handle *ph = (void*)pch;
	bus_space_tag_t regt = ph->ph_regt;
	bus_space_handle_t regh = ph->ph_regh;
	plumreg_t reg;
	
	reg = plum_conf_read(regt, regh, PLUM_PCMCIA_WINEN);
	reg &= ~PLUM_PCMCIA_WINEN_MEM(window);
	plum_conf_write(regt, regh, PLUM_PCMCIA_WINEN, reg);

	ph->ph_memalloc &= ~(1 << window);
}

static int
plumpcmcia_chip_io_alloc(pcmcia_chipset_handle_t pch, bus_addr_t start,
    bus_size_t size, bus_size_t align,
    struct pcmcia_io_handle *pcihp)
{
	struct plumpcmcia_handle *ph = (void*)pch;

	DPRINTF(("plumpcmcia_chip_io_alloc: start=%#x size=%#x ",
	    (unsigned)start, (unsigned)size));
	if (start) {
		if (bus_space_map(ph->ph_iot, ph->ph_iobase + start, 
		    size, 0, &pcihp->ioh)) {
			DPRINTF(("bus_space_map failed\n"));
			return (1);
		}
		pcihp->flags = 0;
		pcihp->addr = start;
		DPRINTF(("(mapped) %#x+%#x\n", (unsigned)start,
		    (unsigned)size));
	} else {
		if (bus_space_alloc(ph->ph_iot, ph->ph_iobase,
		    ph->ph_iobase + ph->ph_iosize, size, 
		    align, 0, 0, 0, &pcihp->ioh)) {
			DPRINTF(("bus_space_alloc failed\n"));
			return 1;
		}
		/* Address offset from IO area base */
		pcihp->addr = pcihp->ioh - ph->ph_iobase - 
		    ((struct bus_space_tag_hpcmips*)ph->ph_iot)->base;
		pcihp->flags = PCMCIA_IO_ALLOCATED;
		DPRINTF(("(allocated) %#x+%#x\n", (unsigned)pcihp->addr,
		    (unsigned)size));
	}
	
	pcihp->iot = ph->ph_iot;
	pcihp->size = size;
	
	return (0);
}

static int
plumpcmcia_chip_io_map(pcmcia_chipset_handle_t pch, int width,
    bus_addr_t offset, bus_size_t size,
    struct pcmcia_io_handle *pcihp, int *windowp)
{
#ifdef PLUMPCMCIA_DEBUG
	static char *width_names[] = { "auto", "io8", "io16" };
#endif /* PLUMPCMCIA_DEBUG */
	struct plumpcmcia_handle *ph = (void*)pch;
	bus_addr_t winofs;
	int i, win;

	winofs = pcihp->addr + offset;

	if (winofs > 0x3ff) {
		printf("plumpcmcia_chip_io_map: WARNING port %#lx > 0x3ff\n",
		    winofs);
	}

	for (win = -1, i = 0; i < PLUM_PCMCIA_IO_WINS; i++) {
		if ((ph->ph_ioalloc & (1 << i)) == 0) {
			win = i;
			ph->ph_ioalloc |= (1 << i);
			break;
		}
	}
	if (win == -1) {
		DPRINTF(("plumpcmcia_chip_io_map: no window\n"));
		return (1);
	}
	*windowp = win;

	ph->ph_io[win].pi_addr = winofs;
	ph->ph_io[win].pi_size = size;
	ph->ph_io[win].pi_width = width;

	plumpcmcia_chip_do_io_map(ph, win);

	DPRINTF(("plumpcmcia_chip_io_map: %#x(kv:%#x)+%#x %s\n", 
	    (unsigned)offset, (unsigned)pcihp->ioh, (unsigned)size,
	    width_names[width]));

	return (0);
}

static void
plumpcmcia_chip_do_io_map(struct plumpcmcia_handle *ph, int win)
{
	bus_space_tag_t regt = ph->ph_regt;
	bus_space_handle_t regh = ph->ph_regh;
	plumreg_t reg;
	bus_addr_t addr;
	bus_size_t size;
	int shift;
	plumreg_t ioctlbits[3] = {
		PLUM_PCMCIA_IOWINCTRL_IOCS16SRC,
		0,
		PLUM_PCMCIA_IOWINCTRL_DATASIZE16
	};
	
	if (win < 0 || win > 1) {
		panic("plumpcmcia_chip_do_io_map: bogus window %d", win);
	}

	addr = ph->ph_io[win].pi_addr;
	size = ph->ph_io[win].pi_size;

	/* Notify I/O area to select for PCMCIA controller */
	plum_conf_write(regt, regh, PLUM_PCMCIA_IOWINADDRCTRL(win), 
	    ph->ph_ioarea);

	/* Start/Stop addr */
	plum_conf_write(regt, regh, PLUM_PCMCIA_IOWINSTARTADDR(win), addr);
	plum_conf_write(regt, regh, PLUM_PCMCIA_IOWINSTOPADDR(win), 
	    addr + size - 1);
	
	/* Set bus width */
	reg = plum_conf_read(regt, regh, PLUM_PCMCIA_IOWINCTRL);
	shift = win == 0 ? PLUM_PCMCIA_IOWINCTRL_WIN0SHIFT :
	    PLUM_PCMCIA_IOWINCTRL_WIN1SHIFT;
	
	reg &= ~(PLUM_PCMCIA_IOWINCTRL_WINMASK << shift);
	reg |= ((ioctlbits[ph->ph_io[win].pi_width] |
	    PLUM_PCMCIA_IOWINCTRL_ZEROWAIT) << shift);
	plum_conf_write(regt, regh, PLUM_PCMCIA_IOWINCTRL, reg);
	
	/* Enable window */
	reg = plum_conf_read(regt, regh, PLUM_PCMCIA_WINEN);
	reg |= (win == 0 ? PLUM_PCMCIA_WINEN_IO0 :
	    PLUM_PCMCIA_WINEN_IO1);
	plum_conf_write(regt, regh, PLUM_PCMCIA_WINEN, reg);

	delay(100);
}

static void
plumpcmcia_chip_io_free(pcmcia_chipset_handle_t pch,
    struct pcmcia_io_handle *pcihp)
{
	if (pcihp->flags & PCMCIA_IO_ALLOCATED) {
		bus_space_free(pcihp->iot, pcihp->ioh, pcihp->size);
	} else {
		bus_space_unmap(pcihp->iot, pcihp->ioh, pcihp->size);
	}

	DPRINTF(("plumpcmcia_chip_io_free %#x+%#x\n", pcihp->ioh, 
	    (unsigned)pcihp->size));
}

static void
plumpcmcia_chip_io_unmap(pcmcia_chipset_handle_t pch, int window)
{
	struct plumpcmcia_handle *ph = (void*)pch;
	bus_space_tag_t regt = ph->ph_regt;
	bus_space_handle_t regh = ph->ph_regh;
	plumreg_t reg;
	
	reg = plum_conf_read(regt, regh, PLUM_PCMCIA_WINEN);
	switch (window) {
	default:
		panic("plumpcmcia_chip_io_unmap: bogus window");
	case 0:
		reg &= ~PLUM_PCMCIA_WINEN_IO0;
		break;
	case 1:
		reg &= ~PLUM_PCMCIA_WINEN_IO1;
		break;
	}
	plum_conf_write(regt, regh, PLUM_PCMCIA_WINEN, reg);
	ph->ph_ioalloc &= ~(1 << window);
}

static void
plumpcmcia_wait_ready(struct plumpcmcia_handle *ph)
{
	bus_space_tag_t regt = ph->ph_regt;
	bus_space_handle_t regh = ph->ph_regh;
	int i;

	for (i = 0; i < 10000; i++) {
		if ((plum_conf_read(regt, regh, PLUM_PCMCIA_STATUS) & 
		    PLUM_PCMCIA_STATUS_READY) &&
		    (plum_conf_read(regt, regh, PLUM_PCMCIA_STATUS) &
			PLUM_PCMCIA_STATUS_PWROK)) {
			return;
		}
		delay(500);

		if ((i > 5000) && (i % 100 == 99)) {
			printf(".");
		}
	}
	printf("plumpcmcia_wait_ready: failed\n");
}

static void
plumpcmcia_chip_socket_enable(pcmcia_chipset_handle_t pch)
{
	struct plumpcmcia_handle *ph = (void *)pch;
	bus_space_tag_t regt = ph->ph_regt;
	bus_space_handle_t regh = ph->ph_regh;
	plumreg_t reg, power;
	int win;

	/* this bit is mostly stolen from pcic_attach_card */

	/* set card type to memory to disable interrupts */
	reg = plum_conf_read(regt, regh, PLUM_PCMCIA_GENCTRL);
	reg &= ~PLUM_PCMCIA_GENCTRL_CARDTYPE_MASK;
	plum_conf_write(regt, regh, PLUM_PCMCIA_GENCTRL, reg);

	/* zero out the address windows */
	plum_conf_write(regt, regh, PLUM_PCMCIA_WINEN, 0);

	/* power down the socket to reset it, clear the card reset pin */
	plum_conf_write(regt, regh, PLUM_PCMCIA_PWRCTRL, 0);

	/* 
	 * wait 300ms until power fails (Tpf).  Then, wait 100ms since
	 * we are changing Vcc (Toff).
	 */
	delay((300 + 100) * 1000);

	/* 
	 *  power up the socket 
	 */
	/* detect voltage */
	reg = plum_conf_read(regt, regh, PLUM_PCMCIA_GENCTRL2);
	if ((reg & PLUM_PCMCIA_GENCTRL2_VCC5V) == 
	    PLUM_PCMCIA_GENCTRL2_VCC5V) {
		power = PLUM_PCMCIA_PWRCTRL_VCC_CTRLBIT1; /* 5V */
	} else {
		power = PLUM_PCMCIA_PWRCTRL_VCC_CTRLBIT0; /* 3.3V */
	}

	plum_conf_write(regt, regh, PLUM_PCMCIA_PWRCTRL, 
	    PLUM_PCMCIA_PWRCTRL_DISABLE_RESETDRV |
	    power |
	    PLUM_PCMCIA_PWRCTRL_PWR_ENABLE);
	
	/*
	 * wait 100ms until power raise (Tpr) and 20ms to become
	 * stable (Tsu(Vcc)).
	 *
	 * some machines require some more time to be settled
	 * (300ms is added here).
	 */
	delay((100 + 20 + 300) * 1000);

	plum_conf_write(regt, regh, PLUM_PCMCIA_PWRCTRL, 
	    PLUM_PCMCIA_PWRCTRL_DISABLE_RESETDRV |
	    power |
	    PLUM_PCMCIA_PWRCTRL_OE |
	    PLUM_PCMCIA_PWRCTRL_PWR_ENABLE);
	plum_conf_write(regt, regh, PLUM_PCMCIA_GENCTRL, 0);

	/*
	 * hold RESET at least 10us.
	 */
	delay(10);

	/* clear the reset flag */
	plum_conf_write(regt, regh, PLUM_PCMCIA_GENCTRL,
	    PLUM_PCMCIA_GENCTRL_RESET);

	/* wait 20ms as per pc card standard (r2.01) section 4.3.6 */

	delay(20000);

	/* wait for the chip to finish initializing */
	plumpcmcia_wait_ready(ph);

	/* reinstall all the memory and io mappings */
	for (win = 0; win < PLUM_PCMCIA_MEM_WINS; win++) {
		if (ph->ph_memalloc & (1 << win)) {
			plumpcmcia_chip_do_mem_map(ph, win);
		}
	}

	for (win = 0; win < PLUM_PCMCIA_IO_WINS; win++) {
		if (ph->ph_ioalloc & (1 << win)) {
			plumpcmcia_chip_do_io_map(ph, win);
		}
	}
}

static void
plumpcmcia_chip_socket_settype(pcmcia_chipset_handle_t pch, int type)
{
	struct plumpcmcia_handle *ph = (void *)pch;
	bus_space_tag_t regt = ph->ph_regt;
	bus_space_handle_t regh = ph->ph_regh;
	plumreg_t reg;

	/* set the card type */

	reg = plum_conf_read(regt, regh, PLUM_PCMCIA_GENCTRL);
	reg &= ~PLUM_PCMCIA_GENCTRL_CARDTYPE_MASK;
	if (type == PCMCIA_IFTYPE_IO)
		reg |= PLUM_PCMCIA_GENCTRL_CARDTYPE_IO;
	else
		reg |= PLUM_PCMCIA_GENCTRL_CARDTYPE_MEM;

	DPRINTF(("%s: plumpcmcia_chip_socket_enable type %s %02x\n",
	    ph->ph_parent->dv_xname, 
	    ((cardtype == PCMCIA_IFTYPE_IO) ? "io" : "mem"), reg));

	plum_conf_write(regt, regh, PLUM_PCMCIA_GENCTRL, reg);
}

static void
plumpcmcia_chip_socket_disable(pcmcia_chipset_handle_t pch)
{
	struct plumpcmcia_handle *ph = (void *)pch;
	bus_space_tag_t regt = ph->ph_regt;
	bus_space_handle_t regh = ph->ph_regh;
	plumreg_t reg;

	/* set card type to memory to disable interrupts */
	reg = plum_conf_read(regt, regh, PLUM_PCMCIA_GENCTRL);
	reg &= ~PLUM_PCMCIA_GENCTRL_CARDTYPE_MASK;
	plum_conf_write(regt, regh, PLUM_PCMCIA_GENCTRL, reg);

	/* zero out the address windows */
	plum_conf_write(regt, regh, PLUM_PCMCIA_WINEN, 0);

	/* power down the socket */
	plum_conf_write(regt, regh, PLUM_PCMCIA_PWRCTRL, 0);

	/*
	 * wait 300ms until power fails (Tpf).
	 */
	delay(300 * 1000);
}

static void
plum_csc_intr_setup(struct plumpcmcia_softc *sc, struct plumpcmcia_handle *ph,
    int irq)
{
	bus_space_tag_t regt = ph->ph_regt;
	bus_space_handle_t regh = ph->ph_regh;
	plumreg_t reg;
	void *ih;
	
	/* enable CARD DETECT ENABLE only */
	plum_conf_write(regt, regh, PLUM_PCMCIA_CSCINT,
	    PLUM_PCMCIA_CSCINT_CARD_DETECT);

	/* don't use explicit writeback csc interrupt status */
	reg = plum_conf_read(regt, regh, PLUM_PCMCIA_GLOBALCTRL);
	reg &= ~PLUM_PCMCIA_GLOBALCTRL_EXPLICIT_WB_CSC_INT;
	plum_conf_write(regt, regh, PLUM_PCMCIA_GLOBALCTRL, reg);
	
	/* install interrupt handler (don't fail) */
	ih = plum_intr_establish(sc->sc_pc, irq, IST_EDGE, IPL_TTY,
	    plum_csc_intr, ph);
	KASSERT(ih != 0);
}

static int
plum_csc_intr(void *arg)
{
	struct plumpcmcia_handle *ph = arg;
	struct plumpcmcia_softc *sc = (void *)ph->ph_parent;
	struct plumpcmcia_event *pe;
	bus_space_tag_t regt = ph->ph_regt;
	bus_space_handle_t regh = ph->ph_regh;
	plumreg_t reg;
	int flag;

	/* read and clear interrupt status */
	reg = plum_conf_read(regt, regh, PLUM_PCMCIA_CSCINT_STAT);
	if (reg & PLUM_PCMCIA_CSCINT_CARD_DETECT) {
		DPRINTF(("%s: card status change.\n", __func__));
	} else {
		DPRINTF(("%s: unhandled csc event. 0x%02x\n",
		    __func__, reg));
		return (0);
	}

	/* inquire card status (insert or remove) */
	reg = plum_conf_read(regt, regh, PLUM_PCMCIA_STATUS);
	reg &= (PLUM_PCMCIA_STATUS_CD1 | PLUM_PCMCIA_STATUS_CD2);
	if (reg == (PLUM_PCMCIA_STATUS_CD1 | PLUM_PCMCIA_STATUS_CD2)) {
		/* insert */
		flag = PLUM_PCMCIA_EVENT_INSERT;
	} else {
		/* remove */
		flag = PLUM_PCMCIA_EVENT_REMOVE;
	}

	/* queue event to event thread and wakeup. */
	pe = plumpcmcia_event_alloc();
	if (pe == 0) {
		printf("%s: event FIFO overflow (%d).\n", __func__,
		    PLUM_PCMCIA_EVENT_QUEUE_MAX);
		return (0);
	}
	pe->pe_type = flag;
	pe->pe_ph = ph;
	SIMPLEQ_INSERT_TAIL(&sc->sc_event_head, pe, pe_link);
	wakeup(sc);

	return (0);
}

static struct plumpcmcia_event *
plumpcmcia_event_alloc()
{
	int i;
	/* I assume called from interrupt context only. so don't lock */
	for (i = 0; i < PLUM_PCMCIA_EVENT_QUEUE_MAX; i++) {
		if (!__event_queue_pool[i].__queued) {
			__event_queue_pool[i].__queued = 1;
			return (&__event_queue_pool[i]);
		}
	}
	return NULL;
}

static void
plumpcmcia_event_free(struct plumpcmcia_event *pe)
{
	/* I assume context is already locked */
	pe->__queued = 0;
}

static void
plumpcmcia_event_thread(void *arg)
{
	struct plumpcmcia_softc *sc = arg;
	struct plumpcmcia_event *pe;
	int s;
	
	while (/*CONSTCOND*/1) { /* XXX shutdown. -uch */
		tsleep(sc, PWAIT, "CSC wait", 0);
		s = spltty();
		while ((pe = SIMPLEQ_FIRST(&sc->sc_event_head))) {
			splx(s);
			switch (pe->pe_type) {
			default:
				printf("%s: unknown event.\n", __func__);
				break;
			case PLUM_PCMCIA_EVENT_INSERT:
				DPRINTF(("%s: insert event.\n", __func__));
				pcmcia_card_attach(pe->pe_ph->ph_pcmcia);
				break;
			case PLUM_PCMCIA_EVENT_REMOVE:
				DPRINTF(("%s: remove event.\n", __func__));
				pcmcia_card_detach(pe->pe_ph->ph_pcmcia,
				    DETACH_FORCE);
				break;
			}
			s = spltty();
			SIMPLEQ_REMOVE_HEAD(&sc->sc_event_head, pe_link);
			plumpcmcia_event_free(pe);
		}
		splx(s);
	}
	/* NOTREACHED */
}

/* power XXX notyet */
int
plumpcmcia_power(void *ctx, int type, long id, void *msg)
{
	struct plumpcmcia_softc *sc = ctx;
	bus_space_tag_t regt = sc->sc_regt;
	bus_space_handle_t regh = sc->sc_regh;
	int why = (int)msg;

	switch (why) {
	case PWR_RESUME:
		DPRINTF(("%s: ON\n", sc->sc_dev.dv_xname));
		/* power on */
		plum_conf_write(regt, regh, PLUM_PCMCIA_CARDPWRCTRL,
		    PLUM_PCMCIA_CARDPWRCTRL_ON);
		break;
	case PWR_SUSPEND:
		/* FALLTHROUGH */
	case PWR_STANDBY:
		plum_conf_write(regt, regh, PLUM_PCMCIA_CARDPWRCTRL,
		    PLUM_PCMCIA_CARDPWRCTRL_OFF);
		DPRINTF(("%s: OFF\n", sc->sc_dev.dv_xname));
		break;
	}

	return (0);
}

#ifdef PLUMPCMCIA_DEBUG
static void
__ioareadump(plumreg_t reg)
{

	if (reg & PLUM_PCMCIA_IOWINADDRCTRL_AREA2) {
		printf("I/O Area 2\n");
	} else {
		printf("I/O Area 1\n");
	}
}

static void
__memareadump(plumreg_t reg)
{
	int maparea;

	maparea = PLUM_PCMCIA_MEMWINCTRL_MAP(reg);
	switch (maparea) {
	case PLUM_PCMCIA_MEMWINCTRL_MAP_AREA1:
		printf("MEM Area1\n");
		break;
	case PLUM_PCMCIA_MEMWINCTRL_MAP_AREA2:
		printf("MEM Area2\n");
		break;
	case PLUM_PCMCIA_MEMWINCTRL_MAP_AREA3:
		printf("MEM Area3\n");
		break;
	case PLUM_PCMCIA_MEMWINCTRL_MAP_AREA4:
		printf("MEM Area4\n");
		break;
	}
}	

static void
plumpcmcia_dump(struct plumpcmcia_softc *sc)
{
	bus_space_tag_t regt = sc->sc_regt;
	bus_space_handle_t regh = sc->sc_regh;
	plumreg_t reg;

	int i, j;
	
	__memareadump(plum_conf_read(regt, regh, PLUM_PCMCIA_MEMWIN0CTRL));
	__memareadump(plum_conf_read(regt, regh, PLUM_PCMCIA_MEMWIN1CTRL));
	__memareadump(plum_conf_read(regt, regh, PLUM_PCMCIA_MEMWIN2CTRL));
	__memareadump(plum_conf_read(regt, regh, PLUM_PCMCIA_MEMWIN3CTRL));
	__memareadump(plum_conf_read(regt, regh, PLUM_PCMCIA_MEMWIN4CTRL));

	__ioareadump(plum_conf_read(regt, regh, PLUM_PCMCIA_IOWIN0ADDRCTRL));
	__ioareadump(plum_conf_read(regt, regh, PLUM_PCMCIA_IOWIN1ADDRCTRL));

	for (j = 0; j < 2; j++) {
		printf("[slot %d]\n", j);
		for (i = 0; i < 0x120; i += 4) {
			reg = plum_conf_read(sc->sc_regt, sc->sc_regh,
			    i + 0x800 * j);
			printf("%03x %08x", i, reg);
			dbg_bit_print(reg);
		}
	}
	printf("\n");
}
#endif /* PLUMPCMCIA_DEBUG */
