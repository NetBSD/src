/*	$NetBSD: if_le_vme.c,v 1.3 1997/03/17 03:17:36 thorpej Exp $	*/

/*-
 * Copyright (c) 1997 Leo Weppelman.  All rights reserved.
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
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_ether.h>
#endif

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/iomap.h>
#include <machine/scu.h>

#include <atari/atari/device.h>
#include <atari/atari/intr.h>

#include <dev/ic/am7990reg.h>
#include <dev/ic/am7990var.h>

#include <atari/vme/vmevar.h>
#include <atari/vme/if_levar.h>

struct le_addresses {
	u_long	reg_addr;
	u_long	mem_addr;
	int	irq;
} lestd[] = {
	{ 0xfe00fff0, 0xfe010000, IRQUNK },	/* Riebl VME	*/
	{ 0xffcffff0, 0xffcf0000,      5 },	/* PAM VME	*/
	{ 0xfecffff0, 0xfecf0000,      5 }	/* Rhotron VME	*/
};

#define	NLESTD	(sizeof(lestd) / sizeof(lestd[0]))

/*
 * All cards have 64KB RAM. However.... On the Riebl cards the area
 * between the offsets 0xee70-0xeec0 is used to store config data.
 */
#define	MEMSIZE	(64*1024)

/*
 * Default mac for RIEBL cards without a (working) battery. The first 4 bytes
 * are the manufacturer id.
 */
static u_char riebl_def_mac[] = {
	0x00, 0x00, 0x36, 0x04, 0x00, 0x00
};

static int le_intr __P((struct le_softc *, int));
static void lepseudointr __P((struct le_softc *, void *));
static int le_vme_match __P((struct device *, struct cfdata *, void *));
static void le_vme_attach __P((struct device *, struct device *, void *));
static int probe_addresses __P((bus_space_tag_t *, bus_space_tag_t *,
				bus_space_handle_t *, bus_space_handle_t *));
static void riebl_skip_reserved_area __P((struct am7990_softc *));

struct cfattach le_vme_ca = {
	sizeof(struct le_softc), le_vme_match, le_vme_attach
};

hide void lewrcsr __P((struct am7990_softc *, u_int16_t, u_int16_t));
hide u_int16_t lerdcsr __P((struct am7990_softc *, u_int16_t));

hide void
lewrcsr(sc, port, val)
	struct am7990_softc	*sc;
	u_int16_t		port, val;
{
	struct le_softc		*lesc = (struct le_softc *)sc;
	int			s;

	s = splhigh();
	bus_space_write_2(lesc->sc_iot, lesc->sc_ioh, LER_RAP, port);
	bus_space_write_2(lesc->sc_iot, lesc->sc_ioh, LER_RDP, val);
	splx(s);
}

hide u_int16_t
lerdcsr(sc, port)
	struct am7990_softc	*sc;
	u_int16_t		port;
{
	struct le_softc		*lesc = (struct le_softc *)sc;
	u_int16_t		val;
	int			s;

	s = splhigh();
	bus_space_write_2(lesc->sc_iot, lesc->sc_ioh, LER_RAP, port);
	val = bus_space_read_2(lesc->sc_iot, lesc->sc_ioh, LER_RDP); 
	splx(s);

	return (val);
}

static int
le_vme_match(parent, cfp, aux)
	struct device	*parent;
	struct cfdata	*cfp;
	void		*aux;
{
	struct vme_attach_args	*va = aux;
	int			i;
	bus_space_tag_t		iot;
	bus_space_tag_t		memt;
	bus_space_handle_t	ioh;
	bus_space_handle_t	memh;

	iot  = va->va_iot;
	memt = va->va_memt;

	for (i = 0; i < NLESTD; i++) {
		struct le_addresses	*le_ap = &lestd[i];
		int			found  = 0;

		if ((va->va_iobase != IOBASEUNK)
		     && (va->va_iobase != le_ap->reg_addr))
			continue;

		if ((va->va_maddr != MADDRUNK)
		     && (va->va_maddr != le_ap->mem_addr))
			continue;

		if ((le_ap->irq != IRQUNK) && (va->va_irq != le_ap->irq))
			continue;

		if (bus_space_map(iot, le_ap->reg_addr, 16, 0, &ioh)) {
			printf("leprobe: cannot map io-area\n");
			return (0);
		}
		if (bus_space_map(memt, le_ap->mem_addr, MEMSIZE, 0, &memh)) {
			bus_space_unmap(iot, (caddr_t)le_ap->reg_addr, 16);
			printf("leprobe: cannot map memory-area\n");
			return (0);
		}
		found = probe_addresses(&iot, &memt, &ioh, &memh);
		bus_space_unmap(iot, (caddr_t)le_ap->reg_addr, 16);
		bus_space_unmap(memt, (caddr_t)le_ap->mem_addr, 8*NBPG);

		if (found) {
			va->va_iobase = le_ap->reg_addr;
			va->va_iosize = 16;
			va->va_maddr  = le_ap->mem_addr;
			va->va_msize  = MEMSIZE;
			if (va->va_irq == IRQUNK)
				va->va_irq = le_ap->irq;
			return 1;
		}
    }
    return (0);
}

