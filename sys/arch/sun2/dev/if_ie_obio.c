/*	$NetBSD: if_ie_obio.c,v 1.6 2003/04/01 15:48:40 thorpej Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg and Matt Fredette.
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

/*-
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Charles D. Cranor.
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
 * Sun2 OBIO front-end for the Intel 82586 Ethernet driver
 *
 * Converted to SUN ie driver by Charles D. Cranor,
 *		October 1994, January 1995.
 */

/*
 * The i82586 is a very painful chip, found in sun[23]'s, sun-4/100's
 * sun-4/200's, and VME based suns.  The byte order is all wrong for a
 * SUN, making life difficult.  Programming this chip is mostly the same,
 * but certain details differ from system to system.  This driver is
 * written so that different "ie" interfaces can be controled by the same
 * driver.
 */

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

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/autoconf.h>
#include <machine/idprom.h>

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

#define IEOB_ADBASE 0x000000  /* KVA base addr of 24 bit address space */


static void ie_obreset __P((struct ie_softc *, int));
static void ie_obattend __P((struct ie_softc *, int));
static void ie_obrun __P((struct ie_softc *));

int ie_obio_match __P((struct device *, struct cfdata *, void *));
void ie_obio_attach __P((struct device *, struct device *, void *));

CFATTACH_DECL(ie_obio, sizeof(struct ie_softc),
    ie_obio_match, ie_obio_attach, NULL, NULL);

/* Supported media */
static int media[] = {
	IFM_ETHER | IFM_10_2,
};      
#define NMEDIA	(sizeof(media) / sizeof(media[0]))


/*
 * OBIO ie support routines
 */
void
ie_obreset(sc, what)
	struct ie_softc *sc;
{
	volatile struct ieob *ieo = (struct ieob *) sc->sc_reg;
	ieo->obctrl = 0;
	delay(100);			/* XXX could be shorter? */
	ieo->obctrl = IEOB_NORSET;
}
void
ie_obattend(sc, why)
	struct ie_softc *sc;
	int why;
{
	volatile struct ieob *ieo = (struct ieob *) sc->sc_reg;

	ieo->obctrl |= IEOB_ATTEN;	/* flag! */
	ieo->obctrl &= ~IEOB_ATTEN;	/* down. */
}

void
ie_obrun(sc)
	struct ie_softc *sc;
{
	volatile struct ieob *ieo = (struct ieob *) sc->sc_reg;

	ieo->obctrl |= (IEOB_ONAIR|IEOB_IENAB|IEOB_NORSET);
}

void ie_obio_memcopyin __P((struct ie_softc *, void *, int, size_t));
void ie_obio_memcopyout __P((struct ie_softc *, const void *, int, size_t));

/*
 * Copy board memory to kernel.
 */
void
ie_obio_memcopyin(sc, p, offset, size)
	struct ie_softc	*sc;
	void *p;
	int offset;
	size_t size;
{
	bus_space_copyin(sc->bt, sc->bh, offset, p, size);
}

/*
 * Copy from kernel space to naord memory.
 */
void
ie_obio_memcopyout(sc, p, offset, size)
	struct ie_softc	*sc;
	const void *p;
	int offset;
	size_t size;
{
	bus_space_copyout(sc->bt, sc->bh, offset, p, size);
}

/* read a 16-bit value at BH offset */
u_int16_t ie_obio_read16 __P((struct ie_softc *, int offset));
/* write a 16-bit value at BH offset */
void ie_obio_write16 __P((struct ie_softc *, int offset, u_int16_t value));
void ie_obio_write24 __P((struct ie_softc *, int offset, int addr));

u_int16_t
ie_obio_read16(sc, offset)
	struct ie_softc *sc;
	int offset;
{
	u_int16_t v = bus_space_read_2(sc->bt, sc->bh, offset);
	return (((v&0xff)<<8) | ((v>>8)&0xff));
}

void
ie_obio_write16(sc, offset, v)
	struct ie_softc *sc;	
	int offset;
	u_int16_t v;
{
	v = (((v&0xff)<<8) | ((v>>8)&0xff));
	bus_space_write_2(sc->bt, sc->bh, offset, v);
}

void
ie_obio_write24(sc, offset, addr)
	struct ie_softc *sc;	
	int offset;
	int addr;
{
	u_char *f = (u_char *)&addr;
	u_int16_t v0, v1;
	u_char *t;

	t = (u_char *)&v0;
	t[0] = f[3]; t[1] = f[2];
	bus_space_write_2(sc->bt, sc->bh, offset, v0);

	t = (u_char *)&v1;
	t[0] = f[1]; t[1] = 0;
	bus_space_write_2(sc->bt, sc->bh, offset+2, v1);
}

