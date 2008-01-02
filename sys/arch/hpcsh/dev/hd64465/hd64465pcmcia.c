/*	$NetBSD: hd64465pcmcia.c,v 1.23.8.1 2008/01/02 21:48:03 bouyer Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hd64465pcmcia.c,v 1.23.8.1 2008/01/02 21:48:03 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kthread.h>
#include <sys/boot_flag.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciachip.h>

#include <sh3/bscreg.h>
#include <sh3/mmu.h>

#include <hpcsh/dev/hd64465/hd64465reg.h>
#include <hpcsh/dev/hd64465/hd64465var.h>
#include <hpcsh/dev/hd64465/hd64465intcreg.h>
#include <hpcsh/dev/hd64461/hd64461pcmciareg.h>

#include "locators.h"

#ifdef	HD64465PCMCIA_DEBUG
#define	DPRINTF_ENABLE
#define	DPRINTF_DEBUG	hd64465pcmcia_debug
#endif
#include <machine/debug.h>

enum memory_window_16 {
	MEMWIN_16M_COMMON_0,
	MEMWIN_16M_COMMON_1,
	MEMWIN_16M_COMMON_2,
	MEMWIN_16M_COMMON_3,
};
#define	MEMWIN_16M_MAX	4

enum hd64465pcmcia_event_type {
	EVENT_NONE,
	EVENT_INSERT,
	EVENT_REMOVE,
};
#define	EVENT_QUEUE_MAX		5

struct hd64465pcmcia_softc; /* forward declaration */

struct hd64465pcmcia_window_cookie {
	bus_space_tag_t wc_tag;
	bus_space_handle_t wc_handle;
	int wc_size;
	int wc_window;
};

struct hd64465pcmcia_channel {
	struct hd64465pcmcia_softc *ch_parent;
	struct device *ch_pcmcia;
	int ch_channel;

	/* memory space */
	bus_space_tag_t ch_memt;
	bus_space_handle_t ch_memh;
	bus_addr_t ch_membase_addr;
	bus_size_t ch_memsize;
	bus_space_tag_t ch_cmemt[MEMWIN_16M_MAX];

	/* I/O space */
	bus_space_tag_t ch_iot;
	bus_addr_t ch_iobase;
	bus_size_t ch_iosize;

	/* card interrupt */
	int (*ch_ih_card_func)(void *);
	void *ch_ih_card_arg;
	int ch_attached;
};

struct hd64465pcmcia_event {
	int __queued;
	enum hd64465pcmcia_event_type pe_type;
	struct hd64465pcmcia_channel *pe_ch;
	SIMPLEQ_ENTRY(hd64465pcmcia_event) pe_link;
};

struct hd64465pcmcia_softc {
	struct device sc_dev;
	enum hd64465_module_id sc_module_id;
	int sc_shutdown;

	/* kv mapped Area 5, 6 */
	vaddr_t sc_area5;
	vaddr_t sc_area6;

	/* CSC event */
	lwp_t *sc_event_thread;
	struct hd64465pcmcia_event sc_event_pool[EVENT_QUEUE_MAX];
	SIMPLEQ_HEAD (, hd64465pcmcia_event) sc_event_head;

	struct hd64465pcmcia_channel sc_ch[2];
};

STATIC int hd64465pcmcia_chip_mem_alloc(pcmcia_chipset_handle_t, bus_size_t,
    struct pcmcia_mem_handle *);
STATIC void hd64465pcmcia_chip_mem_free(pcmcia_chipset_handle_t,
    struct pcmcia_mem_handle *);
STATIC int hd64465pcmcia_chip_mem_map(pcmcia_chipset_handle_t, int, bus_addr_t,
    bus_size_t, struct pcmcia_mem_handle *, bus_size_t *, int *);
STATIC void hd64465pcmcia_chip_mem_unmap(pcmcia_chipset_handle_t, int);
STATIC int hd64465pcmcia_chip_io_alloc(pcmcia_chipset_handle_t, bus_addr_t,
    bus_size_t, bus_size_t, struct pcmcia_io_handle *);
STATIC void hd64465pcmcia_chip_io_free(pcmcia_chipset_handle_t,
    struct pcmcia_io_handle *);
STATIC int hd64465pcmcia_chip_io_map(pcmcia_chipset_handle_t, int, bus_addr_t,
    bus_size_t, struct pcmcia_io_handle *, int *);
