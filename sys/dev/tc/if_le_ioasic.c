/*	$NetBSD: if_le_ioasic.c,v 1.14 1999/10/01 09:19:42 nisimura Exp $	*/

/*
 * Copyright (c) 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * LANCE on DEC IOCTL ASIC.
 */

#include <sys/cdefs.h>			/* RCS ID &  macro defns */
__KERNEL_RCSID(0, "$NetBSD: if_le_ioasic.c,v 1.14 1999/10/01 09:19:42 nisimura Exp $");

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <dev/ic/lancereg.h>
#include <dev/ic/lancevar.h>
#include <dev/ic/am7990reg.h>
#include <dev/ic/am7990var.h>

#include <dev/tc/if_levar.h>
#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicreg.h>
#include <dev/tc/ioasicvar.h>

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_ddb.h"
#endif

caddr_t le_iomem;

static int  le_ioasic_match __P((struct device *, struct cfdata *, void *));
static void le_ioasic_attach __P((struct device *, struct device *, void *));

struct cfattach le_ioasic_ca = {
	sizeof(struct le_softc), le_ioasic_match, le_ioasic_attach
};

static void ioasic_lance_dma_setup __P((struct device *));
static char *ioasic_lance_ether_address __P((void));

#ifdef DDB
#define	integrate
#define hide
#else
#define	integrate	static __inline
#define hide		static
#endif

hide void le_ioasic_copytobuf_gap2 __P((struct lance_softc *, void *,
	    int, int));
hide void le_ioasic_copyfrombuf_gap2 __P((struct lance_softc *, void *,
	    int, int));

hide void le_ioasic_copytobuf_gap16 __P((struct lance_softc *, void *,
	    int, int));
hide void le_ioasic_copyfrombuf_gap16 __P((struct lance_softc *, void *,
	    int, int));
hide void le_ioasic_zerobuf_gap16 __P((struct lance_softc *, int, int));

int
le_ioasic_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct ioasicdev_attach_args *d = aux;

	if (!ioasic_submatch(match, aux))
		return (0);
	if (strncmp("lance", d->iada_modname, TC_ROM_LLEN))
		return (0);

	return (1);
}

void
le_ioasic_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ioasicdev_attach_args *d = aux;
	register struct le_softc *lesc = (void *)self;
	register struct lance_softc *sc = &lesc->sc_am7990.lsc;

	ioasic_lance_dma_setup(parent);

	if (le_iomem == 0) {
		printf("%s: DMA area not set up\n", sc->sc_dev.dv_xname);
		return;
	}

	lesc->sc_r1 = (struct lereg1 *)
		TC_DENSE_TO_SPARSE(TC_PHYS_TO_UNCACHED(d->iada_addr));
	sc->sc_mem = (void *)TC_PHYS_TO_UNCACHED(le_iomem);

	sc->sc_copytodesc = le_ioasic_copytobuf_gap2;
	sc->sc_copyfromdesc = le_ioasic_copyfrombuf_gap2;
	sc->sc_copytobuf = le_ioasic_copytobuf_gap16;
	sc->sc_copyfrombuf = le_ioasic_copyfrombuf_gap16;
	sc->sc_zerobuf = le_ioasic_zerobuf_gap16;

	dec_le_common_attach(&lesc->sc_am7990, ioasic_lance_ether_address());

	ioasic_intr_establish(parent, d->iada_cookie, TC_IPL_NET,
	    am7990_intr, sc);
}

/*
 * Special memory access functions needed by ioasic-attached LANCE
 * chips.
 */

/*
 * gap2: two bytes of data followed by two bytes of pad.
 *
 * Buffers must be 4-byte aligned.  The code doesn't worry about
 * doing an extra byte.
 */

void
le_ioasic_copytobuf_gap2(sc, fromv, boff, len)
	struct lance_softc *sc;  
	void *fromv;
	int boff;
	register int len;
{
	volatile caddr_t buf = sc->sc_mem;
	register caddr_t from = fromv;
	register volatile u_int16_t *bptr;  

	if (boff & 0x1) {
		/* handle unaligned first byte */
		bptr = ((volatile u_int16_t *)buf) + (boff - 1);
		*bptr = (*from++ << 8) | (*bptr & 0xff);
		bptr += 2;  
		len--;
	} else
		bptr = ((volatile u_int16_t *)buf) + boff;
	while (len > 1) {
		*bptr = (from[1] << 8) | (from[0] & 0xff);
		bptr += 2;
		from += 2;
		len -= 2;
	}
	if (len == 1)
		*bptr = (u_int16_t)*from;
}

