/*	$NetBSD: if_le_isa.c,v 1.10.2.2 1997/05/17 00:30:46 thorpej Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Copyright (c) 1995 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)if_le.c	8.2 (Berkeley) 11/16/93
 */

#include "bpfilter.h"

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

#include <vm/vm.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/ic/am7990reg.h>
#include <dev/ic/am7990var.h>

#include <dev/isa/if_levar.h>

static char *card_type[] =
    { "unknown", "BICC Isolan", "NE2100", "DEPCA", "PCnet-ISA" };

int le_isa_probe __P((struct device *, void *, void *));
void le_isa_attach __P((struct device *, struct device *, void *));

struct cfattach le_isa_ca = {
	sizeof(struct le_softc), le_isa_probe, le_isa_attach
};

int depca_isa_probe __P((struct le_softc *, struct isa_attach_args *));
int ne2100_isa_probe __P((struct le_softc *, struct isa_attach_args *));
int bicc_isa_probe __P((struct le_softc *, struct isa_attach_args *));
int lance_isa_probe __P((struct am7990_softc *));

int le_isa_intredge __P((void *));

hide void le_isa_wrcsr __P((struct am7990_softc *, u_int16_t, u_int16_t));
hide u_int16_t le_isa_rdcsr __P((struct am7990_softc *, u_int16_t));  

void	depca_copytobuf __P((struct am7990_softc *, void *, int, int));
void	depca_copyfrombuf __P((struct am7990_softc *, void *, int, int));
void	depca_zerobuf __P((struct am7990_softc *, int, int));

#define	LE_ISA_MEMSIZE	16384

hide void
le_isa_wrcsr(sc, port, val)
	struct am7990_softc *sc;
	u_int16_t port, val;
{
	struct le_softc *lesc = (struct le_softc *)sc;
	bus_space_tag_t iot = lesc->sc_iot;
	bus_space_handle_t ioh = lesc->sc_ioh;

	bus_space_write_2(iot, ioh, lesc->sc_rap, port);
	bus_space_write_2(iot, ioh, lesc->sc_rdp, val);
}

hide u_int16_t
le_isa_rdcsr(sc, port)
	struct am7990_softc *sc;
	u_int16_t port;
{
	struct le_softc *lesc = (struct le_softc *)sc;
	bus_space_tag_t iot = lesc->sc_iot;
	bus_space_handle_t ioh = lesc->sc_ioh; 
	u_int16_t val;

	bus_space_write_2(iot, ioh, lesc->sc_rap, port);
	val = bus_space_read_2(iot, ioh, lesc->sc_rdp); 
	return (val);
}

int
le_isa_probe(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct le_softc *lesc = match;
	struct isa_attach_args *ia = aux;

	if (bicc_isa_probe(lesc, ia))
		return (1);
	if (ne2100_isa_probe(lesc, ia))
		return (1);
	if (depca_isa_probe(lesc, ia))
		return (1);

	return (0);
}

int
depca_isa_probe(lesc, ia)
	struct le_softc *lesc;
	struct isa_attach_args *ia;
{
	struct am7990_softc *sc = &lesc->sc_am7990;
	int iobase = ia->ia_iobase, port;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	bus_space_handle_t memh;
#if 0
	u_int32_t sum, rom_sum;
	u_int8_t x;
#endif
	int i;

	/* Map i/o space. */
	if (bus_space_map(iot, iobase, 16, 0, &ioh))
		return 0;

	if (ia->ia_maddr == MADDRUNK || ia->ia_msize == -1)
		goto bad;

	/* Map card RAM. */
	if (bus_space_map(ia->ia_memt, ia->ia_maddr, ia->ia_msize,
	    0, &memh))
		goto bad;

	/* Just needed to check mapability; don't need it anymore. */
	bus_space_unmap(ia->ia_memt, memh, ia->ia_msize);

	lesc->sc_iot = iot;
	lesc->sc_ioh = ioh;
	lesc->sc_rap = DEPCA_RAP;
	lesc->sc_rdp = DEPCA_RDP;
	lesc->sc_card = DEPCA;

	if (lance_isa_probe(sc) == 0)
		goto bad;

