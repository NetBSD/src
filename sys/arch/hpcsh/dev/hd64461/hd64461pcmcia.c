/*	$NetBSD: hd64461pcmcia.c,v 1.22 2003/07/15 02:29:37 lukem Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: hd64461pcmcia.c,v 1.22 2003/07/15 02:29:37 lukem Exp $");

#include "debug_hpcsh.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kthread.h>
#include <sys/boot_flag.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciachip.h>

#include <sh3/bscreg.h>

#include <hpcsh/dev/hd64461/hd64461reg.h>
#include <hpcsh/dev/hd64461/hd64461var.h>
#include <hpcsh/dev/hd64461/hd64461intcreg.h>
#include <hpcsh/dev/hd64461/hd64461gpioreg.h>
#include <hpcsh/dev/hd64461/hd64461pcmciavar.h>
#include <hpcsh/dev/hd64461/hd64461pcmciareg.h>

#include "locators.h"

#ifdef	HD64461PCMCIA_DEBUG
#define DPRINTF_ENABLE
#define DPRINTF_DEBUG	hd64461pcmcia_debug
#endif
#include <machine/debug.h>

enum controller_channel {
	CHANNEL_0 = 0,
	CHANNEL_1 = 1,
	CHANNEL_MAX = 2
};

enum memory_window_mode {
	MEMWIN_16M_MODE,
	MEMWIN_32M_MODE
};

enum memory_window_16 {
	MEMWIN_16M_COMMON_0,
	MEMWIN_16M_COMMON_1,
	MEMWIN_16M_COMMON_2,
	MEMWIN_16M_COMMON_3,
};
#define MEMWIN_16M_MAX	4

enum memory_window_32 {
	MEMWIN_32M_ATTR,
	MEMWIN_32M_COMMON_0,
	MEMWIN_32M_COMMON_1,
};
#define MEMWIN_32M_MAX	3

enum hd64461pcmcia_event_type {
	EVENT_NONE,
	EVENT_INSERT,
	EVENT_REMOVE,
};
#define EVENT_QUEUE_MAX		5

struct hd64461pcmcia_softc; /* forward declaration */

struct hd64461pcmcia_window_cookie {
	bus_space_tag_t wc_tag;
	bus_space_handle_t wc_handle;
	int wc_size;
	int wc_window;
};

struct hd64461pcmcia_channel {
	struct hd64461pcmcia_softc *ch_parent;
	struct device *ch_pcmcia;
	enum controller_channel ch_channel;

	/* memory space */
	enum memory_window_mode ch_memory_window_mode;
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

struct hd64461pcmcia_event {
	int __queued;
	enum hd64461pcmcia_event_type pe_type;
	struct hd64461pcmcia_channel *pe_ch;
	SIMPLEQ_ENTRY(hd64461pcmcia_event) pe_link;
};

struct hd64461pcmcia_softc {
	struct device sc_dev;
	enum hd64461_module_id sc_module_id;
	int sc_shutdown;

	/* CSC event */
	struct proc *sc_event_thread;
	struct hd64461pcmcia_event sc_event_pool[EVENT_QUEUE_MAX];
	SIMPLEQ_HEAD (, hd64461pcmcia_event) sc_event_head;