int
ie_obio_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct obio_attach_args *oba = aux;
	bus_space_handle_t bh;
	int matched;
	u_int8_t ctrl;

	/* No default obio address. */
	if (oba->oba_paddr == -1)
		return(0);
        
	/* Make sure there is something there... */
	if (bus_space_map(oba->oba_bustag, oba->oba_paddr, sizeof(struct ieob), 
			  0, &bh))
		return (0);
	matched = (!bus_space_poke_1(oba->oba_bustag, bh, 0, IEOB_NORSET) &&
		!bus_space_peek_1(oba->oba_bustag, bh, 0, &ctrl) &&
		(ctrl & (IEOB_ONAIR|IEOB_IENAB)) == 0);
	bus_space_unmap(oba->oba_bustag, bh, sizeof(struct ieob));
	if (!matched)
		return (0);

	/* Default interrupt priority. */
	if (oba->oba_pri == -1)
		oba->oba_pri = 3;

	return (1);
}

void
ie_obio_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void   *aux;
{
	struct obio_attach_args *oba = aux;
	struct ie_softc *sc = (void *) self;
	bus_dma_tag_t dmatag = oba->oba_dmatag;
	bus_space_handle_t bh;
	bus_dma_segment_t seg;
	int rseg;
	int error;
	paddr_t pa;
	struct intrhand *ih;
	bus_size_t msize;
	u_long iebase;
	u_int8_t myaddr[ETHER_ADDR_LEN];

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
	sc->sc_msize = msize = 65536; /* XXX */

	if (bus_space_map(oba->oba_bustag, oba->oba_paddr, sizeof(struct ieob),
  			0, &bh))
		panic("ie_obio_attach: can't map regs");
	sc->sc_reg = (void *)bh;

	/*
	 * Allocate control & buffer memory.
	 */
	if ((error = bus_dmamap_create(dmatag, msize, 1, msize, 0,
					BUS_DMA_NOWAIT|BUS_DMA_24BIT,
					&sc->sc_dmamap)) != 0) {
		printf("%s: DMA map create error %d\n",
			sc->sc_dev.dv_xname, error);
		return;
	}
	if ((error = bus_dmamem_alloc(dmatag, msize, 64*1024, 0,
			     &seg, 1, &rseg,
			     BUS_DMA_NOWAIT | BUS_DMA_24BIT)) != 0) {
		printf("%s: DMA memory allocation error %d\n",
			self->dv_xname, error);
		return;
	}

	/* Map DMA buffer in CPU addressable space */
	if ((error = bus_dmamem_map(dmatag, &seg, rseg, msize,
				    (caddr_t *)&sc->sc_maddr,
				    BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) != 0) {
		printf("%s: DMA buffer map error %d\n",
			sc->sc_dev.dv_xname, error);
		bus_dmamem_free(dmatag, &seg, rseg);
		return;
	}

	/* Load the segment */
	if ((error = bus_dmamap_load(dmatag, sc->sc_dmamap,
				     sc->sc_maddr, msize, NULL,
				     BUS_DMA_NOWAIT)) != 0) {
		printf("%s: DMA buffer map load error %d\n",
			sc->sc_dev.dv_xname, error);
		bus_dmamem_unmap(dmatag, sc->sc_maddr, msize);
		bus_dmamem_free(dmatag, &seg, rseg);
		return;
	}

	w16zero(sc->sc_maddr, msize);
	sc->bh = (bus_space_handle_t)(sc->sc_maddr);

	/*
	 * The i82586's 24-bit address space maps to all of
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
	|         |<------------- msize ------------->|    |       ^     |
	|         |                                                |     |
	|         \@maddr                                 (last page dbl mapped)
	|                                                                |
	\@IEOB_ADBASE                           @IEOB_ADBASE+IE_SCP_ADDR-+

	 *
	 */

	/* Double map the SCP */
	if (pmap_extract(pmap_kernel(), (vaddr_t)sc->sc_maddr, &pa) == FALSE)
		panic("ie pmap_extract");

	pmap_enter(pmap_kernel(), m68k_trunc_page(IEOB_ADBASE+IE_SCP_ADDR),
	    pa | PMAP_NC /*| PMAP_IOC*/,
	    VM_PROT_READ | VM_PROT_WRITE, PMAP_WIRED);

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
	sc->buf_area_sz = msize - PAGE_SIZE;

	if (i82586_proberam(sc) == 0) {
		printf(": memory probe failed\n");
		return;
	}

	idprom_etheraddr(myaddr);
	i82586_attach(sc, "onboard", myaddr, media, NMEDIA, media[0]);

	/* Establish interrupt channel */
	ih = bus_intr_establish(oba->oba_bustag,
				oba->oba_pri, IPL_NET, 0,
				i82586_intr, sc);
}
