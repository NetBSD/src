/*	$NetBSD: if_ie_obio.c,v 1.12 1999/11/13 00:32:12 thorpej Exp $	*/

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

#include <vm/vm.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/pmap.h>
#include <machine/bus.h>

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


static void ie_obreset __P((struct ie_softc *, int));
static void ie_obattend __P((struct ie_softc *));
static void ie_obrun __P((struct ie_softc *));

int ie_obio_match __P((struct device *, struct cfdata *, void *));
void ie_obio_attach __P((struct device *, struct device *, void *));

struct cfattach ie_obio_ca = {
	sizeof(struct ie_softc), ie_obio_match, ie_obio_attach
};

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
ie_obattend(sc)
	struct ie_softc *sc;
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
	void *addr = (void *)((u_long)sc->bh + offset);/*XXX - not MI!*/
	wcopy(addr, p, size);
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
	void *addr = (void *)((u_long)sc->bh + offset);/*XXX - not MI!*/
	wcopy(p, addr, size);
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
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;

	if (uoba->uoba_isobio4 == 0)
		return (0);

	oba = &uoba->uoba_oba4;
	return (bus_space_probe(oba->oba_bustag, 0, oba->oba_paddr,
				1,	/* probe size */
				0,	/* offset */
				0,	/* flags */
				NULL, NULL));
}

void
ie_obio_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void   *aux;
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba = &uoba->uoba_oba4;
	struct ie_softc *sc = (void *) self;
	bus_space_handle_t bh;
	bus_dma_segment_t seg;
	int rseg;
	struct bootpath *bp;
	paddr_t pa;
	struct intrhand *ih;
	u_long iebase;
	u_int8_t myaddr[ETHER_ADDR_LEN];
extern	void myetheraddr(u_char *);	/* should be elsewhere */

	sc->bt = oba->oba_bustag;

	sc->hwreset = ie_obreset;
	sc->chan_attn = ie_obattend;
	sc->hwinit = ie_obrun;
	sc->memcopyout = ie_obio_memcopyout;
	sc->memcopyin = ie_obio_memcopyin;
	sc->ie_bus_read16 = ie_obio_read16;
	sc->ie_bus_write16 = ie_obio_write16;
	sc->ie_bus_write24 = ie_obio_write24;
	sc->sc_msize = 65536; /* XXX */

	if (obio_bus_map(oba->oba_bustag, oba->oba_paddr,
			 0,
			 sizeof(struct ieob),
			 BUS_SPACE_MAP_LINEAR,
			 0, &bh) != 0) {
		printf("%s: cannot map registers\n", self->dv_xname);
		return;
	}
	sc->sc_reg = (void *)bh;

	/*
	 * Allocate control & buffer memory.
	 */
	if (bus_dmamem_alloc(oba->oba_dmatag, sc->sc_msize, 64*1024, 0,
			     &seg, 1, &rseg,
			     BUS_DMA_NOWAIT | BUS_DMA_24BIT) != 0) {
		printf("%s @ obio: DMA memory allocation error\n",
			self->dv_xname);
		return;
	}
#if 0
	if (bus_dmamem_map(oba->oba_dmatag, &seg, rseg, sc->sc_msize,
			   (caddr_t *)&sc->sc_maddr,
			   BUS_DMA_NOWAIT|BUS_DMA_COHERENT) != 0) {
		printf("%s @ obio: DMA memory map error\n", self->dv_xname);
		bus_dmamem_free(oba->oba_dmatag, &seg, rseg);
		return;
	}
#else
	/*
	 * We happen to know we can use the DVMA address directly as
	 * a CPU virtual address on machines where this driver can
	 * attach (sun4's). So we possibly save a MMU resource
	 * by not asking for a double mapping.
	 */
	sc->sc_maddr = (void *)seg.ds_addr;
#endif

	wzero(sc->sc_maddr, sc->sc_msize);
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
	 * SCP; the actual buffers start at maddr+NBPG.
	 *
	 * In a picture:

	|---//--- ISCP-SCB-----scp-|--//- buffers -//-|... |iscp-scb-----SCP-|
	|         |                |                  |    |             |   |
	|         |<----- NBPG --->|                  |    |<----- NBPG -+-->|
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

	pmap_enter(pmap_kernel(), trunc_page(IEOB_ADBASE+IE_SCP_ADDR),
	    pa | PMAP_NC /*| PMAP_IOC*/,
	    VM_PROT_READ | VM_PROT_WRITE, PMAP_WIRED);

	/* Map iscp at location 0 (relative to `maddr') */
	sc->iscp = 0;

	/* scb follows iscp */
	sc->scb = IE_ISCP_SZ;

	/* scp is at the fixed location IE_SCP_ADDR (modulo the page size) */
	sc->scp = IE_SCP_ADDR & PGOFSET;

	/* Calculate the 24-bit base of i82586 operations */
	iebase = (u_long)seg.ds_addr - (u_long)IEOB_ADBASE;
	ie_obio_write16(sc, IE_ISCP_SCB(sc->iscp), sc->scb);
	ie_obio_write24(sc, IE_ISCP_BASE(sc->iscp), iebase);
	ie_obio_write24(sc, IE_SCP_ISCP(sc->scp), iebase + sc->iscp);

	/*
	 * Rest of first page is unused (wasted!); the other pages
	 * are used for buffers.
	 */
	sc->buf_area = NBPG;
	sc->buf_area_sz = sc->sc_msize - NBPG;

	if (i82586_proberam(sc) == 0) {
		printf(": memory probe failed\n");
		return;
	}

	myetheraddr(myaddr);
	i82586_attach(sc, "onboard", myaddr, media, NMEDIA, media[0]);

	/* Establish interrupt channel */
	ih = bus_intr_establish(oba->oba_bustag,
				oba->oba_pri, 0,
				i82586_intr, sc);

	bp = oba->oba_bp;
	if (bp != NULL && strcmp(bp->name, "ie") == 0 &&
	    sc->sc_dev.dv_unit == bp->val[1])
		bp->dev = &sc->sc_dev;
}