static int
probe_addresses(iot, memt, ioh, memh)
bus_space_tag_t		*iot;
bus_space_tag_t		*memt;
bus_space_handle_t	*ioh;
bus_space_handle_t	*memh;
{
	/*
	 * Test accesibility of register and memory area
	 */
	if(!bus_space_peek_2(*iot, *ioh, LER_RDP))
		return 0;
	if(!bus_space_peek_1(*memt, *memh, 0))
		return 0;

	/*
	 * Test for writable memory
	 */
	bus_space_write_2(*memt, *memh, 0, 0xa5a5);
	if (bus_space_read_2(*memt, *memh, 0) != 0xa5a5)
		return 0;

	/*
	 * Test writability of selector port.
	 */
	bus_space_write_2(*iot, *ioh, LER_RAP, LE_CSR1);
	if (bus_space_read_2(*iot, *ioh, LER_RAP) != LE_CSR1)
		return 0;

	/*
	 * Do a small register test
	 */
	bus_space_write_2(*iot, *ioh, LER_RAP, LE_CSR0);
	bus_space_write_2(*iot, *ioh, LER_RDP, LE_C0_INIT | LE_C0_STOP);
	if (bus_space_read_2(*iot, *ioh, LER_RDP) != LE_C0_STOP)
		return 0;

	bus_space_write_2(*iot, *ioh, LER_RDP, LE_C0_STOP);
	if (bus_space_read_2(*iot, *ioh, LER_RDP) != LE_C0_STOP)
		return 0;

	return 1;
}

/*
 * Interrupt mess. Because the card's interrupt is hardwired to either
 * ipl5 or ipl3 (mostly on ipl5) and raising splnet to spl5() just won't do
 * (it kills the serial at the least), we use a 2-level interrupt sceme. The
 * card interrupt is routed to 'le_intr'. If the previous ipl was below
 * splnet, just call the mi-function. If not, save the interrupt status,
 * turn off card interrupts (the card is *very* persistent) and arrange
 * for a softint 'callback' through 'lepseudointr'.
 */
static int
le_intr(lesc, sr)
	struct le_softc	*lesc;
	int		 sr;
{
	struct am7990_softc	*sc = &lesc->sc_am7990;
	u_int16_t		csr0;

	if ((sr & PSL_IPL) < IPL_NET)
		am7990_intr(sc);
	else {
		sc->sc_saved_csr0 = csr0 = lerdcsr(sc, LE_CSR0);
		lewrcsr(sc, LE_CSR0, csr0 & ~LE_C0_INEA);
		add_sicallback((si_farg)lepseudointr, lesc, sc);
	}
	return 1;
}


static void
lepseudointr(lesc, sc)
struct le_softc	*lesc;
void		*sc;
{
	int	s;

	s = splx(lesc->sc_splval);
	am7990_intr(sc);
	splx(s);
}