void
le_ioasic_copyfrombuf_gap2(sc, tov, boff, len)
	struct lance_softc *sc;
	void *tov;
	int boff, len;
{
	volatile caddr_t buf = sc->sc_mem;
	register caddr_t to = tov;
	register volatile u_int16_t *bptr;
	register u_int16_t tmp;

	if (boff & 0x1) {
		/* handle unaligned first byte */
		bptr = ((volatile u_int16_t *)buf) + (boff - 1);
		*to++ = (*bptr >> 8) & 0xff;
		bptr += 2;
		len--;
	} else
		bptr = ((volatile u_int16_t *)buf) + boff;
	while (len > 1) {
		tmp = *bptr;
		*to++ = tmp & 0xff;
		*to++ = (tmp >> 8) & 0xff;
		bptr += 2;
		len -= 2;
	}
	if (len == 1)
		*to = *bptr & 0xff;
}

/*
 * gap16: 16 bytes of data followed by 16 bytes of pad.
 *
 * Buffers must be 32-byte aligned.
 */

void
le_ioasic_copytobuf_gap16(sc, fromv, boff, len)
	struct lance_softc *sc;
	void *fromv;
	int boff;
	register int len;
{
	volatile caddr_t buf = sc->sc_mem;
	register caddr_t from = fromv;
	register caddr_t bptr;

	bptr = buf + ((boff << 1) & ~0x1f);
	boff &= 0xf;

	/*
	 * Dispose of boff so destination of subsequent copies is
	 * 16-byte aligned.
	 */
	if (boff) {
		register int xfer;
		xfer = min(len, 16 - boff);
		bcopy(from, bptr + boff, xfer);
		from += xfer;
		bptr += 32;
		len -= xfer;
	}

	/* Destination of  copies is now 16-byte aligned. */
	if (len >= 16)
		switch ((u_long)from & (sizeof(u_int32_t) -1)) {
		case 2:
			/*  Ethernet headers make this the dominant case. */
		do {
			register u_int32_t *dst = (u_int32_t*)bptr;
			register u_int16_t t0;
			register u_int32_t t1,  t2, t3, t4;

			/* read from odd-16-bit-aligned, cached src */
			t0 = *(u_int16_t*)from;
			t1 = *(u_int32_t*)(from+2);
			t2 = *(u_int32_t*)(from+6);
			t3 = *(u_int32_t*)(from+10);
			t4 = *(u_int16_t*)(from+14);

			/* DMA buffer is uncached on mips */
			dst[0] =         t0 |  (t1 << 16);
			dst[1] = (t1 >> 16) |  (t2 << 16);
			dst[2] = (t2 >> 16) |  (t3 << 16);
			dst[3] = (t3 >> 16) |  (t4 << 16);

			from += 16;
			bptr += 32;
			len -= 16;
		} while (len >= 16);
		break;

		case 0:
		do {
			register u_int32_t *src = (u_int32_t*)from;
			register u_int32_t *dst = (u_int32_t*)bptr;
			register u_int32_t t0, t1, t2, t3;

			t0 = src[0]; t1 = src[1]; t2 = src[2]; t3 = src[3];
			dst[0] = t0; dst[1] = t1; dst[2] = t2; dst[3] = t3;

			from += 16;
			bptr += 32;
			len -= 16;
		} while (len >= 16);
		break;

		default: 
		/* Does odd-aligned case ever happen? */
		do {
			bcopy(from, bptr, 16);
			from += 16;
			bptr += 32;
			len -= 16;
		} while (len >= 16);
		break;
	}
	if (len)
		bcopy(from, bptr, len);
}