STATIC void hd64465pcmcia_chip_io_unmap(pcmcia_chipset_handle_t, int);
STATIC void hd64465pcmcia_chip_socket_enable(pcmcia_chipset_handle_t);
STATIC void hd64465pcmcia_chip_socket_disable(pcmcia_chipset_handle_t);
STATIC void hd64465pcmcia_chip_socket_settype(pcmcia_chipset_handle_t, int);
STATIC void *hd64465pcmcia_chip_intr_establish(pcmcia_chipset_handle_t,
    struct pcmcia_function *, int, int (*)(void *), void *);
STATIC void hd64465pcmcia_chip_intr_disestablish(pcmcia_chipset_handle_t,
    void *);

STATIC struct pcmcia_chip_functions hd64465pcmcia_functions = {
	hd64465pcmcia_chip_mem_alloc,
	hd64465pcmcia_chip_mem_free,
	hd64465pcmcia_chip_mem_map,
	hd64465pcmcia_chip_mem_unmap,
	hd64465pcmcia_chip_io_alloc,
	hd64465pcmcia_chip_io_free,
	hd64465pcmcia_chip_io_map,
	hd64465pcmcia_chip_io_unmap,
	hd64465pcmcia_chip_intr_establish,
	hd64465pcmcia_chip_intr_disestablish,
	hd64465pcmcia_chip_socket_enable,
	hd64465pcmcia_chip_socket_disable,
	hd64465pcmcia_chip_socket_settype,
};

STATIC int hd64465pcmcia_match(struct device *, struct cfdata *, void *);
STATIC void hd64465pcmcia_attach(struct device *, struct device *, void *);
STATIC int hd64465pcmcia_print(void *, const char *);
STATIC int hd64465pcmcia_submatch(struct device *, struct cfdata *,
				  const int *, void *);

CFATTACH_DECL(hd64465pcmcia, sizeof(struct hd64465pcmcia_softc),
    hd64465pcmcia_match, hd64465pcmcia_attach, NULL, NULL);

STATIC void hd64465pcmcia_attach_channel(struct hd64465pcmcia_softc *, int);
/* hot plug */
STATIC void hd64465pcmcia_event_thread(void *);
STATIC void __queue_event(struct hd64465pcmcia_channel *,
    enum hd64465pcmcia_event_type);
/* interrupt handler */
STATIC int hd64465pcmcia_intr(void *);
/* card status */
STATIC enum hd64465pcmcia_event_type __detect_card(int);
STATIC void hd64465pcmcia_memory_window16_switch(int,  enum memory_window_16);
/* bus width */
STATIC void __sh_set_bus_width(int, int);
/* bus space access */
STATIC int __sh_hd64465_map(vaddr_t, paddr_t, size_t, uint32_t);
STATIC vaddr_t __sh_hd64465_map_2page(paddr_t);

#define	DELAY_MS(x)	delay((x) * 1000)

int
hd64465pcmcia_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct hd64465_attach_args *ha = aux;

	return (ha->ha_module_id == HD64465_MODULE_PCMCIA);
}

void
hd64465pcmcia_attach(struct device *parent, struct device *self, void *aux)
{
	struct hd64465_attach_args *ha = aux;
	struct hd64465pcmcia_softc *sc = (struct hd64465pcmcia_softc *)self;
	int error;

	sc->sc_module_id = ha->ha_module_id;

	printf("\n");

	sc->sc_area5 = __sh_hd64465_map_2page(0x14000000); /* area 5 */
	sc->sc_area6 = __sh_hd64465_map_2page(0x18000000); /* area 6 */

	if (sc->sc_area5 == 0 || sc->sc_area6 == 0) {
		printf("%s: can't map memory.\n", sc->sc_dev.dv_xname);
		if (sc->sc_area5)
			uvm_km_free(kernel_map, sc->sc_area5, 0x03000000,
			    UVM_KMF_VAONLY);
		if (sc->sc_area6)
			uvm_km_free(kernel_map, sc->sc_area6, 0x03000000,
			    UVM_KMF_VAONLY);

		return;
	}

	/* Channel 0/1 common CSC event queue */
	SIMPLEQ_INIT (&sc->sc_event_head);

	error = kthread_create(PRI_NONE, 0, NULL, hd64465pcmcia_event_thread,
		sc, &sc->sc_event_thread, "%s", sc->sc_dev.dv_xname);
	KASSERT(error == 0);

	hd64465pcmcia_attach_channel(sc, 0);
	hd64465pcmcia_attach_channel(sc, 1);
}

