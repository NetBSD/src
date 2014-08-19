/*	$NetBSD: if_ie_obio.c,v 1.40.12.1 2014/08/20 00:03:24 tls Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

/*
 * Copyright (c) 1995 Charles D. Cranor
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
 * Sparc OBIO front-end for the Intel 82586 Ethernet driver
 *
 * Converted to SUN ie driver by Charles D. Cranor,
 *		October 1994, January 1995.
 */

/*
 * The i82586 is a very painful chip, found in sun3's, sun-4/100's
 * sun-4/200's, and VME based suns.  The byte order is all wrong for a
 * SUN, making life difficult.  Programming this chip is mostly the same,
 * but certain details differ from system to system.  This driver is
 * written so that different "ie" interfaces can be controled by the same
 * driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ie_obio.c,v 1.40.12.1 2014/08/20 00:03:24 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/protosw.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <uvm/uvm_extern.h>

#include <sys/bus.h>
#include <machine/intr.h>
#include <machine/autoconf.h>

#include <dev/ic/i82586reg.h>
#include <dev/ic/i82586var.h>

/*
 * the on-board interface
 */
struct ieob {
	u_char  obctrl;
};
#define IEOB_NORSET 0x80	/* don't reset the board */
#define IEOB_ONAIR  0x40	/* put us on the air */
#define IEOB_ATTEN  0x20	/* attention! */
#define IEOB_IENAB  0x10	/* interrupt enable */
#define IEOB_XXXXX  0x08	/* free bit */
#define IEOB_XCVRL2 0x04	/* level 2 transceiver? */
#define IEOB_BUSERR 0x02	/* bus error */
#define IEOB_INT    0x01	/* interrupt */

#define IEOB_ADBASE 0xff000000  /* KVA base addr of 24 bit address space */


static void ie_obreset(struct ie_softc *, int);
static void ie_obattend(struct ie_softc *, int);
static void ie_obrun(struct ie_softc *);

int ie_obio_match(device_t, cfdata_t, void *);
void ie_obio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ie_obio, sizeof(struct ie_softc),
    ie_obio_match, ie_obio_attach, NULL, NULL);

/* Supported media */
static int media[] = {
	IFM_ETHER | IFM_10_2,
};
#define NMEDIA	(sizeof(media) / sizeof(media[0]))


/*
 * OBIO ie support routines
 */
static void
ie_obreset(struct ie_softc *sc, int what)
{
	volatile struct ieob *ieo = (struct ieob *) sc->sc_reg;

	ieo->obctrl = 0;
	delay(100);			/* XXX could be shorter? */
	ieo->obctrl = IEOB_NORSET;
}

static void
ie_obattend(struct ie_softc *sc, int why)
{
	volatile struct ieob *ieo = (struct ieob *) sc->sc_reg;

	ieo->obctrl |= IEOB_ATTEN;	/* flag! */
	ieo->obctrl &= ~IEOB_ATTEN;	/* down. */
}

static void
ie_obrun(struct ie_softc *sc)
{
	volatile struct ieob *ieo = (struct ieob *) sc->sc_reg;

	ieo->obctrl |= (IEOB_ONAIR|IEOB_IENAB|IEOB_NORSET);
}

void ie_obio_memcopyin(struct ie_softc *, void *, int, size_t);
void ie_obio_memcopyout(struct ie_softc *, const void *, int, size_t);

/*
 * Copy board memory to kernel.
 */
void
ie_obio_memcopyin(struct ie_softc *sc, void *p, int offset, size_t size)
{
	void *addr = (void *)((u_long)sc->bh + offset);/*XXX - not MI!*/

	wcopy(addr, p, size);
}

/*
 * Copy from kernel space to naord memory.
 */
void
ie_obio_memcopyout(struct ie_softc *sc, const void *p, int offset, size_t size)
{
	void *addr = (void *)((u_long)sc->bh + offset);/*XXX - not MI!*/

	wcopy(p, addr, size);
}

