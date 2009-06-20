/*	$NetBSD: if_ie_gsc.c,v 1.14.44.3 2009/06/20 07:20:03 yamt Exp $	*/

/*	$OpenBSD: if_ie_gsc.c,v 1.6 2001/01/12 22:57:04 mickey Exp $	*/

/*
 * Copyright (c) 1998,1999 Michael Shalayeff
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF MIND,
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Referencies:
 * 1. 82596DX and 82596SX High-Perfomance 32-bit Local Area Network Coprocessor
 *    Intel Corporation, November 1996, Order Number: 290219-006
 *
 * 2. 712 I/O Subsystem ERS Rev 1.0
 *    Hewlett-Packard, June 17 1992, Dwg No. A-A2263-66510-31
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ie_gsc.c,v 1.14.44.3 2009/06/20 07:20:03 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <uvm/uvm_extern.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_types.h>
#include <net/if_media.h>

#include <netinet/in.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <hp700/dev/cpudevs.h>
#include <hp700/gsc/gscbusvar.h>
#include <hp700/hp700/machdep.h>

#include <dev/ic/i82586reg.h>
#include <dev/ic/i82586var.h>

#define	I82596_DEBUG	I82586_DEBUG

/*
 * XXX fredette - I'm defining these on a hunch.  When things
 * appear to be working, remove these.
 */
#if 1
#define fdcache_small fdcache
#define pdcache_small pdcache
#endif

#ifdef __for_reference_only
struct ie_gsc_regs {
	uint32_t	ie_reset;
	uint32_t	ie_port;
	uint32_t	ie_attn;
};
#endif

#define	IE_GSC_BANK_SZ		(12)
#define	IE_GSC_REG_RESET	(0)
#define	IE_GSC_REG_PORT		(4)
#define	IE_GSC_REG_ATTN		(8)

#define	IE_GSC_ALIGN(v)	((((u_int) (v)) + 0xf) & ~0xf)

#define	IE_GSC_SYSBUS	(IE_SYSBUS_596_RSVD_SET	| \
			 IE_SYSBUS_596_82586	| \
			 IE_SYSBUS_596_INTLOW	| \
			 IE_SYSBUS_596_TRGEXT	| \
			 IE_SYSBUS_596_BE)

#define	IE_SIZE	0x8000

struct ie_gsc_softc {
	struct ie_softc ie;
	
	/* tag and handle to hp700-specific adapter registers. */
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	/* bus_dma_tag_t for the memory used by the adapter. */
	bus_dma_tag_t iemt;

	/* interrupt handle. */
	void *sc_ih;

	/* miscellaneous flags. */
	int flags;
#define	IEGSC_GECKO	(1 << 0)
};

int	ie_gsc_probe(device_t, cfdata_t, void *);
void	ie_gsc_attach(device_t, device_t, void *);

CFATTACH_DECL(ie_gsc, sizeof(struct ie_gsc_softc),
    ie_gsc_probe, ie_gsc_attach, NULL, NULL);

static int ie_gsc_media[] = {
	IFM_ETHER | IFM_10_2,
};
#define	IE_NMEDIA	(sizeof(ie_gsc_media) / sizeof(ie_gsc_media[0]))

void ie_gsc_reset(struct ie_softc *, int);
void ie_gsc_attend(struct ie_softc *, int);
void ie_gsc_run(struct ie_softc *);
void ie_gsc_port(struct ie_softc *, u_int);
uint16_t ie_gsc_read16(struct ie_softc *, int);
void ie_gsc_write16(struct ie_softc *, int, uint16_t);
void ie_gsc_write24(struct ie_softc *, int, int);
void ie_gsc_memcopyin(struct ie_softc *, void *, int, size_t);
void ie_gsc_memcopyout(struct ie_softc *, const void *, int, size_t);