void
hd64465pcmcia_event_thread(void *arg)
{
	struct hd64465pcmcia_softc *sc = arg;
	struct hd64465pcmcia_event *pe;
	int s;

	while (!sc->sc_shutdown) {
		tsleep(sc, PWAIT, "CSC wait", 0);
		s = splhigh();
		while ((pe = SIMPLEQ_FIRST(&sc->sc_event_head))) {
			splx(s);
			switch (pe->pe_type) {
			default:
				printf("%s: unknown event.\n", __func__);
				break;
			case EVENT_INSERT:
				DPRINTF("insert event.\n");
				pcmcia_card_attach(pe->pe_ch->ch_pcmcia);
				break;
			case EVENT_REMOVE:
				DPRINTF("remove event.\n");
				pcmcia_card_detach(pe->pe_ch->ch_pcmcia,
				    DETACH_FORCE);
				break;
			}
			s = splhigh();
			SIMPLEQ_REMOVE_HEAD(&sc->sc_event_head, pe_link);
			pe->__queued = 0;
		}
		splx(s);
	}
	/* NOTREACHED */
}

int
hd64465pcmcia_print(void *arg, const char *pnp)
{

	if (pnp)
		aprint_normal("pcmcia at %s", pnp);

	return (UNCONF);
}

int
hd64465pcmcia_submatch(struct device *parent, struct cfdata *cf,
		       const int *ldesc, void *aux)
{
	struct pcmciabus_attach_args *paa = aux;
	struct hd64465pcmcia_channel *ch =
	    (struct hd64465pcmcia_channel *)paa->pch;

	if (ch->ch_channel == 0) {
		if (cf->cf_loc[PCMCIABUSCF_CONTROLLER] !=
		    PCMCIABUSCF_CONTROLLER_DEFAULT &&
		    cf->cf_loc[PCMCIABUSCF_CONTROLLER] != 0)
			return 0;
	} else {
		if (cf->cf_loc[PCMCIABUSCF_CONTROLLER] !=
		    PCMCIABUSCF_CONTROLLER_DEFAULT &&
		    cf->cf_loc[PCMCIABUSCF_CONTROLLER] != 1)
			return 0;
	}
	paa->pct = (pcmcia_chipset_tag_t)&hd64465pcmcia_functions;

	return (config_match(parent, cf, aux));
}

void
hd64465pcmcia_attach_channel(struct hd64465pcmcia_softc *sc, int channel)
{
	struct device *parent = (struct device *)sc;
	struct hd64465pcmcia_channel *ch = &sc->sc_ch[channel];
	struct pcmciabus_attach_args paa;
	bus_addr_t baseaddr;
	uint8_t r;
	int i;

	ch->ch_parent = sc;
	ch->ch_channel = channel;

	/*
	 * Continuous 16-MB Area Mode
	 */
	/* set Continuous 16-MB Area Mode */
	r = hd64465_reg_read_1(HD64461_PCCGCR(channel));
	r &= ~HD64461_PCCGCR_MMOD;
	r |= HD64461_PCCGCR_MMOD_16M;
	hd64465_reg_write_1(HD64461_PCCGCR(channel), r);

	/* Attibute/Common memory extent */
	baseaddr = (channel == 0) ? sc->sc_area6 : sc->sc_area5;

	ch->ch_memt = bus_space_create(0, "PCMCIA attribute memory",
	    baseaddr, 0x01000000); /* 16MB */
	bus_space_alloc(ch->ch_memt, 0, 0x00ffffff, 0x0001000,
	    0x1000, 0x1000, 0, &ch->ch_membase_addr, &ch->ch_memh);

	/* Common memory space extent */
	ch->ch_memsize = 0x01000000;
	for (i = 0; i < MEMWIN_16M_MAX; i++) {
		ch->ch_cmemt[i] = bus_space_create(0, "PCMCIA common memory",
		    baseaddr + 0x01000000, ch->ch_memsize);
	}

	/* I/O port extent */
	ch->ch_iobase = 0;
	ch->ch_iosize = 0x01000000;
	ch->ch_iot = bus_space_create(0, "PCMCIA I/O port",
	    baseaddr + 0x01000000 * 2, ch->ch_iosize);

	/* Interrupt */
	hd64465_intr_establish(channel ? HD64465_PCC1 : HD64465_PCC0,
	    IST_LEVEL, IPL_TTY, hd64465pcmcia_intr, ch);

	paa.paa_busname = "pcmcia";
	paa.pch = (pcmcia_chipset_handle_t)ch;
	paa.iobase = ch->ch_iobase;
	paa.iosize = ch->ch_iosize;

	ch->ch_pcmcia = config_found_sm_loc(parent, "pcmciabus", NULL, &paa,
	    hd64465pcmcia_print, hd64465pcmcia_submatch);

	if (ch->ch_pcmcia && (__detect_card(ch->ch_channel) == EVENT_INSERT)) {
		ch->ch_attached = 1;
		pcmcia_card_attach(ch->ch_pcmcia);
	}
}