void
le_ioasic_copyfrombuf_gap16(sc, tov, boff, len)
	struct lance_softc *sc;
	void *tov;
	int boff, len;
{
	volatile caddr_t buf = sc->sc_mem;
	register caddr_t to = tov;
	register caddr_t bptr;

	bptr = buf + ((boff << 1) & ~0x1f);
	boff &= 0xf;

	/* Dispose of boff. source of copy is subsequently 16-byte aligned. */
	if (boff) {
		register int xfer;
		xfer = min(len, 16 - boff);
		bcopy(bptr+boff, to, xfer);
		to += xfer;
		bptr += 32;
		len -= xfer;
	}
	if (len >= 16)
	switch ((u_long)to & (sizeof(u_int32_t) -1)) {
	case 2:
		/*
		 * to is aligned to an odd 16-bit boundary.  Ethernet headers
		 * make this the dominant case (98% or more).
		 */
		do {
			register u_int32_t *src = (u_int32_t*)bptr;
			register u_int32_t t0, t1, t2, t3;

			/* read from uncached aligned DMA buf */
			t0 = src[0]; t1 = src[1]; t2 = src[2]; t3 = src[3];

			/* write to odd-16-bit-word aligned dst */
			*(u_int16_t *) (to+0)  = (u_short)  t0;
			*(u_int32_t *) (to+2)  = (t0 >> 16) |  (t1 << 16);
			*(u_int32_t *) (to+6)  = (t1 >> 16) |  (t2 << 16);
			*(u_int32_t *) (to+10) = (t2 >> 16) |  (t3 << 16);
			*(u_int16_t *) (to+14) = (t3 >> 16);
			bptr += 32;
			to += 16;
			len -= 16;
		} while (len > 16);
		break;
	case 0:
		/* 32-bit aligned aligned copy. Rare. */
		do {
			register u_int32_t *src = (u_int32_t*)bptr;
			register u_int32_t *dst = (u_int32_t*)to;
			register u_int32_t t0, t1, t2, t3;

			t0 = src[0]; t1 = src[1]; t2 = src[2]; t3 = src[3];
			dst[0] = t0; dst[1] = t1; dst[2] = t2; dst[3] = t3;
			to += 16;
			bptr += 32;
			len -= 16;
		} while (len  > 16);
		break;

	/* XXX Does odd-byte-aligned case ever happen? */
	default:
		do {
			bcopy(bptr, to, 16);
			to += 16;
			bptr += 32;
			len -= 16;
		} while (len  > 16);
		break;
	}
	if (len)
		bcopy(bptr, to, len);
}

void
le_ioasic_zerobuf_gap16(sc, boff, len)
	struct lance_softc *sc;
	int boff, len;
{
	volatile caddr_t buf = sc->sc_mem;
	register caddr_t bptr;
	register int xfer;

	bptr = buf + ((boff << 1) & ~0x1f);
	boff &= 0xf;
	xfer = min(len, 16 - boff);
	while (len > 0) {
		bzero(bptr + boff, xfer);
		bptr += 32;
		boff = 0;
		len -= xfer;
		xfer = min(len, 16);
	}
}

#define	LE_IOASIC_MEMSIZE	(128*1024)
#define	LE_IOASIC_MEMALIGN	(128*1024)

void
ioasic_lance_dma_setup(parent)
	struct device *parent;
{
	struct ioasic_softc *sc = (void *)parent;
	bus_dma_tag_t dmat = sc->sc_dmat;
	bus_dma_segment_t seg;
	tc_addr_t tca;
	u_int32_t ssr;
	int rseg;

	/*
	 * Allocate a DMA area for the chip.
	 */
	if (bus_dmamem_alloc(dmat, LE_IOASIC_MEMSIZE, LE_IOASIC_MEMALIGN,
	    0, &seg, 1, &rseg, BUS_DMA_NOWAIT)) {
		printf("%s: can't allocate DMA area for LANCE\n",
		    sc->sc_dv.dv_xname);
		return;
	}
	if (bus_dmamem_map(dmat, &seg, rseg, LE_IOASIC_MEMSIZE,
	    &le_iomem, BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) {
		printf("%s: can't map DMA area for LANCE\n",
		    sc->sc_dv.dv_xname);
		bus_dmamem_free(dmat, &seg, rseg);
		return;
	}

	/*
	 * Create and load the DMA map for the DMA area.
	 */
	if (bus_dmamap_create(dmat, LE_IOASIC_MEMSIZE, 1,
	    LE_IOASIC_MEMSIZE, 0, BUS_DMA_NOWAIT, &sc->sc_lance_dmam)) {
		printf("%s: can't create DMA map\n", sc->sc_dv.dv_xname);
		goto bad;
	}
	if (bus_dmamap_load(dmat, sc->sc_lance_dmam,
	    le_iomem, LE_IOASIC_MEMSIZE, NULL, BUS_DMA_NOWAIT)) {
		printf("%s: can't load DMA map\n", sc->sc_dv.dv_xname);
		goto bad;
	}

	tca = (tc_addr_t)sc->sc_lance_dmam->dm_segs[0].ds_addr;
	tca = ((tca << 3) & ~0x1f) | ((tca >> 29) & 0x1f);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_LANCE_DMAPTR, tca);
	ssr = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR);
	ssr |= IOASIC_CSR_DMAEN_LANCE;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR, ssr);
	return;

 bad:
	bus_dmamem_unmap(dmat, le_iomem, LE_IOASIC_MEMSIZE);
	bus_dmamem_free(dmat, &seg, rseg);
	le_iomem = 0;
}

/* XXX */
char *
ioasic_lance_ether_address()
{
 
        return (char *)(ioasic_base + IOASIC_SLOT_2_START);
}
