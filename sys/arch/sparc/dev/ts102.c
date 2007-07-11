/*	$OpenBSD: ts102.c,v 1.14 2005/01/27 17:03:23 millert Exp $	*/
/*	$NetBSD: ts102.c,v 1.7.26.1 2007/07/11 20:02:21 mjf Exp $ */
/*
 * Copyright (c) 2003, 2004, Miodrag Vallat.
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

/*
 * Driver for the PCMCIA controller found in Tadpole SPARCbook 3 series
 * notebooks.
 *
 * Based on the information provided in the SPARCbook 3 Technical Reference
 * Manual (s3gxtrmb.pdf), chapter 7.  A few ramblings against this document
 * and/or the chip itself are scattered across this file.
 *
 * Implementation notes:
 *
 * - The TS102 exports its PCMCIA windows as SBus memory ranges: 64MB for
 *   the common memory window, and 16MB for the attribute and I/O windows.
 *
 *   Mapping the whole windows would consume 192MB of address space, which
 *   is much more that what the iospace can offer.
 *
 *   A best-effort solution would be to map the windows on demand. However,
 *   due to the wap mapdev() works, the va used for the mappings would be
 *   lost after unmapping (although using an extent to register iospace memory
 *   usage would fix this). So, instead, we will do a fixed mapping of a subset
 *   of each window upon attach - this is similar to what the stp4020 driver
 *   does.
 *
 * - IPL for the cards interrupt handles are not respected. See the stp4020
 *   driver source for comments about this.
 * 
 * Endianness farce:
 *
 * - The documentation pretends that the endianness settings only affect the
 *   common memory window. Gee, thanks a lot. What about other windows, then?
 *   As a result, this driver runs with endianness conversions turned off.
 *
 * - One of the little-endian SBus and big-endian PCMCIA flags has the reverse
 *   meaning, actually. To achieve a ``no endianness conversion'' status,
 *   one has to be set and the other unset. It does not matter which one,
 *   though.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/extent.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/device.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciachip.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/autoconf.h>

#include <dev/sbus/sbusvar.h>
#include <sparc/dev/ts102reg.h>

#include "tctrl.h"

#if NTCTRL > 0
#include <machine/tctrl.h>
#include <sparc/dev/tctrlvar.h>
#endif

#define	TS102_NUM_SLOTS		2

/*
 * Memory ranges
 */
#define	TS102_RANGE_COMMON	0
#define	TS102_RANGE_ATTR	1
#define	TS102_RANGE_IO		2

#define	TS102_RANGE_CNT		3
#define	TS102_NUM_RANGES	(TS102_RANGE_CNT * TS102_NUM_SLOTS)

#define	TS102_ARBITRARY_MAP_SIZE	(1 * 1024 * 1024)

struct	tslot_softc;

#ifdef TSLOT_DEBUG
#define TSPRINTF	printf
#else
#define TSPRINTF	while (0) printf
#endif

/*
 * Per-slot data
 */
struct	tslot_data {
	struct tslot_softc	*td_parent;
	struct device		*td_pcmcia;

	volatile uint8_t	*td_regs;
	bus_addr_t		td_space[TS102_RANGE_CNT];
	bus_space_tag_t		td_pcmciat;	/* for accessing cards */
	
	/* Interrupt handler */
	int			(*td_intr)(void *);
	void			*td_intrarg;
	void			*td_softint;

	/* Socket status */
	int			td_slot;
	int			td_status;
#define	TS_CARD			0x0001
};

struct	tslot_softc {
	struct device	sc_dev;
	struct sbusdev	sc_sd;
	
	bus_space_tag_t	sc_bustag;		/* socket control io	*/
	bus_space_handle_t	sc_regh;	/*  space		*/

	pcmcia_chipset_tag_t sc_pct;

	lwp_t		*sc_thread;	/* event thread */
	uint32_t	sc_events;	/* sockets with pending events */

	/* bits 0 and 1 are set according to card presence in slot 0 and 1 */
	uint32_t 	sc_active;
	
	struct tslot_data sc_slot[TS102_NUM_SLOTS];
};

static void tslot_attach(struct device *, struct device *, void *);
static void tslot_event_thread(void *);
static int  tslot_intr(void *);
static void tslot_intr_disestablish(pcmcia_chipset_handle_t, void *);
static void *tslot_intr_establish(pcmcia_chipset_handle_t,
    struct pcmcia_function *, int, int (*)(void *), void *);