	bus_space_write_1(iot, ioh, DEPCA_CSR, DEPCA_CSR_DUM);

	/*
	 * Extract the physical MAC address from the ROM.
	 *
	 * The address PROM is 32 bytes wide, and we access it through
	 * a single I/O port.  On each read, it rotates to the next
	 * position.  We find the ethernet address by looking for a
	 * particular sequence of bytes (0xff, 0x00, 0x55, 0xaa, 0xff,
	 * 0x00, 0x55, 0xaa), and then reading the next 8 bytes (the
	 * ethernet address and a checksum).
	 *
	 * It appears that the PROM can be at one of two locations, so
	 * we just try both.
	 */
	port = DEPCA_ADP;
	for (i = 0; i < 32; i++)
		if (bus_space_read_1(iot, ioh, port) == 0xff &&
		    bus_space_read_1(iot, ioh, port) == 0x00 &&
		    bus_space_read_1(iot, ioh, port) == 0x55 &&
		    bus_space_read_1(iot, ioh, port) == 0xaa &&
		    bus_space_read_1(iot, ioh, port) == 0xff &&
		    bus_space_read_1(iot, ioh, port) == 0x00 &&
		    bus_space_read_1(iot, ioh, port) == 0x55 &&
		    bus_space_read_1(iot, ioh, port) == 0xaa)
			goto found;
	port = DEPCA_ADP + 1;
	for (i = 0; i < 32; i++)
		if (bus_space_read_1(iot, ioh, port) == 0xff &&
		    bus_space_read_1(iot, ioh, port) == 0x00 &&
		    bus_space_read_1(iot, ioh, port) == 0x55 &&
		    bus_space_read_1(iot, ioh, port) == 0xaa &&
		    bus_space_read_1(iot, ioh, port) == 0xff &&
		    bus_space_read_1(iot, ioh, port) == 0x00 &&
		    bus_space_read_1(iot, ioh, port) == 0x55 &&
		    bus_space_read_1(iot, ioh, port) == 0xaa)
			goto found;
	printf("%s: address not found\n", sc->sc_dev.dv_xname);
	goto bad;

found:
	for (i = 0; i < sizeof(sc->sc_enaddr); i++)
		sc->sc_enaddr[i] = bus_space_read_1(iot, ioh, port);

#if 0
	sum =
	    (sc->sc_enaddr[0] <<  2) +
	    (sc->sc_enaddr[1] << 10) +
	    (sc->sc_enaddr[2] <<  1) +
	    (sc->sc_enaddr[3] <<  9) +
	    (sc->sc_enaddr[4] <<  0) +
	    (sc->sc_enaddr[5] <<  8);
	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);

	rom_sum = bus_space_read_1(iot, ioh, port);
	rom_sum |= bus_space_read_1(iot, ioh, port) << 8;

	if (sum != rom_sum) {
		printf("%s: checksum mismatch; calculated %04x != read %04x",
		    sc->sc_dev.dv_xname, sum, rom_sum);
		goto bad;
	}
#endif

	bus_space_write_1(iot, ioh, DEPCA_CSR, DEPCA_CSR_NORMAL);

	/*
	 * XXX INDIRECT BROKENNESS!
	 * XXX Should always unmap, and re-map in if_le_isa_attach().
	 */

	ia->ia_iosize = 16;
	ia->ia_drq = DRQUNK;
	return 1;

 bad:
	bus_space_unmap(iot, ioh, 16);
	return 0;
}

int
ne2100_isa_probe(lesc, ia)
	struct le_softc *lesc;
	struct isa_attach_args *ia;
{
	struct am7990_softc *sc = &lesc->sc_am7990;
	int iobase = ia->ia_iobase;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	int i;

	/* Map i/o space. */
	if (bus_space_map(iot, iobase, 24, 0, &ioh))
		return 0;

	lesc->sc_iot = iot;
	lesc->sc_ioh = ioh;
	lesc->sc_rap = NE2100_RAP;
	lesc->sc_rdp = NE2100_RDP;
	lesc->sc_card = NE2100;