int
hd64465pcmcia_intr(void *arg)
{
	struct hd64465pcmcia_channel *ch = (struct hd64465pcmcia_channel *)arg;
	uint32_t cscr;
	uint8_t r;
	int ret = 0;

	cscr = HD64461_PCCCSCR(ch->ch_channel);
	r = hd64465_reg_read_1(cscr);

	/* clear interrtupt (don't change power switch select) */
	hd64465_reg_write_1(cscr, r & ~0x40);

	if (r & (0x60 | 0x04/* for memory mapped mode*/)) {
		if (ch->ch_ih_card_func) {
			ret = (*ch->ch_ih_card_func)(ch->ch_ih_card_arg);
		} else {
			DPRINTF("spurious IREQ interrupt.\n");
		}
	}

	if (r & HD64461_PCC0CSCR_P0CDC)
		__queue_event(ch, __detect_card(ch->ch_channel));

	return (ret);
}

void
__queue_event(struct hd64465pcmcia_channel *ch,
    enum hd64465pcmcia_event_type type)
{
	struct hd64465pcmcia_event *pe, *pool;
	struct hd64465pcmcia_softc *sc = ch->ch_parent;
	int i;
	int s = splhigh();

	if (type == EVENT_NONE)
		goto out;

	pe = 0;
	pool = sc->sc_event_pool;
	for (i = 0; i < EVENT_QUEUE_MAX; i++) {
		if (!pool[i].__queued) {
			pe = &pool[i];
			break;
		}
	}

	if (pe == 0) {
		printf("%s: event FIFO overflow (max %d).\n", __func__,
		    EVENT_QUEUE_MAX);
		goto out;
	}

	if ((ch->ch_attached && (type == EVENT_INSERT)) ||
	    (!ch->ch_attached && (type == EVENT_REMOVE))) {
		DPRINTF("spurious CSC interrupt.\n");
		goto out;
	}

	ch->ch_attached = (type == EVENT_INSERT);
	pe->__queued = 1;
	pe->pe_type = type;
	pe->pe_ch = ch;
	SIMPLEQ_INSERT_TAIL(&sc->sc_event_head, pe, pe_link);
	wakeup(sc);
 out:
	splx(s);
}

/*
 * Interface for pcmcia driver.
 */
/*
 * Interrupt.
 */
void *
hd64465pcmcia_chip_intr_establish(pcmcia_chipset_handle_t pch,
    struct pcmcia_function *pf, int ipl, int (*ih_func)(void *), void *ih_arg)
{
	struct hd64465pcmcia_channel *ch = (struct hd64465pcmcia_channel *)pch;
	int channel = ch->ch_channel;
	bus_addr_t cscier = HD64461_PCCCSCIER(channel);
	uint8_t r;
	int s = splhigh();

	hd6446x_intr_priority(ch->ch_channel == 0 ? HD64465_PCC0 : HD64465_PCC1,
	    ipl);

	ch->ch_ih_card_func = ih_func;
	ch->ch_ih_card_arg = ih_arg;

	/* Enable card interrupt */
	r = hd64465_reg_read_1(cscier);
	/* set level mode */
	r &= ~HD64461_PCC0CSCIER_P0IREQE_MASK;
	r |= HD64461_PCC0CSCIER_P0IREQE_LEVEL;
	hd64465_reg_write_1(cscier, r);

	splx(s);

	return (void *)ih_func;
}