/* read a 16-bit value at BH offset */
uint16_t ie_obio_read16(struct ie_softc *, int);
/* write a 16-bit value at BH offset */
void ie_obio_write16(struct ie_softc *, int, uint16_t);
void ie_obio_write24(struct ie_softc *, int, int);

uint16_t
ie_obio_read16(struct ie_softc *sc, int offset)
{
	uint16_t v = bus_space_read_2(sc->bt, sc->bh, offset);

	return (((v&0xff)<<8) | ((v>>8)&0xff));
}

void
ie_obio_write16(struct ie_softc *sc, int offset, uint16_t v)
{
	v = (((v&0xff)<<8) | ((v>>8)&0xff));
	bus_space_write_2(sc->bt, sc->bh, offset, v);
}

void
ie_obio_write24(struct ie_softc *sc, int offset, int addr)
{
	u_char *f = (u_char *)&addr;
	uint16_t v0, v1;
	u_char *t;

	t = (u_char *)&v0;
	t[0] = f[3]; t[1] = f[2];
	bus_space_write_2(sc->bt, sc->bh, offset, v0);

	t = (u_char *)&v1;
	t[0] = f[1]; t[1] = 0;
	bus_space_write_2(sc->bt, sc->bh, offset+2, v1);
}

int
ie_obio_match(device_t parent, cfdata_t cf, void *aux)
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;

	if (uoba->uoba_isobio4 == 0)
		return (0);

	oba = &uoba->uoba_oba4;
	return (bus_space_probe(oba->oba_bustag, oba->oba_paddr,
				1,	/* probe size */
				0,	/* offset */
				0,	/* flags */
				NULL, NULL));
}