const char  *tslot_intr_string(pcmcia_chipset_handle_t, void *);
static int  tslot_io_alloc(pcmcia_chipset_handle_t, bus_addr_t, bus_size_t,
    bus_size_t, struct pcmcia_io_handle *);
static void tslot_io_free(pcmcia_chipset_handle_t, struct pcmcia_io_handle *);
static int  tslot_io_map(pcmcia_chipset_handle_t, int, bus_addr_t, bus_size_t,
    struct pcmcia_io_handle *, int *);
static void tslot_io_unmap(pcmcia_chipset_handle_t, int);
static int  tslot_match(struct device *, struct cfdata *, void *);
static int  tslot_mem_alloc(pcmcia_chipset_handle_t, bus_size_t,
    struct pcmcia_mem_handle *);
static void tslot_mem_free(pcmcia_chipset_handle_t, struct pcmcia_mem_handle *);
static int  tslot_mem_map(pcmcia_chipset_handle_t, int, bus_addr_t, bus_size_t,
    struct pcmcia_mem_handle *, bus_size_t *, int *);
static void tslot_mem_unmap(pcmcia_chipset_handle_t, int);
static int  tslot_print(void *, const char *);
static void tslot_queue_event(struct tslot_softc *, int);
static void tslot_reset(struct tslot_data *, uint32_t);
static void tslot_slot_disable(pcmcia_chipset_handle_t);
static void tslot_slot_enable(pcmcia_chipset_handle_t);
static void tslot_slot_intr(struct tslot_data *, int);
static void tslot_slot_settype(pcmcia_chipset_handle_t, int);
static void tslot_update_lcd(struct tslot_softc *, int, int);
static void tslot_intr_dispatch(void *arg);

CFATTACH_DECL(tslot, sizeof(struct tslot_softc),
    tslot_match, tslot_attach, NULL, NULL);

extern struct cfdriver tslot_cd;

/*
 * PCMCIA chipset methods
 */
struct	pcmcia_chip_functions tslot_functions = {
	tslot_mem_alloc,
	tslot_mem_free,
	tslot_mem_map,
	tslot_mem_unmap,

	tslot_io_alloc,
	tslot_io_free,
	tslot_io_map,
	tslot_io_unmap,

	tslot_intr_establish,
	tslot_intr_disestablish,

	tslot_slot_enable,
	tslot_slot_disable,
	tslot_slot_settype
};

static	uint16_t ts102_read_2(bus_space_tag_t,
				 bus_space_handle_t,
				 bus_size_t);
static	uint32_t ts102_read_4(bus_space_tag_t,
				 bus_space_handle_t,
				 bus_size_t);
static	uint64_t ts102_read_8(bus_space_tag_t,
				 bus_space_handle_t,
				 bus_size_t);
static	void	ts102_write_2(bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t,
				uint16_t);
static	void	ts102_write_4(bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t,
				uint32_t);
static	void	ts102_write_8(bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t,
				uint64_t);

static uint16_t
ts102_read_2(bus_space_tag_t space, bus_space_handle_t handle,
    bus_size_t offset)
{
	return (le16toh(*(volatile uint16_t *)(handle +
	    offset)));
}

static uint32_t
ts102_read_4(bus_space_tag_t space, bus_space_handle_t handle,
    bus_size_t offset)
{
	return (le32toh(*(volatile uint32_t *)(handle +
	    offset)));
}

static uint64_t
ts102_read_8(bus_space_tag_t space, bus_space_handle_t handle,
    bus_size_t offset)
{
	return (le64toh(*(volatile uint64_t *)(handle +
	    offset)));
}

static void
ts102_write_2(bus_space_tag_t space, bus_space_handle_t handle,
    bus_size_t offset, uint16_t value)
{
	(*(volatile uint16_t *)(handle + offset)) =
	    htole16(value);
}

static void
ts102_write_4(bus_space_tag_t space, bus_space_handle_t handle,
    bus_size_t offset, uint32_t value)
{
	(*(volatile uint32_t *)(handle + offset)) =
	    htole32(value);
}