void
hd64465pcmcia_chip_intr_disestablish(pcmcia_chipset_handle_t pch, void *ih)
{
	struct hd64465pcmcia_channel *ch = (struct hd64465pcmcia_channel *)pch;
	int channel = ch->ch_channel;
	bus_addr_t cscier = HD64461_PCCCSCIER(channel);
	int s = splhigh();
	uint8_t r;

	hd6446x_intr_priority(ch->ch_channel == 0 ? HD64465_PCC0 : HD64465_PCC1,
	    IPL_TTY);

	/* Disable card interrupt */
	r = hd64465_reg_read_1(cscier);
	r &= ~HD64461_PCC0CSCIER_P0IREQE_MASK;
	r |= HD64461_PCC0CSCIER_P0IREQE_NONE;
	hd64465_reg_write_1(cscier, r);

	ch->ch_ih_card_func = 0;

	splx(s);
}

/*
 * Bus resources.
 */
int
hd64465pcmcia_chip_mem_alloc(pcmcia_chipset_handle_t pch, bus_size_t size,
    struct pcmcia_mem_handle *pcmhp)
{
	struct hd64465pcmcia_channel *ch = (struct hd64465pcmcia_channel *)pch;

	pcmhp->memt = ch->ch_memt;
	pcmhp->addr = ch->ch_membase_addr;
	pcmhp->memh = ch->ch_memh;
	pcmhp->size = size;
	pcmhp->realsize = size;

	DPRINTF("base 0x%08lx size %#lx\n", pcmhp->addr, size);

	return (0);
}

void
hd64465pcmcia_chip_mem_free(pcmcia_chipset_handle_t pch,
    struct pcmcia_mem_handle *pcmhp)
{
	/* NO-OP */
}

int
hd64465pcmcia_chip_mem_map(pcmcia_chipset_handle_t pch, int kind,
    bus_addr_t card_addr, bus_size_t size, struct pcmcia_mem_handle *pcmhp,
    bus_size_t *offsetp, int *windowp)
{
	struct hd64465pcmcia_channel *ch = (struct hd64465pcmcia_channel *)pch;
	struct hd64465pcmcia_window_cookie *cookie;
	bus_addr_t ofs;

	cookie = malloc(sizeof(struct hd64465pcmcia_window_cookie),
	    M_DEVBUF, M_NOWAIT);
	KASSERT(cookie);
	memset(cookie, 0, sizeof(struct hd64465pcmcia_window_cookie));

	/* Address */
	if ((kind & ~PCMCIA_WIDTH_MEM_MASK) == PCMCIA_MEM_ATTR) {
		cookie->wc_tag = ch->ch_memt;
		if (bus_space_subregion(ch->ch_memt, ch->ch_memh, card_addr,
		    size, &cookie->wc_handle) != 0)
			goto bad;

		*offsetp = card_addr;
		cookie->wc_window = -1;
	} else {
		int window = card_addr / ch->ch_memsize;
		KASSERT(window < MEMWIN_16M_MAX);

		cookie->wc_tag = ch->ch_cmemt[window];
		ofs = card_addr - window * ch->ch_memsize;
		if (bus_space_map(cookie->wc_tag, ofs, size, 0,
		    &cookie->wc_handle) != 0)
			goto bad;

		/* XXX bogus. check window per common memory access. */
		hd64465pcmcia_memory_window16_switch(ch->ch_channel, window);
		*offsetp = ofs + 0x01000000; /* skip attribute area */
		cookie->wc_window = window;
	}
	cookie->wc_size = size;
	*windowp = (int)cookie;

	DPRINTF("(%s) %#lx+%#lx-> %#lx+%#lx\n", kind == PCMCIA_MEM_ATTR ?
	    "attribute" : "common", ch->ch_memh, card_addr, *offsetp, size);

	return (0);
 bad:
	DPRINTF("%#lx-%#lx map failed.\n", card_addr, size);
	free(cookie, M_DEVBUF);

	return (1);
}

void
hd64465pcmcia_chip_mem_unmap(pcmcia_chipset_handle_t pch, int window)
{
	struct hd64465pcmcia_window_cookie *cookie = (void *)window;

	if (cookie->wc_window != -1)
		bus_space_unmap(cookie->wc_tag, cookie->wc_handle,
		    cookie->wc_size);
	DPRINTF("%#lx-%#x\n", cookie->wc_handle, cookie->wc_size);
	free(cookie, M_DEVBUF);
}