	if (lance_isa_probe(sc) == 0) {
		bus_space_unmap(iot, ioh, 24);
		return 0;
	}

	/*
	 * Extract the physical MAC address from the ROM.
	 */
	for (i = 0; i < sizeof(sc->sc_enaddr); i++)
		sc->sc_enaddr[i] = bus_space_read_1(iot, ioh, i);

	/*
	 * XXX INDIRECT BROKENNESS!
	 * XXX Should always unmap, and re-map in if_le_isa_attach().
	 */

	ia->ia_iosize = 24;
	return 1;
}

int
bicc_isa_probe(lesc, ia)
	struct le_softc *lesc;
	struct isa_attach_args *ia;
{
	struct am7990_softc *sc = &lesc->sc_am7990;
	int iobase = ia->ia_iobase;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	int i;

	/* Map i/o space. */
	if (bus_space_map(iot, iobase, 16, 0, &ioh))
		return 0;

	lesc->sc_iot = iot;
	lesc->sc_ioh = ioh;
	lesc->sc_rap = BICC_RAP;
	lesc->sc_rdp = BICC_RDP;
	lesc->sc_card = BICC;

	if (lance_isa_probe(sc) == 0) {
		bus_space_unmap(iot, ioh, 16);
		return 0;
	}

	/*
	 * Extract the physical MAC address from the ROM.
	 */
	for (i = 0; i < sizeof(sc->sc_enaddr); i++)
		sc->sc_enaddr[i] = bus_space_read_1(iot, ioh, i * 2);

	/*
	 * XXX INDIRECT BROKENNESS!
	 * XXX Should always unmap, and re-map in if_le_isa_attach().
	 */

	ia->ia_iosize = 16;
	return 1;
}

/*
 * Determine which chip is present on the card.
 */
int
lance_isa_probe(sc)
	struct am7990_softc *sc;
{

	/* Stop the LANCE chip and put it in a known state. */
	le_isa_wrcsr(sc, LE_CSR0, LE_C0_STOP);
	delay(100);

	if (le_isa_rdcsr(sc, LE_CSR0) != LE_C0_STOP)
		return 0;

	le_isa_wrcsr(sc, LE_CSR3, sc->sc_conf3);
	return 1;
}

