/*	$NetBSD: if_depca_isa.c,v 1.5 2000/06/28 16:27:54 mrg Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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

#include "opt_inet.h"
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

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/ic/lancereg.h>
#include <dev/ic/lancevar.h>
#include <dev/ic/am7990reg.h>
#include <dev/ic/am7990var.h>

#define	DEPCA_CSR	0x0
#define	DEPCA_CSR_SHE		0x80	/* Shared memory enabled */
#define	DEPCA_CSR_LOW32K	0x40	/* Map lower 32K chunk */
#define	DEPCA_CSR_DUM		0x08	/* rev E compatibility */
#define	DEPCA_CSR_IM		0x04	/* Interrupt masked */
#define	DEPCA_CSR_IEN		0x02	/* Interrupt enabled */
#define	DEPCA_RDP	0x4
#define	DEPCA_RAP	0x6
#define	DEPCA_ADP	0xc

struct depca_softc {
	struct	am7990_softc sc_am7990;	/* glue to MI code */

	void	*sc_ih;
	bus_space_tag_t sc_iot;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_ioh;
	bus_space_handle_t sc_memh;
};

int depca_isa_probe __P((struct device *, struct cfdata *, void *));
void depca_isa_attach __P((struct device *, struct device *, void *));
int le_depca_probe __P((struct device *, struct cfdata *, void *));
void le_depca_attach __P((struct device *, struct device *, void *));

struct cfattach depca_isa_ca = {
	sizeof(struct device), depca_isa_probe, depca_isa_attach
}, le_depca_ca = {
	sizeof(struct depca_softc), le_depca_probe, le_depca_attach
};

int depca_readprom __P((bus_space_tag_t, bus_space_handle_t, u_int8_t *));
int depca_intredge __P((void *));

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_ddb.h"
#endif

#ifdef DDB
#define	integrate
#define hide
#else
#define	integrate	static __inline
#define hide		static
#endif

hide void depca_wrcsr __P((struct lance_softc *, u_int16_t, u_int16_t));
hide u_int16_t depca_rdcsr __P((struct lance_softc *, u_int16_t));  

void	depca_copytobuf __P((struct lance_softc *, void *, int, int));
void	depca_copyfrombuf __P((struct lance_softc *, void *, int, int));
void	depca_zerobuf __P((struct lance_softc *, int, int));

hide void
depca_wrcsr(sc, port, val)
	struct lance_softc *sc;
	u_int16_t port, val;
{
	struct depca_softc *lesc = (struct depca_softc *)sc;
	bus_space_tag_t iot = lesc->sc_iot;
	bus_space_handle_t ioh = lesc->sc_ioh;

	bus_space_write_2(iot, ioh, DEPCA_RAP, port);
	bus_space_write_2(iot, ioh, DEPCA_RDP, val);
}

hide u_int16_t
depca_rdcsr(sc, port)
	struct lance_softc *sc;
	u_int16_t port;
{
	struct depca_softc *lesc = (struct depca_softc *)sc;
	bus_space_tag_t iot = lesc->sc_iot;
	bus_space_handle_t ioh = lesc->sc_ioh; 
	u_int16_t val;

	bus_space_write_2(iot, ioh, DEPCA_RAP, port);
	val = bus_space_read_2(iot, ioh, DEPCA_RDP); 
	return (val);
}

int
depca_readprom(iot, ioh, laddr)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	u_int8_t *laddr;
{
	int port, i;

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
	printf("depca: address not found\n");
	return (-1);

found:

	if (laddr) {
		for (i = 0; i < 6; i++)
			laddr[i] = bus_space_read_1(iot, ioh, port);
	}

#if 0
	sum =
	    (laddr[0] <<  2) +
	    (laddr[1] << 10) +
	    (laddr[2] <<  1) +
	    (laddr[3] <<  9) +
	    (laddr[4] <<  0) +
	    (laddr[5] <<  8);
	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);

	rom_sum = bus_space_read_1(iot, ioh, port);
	rom_sum |= bus_space_read_1(iot, ioh, port) << 8;

	if (sum != rom_sum) {
		printf("depca: checksum mismatch; calculated %04x != read %04x",
		       sum, rom_sum);
		return (-1);
	}
#endif

	return (0);
}