static void
ts102_write_8(bus_space_tag_t space, bus_space_handle_t handle,
    bus_size_t offset, uint64_t value)
{
	(*(volatile uint64_t *)(handle + offset)) = 
	    htole64(value);
}


#define	TSLOT_READ(slot, offset) \
	*(volatile uint16_t *)((slot)->td_regs + (offset))
#define	TSLOT_WRITE(slot, offset, value) \
	*(volatile uint16_t *)((slot)->td_regs + (offset)) = (value)

/*
 * Attachment and initialization
 */

static int
tslot_match(struct device *parent, struct cfdata *vcf, void *aux)
{
	struct sbus_attach_args *sa = aux;

	return (strcmp("ts102", sa->sa_name) == 0);
}

static void
tslot_attach(struct device *parent, struct device *self, void *args)
{
	struct sbus_attach_args *sa = args;
	struct tslot_softc *sc = (struct tslot_softc *)self;
	struct tslot_data *td;
	volatile uint8_t *regs;
	int node, slot, rnum, base, size;
	uint32_t ranges[30];
	void *rptr = ranges;
	bus_space_handle_t hrang = 0;
	bus_space_tag_t tag;

	node = sa->sa_node;
	sc->sc_bustag=sa->sa_bustag;
	if (sbus_bus_map(sa->sa_bustag,
			 sa->sa_slot,
			 sa->sa_offset,
			 sa->sa_size,
			 0, &sc->sc_regh) != 0) {
		printf("%s: cannot map registers\n", self->dv_xname);
		return;
	}
	regs = (uint8_t *)bus_space_vaddr(sa->sa_bustag, sc->sc_regh);

	tag = bus_space_tag_alloc(sa->sa_bustag, sc);
	if (tag == NULL) {
		printf("%s: attach: out of memory\n", self->dv_xname);
		return;
	}
	tag->sparc_read_2 = ts102_read_2;
	tag->sparc_read_4 = ts102_read_4;
	tag->sparc_read_8 = ts102_read_8;
	tag->sparc_write_2 = ts102_write_2;
	tag->sparc_write_4 = ts102_write_4;
	tag->sparc_write_8 = ts102_write_8;

	sbus_establish(&sc->sc_sd, self);

	bus_intr_establish(sa->sa_bustag, sa->sa_intr[0].oi_pri,
	    IPL_NONE, tslot_intr, sc);

	printf(": %d slots\n", TS102_NUM_SLOTS);

	size = sizeof(ranges);
	if (prom_getprop(node, "ranges", 4, &size, &rptr) != 0) {
		printf("couldn't read ranges\n");
		return;
	}

	/*
	 * Setup asynchronous event handler
	 */
	sc->sc_events = 0;

	TSPRINTF("starting event thread...\n");
	if (kthread_create(PRI_NONE, 0, NULL, tslot_event_thread, sc,
	    &sc->sc_thread, "%s", self->dv_xname) != 0) {
		panic("%s: unable to create event kthread",
		    self->dv_xname);
	}

	sc->sc_pct = (pcmcia_chipset_tag_t)&tslot_functions;
	sc->sc_active = 0;

	/*
	 * Setup slots
	 */
	TSPRINTF("mapping resources...\n");
	for (slot = 0; slot < TS102_NUM_SLOTS; slot++) {
		td = &sc->sc_slot[slot];
		TSPRINTF("slot %d, ",slot);
		for (rnum = 0; rnum < TS102_RANGE_CNT; rnum++) {
			base = (slot * TS102_RANGE_CNT + rnum) * 5;
			TSPRINTF("%d: %08x %08x ",rnum,ranges[base + 3],
			    ranges[base + 4]);
			if(sbus_bus_map(sc->sc_bustag,
					sa->sa_slot,
				 	ranges[base+3],
				 	TS102_ARBITRARY_MAP_SIZE,
					0, &hrang) != 0) {
				printf("%s: cannot map registers\n",
				    self->dv_xname);
				return;
			}
			TSPRINTF("%08x: %08x ",(uint32_t)ranges[base + 3],
			    (uint32_t)hrang);
			td->td_space[rnum] = hrang;
		}
		td->td_parent = sc;
		td->td_pcmciat = tag;
		td->td_softint = NULL;
		td->td_regs = regs + slot * (TS102_REG_CARD_B_INT -
		    TS102_REG_CARD_A_INT);
		td->td_slot = slot;

		TSPRINTF("resetting slot %d %d\n", slot, (int)td->td_regs);
		tslot_reset(td, TS102_ARBITRARY_MAP_SIZE);
	}
}