void
le_isa_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct le_softc *lesc = (void *)self;
	struct am7990_softc *sc = &lesc->sc_am7990;
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_tag_t memt = ia->ia_memt;
	bus_dma_tag_t dmat = ia->ia_dmat;
	bus_space_handle_t memh;
	bus_dma_segment_t seg;
	int rseg;

	printf(": %s Ethernet\n", card_type[lesc->sc_card]);

	lesc->sc_iot = iot;
	lesc->sc_memt = memt;
	lesc->sc_dmat = dmat;

	/* XXX SHOULD RE-MAP I/O SPACE HERE. */

	if (lesc->sc_card == DEPCA) {
		u_char val;
		int i;

		/* Map shared memory. */
		if (bus_space_map(memt, ia->ia_maddr, ia->ia_msize,
		    0, &memh))
			panic("le_isa_attach: can't map shared memory");

		lesc->sc_memh = memh;

		val = 0xff;
		for (;;) {
#if 0	/* XXX !! */
			bus_space_set_region_1(memt, memh, 0, val);
#else
			for (i = 0; i < ia->ia_msize; i++)
				bus_space_write_1(memt, memh, i, val);
#endif
			for (i = 0; i < ia->ia_msize; i++)
				if (bus_space_read_1(memt, memh, 1) != val) {
					printf("%s: failed to clear memory\n",
					    sc->sc_dev.dv_xname);
					return;
				}
			if (val == 0x00)
				break;
			val -= 0x55;
		}

		sc->sc_conf3 = LE_C3_ACON;
		sc->sc_mem = 0;			/* Not used. */
		sc->sc_addr = 0;
		sc->sc_memsize = ia->ia_msize;
	} else {
		/*
		 * Allocate a DMA area for the card.
		 */
		if (bus_dmamem_alloc(dmat, LE_ISA_MEMSIZE, NBPG, 0, &seg, 1,
		    &rseg, BUS_DMA_NOWAIT)) {
			printf("%s: couldn't allocate memory for card\n",
			    sc->sc_dev.dv_xname);
			return;
		}
		if (bus_dmamem_map(dmat, &seg, rseg, LE_ISA_MEMSIZE,
		    (caddr_t *)&sc->sc_mem,
		    BUS_DMA_NOWAIT|BUS_DMAMEM_NOSYNC)) {
			printf("%s: couldn't map memory for card\n",
			    sc->sc_dev.dv_xname);
			return;
		}

		/*
		 * Create and load the DMA map for the DMA area.
		 */
		if (bus_dmamap_create(dmat, LE_ISA_MEMSIZE, 1,
		    LE_ISA_MEMSIZE, 0, BUS_DMA_NOWAIT, &lesc->sc_dmam)) {
			printf("%s: couldn't create DMA map\n",
			    sc->sc_dev.dv_xname);
			bus_dmamem_free(dmat, &seg, rseg);
			return;
		}
		if (bus_dmamap_load(dmat, lesc->sc_dmam,
		    sc->sc_mem, LE_ISA_MEMSIZE, NULL, BUS_DMA_NOWAIT)) {
			printf("%s: coundn't load DMA map\n",
			    sc->sc_dev.dv_xname);
			bus_dmamem_free(dmat, &seg, rseg);
			return;
		}

		sc->sc_conf3 = 0;
		sc->sc_addr = lesc->sc_dmam->dm_segs[0].ds_addr;
		sc->sc_memsize = LE_ISA_MEMSIZE;
	}

	if (lesc->sc_card == DEPCA) {
		sc->sc_copytodesc = depca_copytobuf;
		sc->sc_copyfromdesc = depca_copyfrombuf;
		sc->sc_copytobuf = depca_copytobuf;
		sc->sc_copyfrombuf = depca_copyfrombuf;
		sc->sc_zerobuf = depca_zerobuf;
	} else {
		sc->sc_copytodesc = am7990_copytobuf_contig;
		sc->sc_copyfromdesc = am7990_copyfrombuf_contig;
		sc->sc_copytobuf = am7990_copytobuf_contig;
		sc->sc_copyfrombuf = am7990_copyfrombuf_contig;
		sc->sc_zerobuf = am7990_zerobuf_contig;
	}

	sc->sc_rdcsr = le_isa_rdcsr;
	sc->sc_wrcsr = le_isa_wrcsr;
	sc->sc_hwinit = NULL;

	printf("%s", sc->sc_dev.dv_xname);
	am7990_config(sc);

	if (ia->ia_drq != DRQUNK)
		isa_dmacascade(parent, ia->ia_drq);

	lesc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq, IST_EDGE,
	    IPL_NET, le_isa_intredge, sc);
}

/*
 * Controller interrupt.
 */
int
le_isa_intredge(arg)
	void *arg;
{

	if (am7990_intr(arg) == 0)
		return (0);
	for (;;)
		if (am7990_intr(arg) == 0)
			return (1);
}

/*
 * DEPCA shared memory access functions.
 */

void
depca_copytobuf(sc, from, boff, len)
	struct am7990_softc *sc;
	void *from;
	int boff, len;
{
	struct le_softc *lesc = (struct le_softc *)sc;

	bus_space_write_region_1(lesc->sc_memt, lesc->sc_memh, boff,
	    from, len);
}

void
depca_copyfrombuf(sc, to, boff, len)
	struct am7990_softc *sc;  
	void *to;
	int boff, len;
{
	struct le_softc *lesc = (struct le_softc *)sc;

	bus_space_read_region_1(lesc->sc_memt, lesc->sc_memh, boff,
	    to, len);
}

void
depca_zerobuf(sc, boff, len)
	struct am7990_softc *sc;
	int boff, len;
{
	struct le_softc *lesc = (struct le_softc *)sc;

#if 0	/* XXX !! */
	bus_space_set_region_1(lesc->sc_memt, lesc->sc_memh, boff,
	    0x00, len);
#else
	for (; len != 0; boff++, len--)
		bus_space_write_1(lesc->sc_memt, lesc->sc_memh, boff, 0x00);
#endif
}