	struct hd64461pcmcia_channel sc_ch[CHANNEL_MAX];
};

STATIC int hd64461pcmcia_chip_mem_alloc(pcmcia_chipset_handle_t, bus_size_t,
    struct pcmcia_mem_handle *);
STATIC void hd64461pcmcia_chip_mem_free(pcmcia_chipset_handle_t,
    struct pcmcia_mem_handle *);
STATIC int hd64461pcmcia_chip_mem_map(pcmcia_chipset_handle_t, int, bus_addr_t,
    bus_size_t, struct pcmcia_mem_handle *, bus_size_t *, int *);
STATIC void hd64461pcmcia_chip_mem_unmap(pcmcia_chipset_handle_t, int);
STATIC int hd64461pcmcia_chip_io_alloc(pcmcia_chipset_handle_t, bus_addr_t,
    bus_size_t, bus_size_t, struct pcmcia_io_handle *);
STATIC void hd64461pcmcia_chip_io_free(pcmcia_chipset_handle_t,
    struct pcmcia_io_handle *);
STATIC int hd64461pcmcia_chip_io_map(pcmcia_chipset_handle_t, int, bus_addr_t,
    bus_size_t, struct pcmcia_io_handle *, int *);
STATIC void hd64461pcmcia_chip_io_unmap(pcmcia_chipset_handle_t, int);
STATIC void hd64461pcmcia_chip_socket_enable(pcmcia_chipset_handle_t);
STATIC void hd64461pcmcia_chip_socket_disable(pcmcia_chipset_handle_t);
STATIC void *hd64461pcmcia_chip_intr_establish(pcmcia_chipset_handle_t,
    struct pcmcia_function *, int, int (*)(void *), void *);
STATIC void hd64461pcmcia_chip_intr_disestablish(pcmcia_chipset_handle_t,
    void *);

STATIC struct pcmcia_chip_functions hd64461pcmcia_functions = {
	hd64461pcmcia_chip_mem_alloc,
	hd64461pcmcia_chip_mem_free,
	hd64461pcmcia_chip_mem_map,
	hd64461pcmcia_chip_mem_unmap,
	hd64461pcmcia_chip_io_alloc,
	hd64461pcmcia_chip_io_free,
	hd64461pcmcia_chip_io_map,
	hd64461pcmcia_chip_io_unmap,
	hd64461pcmcia_chip_intr_establish,
	hd64461pcmcia_chip_intr_disestablish,
	hd64461pcmcia_chip_socket_enable,
	hd64461pcmcia_chip_socket_disable,
};

STATIC int hd64461pcmcia_match(struct device *, struct cfdata *, void *);
STATIC void hd64461pcmcia_attach(struct device *, struct device *, void *);
STATIC int hd64461pcmcia_print(void *, const char *);
STATIC int hd64461pcmcia_submatch(struct device *, struct cfdata *, void *);

CFATTACH_DECL(hd64461pcmcia, sizeof(struct hd64461pcmcia_softc),
    hd64461pcmcia_match, hd64461pcmcia_attach, NULL, NULL);

STATIC void hd64461pcmcia_attach_channel(struct hd64461pcmcia_softc *,
    enum controller_channel);
/* hot plug */
STATIC void hd64461pcmcia_create_event_thread(void *);
STATIC void hd64461pcmcia_event_thread(void *);
STATIC void queue_event(struct hd64461pcmcia_channel *,
    enum hd64461pcmcia_event_type);
/* interrupt handler */
STATIC int hd64461pcmcia_channel0_intr(void *);
STATIC int hd64461pcmcia_channel1_intr(void *);
/* card status */
STATIC enum hd64461pcmcia_event_type detect_card(enum controller_channel);
STATIC void hd64461pcmcia_power_off(enum controller_channel)
	__attribute__((__unused__));
STATIC void hd64461pcmcia_power_on(enum controller_channel)
	__attribute__((__unused__));
/* memory window access ops */
STATIC void hd64461pcmcia_memory_window_mode(enum controller_channel,
    enum memory_window_mode)__attribute__((__unused__));
STATIC void hd64461pcmcia_memory_window_16(enum controller_channel,
    enum memory_window_16);
/* bus width */
STATIC void hd64461_set_bus_width(enum controller_channel, int);
#ifdef HD64461PCMCIA_DEBUG
STATIC void hd64461pcmcia_info(struct hd64461pcmcia_softc *);
#endif
/* fix SH3 Area[56] bug */
STATIC void fixup_sh3_pcmcia_area(bus_space_tag_t);
#define _BUS_SPACE_ACCESS_HOOK()					\
do {									\
	u_int8_t dummy __attribute__((__unused__)) =			\
	 *(volatile u_int8_t *)0xba000000;				\
} while (/*CONSTCOND*/0)
_BUS_SPACE_WRITE(_sh3_pcmcia_bug, 1, 8)
_BUS_SPACE_WRITE_MULTI(_sh3_pcmcia_bug, 1, 8)
_BUS_SPACE_WRITE_REGION(_sh3_pcmcia_bug, 1, 8)
_BUS_SPACE_SET_MULTI(_sh3_pcmcia_bug, 1, 8)
#undef _BUS_SPACE_ACCESS_HOOK

#define DELAY_MS(x)	delay((x) * 1000)

int
hd64461pcmcia_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct hd64461_attach_args *ha = aux;

	return (ha->ha_module_id == HD64461_MODULE_PCMCIA);
}

void
hd64461pcmcia_attach(struct device *parent, struct device *self, void *aux)
{
	struct hd64461_attach_args *ha = aux;
	struct hd64461pcmcia_softc *sc = (struct hd64461pcmcia_softc *)self;

	sc->sc_module_id = ha->ha_module_id;
	
	printf("\n");

#ifdef HD64461PCMCIA_DEBUG
	hd64461pcmcia_info(sc);
#endif
	/* Channel 0/1 common CSC event queue */
	SIMPLEQ_INIT (&sc->sc_event_head);
	kthread_create(hd64461pcmcia_create_event_thread, sc);

	hd64461pcmcia_attach_channel(sc, CHANNEL_0);
	hd64461pcmcia_attach_channel(sc, CHANNEL_1);
}

void
hd64461pcmcia_create_event_thread(void *arg)
{
	struct hd64461pcmcia_softc *sc = arg;
	int error;

	error = kthread_create1(hd64461pcmcia_event_thread, sc,
	    &sc->sc_event_thread, "%s",
	    sc->sc_dev.dv_xname);
	KASSERT(error == 0);
}