static void
tslot_reset(struct tslot_data *td, uint32_t iosize)
{
	struct pcmciabus_attach_args paa;
	int ctl, status;

	paa.paa_busname = "pcmcia";
	paa.pct = (pcmcia_chipset_tag_t)td->td_parent->sc_pct;
	paa.pch = (pcmcia_chipset_handle_t)td;
	paa.iobase = 0;
	paa.iosize = iosize;

	td->td_pcmcia = config_found(&td->td_parent->sc_dev, &paa, tslot_print);

	if (td->td_pcmcia == NULL) {
		/*
		 * If no pcmcia attachment, power down the slot.
		 */
		tslot_slot_disable((pcmcia_chipset_handle_t)td);
		return;
	}

	/*
	 * Initialize the slot
	 */

	ctl = TSLOT_READ(td, TS102_REG_CARD_A_CTL);

	/* force low addresses */
	ctl &= ~(TS102_CARD_CTL_AA_MASK | TS102_CARD_CTL_IA_MASK);

	/* Put SBus and PCMCIA in their respective endian mode */
	ctl |= TS102_CARD_CTL_SBLE;	/* this is not what it looks like! */	
	ctl &= ~TS102_CARD_CTL_PCMBE;	/* default */

	/* disable read ahead and address increment */
	ctl &= ~TS102_CARD_CTL_RAHD;
	ctl |= TS102_CARD_CTL_INCDIS;

	/* power on */
	ctl &= ~TS102_CARD_CTL_PWRD;
	TSLOT_WRITE(td, TS102_REG_CARD_A_CTL, ctl);
	TSPRINTF("ctl: %x\n", ctl);

	/*
	 * Enable interrupt upon insertion/removal
	 */

	TSLOT_WRITE(td, TS102_REG_CARD_A_INT,
	    TS102_CARD_INT_MASK_CARDDETECT_STATUS);

	status = TSLOT_READ(td, TS102_REG_CARD_A_STS);
	if (status & TS102_CARD_STS_PRES) {
		td->td_status = TS_CARD;
		pcmcia_card_attach(td->td_pcmcia);
	} else
		td->td_status = 0;
}

/* XXX there ought to be a common function for this... */
static int
tslot_print(void *aux, const char *description)
{
	struct pcmciabus_attach_args *paa = aux;
	struct tslot_data *td = (struct tslot_data *)paa->pch;

	printf(" socket %d", td->td_slot);
	return (UNCONF);
}

/*
 * PCMCIA Helpers
 */

static int
tslot_io_alloc(pcmcia_chipset_handle_t pch, bus_addr_t start, bus_size_t size,
    bus_size_t align, struct pcmcia_io_handle *pih)
{
	struct tslot_data *td = (struct tslot_data *)pch;

#ifdef TSLOT_DEBUG
	printf("[io alloc %x]", (uint32_t)size);
#endif

	pih->iot = td->td_pcmciat;
	pih->ioh = td->td_space[TS102_RANGE_IO];
	pih->addr = start;
	pih->size = size;
	pih->flags = 0;

	return (0);
}

static void
tslot_io_free(pcmcia_chipset_handle_t pch, struct pcmcia_io_handle *pih)
{
#ifdef TSLOT_DEBUG
	printf("[io free]");
#endif
}

static int
tslot_io_map(pcmcia_chipset_handle_t pch, int width, bus_addr_t offset,
    bus_size_t size, struct pcmcia_io_handle *pih, int *windowp)
{
	struct tslot_data *td = (struct tslot_data *)pch;

#ifdef TSLOT_DEBUG
	printf("[io map %x/%x", (uint32_t)offset, (uint32_t)size);
#endif

	pih->iot = td->td_pcmciat;
	if (bus_space_subregion(pih->iot, td->td_space[TS102_RANGE_IO],
	    offset, size, &pih->ioh) != 0)
		printf("io_map failed, offset %x\n", (uint32_t)offset);
	*windowp = 0; /* TS102_RANGE_IO */

#ifdef TSLOT_DEBUG
	printf("->%x/%x]", (uint32_t)pih->ioh, (uint32_t)size);
	{
		int addr, line;
		for( addr = offset; addr < (offset + size); addr += 16) {
			printf("%04x:", addr);
			for(line = addr; line < (addr + 16); line += 2) {
				printf(" %04x", bus_space_read_2(pih->iot,
				    pih->ioh, line));
			}
			printf("\n");
		}
	}
#endif

	return (0);
}