/* Reset the adapter. */
void
ie_gsc_reset(struct ie_softc *sc, int what)
{
	struct ie_gsc_softc *gsc = (struct ie_gsc_softc *) sc;
	int i;
	
	switch (what) {
	case CHIP_PROBE:
		bus_space_write_4(gsc->iot, gsc->ioh, IE_GSC_REG_RESET, 0);
		break;

	case CARD_RESET:
		bus_space_write_4(gsc->iot, gsc->ioh, IE_GSC_REG_RESET, 0);

		/*
		 * per [2] 4.6.2.1
		 * delay for 10 system clocks + 5 transmit clocks,
		 * NB: works for system clocks over 10MHz
		 */
		DELAY(1000);

		/*
		 * after the hardware reset:
		 * inform i825[89]6 about new SCP address,
		 * which must be at least 16-byte aligned
		 */
		ie_gsc_port(sc, IE_PORT_ALT_SCP);
		ie_gsc_attend(sc, what);

		for (i = 9000; i-- && ie_gsc_read16(sc, IE_ISCP_BUSY(sc->iscp));
		     DELAY(100))
			pdcache(0, (vaddr_t)sc->sc_maddr + sc->iscp, IE_ISCP_SZ);

#if I82596_DEBUG
		if (i < 0) {
			printf("timeout for PORT command (%x)%s\n",
			       ie_gsc_read16(sc, IE_ISCP_BUSY(sc->iscp)),
			       (gsc->flags & IEGSC_GECKO)? " on gecko":"");
			return;
		}
#endif
		break;
	}
}

/* Do a channel attention on the adapter. */
void
ie_gsc_attend(struct ie_softc *sc, int why)
{
	struct ie_gsc_softc *gsc = (struct ie_gsc_softc *) sc;

	bus_space_write_4(gsc->iot, gsc->ioh, IE_GSC_REG_ATTN, 0);
}

/* Enable the adapter. */
void
ie_gsc_run(struct ie_softc *sc)
{
}

/* Run an i82596 PORT command on the adapter. */
void
ie_gsc_port(struct ie_softc *sc, u_int cmd)
{
	struct ie_gsc_softc *gsc = (struct ie_gsc_softc *) sc;

	switch (cmd) {
	case IE_PORT_RESET:
	case IE_PORT_DUMP:
		break;
	case IE_PORT_SELF_TEST:
		cmd |= (sc->sc_dmamap->dm_segs[0].ds_addr + 0);
		break;
	case IE_PORT_ALT_SCP:
		cmd |= (sc->sc_dmamap->dm_segs[0].ds_addr + sc->scp);
		break;
	}

	if (gsc->flags & IEGSC_GECKO) {
		bus_space_write_4(gsc->iot, gsc->ioh,
				  IE_GSC_REG_PORT, (cmd & 0xffff));
		DELAY(1000);
		bus_space_write_4(gsc->iot, gsc->ioh,
				  IE_GSC_REG_PORT, (cmd >> 16));
		DELAY(1000);
	} else {
		bus_space_write_4(gsc->iot, gsc->ioh,
				  IE_GSC_REG_PORT, (cmd >> 16));
		DELAY(1000);
		bus_space_write_4(gsc->iot, gsc->ioh,
				  IE_GSC_REG_PORT, (cmd & 0xffff));
		DELAY(1000);
	}
}

uint16_t
ie_gsc_read16(struct ie_softc *sc, int offset)
{
	uint16_t val;

	__asm volatile(
	"	ldh	0(%1), %0	\n"
	"	fdc	%%r0(%1)	\n"
	: "=&r" (val)
	: "r" ((char *)sc->sc_maddr + offset));
	return (val);
}

void
ie_gsc_write16(struct ie_softc *sc, int offset, uint16_t v)
{

	__asm volatile(
	"	sth	%0, 0(%1)	\n"
	"	fdc	%%r0(%1)	\n"
	: /* no outputs */
	: "r" (v), "r" ((char *)sc->sc_maddr + offset));
}