void
hd64461pcmcia_event_thread(void *arg)
{
	struct hd64461pcmcia_softc *sc = arg;
	struct hd64461pcmcia_event *pe;
	int s;
	
	while (!sc->sc_shutdown) {
		tsleep(sc, PWAIT, "CSC wait", 0);
		s = splhigh();
		while ((pe = SIMPLEQ_FIRST(&sc->sc_event_head))) {
			splx(s);
			switch (pe->pe_type) {
			default:
				printf("%s: unknown event.\n", __FUNCTION__);
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
hd64461pcmcia_print(void *arg, const char *pnp)
{

	if (pnp)
		aprint_normal("pcmcia at %s", pnp);

	return (UNCONF);
}

int
hd64461pcmcia_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct pcmciabus_attach_args *paa = aux;
	struct hd64461pcmcia_channel *ch =
	    (struct hd64461pcmcia_channel *)paa->pch;

	if (ch->ch_channel == CHANNEL_0) {
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
	paa->pct = (pcmcia_chipset_tag_t)&hd64461pcmcia_functions;

	return (config_match(parent, cf, aux));
}

void
hd64461pcmcia_attach_channel(struct hd64461pcmcia_softc *sc,
    enum controller_channel channel)
{
	struct device *parent = (struct device *)sc;
	struct hd64461pcmcia_channel *ch = &sc->sc_ch[channel];
	struct pcmciabus_attach_args paa;	
	bus_addr_t membase;
	int i;

	ch->ch_parent = sc;
	ch->ch_channel = channel;

	/* 
	 * Continuous 16-MB Area Mode 
	 */
	/* Attibute/Common memory extent */
	membase = (channel == CHANNEL_0)
	    ? HD64461_PCC0_MEMBASE : HD64461_PCC1_MEMBASE;

	ch->ch_memt = bus_space_create(0, "PCMCIA attribute memory",
	    membase, 0x01000000); /* 16MB */
	bus_space_alloc(ch->ch_memt, 0, 0x00ffffff, 0x01000000,
	    0x01000000, 0x01000000, 0, &ch->ch_membase_addr,
	    &ch->ch_memh);
	fixup_sh3_pcmcia_area(ch->ch_memt);

	/* Common memory space extent */
	ch->ch_memsize = 0x01000000;
	for (i = 0; i < MEMWIN_16M_MAX; i++) {
		ch->ch_cmemt[i] = bus_space_create(0, "PCMCIA common memory",
		    membase + 0x01000000,
		    ch->ch_memsize);
		fixup_sh3_pcmcia_area(ch->ch_cmemt[i]);
	}

	/* I/O port extent and interrupt staff */
	hd64461pcmcia_chip_socket_disable(ch); /* enable CSC interrupt only */

	if (channel == CHANNEL_0) {
		ch->ch_iobase = 0;
		ch->ch_iosize = HD64461_PCC0_IOSIZE;
		ch->ch_iot = bus_space_create(0, "PCMCIA I/O port", 
		    HD64461_PCC0_IOBASE,
		    ch->ch_iosize);
		fixup_sh3_pcmcia_area(ch->ch_iot);

		hd6446x_intr_establish(HD64461_INTC_PCC0, IST_LEVEL, IPL_TTY,
		    hd64461pcmcia_channel0_intr, ch);
	} else {
		hd64461_set_bus_width(CHANNEL_1, PCMCIA_WIDTH_IO16);
		hd6446x_intr_establish(HD64461_INTC_PCC1, IST_EDGE, IPL_TTY,
		    hd64461pcmcia_channel1_intr, ch);
	}

	paa.paa_busname = "pcmcia";
	paa.pch = (pcmcia_chipset_handle_t)ch;
	paa.iobase = ch->ch_iobase;
	paa.iosize = ch->ch_iosize;

	ch->ch_pcmcia = config_found_sm(parent, &paa, hd64461pcmcia_print,
	    hd64461pcmcia_submatch);

	if (ch->ch_pcmcia && (detect_card(ch->ch_channel) == EVENT_INSERT)) {
		ch->ch_attached = 1;
		pcmcia_card_attach(ch->ch_pcmcia);
	}
}

int
hd64461pcmcia_channel0_intr(void *arg)
{
	struct hd64461pcmcia_channel *ch = (struct hd64461pcmcia_channel *)arg;
	u_int8_t r;
	int ret = 0;

	r = hd64461_reg_read_1(HD64461_PCC0CSCR_REG8);
	/* clear interrtupt (edge source only) */
	hd64461_reg_write_1(HD64461_PCC0CSCR_REG8, 0);

	if (r & HD64461_PCC0CSCR_P0IREQ) {
		if (ch->ch_ih_card_func) {
			ret = (*ch->ch_ih_card_func)(ch->ch_ih_card_arg);
		} else
			DPRINTF("spurious IREQ interrupt.\n");
	}

	if (r & HD64461_PCC0CSCR_P0CDC)
		queue_event(ch, detect_card(ch->ch_channel));

	return ret;
}

int
hd64461pcmcia_channel1_intr(void *arg)
{
	struct hd64461pcmcia_channel *ch = (struct hd64461pcmcia_channel *)arg;
	u_int8_t r;
	int ret = 0;

	r = hd64461_reg_read_1(HD64461_PCC1CSCR_REG8);
	/* clear interrtupt */
	hd64461_reg_write_1(HD64461_PCC1CSCR_REG8, 0);

	if (r & HD64461_PCC1CSCR_P1RC) {
		if (ch->ch_ih_card_func)
			ret = (*ch->ch_ih_card_func)(ch->ch_ih_card_arg);
		else
			DPRINTF("spurious READY interrupt.\n");
	}

	if (r & HD64461_PCC1CSCR_P1CDC)
		queue_event(ch, detect_card(ch->ch_channel));

	return ret;
}

void
queue_event(struct hd64461pcmcia_channel *ch,
    enum hd64461pcmcia_event_type type)
{
	struct hd64461pcmcia_event *pe, *pool;
	struct hd64461pcmcia_softc *sc = ch->ch_parent;
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
		printf("%s: event FIFO overflow (max %d).\n", __FUNCTION__,
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
 * interface for pcmcia driver.
 */
void *
hd64461pcmcia_chip_intr_establish(pcmcia_chipset_handle_t pch,
    struct pcmcia_function *pf,
    int ipl, int (*ih_func)(void *), void *ih_arg)
{
	struct hd64461pcmcia_channel *ch = (struct hd64461pcmcia_channel *)pch;
	int channel = ch->ch_channel;
	bus_addr_t cscier = HD64461_PCCCSCIER(channel);
	int s = splhigh();
	u_int8_t r;

	ch->ch_ih_card_func = ih_func;
	ch->ch_ih_card_arg = ih_arg;

	/* enable card interrupt */
	r = hd64461_reg_read_1(cscier);
	if (channel == CHANNEL_0) {
		/* set level mode */
		r &= ~HD64461_PCC0CSCIER_P0IREQE_MASK;
		r |= HD64461_PCC0CSCIER_P0IREQE_LEVEL;
		hd6446x_intr_priority(HD64461_INTC_PCC0, ipl);
	} else {
		/* READY-pin LOW to HIGH changes generates interrupt */
		r |= HD64461_PCC1CSCIER_P1RE;
		hd6446x_intr_priority(HD64461_INTC_PCC1, ipl);
	}
	hd64461_reg_write_1(cscier, r);

	splx(s);

	return (void *)ih_func;
}

void
hd64461pcmcia_chip_intr_disestablish(pcmcia_chipset_handle_t pch, void *ih)
{
	struct hd64461pcmcia_channel *ch = (struct hd64461pcmcia_channel *)pch;
	int channel = ch->ch_channel;
	bus_addr_t cscier = HD64461_PCCCSCIER(channel);
	int s = splhigh();
	u_int8_t r;

	/* disable card interrupt */
	r = hd64461_reg_read_1(cscier);
	if (channel == CHANNEL_0) {
		r &= ~HD64461_PCC0CSCIER_P0IREQE_MASK;
		r |= HD64461_PCC0CSCIER_P0IREQE_NONE;
		hd6446x_intr_priority(HD64461_INTC_PCC0, IPL_TTY);
	} else {
		r &= ~HD64461_PCC1CSCIER_P1RE;
		hd6446x_intr_priority(HD64461_INTC_PCC1, IPL_TTY);
	}
	hd64461_reg_write_1(cscier, r);

	ch->ch_ih_card_func = 0;

	splx(s);
}

int
hd64461pcmcia_chip_mem_alloc(pcmcia_chipset_handle_t pch, bus_size_t size,
    struct pcmcia_mem_handle *pcmhp)
{
	struct hd64461pcmcia_channel *ch = (struct hd64461pcmcia_channel *)pch;

	pcmhp->memt = ch->ch_memt;
	pcmhp->addr = ch->ch_membase_addr;
	pcmhp->memh = ch->ch_memh;
	pcmhp->size = size;
	pcmhp->realsize = size;

	DPRINTF("base 0x%08lx size %#lx\n", pcmhp->addr, size);

	return (0);
}

void
hd64461pcmcia_chip_mem_free(pcmcia_chipset_handle_t pch,
    struct pcmcia_mem_handle *pcmhp)
{
	/* nothing to do */
}

int
hd64461pcmcia_chip_mem_map(pcmcia_chipset_handle_t pch, int kind,
    bus_addr_t card_addr,
    bus_size_t size, struct pcmcia_mem_handle *pcmhp,
    bus_size_t *offsetp, int *windowp)
{
	struct hd64461pcmcia_channel *ch = (struct hd64461pcmcia_channel *)pch;
	struct hd64461pcmcia_window_cookie *cookie;
	bus_addr_t ofs;

	cookie = malloc(sizeof(struct hd64461pcmcia_window_cookie),
	    M_DEVBUF, M_NOWAIT);
	KASSERT(cookie);
	memset(cookie, 0, sizeof(struct hd64461pcmcia_window_cookie));

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
		hd64461pcmcia_memory_window_16(ch->ch_channel, window);
		*offsetp = ofs + 0x01000000; /* skip attribute area */
		cookie->wc_window = window;
	}
	cookie->wc_size = size;
	*windowp = (int)cookie;

	DPRINTF("(%s) %#lx+%#lx-> %#lx+%#lx\n", kind == PCMCIA_MEM_ATTR ?
	    "attribute" : "common", ch->ch_memh, card_addr, *offsetp,
	    size);

	return (0);
 bad:
	DPRINTF("%#lx-%#lx map failed.\n", card_addr, size);
	free(cookie, M_DEVBUF);

	return (1);
}

void
hd64461pcmcia_chip_mem_unmap(pcmcia_chipset_handle_t pch, int window)
{
	struct hd64461pcmcia_window_cookie *cookie = (void *)window;

	if (cookie->wc_window != -1)
		bus_space_unmap(cookie->wc_tag, cookie->wc_handle,
		    cookie->wc_size);
	DPRINTF("%#lx-%#x\n", cookie->wc_handle, cookie->wc_size);
	free(cookie, M_DEVBUF);
}

int
hd64461pcmcia_chip_io_alloc(pcmcia_chipset_handle_t pch, bus_addr_t start,
    bus_size_t size, bus_size_t align, struct pcmcia_io_handle *pcihp)
{
	struct hd64461pcmcia_channel *ch = (struct hd64461pcmcia_channel *)pch;

	if (ch->ch_channel == CHANNEL_1)
		return (1);

	if (start) {
		if (bus_space_map(ch->ch_iot, start, size, 0, &pcihp->ioh)) {
			DPRINTF("couldn't map %#lx+%#lx\n", start, size);
			return (1);
		}
		DPRINTF("map %#lx+%#lx\n", start, size);
	} else {
		if (bus_space_alloc(ch->ch_iot, ch->ch_iobase,
		    ch->ch_iobase + ch->ch_iosize - 1,
		    size, align, 0, 0, &pcihp->addr, 
		    &pcihp->ioh)) {
			DPRINTF("couldn't allocate %#lx\n", size);
			return (1);
		}
		pcihp->flags = PCMCIA_IO_ALLOCATED;
		DPRINTF("%#lx from %#lx\n", size, pcihp->addr);
	}

	pcihp->iot = ch->ch_iot;
	pcihp->size = size;
	
	return (0);
}

int
hd64461pcmcia_chip_io_map(pcmcia_chipset_handle_t pch, int width,
    bus_addr_t offset,
    bus_size_t size, struct pcmcia_io_handle *pcihp, int *windowp)
{
	struct hd64461pcmcia_channel *ch = (struct hd64461pcmcia_channel *)pch;
#ifdef HD64461PCMCIA_DEBUG
	static char *width_names[] = { "auto", "io8", "io16" };
#endif
	if (ch->ch_channel == CHANNEL_1)
		return (1);

	hd64461_set_bus_width(CHANNEL_0, width);

	DPRINTF("%#lx:%#lx+%#lx %s\n", pcihp->ioh, offset, size,
	    width_names[width]);

	return (0);
}

void
hd64461pcmcia_chip_io_free(pcmcia_chipset_handle_t pch,
    struct pcmcia_io_handle *pcihp)
{
	struct hd64461pcmcia_channel *ch = (struct hd64461pcmcia_channel *)pch;

	if (ch->ch_channel == CHANNEL_1)
		return;

	if (pcihp->flags & PCMCIA_IO_ALLOCATED)
		bus_space_free(pcihp->iot, pcihp->ioh, pcihp->size);
	else
		bus_space_unmap(pcihp->iot, pcihp->ioh, pcihp->size);

	DPRINTF("%#lx+%#lx\n", pcihp->ioh, pcihp->size);
}

void
hd64461pcmcia_chip_io_unmap(pcmcia_chipset_handle_t pch, int window)
{
	/* nothing to do */
}

void
hd64461pcmcia_chip_socket_enable(pcmcia_chipset_handle_t pch)
{
	struct hd64461pcmcia_channel *ch = (struct hd64461pcmcia_channel *)pch;
	int channel = ch->ch_channel;
	bus_addr_t isr, gcr;
	u_int8_t r;
	int cardtype;

	DPRINTF("enable channel %d\n", channel);
	isr = HD64461_PCCISR(channel);
	gcr = HD64461_PCCGCR(channel);

	hd64461pcmcia_power_off(channel);
	hd64461pcmcia_power_on(channel);
#if notyet
	{
		int i;
		/* assert reset */
		r = hd64461_reg_read_1(gcr);
		r |= HD64461_PCCGCR_PCCR;
		hd64461_reg_write_1(gcr, r);

		/*
		 * hold RESET at least 10us.
		 */
		DELAY_MS(20);
	
		/* clear the reset flag */
		r &= ~HD64461_PCCGCR_PCCR;
		hd64461_reg_write_1(gcr, r);
		DELAY_MS(2000);

		/* wait for the chip to finish initializing */	
		for (i = 0; i < 10000; i++) {
			if ((hd64461_reg_read_1(isr) & HD64461_PCCISR_READY))
				goto reset_ok;
			DELAY_MS(500);
			
			if ((i > 5000) && (i % 100 == 99))
				printf(".");
		}
		printf("reset failed.\n");
		hd64461pcmcia_power_off(channel);
		return;
	reset_ok:
	}
#endif /* notyet */
	/* set Continuous 16-MB Area Mode */
	ch->ch_memory_window_mode = MEMWIN_16M_MODE;
	hd64461pcmcia_memory_window_mode(channel, ch->ch_memory_window_mode);

	/* 
	 * set Common memory area.
	 */
	hd64461pcmcia_memory_window_16(channel, MEMWIN_16M_COMMON_0);

	/* set the card type */
	r = hd64461_reg_read_1(gcr);
	if (channel == CHANNEL_0) {
		cardtype = pcmcia_card_gettype(ch->ch_pcmcia);
		if (cardtype == PCMCIA_IFTYPE_IO)
			r |= HD64461_PCC0GCR_P0PCCT;
		else
			r &= ~HD64461_PCC0GCR_P0PCCT;
	} else {
		/* reserved bit must be 0 */
 		r &= ~HD64461_PCC1GCR_RESERVED;		
	}
	hd64461_reg_write_1(gcr, r);

	DPRINTF("OK.\n");
}

void
hd64461pcmcia_chip_socket_disable(pcmcia_chipset_handle_t pch)
{
	struct hd64461pcmcia_channel *ch = (struct hd64461pcmcia_channel *)pch;
	int channel = ch->ch_channel;

	/* dont' disable CSC interrupt */
	hd64461_reg_write_1(HD64461_PCCCSCIER(channel), HD64461_PCCCSCIER_CDE);
	hd64461_reg_write_1(HD64461_PCCCSCR(channel), 0);

	/* power down the socket */
	hd64461pcmcia_power_off(channel);
}

/*
 * Card detect
 */
void
hd64461pcmcia_power_off(enum controller_channel channel)
{
#if notyet
	u_int8_t r;
	u_int16_t r16;
	bus_addr_t scr, gcr;
	
	gcr = HD64461_PCCGCR(channel);
	scr = HD64461_PCCSCR(channel);

	/* DRV (external buffer) high level */
	r = hd64461_reg_read_1(gcr);
	r &= ~HD64461_PCCGCR_DRVE;
	hd64461_reg_write_1(gcr, r);

	/* stop power */
	r = hd64461_reg_read_1(scr);
	r |= HD64461_PCCSCR_VCC1; /* VCC1 high */
	hd64461_reg_write_1(scr, r);
	r = hd64461_reg_read_1(gcr);
	r |= HD64461_PCCGCR_VCC0; /* VCC0 high */
	hd64461_reg_write_1(gcr, r);
	/* 
	 * wait 300ms until power fails (Tpf).  Then, wait 100ms since
	 * we are changing Vcc (Toff).
	 */
	DELAY_MS(300 + 100);

	/* stop clock */
	r16 = hd64461_reg_read_2(HD64461_SYSSTBCR_REG16);
	r16 |= (channel == CHANNEL_0 ? HD64461_SYSSTBCR_SPC0ST :
	    HD64461_SYSSTBCR_SPC1ST);
	hd64461_reg_write_2(HD64461_SYSSTBCR_REG16, r16);

	if (channel == CHANNEL_0) {
		/* GPIO Port A XXX Jornada690 specific? */
		r16 = hd64461_reg_read_2(HD64461_GPADR_REG16);
		r16 |= 0xf;
		hd64461_reg_write_2(HD64461_GPADR_REG16, r16);
	}

#endif /* notyet */
}

void
hd64461pcmcia_power_on(enum controller_channel channel)
{
	u_int8_t r;
	u_int16_t r16;
	bus_addr_t scr, gcr, isr;
	
	isr = HD64461_PCCISR(channel);
	gcr = HD64461_PCCGCR(channel);
	scr = HD64461_PCCSCR(channel);

	/* 
	 * XXX to access attribute memory, this is required.
	 */
	if (channel == CHANNEL_0) {
		/* GPIO Port A XXX Jonanada690 specific? */
		r16 = hd64461_reg_read_2(HD64461_GPADR_REG16);
		r16 &= ~0xf;
		r16 |= 0x5;
		hd64461_reg_write_2(HD64461_GPADR_REG16, r16);
	}

	if (channel == CHANNEL_1) {
		/* GPIO Port C, Port D XXX HP620LX specific? */
		hd64461_reg_write_2(HD64461_GPCCR_REG16, 0xa800);
		hd64461_reg_write_2(HD64461_GPDCR_REG16, 0xaa0a);
	}

	/* supply clock */
	r16 = hd64461_reg_read_2(HD64461_SYSSTBCR_REG16);
	r16 &= ~(channel == CHANNEL_0 ? HD64461_SYSSTBCR_SPC0ST :
	    HD64461_SYSSTBCR_SPC1ST);
	hd64461_reg_write_2(HD64461_SYSSTBCR_REG16, r16);
	DELAY_MS(200);

	/* detect voltage and supply VCC */
	r = hd64461_reg_read_1(isr);

	switch (r & (HD64461_PCCISR_VS1 | HD64461_PCCISR_VS2)) {
	case (HD64461_PCCISR_VS1 | HD64461_PCCISR_VS2): /* 5 V */
		DPRINTF("5V card\n");
		hd64461pcmcia_power(channel, V_5, 1);
		break;
	case HD64461_PCCISR_VS2:	/* 3.3 / 5 V */
		/* FALLTHROUGH */
	case 0:				/* x.x / 3.3 / 5 V */
		DPRINTF("3.3V card\n");
		hd64461pcmcia_power(channel, V_3_3, 1);
		break;
	case HD64461_PCCISR_VS1:	/* x.x V */
		/* FALLTHROUGH */
		DPRINTF("x.x V card\n");
		hd64461pcmcia_power(channel, V_X_X, 1);
		return;
	default:
		printf("\nunknown Voltage. don't attach.\n");
		return;
	}

	/*
	 * wait 100ms until power raise (Tpr) and 20ms to become
	 * stable (Tsu(Vcc)).
	 *
	 * some machines require some more time to be settled
	 * (300ms is added here).
	 */
	DELAY_MS(100 + 20 + 300);

	/* DRV (external buffer) low level */
	r = hd64461_reg_read_1(gcr);
	r |= HD64461_PCCGCR_DRVE;
	hd64461_reg_write_1(gcr, r);

	/* clear interrupt */
	hd64461_reg_write_1(channel == CHANNEL_0 ? HD64461_PCC0CSCR_REG8 :
	    HD64461_PCC1CSCR_REG8, 0);
}

enum hd64461pcmcia_event_type
detect_card(enum controller_channel channel)
{
	u_int8_t r;

	r = hd64461_reg_read_1(HD64461_PCCISR(channel)) &
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

	return EVENT_NONE;
}

/*
 * Memory window access ops.
 */
void
hd64461pcmcia_memory_window_mode(enum controller_channel channel,
    enum memory_window_mode mode)
{
	bus_addr_t a = HD64461_PCCGCR(channel);
	u_int8_t r = hd64461_reg_read_1(a);
	
	r &= ~HD64461_PCCGCR_MMOD;
	r |= (mode == MEMWIN_16M_MODE) ? HD64461_PCCGCR_MMOD_16M :
	    HD64461_PCCGCR_MMOD_32M;
	hd64461_reg_write_1(a, r);
}

void
hd64461pcmcia_memory_window_16(enum controller_channel channel,
    enum memory_window_16 window)
{
	bus_addr_t a = HD64461_PCCGCR(channel);
	u_int8_t r;

	r = hd64461_reg_read_1(a);
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

	hd64461_reg_write_1(a, r);
}

#if unused
void
memory_window_32(enum controller_channel channel, enum memory_window_32 window)
{
	bus_addr_t a = HD64461_PCCGCR(channel);
	u_int8_t r;

	r = hd64461_reg_read_1(a);
	r &= ~(HD64461_PCCGCR_PA25 | HD64461_PCCGCR_PREG);

	switch (window) {
	case MEMWIN_32M_ATTR:
		break;
	case MEMWIN_32M_COMMON_0:
		r |= HD64461_PCCGCR_PREG;
		break;
	case MEMWIN_32M_COMMON_1:
		r |= (HD64461_PCCGCR_PA25 | HD64461_PCCGCR_PREG);
		break;
	}

	hd64461_reg_write_1(a, r);
}
#endif

void
hd64461_set_bus_width(enum controller_channel channel, int width)
{
	u_int16_t r16;

	r16 = _reg_read_2(SH3_BCR2);
	if (channel == CHANNEL_0) {
		r16 &= ~((1 << 13)|(1 << 12));
		r16 |= 1 << (width == PCMCIA_WIDTH_IO8 ? 12 : 13);
	} else {
		r16 &= ~((1 << 11)|(1 << 10));
		r16 |= 1 << (width == PCMCIA_WIDTH_IO8 ? 10 : 11);
	}
	_reg_write_2(SH3_BCR2, r16);
}

void
fixup_sh3_pcmcia_area(bus_space_tag_t t)
{
	struct hpcsh_bus_space *hbs = (void *)t;

	hbs->hbs_w_1	= _sh3_pcmcia_bug_write_1;
	hbs->hbs_wm_1	= _sh3_pcmcia_bug_write_multi_1;
	hbs->hbs_wr_1	= _sh3_pcmcia_bug_write_region_1;
	hbs->hbs_sm_1	= _sh3_pcmcia_bug_set_multi_1;
}

#ifdef HD64461PCMCIA_DEBUG
void
hd64461pcmcia_info(struct hd64461pcmcia_softc *sc)
{
	u_int8_t r8;

	dbg_banner_function();
	/*
	 * PCC0
	 */
	printf("[PCC0 memory and I/O card (SH3 Area 6)]\n");
	printf("PCC0 Interface Status Register\n");
	r8 = hd64461_reg_read_1(HD64461_PCC0ISR_REG8);

#define _(m)	dbg_bitmask_print(r8, HD64461_PCC0ISR_##m, #m)
	_(P0READY);_(P0MWP);_(P0VS2);_(P0VS1);_(P0CD2);_(P0CD1);
	_(P0BVD2);_(P0BVD1);
#undef _
	printf("\n");

	printf("PCC0 General Control Register\n");
	r8 = hd64461_reg_read_1(HD64461_PCC0GCR_REG8);	
#define _(m)	dbg_bitmask_print(r8, HD64461_PCC0GCR_##m, #m)
	_(P0DRVE);_(P0PCCR);_(P0PCCT);_(P0VCC0);_(P0MMOD);
	_(P0PA25);_(P0PA24);_(P0REG);
#undef _
	printf("\n");

	printf("PCC0 Card Status Change Register\n");
	r8 = hd64461_reg_read_1(HD64461_PCC0CSCR_REG8);
#define _(m)	dbg_bitmask_print(r8, HD64461_PCC0CSCR_##m, #m)
	_(P0SCDI);_(P0IREQ);_(P0SC);_(P0CDC);_(P0RC);_(P0BW);_(P0BD);
#undef _
	printf("\n");

	printf("PCC0 Card Status Change Interrupt Enable Register\n");
	r8 = hd64461_reg_read_1(HD64461_PCC0CSCIER_REG8);
#define _(m)	dbg_bitmask_print(r8, HD64461_PCC0CSCIER_##m, #m)
	_(P0CRE);_(P0SCE);_(P0CDE);_(P0RE);_(P0BWE);_(P0BDE);
#undef _
	printf("\ninterrupt type: ");
	switch (r8 & HD64461_PCC0CSCIER_P0IREQE_MASK) {
	case HD64461_PCC0CSCIER_P0IREQE_NONE:
		printf("none\n");
		break;
	case HD64461_PCC0CSCIER_P0IREQE_LEVEL:
		printf("level\n");
		break;
	case HD64461_PCC0CSCIER_P0IREQE_FEDGE:
		printf("falling edge\n");
		break;
	case HD64461_PCC0CSCIER_P0IREQE_REDGE:
		printf("rising edge\n");
		break;
	}

	printf("PCC0 Software Control Register\n");
	r8 = hd64461_reg_read_1(HD64461_PCC0SCR_REG8);
#define _(m)	dbg_bitmask_print(r8, HD64461_PCC0SCR_##m, #m)
	_(P0VCC1);_(P0SWP);	
#undef _
	printf("\n");

	/*
	 * PCC1
	 */
	printf("[PCC1 memory card only (SH3 Area 5)]\n");
	printf("PCC1 Interface Status Register\n");
	r8 = hd64461_reg_read_1(HD64461_PCC1ISR_REG8);
#define _(m)	dbg_bitmask_print(r8, HD64461_PCC1ISR_##m, #m)
	_(P1READY);_(P1MWP);_(P1VS2);_(P1VS1);_(P1CD2);_(P1CD1);
	_(P1BVD2);_(P1BVD1);
#undef _
	printf("\n");

	printf("PCC1 General Contorol Register\n");
	r8 = hd64461_reg_read_1(HD64461_PCC1GCR_REG8);
#define _(m)	dbg_bitmask_print(r8, HD64461_PCC1GCR_##m, #m)
	_(P1DRVE);_(P1PCCR);_(P1VCC0);_(P1MMOD);_(P1PA25);_(P1PA24);_(P1REG);
#undef _
	printf("\n");

	printf("PCC1 Card Status Change Register\n");
	r8 = hd64461_reg_read_1(HD64461_PCC1CSCR_REG8);
#define _(m)	dbg_bitmask_print(r8, HD64461_PCC1CSCR_##m, #m)
	_(P1SCDI);_(P1CDC);_(P1RC);_(P1BW);_(P1BD);
#undef _
	printf("\n");

	printf("PCC1 Card Status Change Interrupt Enable Register\n");
	r8 = hd64461_reg_read_1(HD64461_PCC1CSCIER_REG8);
#define _(m)	dbg_bitmask_print(r8, HD64461_PCC1CSCIER_##m, #m)
	_(P1CRE);_(P1CDE);_(P1RE);_(P1BWE);_(P1BDE);
#undef _
	printf("\n");

	printf("PCC1 Software Control Register\n");
	r8 = hd64461_reg_read_1(HD64461_PCC1SCR_REG8);
#define _(m)	dbg_bitmask_print(r8, HD64461_PCC1SCR_##m, #m)
	_(P1VCC1);_(P1SWP);
#undef _
	printf("\n");

	/*
	 * General Control
	 */
	printf("[General Control]\n");
	printf("PCC0 Output pins Control Register\n");
	r8 = hd64461_reg_read_1(HD64461_PCCP0OCR_REG8);
#define _(m)	dbg_bitmask_print(r8, HD64461_PCCP0OCR_##m, #m)
	_(P0DEPLUP);_(P0AEPLUP);
#undef _
	printf("\n");

	printf("PCC1 Output pins Control Register\n");
	r8 = hd64461_reg_read_1(HD64461_PCCP1OCR_REG8);
#define _(m)	dbg_bitmask_print(r8, HD64461_PCCP1OCR_##m, #m)
	_(P1RST8MA);_(P1RST4MA);_(P1RAS8MA);_(P1RAS4MA);
#undef _
	printf("\n");

	printf("PC Card General Control Register\n");
	r8 = hd64461_reg_read_1(HD64461_PCCPGCR_REG8);
#define _(m)	dbg_bitmask_print(r8, HD64461_PCCPGCR_##m, #m)
	_(PSSDIR);_(PSSRDWR);
#undef _
	printf("\n");

	dbg_banner_line();
}
#endif /* HD64461PCMCIA_DEBUG */
