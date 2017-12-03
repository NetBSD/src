/*	$NetBSD: mkclock_isa.c,v 1.14.12.1 2017/12/03 11:36:31 jdolecek Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus J. Klein.
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
 * Mostek MK48T18 time-of-day chip attachment to ISA bus, using two
 * 8-bit ports for address selection and one 8-bit port for data.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: mkclock_isa.c,v 1.14.12.1 2017/12/03 11:36:31 jdolecek Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <dev/clock_subr.h>
#include <dev/ic/mk48txxreg.h>
#include <dev/ic/mk48txxvar.h>

#include <dev/isa/isavar.h>


/* Offsets of registers into ISA I/O space */
#define	MKCLOCK_STB0	0		/* Address low		*/
#define	MKCLOCK_STB1	1		/* Address high		*/
#define	MKCLOCK_DATA	3		/* Data port		*/

#define	MKCLOCK_NPORTS	(MKCLOCK_DATA - MKCLOCK_STB0 + 1)


/* Autoconfiguration interface */
int	mkclock_isa_match(device_t, cfdata_t, void *);
void	mkclock_isa_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mkclock_isa, sizeof(struct mk48txx_softc),
    mkclock_isa_match, mkclock_isa_attach, NULL, NULL);


/* mk48txx interface */
uint8_t	mkclock_isa_nvrd(struct mk48txx_softc *, int);
void	mkclock_isa_nvwr(struct mk48txx_softc *, int, uint8_t);


int
mkclock_isa_match(device_t parent, cfdata_t cf, void *aux)
{
	struct isa_attach_args *ia = aux;
	struct mk48txx_softc mk48txx, *sc;
	uint8_t csr, ocsr;
	unsigned int t1, t2;
	int found;

	found = 0;

	if (ia->ia_nio < 1 ||
	    (ia->ia_io[0].ir_addr != ISA_UNKNOWN_PORT &&
	     ia->ia_io[0].ir_addr != 0x74))
		return (0);

        if (ia->ia_niomem > 0 &&
	    (ia->ia_iomem[0].ir_addr != ISA_UNKNOWN_IOMEM)) 
		return (0);

	if (ia->ia_nirq > 0 &&
	    (ia->ia_irq[0].ir_irq != ISA_UNKNOWN_IRQ))
		return (0);
										
	if (ia->ia_ndrq > 0 &&
	    (ia->ia_drq[0].ir_drq != ISA_UNKNOWN_DRQ))
		return (0); 

	/*
	 * Map I/O space, then try to determine if it's really there.
	 */
	sc = &mk48txx;
	sc->sc_bst = ia->ia_iot;
	if (bus_space_map(sc->sc_bst, 0x74, MKCLOCK_NPORTS, 0, &sc->sc_bsh))
		return (0);

	/* Supposedly no control bits are set after POST; check for this. */
	ocsr = mkclock_isa_nvrd(sc, MK48T18_CLKOFF + MK48TXX_ICSR);
	if (ocsr != 0)
		goto unmap;

	/* Set clock data to read mode, prohibiting updates from clock. */
	csr = ocsr | MK48TXX_CSR_READ;
	mkclock_isa_nvwr(sc, MK48T18_CLKOFF + MK48TXX_ICSR, csr);
	/* Compare. */
	if (mkclock_isa_nvrd(sc, MK48T18_CLKOFF + MK48TXX_ICSR) != csr)
		goto restore;

	/* Read from the seconds counter. */
	t1 = bcdtobin(mkclock_isa_nvrd(sc, MK48T18_CLKOFF + MK48TXX_ISEC));
	if (t1 > 59)
		goto restore;

	/* Make it tick again, wait, then look again. */
	mkclock_isa_nvwr(sc, MK48T18_CLKOFF + MK48TXX_ICSR, ocsr);
	DELAY(1100000);
	mkclock_isa_nvwr(sc, MK48T18_CLKOFF + MK48TXX_ICSR, csr);
	t2 = bcdtobin(mkclock_isa_nvrd(sc, MK48T18_CLKOFF + MK48TXX_ISEC));
	if (t2 > 59)
		goto restore;

	/* If [1,2) seconds have passed since, call it a clock. */
	if ((t1 + 1) % 60 == t2 || (t1 + 2) % 60 == t2)
		found = 1;

 restore:
	mkclock_isa_nvwr(sc, MK48T18_CLKOFF + MK48TXX_ICSR, ocsr);
 unmap:
	bus_space_unmap(sc->sc_bst, sc->sc_bsh, MKCLOCK_NPORTS);

	if (found) {
		ia->ia_nio = 1;
		ia->ia_io[0].ir_addr = 0x74;
		ia->ia_io[0].ir_size = MKCLOCK_NPORTS;

		ia->ia_niomem = 0;
		ia->ia_nirq = 0;
		ia->ia_ndrq = 0;
	}

	return (found);
}

void
mkclock_isa_attach(device_t parent, device_t self, void *aux)
{
	struct mk48txx_softc *sc = device_private(self);
	struct isa_attach_args *ia = aux;

	sc->sc_dev = self;

	/* Map I/O space. */
	sc->sc_bst = ia->ia_iot;
	if (bus_space_map(sc->sc_bst, ia->ia_io[0].ir_addr,
	    ia->ia_io[0].ir_size, 0, &sc->sc_bsh))
		panic("mkclock_isa_attach: couldn't map clock I/O space");

	/* Attach to MI mk48txx driver. */
	sc->sc_model = "mk48t18";
	sc->sc_year0 = 1968;
	sc->sc_nvrd = mkclock_isa_nvrd;
	sc->sc_nvwr = mkclock_isa_nvwr;

	mk48txx_attach(sc);

	aprint_normal(" Timekeeper NVRAM/RTC\n");
}

/*
 * Bus access methods for MI mk48txx driver.
 */
uint8_t
mkclock_isa_nvrd(struct mk48txx_softc *sc, int off)
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	uint8_t datum;
	int s;

	iot = sc->sc_bst;
	ioh = sc->sc_bsh;

	s = splclock();
	bus_space_write_1(iot, ioh, MKCLOCK_STB0, off & 0xff);
	bus_space_write_1(iot, ioh, MKCLOCK_STB1, off >> 8);
	datum = bus_space_read_1(iot, ioh, MKCLOCK_DATA);
	splx(s);

	return (datum);
}

void
mkclock_isa_nvwr(struct mk48txx_softc *sc, int off, uint8_t datum)
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int s;

	iot = sc->sc_bst;
	ioh = sc->sc_bsh;

	s = splclock();
	bus_space_write_1(iot, ioh, MKCLOCK_STB0, off & 0xff);
	bus_space_write_1(iot, ioh, MKCLOCK_STB1, off >> 8);
	bus_space_write_1(iot, ioh, MKCLOCK_DATA, datum);
	splx(s);
}