static void
tslot_io_unmap(pcmcia_chipset_handle_t pch, int win)
{
#ifdef TSLOT_DEBUG
	struct tslot_data *td = (struct tslot_data *)pch;

	printf("[io unmap]");
	{
		int addr, line, offset = 0, size = 0x80;
		for (addr = offset; addr < (offset + size); addr += 16) {
			printf("%04x:", addr);
			for (line = addr; line < (addr + 16); line += 2){
				printf(" %04x", bus_space_read_2(td->td_pcmciat,
				    td->td_space[2], line));
			}
			printf("\n");
		}
	}
#endif
}

static int
tslot_mem_alloc(pcmcia_chipset_handle_t pch, bus_size_t size,
    struct pcmcia_mem_handle *pmh)
{
	struct tslot_data *td = (struct tslot_data *)pch;

#ifdef TSLOT_DEBUG
	printf("[mem alloc %x]", (uint32_t)size);
#endif
	pmh->memt = td->td_pcmciat;
	pmh->size = size;
	pmh->addr = 0;
	pmh->mhandle = 0;
	pmh->realsize = size;	/* nothing so far! */

	return (0);
}

static void
tslot_mem_free(pcmcia_chipset_handle_t pch, struct pcmcia_mem_handle *pmh)
{
#ifdef TSLOT_DEBUG
	printf("[mem free]");
#endif
}

static int
tslot_mem_map(pcmcia_chipset_handle_t pch, int kind, bus_addr_t addr,
    bus_size_t size, struct pcmcia_mem_handle *pmh, bus_size_t *offsetp,
    int *windowp)
{
	struct tslot_data *td = (struct tslot_data *)pch;
	int slot;

	slot = kind & PCMCIA_MEM_ATTR ? TS102_RANGE_ATTR : TS102_RANGE_COMMON;
#ifdef TSLOT_DEBUG
	printf("[mem map %d %x/%x", slot, (uint32_t)addr, (uint32_t)size);
#endif

	pmh->memt = td->td_parent->sc_bustag;
	if (bus_space_subregion(pmh->memt, td->td_space[slot],
	    addr, size, &pmh->memh) != 0)
		printf("mem_map failed, offset %x\n", (uint32_t)addr);
	pmh->realsize = TS102_ARBITRARY_MAP_SIZE - addr;
	pmh->size = size;
	*offsetp = 0;
	*windowp = 0;

#ifdef TSLOT_DEBUG
	printf("->%x/%x]", (uint32_t)pmh->memh, (uint32_t)size);
#endif

	return (0);
}

static void
tslot_mem_unmap(pcmcia_chipset_handle_t pch, int win)
{
#ifdef TSLOT_DEBUG
	printf("[mem unmap %d]", win);
#endif
}

static void
tslot_slot_disable(pcmcia_chipset_handle_t pch)
{
	struct tslot_data *td = (struct tslot_data *)pch;
#ifdef TSLOT_DEBUG
	printf("%s: disable slot %d\n",
	    td->td_parent->sc_dev.dv_xname, td->td_slot);
#endif

	/*
	 * Disable card access.
	 */
	TSLOT_WRITE(td, TS102_REG_CARD_A_STS,
	    TSLOT_READ(td, TS102_REG_CARD_A_STS) & ~TS102_CARD_STS_ACEN);

	/*
	 * Disable interrupts, except for insertion.
	 */
	TSLOT_WRITE(td, TS102_REG_CARD_A_INT,
	    TS102_CARD_INT_MASK_CARDDETECT_STATUS);
}