void
ie_obio_attach(device_t parent, device_t self, void *aux)
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba = &uoba->uoba_oba4;
	struct ie_softc *sc = device_private(self);
	bus_dma_tag_t dmatag = oba->oba_dmatag;
	bus_space_handle_t bh;
	bus_dma_segment_t seg;
	int rseg;
	int error;
	paddr_t pa;
	bus_size_t memsize;
	u_long iebase;
	uint8_t myaddr[ETHER_ADDR_LEN];

	sc->sc_dev = self;
	sc->bt = oba->oba_bustag;

	sc->hwreset = ie_obreset;
	sc->chan_attn = ie_obattend;
	sc->hwinit = ie_obrun;
	sc->memcopyout = ie_obio_memcopyout;
	sc->memcopyin = ie_obio_memcopyin;

	sc->ie_bus_barrier = NULL;
	sc->ie_bus_read16 = ie_obio_read16;
	sc->ie_bus_write16 = ie_obio_write16;
	sc->ie_bus_write24 = ie_obio_write24;
	sc->sc_msize = memsize = 65536; /* XXX */

	if (bus_space_map(oba->oba_bustag, oba->oba_paddr,
			  sizeof(struct ieob),
			  BUS_SPACE_MAP_LINEAR,
			  &bh) != 0) {
		printf("%s: cannot map registers\n", device_xname(self));
		return;
	}
	sc->sc_reg = (void *)bh;

	/*
	 * Allocate control & buffer memory.
	 */
	if ((error = bus_dmamap_create(dmatag, memsize, 1, memsize, 0,
					BUS_DMA_NOWAIT|BUS_DMA_24BIT,
					&sc->sc_dmamap)) != 0) {
		printf("%s: DMA map create error %d\n",
		    device_xname(self), error);
		return;
	}
	if ((error = bus_dmamem_alloc(dmatag, memsize, 64*1024, 0,
			     &seg, 1, &rseg,
			     BUS_DMA_NOWAIT | BUS_DMA_24BIT)) != 0) {
		printf("%s: DMA memory allocation error %d\n",
		    device_xname(self), error);
		return;
	}

	/* Map DMA buffer in CPU addressable space */
	if ((error = bus_dmamem_map(dmatag, &seg, rseg, memsize,
				    (void **)&sc->sc_maddr,
				    BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) != 0) {
		printf("%s: DMA buffer map error %d\n",
		    device_xname(self), error);
		bus_dmamem_free(dmatag, &seg, rseg);
		return;
	}

	/* Load the segment */
	if ((error = bus_dmamap_load(dmatag, sc->sc_dmamap,
				     sc->sc_maddr, memsize, NULL,
				     BUS_DMA_NOWAIT)) != 0) {
		printf("%s: DMA buffer map load error %d\n",
		    device_xname(self), error);
		bus_dmamem_unmap(dmatag, sc->sc_maddr, memsize);
		bus_dmamem_free(dmatag, &seg, rseg);
		return;
	}

	wzero(sc->sc_maddr, memsize);
	sc->bh = (bus_space_handle_t)(sc->sc_maddr);

	/*
	 * The i82586's 24-bit address space maps to the last 16MB of
	 * KVA space ().  In addition, the SCP must appear
	 * at IE_SCP_ADDR within the 24-bit address space,
	 * i.e. at KVA  IEOB_ADBASE+IE_SCP_ADDR, at the very top of
	 * kernel space.  We double-map this last page to the first
	 * page (starting at `maddr') of the memory we allocate to the chip.
	 * (a side-effect of this double-map is that the ISCP and SCB
	 * structures also get aliased there, but we ignore this). The
	 * first page at `maddr' is only used for ISCP, SCB and the aliased
	 * SCP; the actual buffers start at maddr+PAGE_SIZE.
	 *
	 * In a picture:

	|---//--- ISCP-SCB-----scp-|--//- buffers -//-|... |iscp-scb-----SCP-|
	|         |                |                  |    |             |   |
	|         |<---PAGE_SIZE-->|                  |    |<--PAGE_SIZE-+-->|
	|         |<------------ memsize ------------>|    |       ^     |
	|         |                                                |     |
	|         \@maddr                                 (last page dbl mapped)
	|                                                                |
	\@IEOB_ADBASE                           @IEOB_ADBASE+IE_SCP_ADDR-+

	 *
	 */

	/* Double map the SCP */
	if (pmap_extract(pmap_kernel(), (vaddr_t)sc->sc_maddr, &pa) == false)
		panic("ie pmap_extract");

	pmap_enter(pmap_kernel(), trunc_page(IEOB_ADBASE+IE_SCP_ADDR),
	    pa | PMAP_NC /*| PMAP_IOC*/,
	    VM_PROT_READ | VM_PROT_WRITE, PMAP_WIRED);
	pmap_update(pmap_kernel());

	/* Map iscp at location 0 (relative to `maddr') */
	sc->iscp = 0;

	/* scb follows iscp */
	sc->scb = IE_ISCP_SZ;

	/* scp is at the fixed location IE_SCP_ADDR (modulo the page size) */
	sc->scp = IE_SCP_ADDR & PGOFSET;

	/* Calculate the 24-bit base of i82586 operations */
	iebase = (u_long)sc->sc_dmamap->dm_segs[0].ds_addr -
			(u_long)IEOB_ADBASE;
	ie_obio_write16(sc, IE_ISCP_SCB(sc->iscp), sc->scb);
	ie_obio_write24(sc, IE_ISCP_BASE(sc->iscp), iebase);
	ie_obio_write24(sc, IE_SCP_ISCP(sc->scp), iebase + sc->iscp);

	/*
	 * Rest of first page is unused (wasted!); the other pages
	 * are used for buffers.
	 */
	sc->buf_area = PAGE_SIZE;
	sc->buf_area_sz = memsize - PAGE_SIZE;

	if (i82586_proberam(sc) == 0) {
		printf(": memory probe failed\n");
		return;
	}

	prom_getether(0, myaddr);
	i82586_attach(sc, "onboard", myaddr, media, NMEDIA, media[0]);

	/* Establish interrupt channel */
	(void)bus_intr_establish(oba->oba_bustag,
				 oba->oba_pri, IPL_NET,
				 i82586_intr, sc);
}