int
hd64465pcmcia_chip_io_alloc(pcmcia_chipset_handle_t pch, bus_addr_t start,
    bus_size_t size, bus_size_t align, struct pcmcia_io_handle *pcihp)
{
	struct hd64465pcmcia_channel *ch = (struct hd64465pcmcia_channel *)pch;

	if (start) {
		if (bus_space_map(ch->ch_iot, start, size, 0, &pcihp->ioh)) {
			DPRINTF("couldn't map %#lx+%#lx\n", start, size);
			return (1);
		}
		pcihp->addr = pcihp->ioh;
		DPRINTF("map %#lx+%#lx\n", start, size);
	} else {
		if (bus_space_alloc(ch->ch_iot, ch->ch_iobase,
		    ch->ch_iobase + ch->ch_iosize - 1,
		    size, align, 0, 0, &pcihp->addr, &pcihp->ioh)) {
			DPRINTF("couldn't allocate %#lx\n", size);
			return (1);
		}
		pcihp->flags = PCMCIA_IO_ALLOCATED;
	}
	DPRINTF("%#lx from %#lx\n", size, pcihp->addr);

	pcihp->iot = ch->ch_iot;
	pcihp->size = size;

	return (0);
}

int
hd64465pcmcia_chip_io_map(pcmcia_chipset_handle_t pch, int width,
    bus_addr_t offset, bus_size_t size, struct pcmcia_io_handle *pcihp,
    int *windowp)
{
	struct hd64465pcmcia_channel *ch = (struct hd64465pcmcia_channel *)pch;
#ifdef HD64465PCMCIA_DEBUG
	static const char *width_names[] = { "auto", "io8", "io16" };
#endif

	__sh_set_bus_width(ch->ch_channel, width);

	DPRINTF("%#lx:%#lx+%#lx %s\n", pcihp->ioh, offset, size,
	    width_names[width]);

	return (0);
}

void
hd64465pcmcia_chip_io_free(pcmcia_chipset_handle_t pch,
    struct pcmcia_io_handle *pcihp)
{

	if (pcihp->flags & PCMCIA_IO_ALLOCATED)
		bus_space_free(pcihp->iot, pcihp->ioh, pcihp->size);
	else
		bus_space_unmap(pcihp->iot, pcihp->ioh, pcihp->size);

	DPRINTF("%#lx+%#lx\n", pcihp->ioh, pcihp->size);
}

void
hd64465pcmcia_chip_io_unmap(pcmcia_chipset_handle_t pch, int window)
{
	/* nothing to do */
}

/*
 * Enable/Disable
 */
void
hd64465pcmcia_chip_socket_enable(pcmcia_chipset_handle_t pch)
{
	struct hd64465pcmcia_channel *ch = (struct hd64465pcmcia_channel *)pch;
	int channel = ch->ch_channel;
	bus_addr_t gcr;
	uint8_t r;

	DPRINTF("enable channel %d\n", channel);
	gcr = HD64461_PCCGCR(channel);

	r = hd64465_reg_read_1(gcr);
	r &= ~HD64461_PCC0GCR_P0PCCT;
	hd64465_reg_write_1(gcr, r);

	/* Set Common memory area #0. */
	hd64465pcmcia_memory_window16_switch(channel, MEMWIN_16M_COMMON_0);

	DPRINTF("OK.\n");
}

void
hd64465pcmcia_chip_socket_settype(pcmcia_chipset_handle_t pch, int type)
{
	struct hd64465pcmcia_channel *ch = (struct hd64465pcmcia_channel *)pch;
	int channel = ch->ch_channel;
	bus_addr_t gcr;
	uint8_t r;

	DPRINTF("settype channel %d\n", channel);
	gcr = HD64461_PCCGCR(channel);

	/* Set the card type */
	r = hd64465_reg_read_1(gcr);
	if (type == PCMCIA_IFTYPE_IO)
		r |= HD64461_PCC0GCR_P0PCCT;
	else
		r &= ~HD64461_PCC0GCR_P0PCCT;
	hd64465_reg_write_1(gcr, r);

	DPRINTF("OK.\n");
}

