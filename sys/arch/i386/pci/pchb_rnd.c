/*	$NetBSD: pchb_rnd.c,v 1.6.4.1 2001/04/09 01:53:37 nathanw Exp $	*/

/*
 * Copyright (c) 2000 Michael Shalayeff
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
 *      This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	from OpenBSD: pchb.c,v 1.23 2000/10/23 20:07:30 deraadt Exp
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/time.h>

#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <arch/i386/pci/i82802reg.h>
#include <arch/i386/pci/pchbvar.h>

void pchb_rnd_callout(void *v);

void
pchb_attach_rnd(struct pchb_softc *sc, struct pci_attach_args *pa)
{
	int i;
	u_int8_t reg8;

	switch (PCI_VENDOR(pa->pa_id)) {
	case PCI_VENDOR_INTEL:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_INTEL_82810E_MCH:
		case PCI_PRODUCT_INTEL_82810_DC100_MCH:
		case PCI_PRODUCT_INTEL_82810_MCH:
		case PCI_PRODUCT_INTEL_82815_DC100_HUB:
		case PCI_PRODUCT_INTEL_82815_NOAGP_HUB:
		case PCI_PRODUCT_INTEL_82815_NOGRAPH_HUB:
		case PCI_PRODUCT_INTEL_82815_FULL_HUB:
		case PCI_PRODUCT_INTEL_82820_MCH:
		case PCI_PRODUCT_INTEL_82840_HB:
			sc->sc_st = pa->pa_memt;
			if (bus_space_map(sc->sc_st, I82802_IOBASE,
			    I82802_IOSIZE, 0, &sc->sc_sh) != 0) {
				/*
				 * Unable to map control registers, punt,
				 * but don't bother issuing a warning.
				 */
				return;
			}

			if ((bus_space_read_1(sc->sc_st, sc->sc_sh,
			    I82802_RNG_HWST) & I82802_RNG_HWST_PRESENT) == 0) {
				/*
				 * Random number generator is not present.
				 */
				return;
			}

			/* Enable the RNG. */
			reg8 = bus_space_read_1(sc->sc_st, sc->sc_sh,
			    I82802_RNG_HWST);
			bus_space_write_1(sc->sc_st, sc->sc_sh,
			    I82802_RNG_HWST, reg8 | I82802_RNG_HWST_ENABLE);

			/* Check to see if we can read data from the RNG. */
			for (i = 0; i < 1000; i++) {
				reg8 = bus_space_read_1(sc->sc_st, sc->sc_sh,
				    I82802_RNG_RNGST);
				if (reg8 & I82802_RNG_RNGST_DATAV)
					break;
				delay(10);
			}

			if ((reg8 & I82802_RNG_RNGST_DATAV) == 0) {
				printf("%s: unable to read from random "
				    "number generator.\n",
				    sc->sc_dev.dv_xname);
				return;
			}

			/*
			 * Should test entropy source to ensure
			 * that it passes the Statistical Random
			 * Number Generator Tests in section 4.11.1,
			 * FIPS PUB 140-1.
			 *
			 *	http://csrc.nist.gov/fips/fips1401.htm
			 */

			/*
			 * See how quickly the RNG provides entropy.
			 */
			for (i = 0; i < 8192; i++) {
				while ((bus_space_read_1(sc->sc_st, sc->sc_sh,
				    I82802_RNG_RNGST) &
				    I82802_RNG_RNGST_DATAV) == 0)
					/* spin */;
				(void) bus_space_read_1(sc->sc_st, sc->sc_sh,
				    I82802_RNG_DATA);
			}

			printf("%s: random number generator enabled\n",
			    sc->sc_dev.dv_xname);

			callout_init(&sc->sc_rnd_ch);
			rnd_attach_source(&sc->sc_rnd_source,
			    sc->sc_dev.dv_xname, RND_TYPE_UNKNOWN, 0); /* XXX */
			sc->sc_rnd_i = 4;
			pchb_rnd_callout(sc);
			break;
		default:
			break;
		}
	}
}

void
pchb_rnd_callout(void *v)
{
	struct pchb_softc *sc = v;

	if ((bus_space_read_1(sc->sc_st, sc->sc_sh, I82802_RNG_RNGST) &
	     I82802_RNG_RNGST_DATAV) != 0) {
		sc->sc_rnd_ax = (sc->sc_rnd_ax << 8) |
		    bus_space_read_1(sc->sc_st, sc->sc_sh, I82802_RNG_DATA);
		if (--sc->sc_rnd_i == 0) {
			sc->sc_rnd_i = 4;
			rnd_add_uint32(&sc->sc_rnd_source, sc->sc_rnd_ax);
		}
	}
	callout_reset(&sc->sc_rnd_ch, 1, pchb_rnd_callout, sc);
}