int
depca_isa_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	bus_space_handle_t memh;
	int rv = 0;

	/* Disallow impossible i/o address. */
	if (ia->ia_iobase != 0x200 && ia->ia_iobase != 0x300)
		return (0);

	/* Map i/o space. */
	if (bus_space_map(iot, ia->ia_iobase, 16, 0, &ioh))
		return 0;

	if (ia->ia_maddr == MADDRUNK ||
	    (ia->ia_msize != 32*1024 && ia->ia_msize != 64*1024))
		goto bad;

	/* Map card RAM. */
	if (bus_space_map(ia->ia_memt, ia->ia_maddr, ia->ia_msize, 0, &memh))
		goto bad;

	/* Just needed to check mapability; don't need it anymore. */
	bus_space_unmap(ia->ia_memt, memh, ia->ia_msize);

	/* Stop the LANCE chip and put it in a known state. */
	bus_space_write_2(iot, ioh, DEPCA_RAP, LE_CSR0);
	bus_space_write_2(iot, ioh, DEPCA_RDP, LE_C0_STOP);
	delay(100);

	bus_space_write_2(iot, ioh, DEPCA_RAP, LE_CSR0);
	if (bus_space_read_2(iot, ioh, DEPCA_RDP) != LE_C0_STOP)
		goto bad;

	bus_space_write_2(iot, ioh, DEPCA_RAP, LE_CSR3);
	bus_space_write_2(iot, ioh, DEPCA_RDP, LE_C3_ACON);

	bus_space_write_1(iot, ioh, DEPCA_CSR, DEPCA_CSR_DUM);

	if (depca_readprom(iot, ioh, 0))
		goto bad;

	ia->ia_iosize = 16;
	rv = 1;

bad:
	bus_space_unmap(iot, ioh, 16);
	return (rv);
}

void
depca_isa_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	printf("\n");

	config_found(self, aux, 0);
}

int
le_depca_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return (1);
}

void
le_depca_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct depca_softc *lesc = (void *)self;
	struct lance_softc *sc = &lesc->sc_am7990.lsc;
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_tag_t memt = ia->ia_memt;
	bus_space_handle_t ioh, memh;
	u_char val;
	int i;

	printf("\n");

	if (bus_space_map(iot, ia->ia_iobase, 16, 0, &ioh))
		panic("%s: can't map io", self->dv_xname);

	/* Map shared memory. */
	if (bus_space_map(memt, ia->ia_maddr, ia->ia_msize, 0, &memh))
		panic("%s: can't map mem space", self->dv_xname);

	lesc->sc_iot = iot;
	lesc->sc_ioh = ioh;
	lesc->sc_memt = memt;
	lesc->sc_memh = memh;

	if (depca_readprom(iot, ioh, sc->sc_enaddr)) {
		printf("%s: can't read PROM\n", self->dv_xname);
		return;
	}

	bus_space_write_1(iot, ioh, DEPCA_CSR,
			  DEPCA_CSR_DUM | DEPCA_CSR_IEN | DEPCA_CSR_SHE
			  | (ia->ia_msize == 32*1024 ? DEPCA_CSR_LOW32K : 0));

	val = 0xff;
	for (;;) {
		bus_space_set_region_1(memt, memh, 0, val,
				       ia->ia_msize);
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

	sc->sc_copytodesc = depca_copytobuf;
	sc->sc_copyfromdesc = depca_copyfrombuf;
	sc->sc_copytobuf = depca_copytobuf;
	sc->sc_copyfrombuf = depca_copyfrombuf;
	sc->sc_zerobuf = depca_zerobuf;

	sc->sc_rdcsr = depca_rdcsr;
	sc->sc_wrcsr = depca_wrcsr;
	sc->sc_hwinit = NULL;

	printf("%s", sc->sc_dev.dv_xname);
	am7990_config(&lesc->sc_am7990);

	lesc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq, IST_EDGE,
	    IPL_NET, depca_intredge, sc);
}

/*
 * Controller interrupt.
 */
int
depca_intredge(arg)
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
	struct lance_softc *sc;
	void *from;
	int boff, len;
{
	struct depca_softc *lesc = (struct depca_softc *)sc;

	bus_space_write_region_1(lesc->sc_memt, lesc->sc_memh, boff,
	    from, len);
}

void
depca_copyfrombuf(sc, to, boff, len)
	struct lance_softc *sc;  
	void *to;
	int boff, len;
{
	struct depca_softc *lesc = (struct depca_softc *)sc;

	bus_space_read_region_1(lesc->sc_memt, lesc->sc_memh, boff,
	    to, len);
}

void
depca_zerobuf(sc, boff, len)
	struct lance_softc *sc;
	int boff, len;
{
	struct depca_softc *lesc = (struct depca_softc *)sc;

#if 0	/* XXX !! */
	bus_space_set_region_1(lesc->sc_memt, lesc->sc_memh, boff,
	    0x00, len);
#else
	for (; len != 0; boff++, len--)
		bus_space_write_1(lesc->sc_memt, lesc->sc_memh, boff, 0x00);
#endif
}