static void
le_vme_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct le_softc		*lesc = (struct le_softc *)self;
	struct am7990_softc	*sc = &lesc->sc_am7990;
	struct vme_attach_args	*va = aux;
	bus_space_handle_t	ioh;
	bus_space_handle_t	memh;
	int			i;

	printf("\n%s: ", sc->sc_dev.dv_xname);

	if (bus_space_map(va->va_iot, va->va_iobase, va->va_iosize, 0, &ioh))
		panic("leattach: cannot map io-area\n");
	if (bus_space_map(va->va_memt, va->va_maddr, va->va_msize, 0, &memh))
		panic("leattach: cannot map mem-area\n");

	lesc->sc_iot    = va->va_iot;
	lesc->sc_ioh    = ioh;
	lesc->sc_memt   = va->va_memt;
	lesc->sc_memh   = memh;
	lesc->sc_splval = (va->va_irq << 8) | PSL_S; /* XXX */

	/*
	 * Go on to find board type
	 */
	if (bus_space_peek_1(va->va_iot, ioh, LER_EEPROM)) {
		printf("PAM card");
		lesc->sc_type = LE_PAM;
		bus_space_read_1(va->va_iot, ioh, LER_MEME);
	}
	else {
		printf("Riebl card");
		if(bus_space_read_4(va->va_memt, memh, RIEBL_MAGIC_ADDR)
								== RIEBL_MAGIC)
			lesc->sc_type = LE_NEW_RIEBL;
		else {
			printf("(without battery) ");
			lesc->sc_type = LE_OLD_RIEBL;
		}
	}

	sc->sc_copytodesc   = am7990_copytobuf_contig;
	sc->sc_copyfromdesc = am7990_copyfrombuf_contig;
	sc->sc_copytobuf    = am7990_copytobuf_contig;
	sc->sc_copyfrombuf  = am7990_copyfrombuf_contig;
	sc->sc_zerobuf      = am7990_zerobuf_contig;

	sc->sc_rdcsr   = lerdcsr;
	sc->sc_wrcsr   = lewrcsr;
	sc->sc_hwinit  = NULL;
	sc->sc_conf3   = LE_C3_BSWP;
	sc->sc_addr    = 0;
	sc->sc_memsize = va->va_msize;
	sc->sc_mem     = (void *)memh; /* XXX */

	/*
	 * Get MAC address
	 */
	switch (lesc->sc_type) {
	    case LE_OLD_RIEBL:
		bcopy(riebl_def_mac, sc->sc_arpcom.ac_enaddr,
					sizeof(sc->sc_arpcom.ac_enaddr));
		break;
	    case LE_NEW_RIEBL:
		for (i = 0; i < sizeof(sc->sc_arpcom.ac_enaddr); i++)
		    sc->sc_arpcom.ac_enaddr[i] =
			bus_space_read_1(va->va_memt, memh, i + RIEBL_MAC_ADDR);
			break;
	    case LE_PAM:
		i = bus_space_read_1(va->va_iot, ioh, LER_EEPROM);
		for (i = 0; i < sizeof(sc->sc_arpcom.ac_enaddr); i++) {
		    sc->sc_arpcom.ac_enaddr[i] =
			(bus_space_read_2(va->va_memt, memh, 2 * i) << 4) |
			(bus_space_read_2(va->va_memt, memh, 2 * i + 1) & 0xf);
		}
		i = bus_space_read_1(va->va_iot, ioh, LER_MEME);
		break;
	}

	am7990_config(sc);

	if ((lesc->sc_type == LE_OLD_RIEBL) || (lesc->sc_type == LE_NEW_RIEBL))
		riebl_skip_reserved_area(sc);

	/*
	 * XXX: We always use uservector 64....
	 */
	if ((lesc->sc_intr = intr_establish(64, USER_VEC, 0, 
				(hw_ifun_t)le_intr, lesc)) == NULL) {
		printf("le_vme_attach: Can't establish interrupt\n");
		return;
	}

	/*
	 * Notify the card of the vector
	 */
	switch (lesc->sc_type) {
		case LE_OLD_RIEBL:
		case LE_NEW_RIEBL:
			bus_space_write_2(va->va_memt, memh, RIEBL_IVEC_ADDR,
								64 + 64);
			break;
		case LE_PAM:
			bus_space_write_1(va->va_iot, ioh, LER_IVEC, 64 + 64);
			break;
	}

	/*
	 * Unmask the VME-interrupt we're on
	 */
	if (machineid & ATARI_TT)
		SCU->vme_mask |= 1 << va->va_irq;
}

/*
 * True if 'addr' containe within [start,len]
 */
#define WITHIN(start, len, addr)	\
		((addr >= start) && ((addr) <= ((start) + (len))))
static void
riebl_skip_reserved_area(sc)
	struct am7990_softc	*sc;
{
	int	offset = 0;
	int	i;

	for(i = 0; i < sc->sc_nrbuf; i++) {
		if (WITHIN(sc->sc_rbufaddr[i], LEBLEN, RIEBL_RES_START)
		    || WITHIN(sc->sc_rbufaddr[i], LEBLEN, RIEBL_RES_END)) {
			offset = RIEBL_RES_END - sc->sc_rbufaddr[i];
		}
		sc->sc_rbufaddr[i] += offset;
	}

	for(i = 0; i < sc->sc_ntbuf; i++) {
		if (WITHIN(sc->sc_tbufaddr[i], LEBLEN, RIEBL_RES_START)
		    || WITHIN(sc->sc_tbufaddr[i], LEBLEN, RIEBL_RES_END)) {
			offset = RIEBL_RES_END - sc->sc_tbufaddr[i];
		}
		sc->sc_tbufaddr[i] += offset;
	}
}