static void
tslot_slot_enable(pcmcia_chipset_handle_t pch)
{
	struct tslot_data *td = (struct tslot_data *)pch;
	int status, intr, i;

#ifdef TSLOT_DEBUG
	printf("%s: enable slot %d\n",
	    td->td_parent->sc_dev.dv_xname, td->td_slot);
#endif

	/* Power down the socket to reset it */
	status = TSLOT_READ(td, TS102_REG_CARD_A_STS);
	TSPRINTF("status: %x\n", status);
	TSLOT_WRITE(td, TS102_REG_CARD_A_STS, status | TS102_CARD_STS_VCCEN);

	/*
	 * wait 300ms until power fails (Tpf).  Then, wait 100ms since we
	 * are changing Vcc (Toff).
	 */
	DELAY((300 + 100) * 1000);

	/*
	 * Power on the card if not already done, and enable card access
	 */
	status |= TS102_CARD_STS_ACEN;
	status &= ~TS102_CARD_STS_VCCEN;
	TSLOT_WRITE(td, TS102_REG_CARD_A_STS, status);

	/*
	 * wait 100ms until power raise (Tpr) and 20ms to become
	 * stable (Tsu(Vcc)).
	 */
	DELAY((100 + 20) * 1000);

	status &= ~TS102_CARD_STS_VPP1_MASK;
	status |= TS102_CARD_STS_VPP1_VCC;
	TSLOT_WRITE(td, TS102_REG_CARD_A_STS, status);

	/*
	 * hold RESET at least 20us.
	 */
	intr = TSLOT_READ(td, TS102_REG_CARD_A_INT);
	TSLOT_WRITE(td, TS102_REG_CARD_A_INT, TS102_CARD_INT_SOFT_RESET);
	DELAY(20);
	TSLOT_WRITE(td, TS102_REG_CARD_A_INT, intr);

	/* wait 20ms as per pc card standard (r2.01) section 4.3.6 */
	DELAY(20 * 1000);

	/* We need level-triggered interrupts for PC Card hardware */
	TSLOT_WRITE(td, TS102_REG_CARD_A_STS,
		TSLOT_READ(td, TS102_REG_CARD_A_STS) | TS102_CARD_STS_LVL);

	/*
	 * Wait until the card is unbusy. If it is still busy after 3 seconds,
	 * give up. We could enable card interrupts and wait for the interrupt
	 * to happen when BUSY is released, but the interrupt could also be
	 * triggered by the card itself if it's an I/O card, so better poll
	 * here.
	 */
	for (i = 30000; i != 0; i--) {
		status = TSLOT_READ(td, TS102_REG_CARD_A_STS);
		/* If the card has been removed, abort */
		if ((status & TS102_CARD_STS_PRES) == 0) {
			tslot_slot_disable(pch);
			return;
		}
		if (status & TS102_CARD_STS_RDY)
			break;
		else
			DELAY(100);
	}

	if (i == 0) {
		printf("%s: slot %d still busy after 3 seconds, status 0x%x\n",
		    td->td_parent->sc_dev.dv_xname, td->td_slot,
		    TSLOT_READ(td, TS102_REG_CARD_A_STS));
		return;
	}
}
static void
tslot_event_thread(void *v)
{
	struct tslot_softc *sc = v;
	struct tslot_data *td;
	int s, status;
	unsigned int socket;

#if NTCTRL > 0
	int i;

	/* 
	 * First-time setup of our LCD symbol. When a card is present at boot 
	 * time we won't detect a change here and therefore the LCD symbol won't
	 * light up.
	 */
	for (i = 0; i < TS102_NUM_SLOTS; i++) {
		td = &sc->sc_slot[i];
		status = TSLOT_READ(td, TS102_REG_CARD_A_STS);
		tslot_update_lcd(sc, i, status & TS102_CARD_STS_PRES);
	}
#endif

	for (;;) {
		s = splhigh();

		if ((socket = ffs(sc->sc_events)) == 0) {
			splx(s);
			tsleep(&sc->sc_events, PWAIT, "tslot_event", hz * 30);
			continue;
		}
		socket--;
		sc->sc_events &= ~(1 << socket);
		splx(s);

		if (socket >= TS102_NUM_SLOTS) {
#ifdef DEBUG
			printf("%s: invalid slot number %d\n",
			    sc->sc_dev.dv_xname, socket);
#endif
			continue;
		}

		td = &sc->sc_slot[socket];
		status = TSLOT_READ(td, TS102_REG_CARD_A_STS);

		if (status & TS102_CARD_STS_PRES) {
			/* Card insertion */
			if ((td->td_status & TS_CARD) == 0) {
				td->td_status |= TS_CARD;
				tslot_update_lcd(sc, socket, 1);
				pcmcia_card_attach(td->td_pcmcia);
			}
		} else {
			/* Card removal */
			if ((td->td_status & TS_CARD) != 0) {
				tslot_update_lcd(sc, socket, 0);
				td->td_status &= ~TS_CARD;
				pcmcia_card_detach(td->td_pcmcia,
				    DETACH_FORCE);
			}
		}
	}
}