void
ie_gsc_write24(struct ie_softc *sc, int offset, int addr)
{

	/*
	 * i82586.c assumes that the chip address space starts at
	 * zero, so we have to add in the appropriate offset here.
	 */
	addr += sc->sc_dmamap->dm_segs[0].ds_addr;
	__asm volatile(
	"	ldi	2, %%r21		\n"
	"	extru	%0, 15, 16, %%r22	\n"
	"	sth	%0, 0(%1)		\n"
	"	sth	%%r22, 2(%1)		\n"
	"	fdc	%%r0(%1)		\n"
	"	fdc	%%r21(%1)		\n"
	: /* No outputs */
	: "r" (addr), "r" ((char *)sc->sc_maddr + offset)
	: "r21", "r22");
}

void
ie_gsc_memcopyin(struct ie_softc *sc, void *p, int offset, size_t size)
{
	struct ie_gsc_softc *gsc = (struct ie_gsc_softc *) sc;

	if (size == 0)
		return;

	memcpy(p, (char *)sc->sc_maddr + offset, size);
	bus_dmamap_sync(gsc->iemt, sc->sc_dmamap, offset, size,
			BUS_DMASYNC_PREREAD);
	hp700_led_blink(HP700_LED_NETRCV);
}

void
ie_gsc_memcopyout(struct ie_softc *sc, const void *p, int offset, size_t size)
{
	struct ie_gsc_softc *gsc = (struct ie_gsc_softc *) sc;

	if (size == 0)
		return;

	memcpy((char *)sc->sc_maddr + offset, p, size);
	bus_dmamap_sync(gsc->iemt, sc->sc_dmamap, offset, size,
			BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);
	hp700_led_blink(HP700_LED_NETSND);
}

/*
 * i82596 probe routine
 */