void
hd64465pcmcia_chip_socket_disable(pcmcia_chipset_handle_t pch)
{
	struct hd64465pcmcia_channel *ch = (struct hd64465pcmcia_channel *)pch;
	int channel = ch->ch_channel;

	/* dont' disable CSC interrupt */
	hd64465_reg_write_1(HD64461_PCCCSCIER(channel), HD64461_PCCCSCIER_CDE);
	hd64465_reg_write_1(HD64461_PCCCSCR(channel), 0);
}

/*
 * Card detect
 */
enum hd64465pcmcia_event_type
__detect_card(int channel)
{
	uint8_t r;

	r = hd64465_reg_read_1(HD64461_PCCISR(channel)) &
	    (HD64461_PCCISR_CD2 | HD64461_PCCISR_CD1);

	if (r == (HD64461_PCCISR_CD2 | HD64461_PCCISR_CD1)) {
		DPRINTF("remove\n");
		return EVENT_REMOVE;
	}
	if (r == 0) {
		DPRINTF("insert\n");
		return EVENT_INSERT;
	}
	DPRINTF("transition\n");

	return (EVENT_NONE);
}

/*
 * Memory window access ops.
 */
void
hd64465pcmcia_memory_window16_switch(int channel, enum memory_window_16 window)
{
	bus_addr_t a = HD64461_PCCGCR(channel);
	uint8_t r;

	r = hd64465_reg_read_1(a);
	r &= ~(HD64461_PCCGCR_PA25 | HD64461_PCCGCR_PA24);

	switch (window) {
	case MEMWIN_16M_COMMON_0:
		break;
	case MEMWIN_16M_COMMON_1:
		r |= HD64461_PCCGCR_PA24;
		break;
	case MEMWIN_16M_COMMON_2:
		r |= HD64461_PCCGCR_PA25;
		break;
	case MEMWIN_16M_COMMON_3:
		r |= (HD64461_PCCGCR_PA25 | HD64461_PCCGCR_PA24);
		break;
	}

	hd64465_reg_write_1(a, r);
}

/*
 * SH interface.
 */
void
__sh_set_bus_width(int channel, int width)
{
	uint16_t r16;

	r16 = _reg_read_2(SH4_BCR2);
#ifdef HD64465PCMCIA_DEBUG
	dbg_bit_print_msg(r16, "BCR2");
#endif
	if (channel == 0) {
		r16 &= ~((1 << 13)|(1 << 12));
		r16 |= 1 << (width == PCMCIA_WIDTH_IO8 ? 12 : 13);
	} else {
		r16 &= ~((1 << 11)|(1 << 10));
		r16 |= 1 << (width == PCMCIA_WIDTH_IO8 ? 10 : 11);
	}
	_reg_write_2(SH4_BCR2, r16);
}

vaddr_t
__sh_hd64465_map_2page(paddr_t pa)
{
	static const uint32_t mode[] =
	{ _PG_PCMCIA_ATTR16, _PG_PCMCIA_MEM16, _PG_PCMCIA_IO };
	vaddr_t va, v;
	int i;

	/* allocate kernel virtual */
	v = va = uvm_km_alloc(kernel_map, 0x03000000, 0, UVM_KMF_VAONLY);
	if (va == 0) {
		PRINTF("can't allocate virtual for paddr 0x%08x\n",
		    (unsigned)pa);

		return (0);
	}

 	/* map to physical addreess with specified memory type. */
	for (i = 0; i < 3; i++, pa += 0x01000000, va += 0x01000000) {
		if (__sh_hd64465_map(va, pa, 0x2000, mode[i]) != 0) {
			pmap_kremove(v, 0x03000000);
			uvm_km_free(kernel_map, v, 0x03000000, UVM_KMF_VAONLY);
			return (0);
		}
	}

	return (v);
}

int
__sh_hd64465_map(vaddr_t va, paddr_t pa, size_t sz, uint32_t flags)
{
	pt_entry_t *pte;
	paddr_t epa;

	KDASSERT(((pa & PAGE_MASK) == 0) && ((va & PAGE_MASK) == 0) &&
	    ((sz & PAGE_MASK) == 0));

	epa = pa + sz;
	while (pa < epa) {
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE);
		pte = __pmap_kpte_lookup(va);
		KDASSERT(pte);
		*pte |= flags;  /* PTEA PCMCIA assistant bit */
		sh_tlb_update(0, va, *pte);
		pa += PAGE_SIZE;
		va += PAGE_SIZE;
	}

	return (0);
}