/*
 * Interrupt handling
 */

static int
tslot_intr(void *v)
{
	struct tslot_softc *sc = v;
	struct tslot_data *td;
	int intregs[TS102_NUM_SLOTS], *intreg;
	int i, s, rc = 0;

	s = splhigh();

	/*
	 * Scan slots, and acknowledge the interrupt if necessary first
	 */
	for (i = 0; i < TS102_NUM_SLOTS; i++) {
		td = &sc->sc_slot[i];
		intreg = &intregs[i];
		*intreg = TSLOT_READ(td, TS102_REG_CARD_A_INT);

		/*
		 * Acknowledge all interrupt situations at once, even if they
		 * did not occur.
		 */
		if ((*intreg & (TS102_CARD_INT_STATUS_IRQ |
		    TS102_CARD_INT_STATUS_WP_STATUS_CHANGED |
		    TS102_CARD_INT_STATUS_BATTERY_STATUS_CHANGED |
		    TS102_CARD_INT_STATUS_CARDDETECT_STATUS_CHANGED)) != 0) {
			rc = 1;
			TSLOT_WRITE(td, TS102_REG_CARD_A_INT, *intreg |
			    TS102_CARD_INT_RQST_IRQ |
			    TS102_CARD_INT_RQST_WP_STATUS_CHANGED |
			    TS102_CARD_INT_RQST_BATTERY_STATUS_CHANGED |
			    TS102_CARD_INT_RQST_CARDDETECT_STATUS_CHANGED);
		}
	}

#ifdef TSLOT_DEBUG
	printf("tslot_intr: %x %x\n", intregs[0], intregs[1]);
#endif

	/*
	 * Invoke the interrupt handler for each slot
	 */
	for (i = 0; i < TS102_NUM_SLOTS; i++) {
		td = &sc->sc_slot[i];
		intreg = &intregs[i];

		if ((*intreg & (TS102_CARD_INT_STATUS_IRQ |
		    TS102_CARD_INT_STATUS_WP_STATUS_CHANGED |
		    TS102_CARD_INT_STATUS_BATTERY_STATUS_CHANGED |
		    TS102_CARD_INT_STATUS_CARDDETECT_STATUS_CHANGED)) != 0)
			tslot_slot_intr(td, *intreg);
	}
	splx(s);
	
	return (rc);
}

static void
tslot_queue_event(struct tslot_softc *sc, int slot)
{
	int s;

	s = splhigh();
	sc->sc_events |= (1 << slot);
	splx(s);
	wakeup(&sc->sc_events);
}

static void
tslot_slot_intr(struct tslot_data *td, int intreg)
{
	struct tslot_softc *sc = td->td_parent;
	int status, sockstat;
	uint32_t ireg;

	status = TSLOT_READ(td, TS102_REG_CARD_A_STS);
#ifdef TSLOT_DEBUG
	printf("%s: interrupt on socket %d ir %x sts %x\n",
	    sc->sc_dev.dv_xname, td->td_slot, intreg, status);
#endif

	sockstat = td->td_status;

	/*
	 * The TS102 queues interrupt request, and may trigger an interrupt
	 * for a condition the driver does not want to receive anymore (for
	 * example, after a card gets removed).
	 * Thus, only proceed if the driver is currently allowing a particular
	 * condition.
	 */

	if ((intreg & TS102_CARD_INT_STATUS_CARDDETECT_STATUS_CHANGED) != 0 &&
	    (intreg & TS102_CARD_INT_MASK_CARDDETECT_STATUS) != 0) {
		tslot_queue_event(sc, td->td_slot);
#ifdef TSLOT_DEBUG
		printf("%s: slot %d status changed from %d to %d\n",
		    sc->sc_dev.dv_xname, td->td_slot, sockstat, td->td_status);
#endif
		/*
		 * Ignore extra interrupt bits, they are part of the change.
		 */
		return;
	}

	if ((intreg & TS102_CARD_INT_STATUS_IRQ) != 0 &&
	    (intreg & TS102_CARD_INT_MASK_IRQ) != 0) {
		/* ignore interrupts if we have a pending state change */
		if (sc->sc_events & (1 << td->td_slot))
		{
			TSPRINTF("ev: %d\n", sc->sc_events);
			return;
		}
		if ((sockstat & TS_CARD) == 0) {
			printf("%s: spurious interrupt on slot %d isr %x\n",
			    sc->sc_dev.dv_xname, td->td_slot, intreg);
			return;
		}

		if (td->td_intr != NULL) {

			if (td->td_softint != NULL)
				softintr_schedule(td->td_softint);
			/*
			 * Disable this sbus interrupt, until the soft-int
			 * handler had a chance to run
			 */
			ireg = TSLOT_READ(td, TS102_REG_CARD_A_INT);
			TSLOT_WRITE(td, TS102_REG_CARD_A_INT, ireg &
			    ~TS102_CARD_INT_MASK_IRQ);
		}
	}
}