int i82596_probe(struct ie_softc *);
int
i82596_probe(struct ie_softc *sc)
{
	struct ie_gsc_softc *gsc = (struct ie_gsc_softc *) sc;
	int i;

	/* Set up the SCP. */
	sc->ie_bus_write16(sc, IE_SCP_BUS_USE(sc->scp), IE_GSC_SYSBUS);
	sc->ie_bus_write24(sc, IE_SCP_ISCP(sc->scp), sc->iscp);

	/* Set up the ISCP. */
	sc->ie_bus_write16(sc, IE_ISCP_SCB(sc->iscp), sc->scb);
	sc->ie_bus_write24(sc, IE_ISCP_BASE(sc->iscp), 0);

	/* Set BUSY in the ISCP. */
	sc->ie_bus_write16(sc, IE_ISCP_BUSY(sc->iscp), 1);

	/* Reset the adapter. */
	bus_dmamap_sync(gsc->iemt, sc->sc_dmamap, 0, sc->sc_msize,
			BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	sc->hwreset(sc, CARD_RESET);

	/* Make sure that BUSY got cleared. */
	if (sc->ie_bus_read16(sc, IE_ISCP_BUSY(sc->iscp))) {
#if I82596_DEBUG
		printf ("%s: ISCP set failed\n", sc->sc_dev.dv_xname);
#endif
		return 0;
	}

	/* Run the chip self-test. */
	sc->ie_bus_write24(sc, 0, -sc->sc_dmamap->dm_segs[0].ds_addr);
	sc->ie_bus_write24(sc, 4, -(sc->sc_dmamap->dm_segs[0].ds_addr + 1));
	bus_dmamap_sync(gsc->iemt, sc->sc_dmamap, 0, sc->sc_msize,
			BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	ie_gsc_port(sc, IE_PORT_SELF_TEST);
	for (i = 9000; i-- &&
		     sc->ie_bus_read16(sc, 4);
	     DELAY(100))
		pdcache(0, (vaddr_t)sc->sc_maddr, sc->sc_msize);

#if I82596_DEBUG
	printf (": test %x:%x\n%s",
		*((volatile int32_t *)((char *)sc->sc_maddr + 0)),
		*((volatile int32_t *)((char *)sc->sc_maddr + 4)),
		sc->sc_dev.dv_xname);
#endif
	return 1;
}

int
ie_gsc_probe(device_t parent, cfdata_t match, void *aux)
{
	struct gsc_attach_args *ga = aux;

	if (ga->ga_type.iodc_type != HPPA_TYPE_FIO ||
	    (ga->ga_type.iodc_sv_model != HPPA_FIO_LAN &&
	     ga->ga_type.iodc_sv_model != HPPA_FIO_GLAN))
		return 0;

	return 1;
}

void
ie_gsc_attach(device_t parent, device_t self, void *aux)
{
	struct ie_gsc_softc *gsc = device_private(self);
	struct ie_softc *sc = &gsc->ie;
	struct gsc_attach_args *ga = aux;
	bus_dma_segment_t seg;
	int rseg;
	int rv;
	uint8_t myaddr[ETHER_ADDR_LEN];
#ifdef PMAPDEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;
	pmapdebug = 0;
#endif

	if (ga->ga_type.iodc_sv_model == HPPA_FIO_GLAN)
		gsc->flags |= IEGSC_GECKO;

	/*
	 * Map the GSC registers.
	 */
	if (bus_space_map(ga->ga_iot, ga->ga_hpa,
			  IE_GSC_BANK_SZ, 0, &gsc->ioh)) {
		printf(": can't map i/o space\n");
		return;
	}

	/* Set up some initial glue. */
	gsc->iot = ga->ga_iot;
	gsc->iemt = ga->ga_dmatag;
	sc->bt = ga->ga_iot;
	sc->sc_msize = IE_SIZE;

	/*
	 * Allocate one contiguous segment of physical memory
	 * to be used with the i82596.  Since we're running the
	 * chip in i82586 mode, we're restricted to 24-bit
	 * physical addresses.
	 */
	if (bus_dmamem_alloc(gsc->iemt, sc->sc_msize, PAGE_SIZE, 0,
			     &seg, 1, &rseg, BUS_DMA_NOWAIT | BUS_DMA_24BIT)) {
		printf (": can't allocate %d bytes of DMA memory\n",
			sc->sc_msize);
		return;
	}

	/*
	 * Map that physical memory into kernel virtual space.
	 */
	if (bus_dmamem_map(gsc->iemt, &seg, rseg, sc->sc_msize,
			   (void **)&sc->sc_maddr, BUS_DMA_NOWAIT)) {
		printf (": can't map DMA memory\n");
		bus_dmamem_free(gsc->iemt, &seg, rseg);
		return;
	}

	/*
	 * Create a DMA map for the memory.
	 */
	if (bus_dmamap_create(gsc->iemt, sc->sc_msize, rseg, sc->sc_msize,
			      0, BUS_DMA_NOWAIT, &sc->sc_dmamap)) {
		printf(": can't create DMA map\n");
		bus_dmamem_unmap(gsc->iemt,
				 (void *)sc->sc_maddr, sc->sc_msize);
		bus_dmamem_free(gsc->iemt, &seg, rseg);
		return;
	}

	/*
	 * Load the mapped DMA memory into the DMA map.
	 */
	if (bus_dmamap_load(gsc->iemt, sc->sc_dmamap,
			    sc->sc_maddr, sc->sc_msize,
			    NULL, BUS_DMA_NOWAIT)) {
		printf(": can't load DMA map\n");
		bus_dmamap_destroy(gsc->iemt, sc->sc_dmamap);
		bus_dmamem_unmap(gsc->iemt,
				 (void *)sc->sc_maddr, sc->sc_msize);
		bus_dmamem_free(gsc->iemt, &seg, rseg);
		return;
	}

#if 1
	/* XXX - this should go away. */
	sc->bh = (bus_space_handle_t) sc->sc_maddr;
#endif

#if I82596_DEBUG
	printf(" mem %x[%p]/%x\n%s",
		(u_int)sc->sc_dmamap->dm_segs[0].ds_addr,
		sc->sc_maddr,
		sc->sc_msize,
		sc->sc_dev.dv_xname);
	sc->sc_debug = IED_ALL;
#endif

	/* Initialize our bus glue. */
	sc->hwreset = ie_gsc_reset;
	sc->chan_attn = ie_gsc_attend;
	sc->hwinit = ie_gsc_run;
	sc->memcopyout = ie_gsc_memcopyout;
	sc->memcopyin = ie_gsc_memcopyin;
	sc->ie_bus_read16 = ie_gsc_read16;
	sc->ie_bus_write16 = ie_gsc_write16;
	sc->ie_bus_write24 = ie_gsc_write24;
	sc->intrhook = NULL;

	/* Clear all RAM. */
	memset(sc->sc_maddr, 0, sc->sc_msize);
	bus_dmamap_sync(gsc->iemt, sc->sc_dmamap, 0, sc->sc_msize,
			BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	/*
	 * We use low memory to set up SCP, ICSP and SCB data
	 * structures. The remaining pages become the buffer area
	 * (managed in i82586.c).
	 */

	/*
	 * Since we have an i82596, we can control where where
	 * the chip looks for SCP.  We plan to use the first
	 * two 32-bit words of memory for the self-test, so the
	 * SCP can go after that.
	 */
	sc->scp = IE_GSC_ALIGN(8);
	
	/* ISCP follows SCP */
	sc->iscp = IE_GSC_ALIGN(sc->scp + IE_SCP_SZ);

	/* SCB follows ISCP */
	sc->scb = IE_GSC_ALIGN(sc->iscp + IE_ISCP_SZ);

	/* The remainder of the memory is for buffers. */
	sc->buf_area = IE_GSC_ALIGN(sc->scb + IE_SCB_SZ);
	sc->buf_area_sz = sc->sc_msize - sc->buf_area;

	/* Finally, we can probe the chip. */
	rv = i82596_probe(sc);
	if (!rv) {
		bus_dmamap_destroy(gsc->iemt, sc->sc_dmamap);
		bus_dmamem_unmap(gsc->iemt,
				 (void *)sc->sc_maddr, sc->sc_msize);
		bus_dmamem_free(gsc->iemt, &seg, rseg);
		return;
	}
#ifdef PMAPDEBUG
	pmapdebug = opmapdebug;
#endif
	if (!rv)
		return;

	/* Get our Ethernet address. */
	memcpy(myaddr, ga->ga_ether_address, ETHER_ADDR_LEN);

	/* Set up the SCP. */
	sc->ie_bus_write16(sc, IE_SCP_BUS_USE(sc->scp), IE_GSC_SYSBUS);
	sc->ie_bus_write24(sc, IE_SCP_ISCP(sc->scp), sc->iscp);

	/* Set up the ISCP. */
	sc->ie_bus_write16(sc, IE_ISCP_SCB(sc->iscp), sc->scb);
	sc->ie_bus_write24(sc, IE_ISCP_BASE(sc->iscp), 0);

	/* Set BUSY in the ISCP. */
	sc->ie_bus_write16(sc, IE_ISCP_BUSY(sc->iscp), 1);

	/* Reset the adapter. */
	bus_dmamap_sync(gsc->iemt, sc->sc_dmamap, 0, sc->sc_msize,
			BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	sc->hwreset(sc, CARD_RESET);
	bus_dmamap_sync(gsc->iemt, sc->sc_dmamap, 0, sc->sc_msize,
			BUS_DMASYNC_PREREAD);

	/* Now call the MI attachment. */
	printf(": v%d.%d", ga->ga_type.iodc_model, ga->ga_type.iodc_sv_rev);
	i82586_attach(sc,
		      (gsc->flags & IEGSC_GECKO) ?
		      "LASI/i82596CA" :
		      "i82596DX",
		      myaddr, ie_gsc_media, IE_NMEDIA, ie_gsc_media[0]);
	gsc->sc_ih = hp700_intr_establish(&sc->sc_dev, IPL_NET,
					  i82586_intr, sc,
					  ga->ga_int_reg, ga->ga_irq);
}