static void
tslot_intr_disestablish(pcmcia_chipset_handle_t pch, void *ih)
{
	struct tslot_data *td = (struct tslot_data *)pch;

	td->td_intr = NULL;
	td->td_intrarg = NULL;
	if (td->td_softint) {
		softintr_disestablish(td->td_softint);
		td->td_softint = NULL;
	}
}

const char *
tslot_intr_string(pcmcia_chipset_handle_t pch, void *ih)
{
	if (ih == NULL)
		return ("couldn't establish interrupt");
	else
		return ("");	/* nothing for now */
}

static void *
tslot_intr_establish(pcmcia_chipset_handle_t pch, struct pcmcia_function *pf,
    int ipl, int (*handler)(void *), void *arg)
{
	struct tslot_data *td = (struct tslot_data *)pch;

	td->td_intr = handler;
	td->td_intrarg = arg;
	td->td_softint = softintr_establish(ipl, tslot_intr_dispatch, td);

	return (td);
}

/*
 * Softinterrupt called to invoke the real driver interrupt handler.
 */
static void
tslot_intr_dispatch(void *arg)
{
	struct tslot_data *td = arg;
	int s;
	uint32_t ireg;

	/* invoke driver handler */
	td->td_intr(td->td_intrarg);

	/* enable SBUS interrupts for pcmcia interrupts again */
	s = splhigh();
	ireg = TSLOT_READ(td, TS102_REG_CARD_A_INT);
	TSLOT_WRITE(td, TS102_REG_CARD_A_INT, ireg | TS102_CARD_INT_MASK_IRQ);
	splx(s);
}

static void
tslot_slot_settype(pcmcia_chipset_handle_t pch, int type)
{
	struct tslot_data *td = (struct tslot_data *)pch;
	uint32_t reg;

	/*
	 * Enable the card interrupts if this is an I/O card.
	 * Note that the TS102_CARD_STS_IO bit in the status register will
	 * never get set, despite what the documentation says!
	 */
	TSPRINTF("tslot_slot_settype(%d)\n",type);
	if (type == PCMCIA_IFTYPE_IO) {
		TSLOT_WRITE(td, TS102_REG_CARD_A_STS,
		    TSLOT_READ(td, TS102_REG_CARD_A_STS) | TS102_CARD_STS_IO);
		TSLOT_WRITE(td, TS102_REG_CARD_A_INT,
		    TS102_CARD_INT_MASK_CARDDETECT_STATUS |
		    TS102_CARD_INT_MASK_IRQ);
		reg=TSLOT_READ(td, TS102_REG_CARD_A_STS);
		TSPRINTF("status: %x\n", reg);
	}
}

static void
tslot_update_lcd(struct tslot_softc *sc, int socket, int status)
{
#if NTCTRL > 0
	int was = (sc->sc_active != 0), is;
	int mask = 1 << socket;

	if (status > 0) {
		sc->sc_active |= mask;
	} else {
		sc->sc_active &= (mask ^ 3);
	}
	is = (sc->sc_active != 0);
	if (was != is) {
		tadpole_set_lcd(is, 0x40);
	}
#endif
}
