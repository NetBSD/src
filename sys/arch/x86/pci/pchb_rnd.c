/*	$NetBSD: pchb_rnd.c,v 1.7 2008/01/03 04:50:19 dyoung Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pchb_rnd.c,v 1.7 2008/01/03 04:50:19 dyoung Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/time.h>

#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <arch/x86/pci/i82802reg.h>
#include <arch/x86/pci/pchbvar.h>

static void pchb_rnd_callout(void *v);

#define	PCHB_RNG_RETRIES	1000
#define	PCHB_RNG_MIN_SAMPLES	10

void
pchb_detach_rnd(struct pchb_softc *sc)
{
	uint8_t reg8;

	if (!sc->sc_rnd_attached)
		return;

	/* pch is polled for entropy, so no estimate is available. */
	rnd_detach_source(&sc->sc_rnd_source);

	callout_stop(&sc->sc_rnd_ch);
	callout_destroy(&sc->sc_rnd_ch);

	/* Disable the RNG. */
	reg8 = bus_space_read_1(sc->sc_st, sc->sc_sh, I82802_RNG_HWST);
	bus_space_write_1(sc->sc_st, sc->sc_sh, I82802_RNG_HWST,
	    reg8 & ~I82802_RNG_HWST_ENABLE);

	bus_space_unmap(sc->sc_st, sc->sc_sh, I82802_IOSIZE);

	sc->sc_rnd_attached = false;
}

void
pchb_attach_rnd(struct pchb_softc *sc, struct pci_attach_args *pa)
{
	int i, j, count_ff;
	uint8_t reg8;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_INTEL)
		return;

	switch (PCI_PRODUCT(pa->pa_id)) {
#if defined(__i386__)
	/* Old chipsets which only support IA32 CPUs. */
	case PCI_PRODUCT_INTEL_82810E_MCH:
	case PCI_PRODUCT_INTEL_82810_DC100_MCH:
	case PCI_PRODUCT_INTEL_82810_MCH:
	case PCI_PRODUCT_INTEL_82815_DC100_HUB:
	case PCI_PRODUCT_INTEL_82815_NOAGP_HUB:
	case PCI_PRODUCT_INTEL_82815_NOGRAPH_HUB:
	case PCI_PRODUCT_INTEL_82815_FULL_HUB:
	case PCI_PRODUCT_INTEL_82820_MCH:
	case PCI_PRODUCT_INTEL_82840_HB:
	case PCI_PRODUCT_INTEL_82845_HB:
	case PCI_PRODUCT_INTEL_82850_HB:
	case PCI_PRODUCT_INTEL_82860_HB:
	case PCI_PRODUCT_INTEL_82865_HB:
	case PCI_PRODUCT_INTEL_82875P_HB:
#endif	/* defined((__i386__) */
	/* New chipsets which support EM64T CPUs. */
	case PCI_PRODUCT_INTEL_82915G_HB:
	case PCI_PRODUCT_INTEL_82915GM_HB:
	case PCI_PRODUCT_INTEL_82925X_HB:
	case PCI_PRODUCT_INTEL_82945P_MCH:
	case PCI_PRODUCT_INTEL_82955X_HB:
		break;
	default:
		return;
	}

	sc->sc_st = pa->pa_memt;
	if (bus_space_map(sc->sc_st, I82802_IOBASE, I82802_IOSIZE, 0,
	    &sc->sc_sh) != 0) {
		/*
		 * Unable to map control registers, punt,
		 * but don't bother issuing a warning.
		 */
		return;
	}

	reg8 = bus_space_read_1(sc->sc_st, sc->sc_sh,
	    I82802_RNG_HWST);
	if ((reg8 & I82802_RNG_HWST_PRESENT) == 0) {
		/*
		 * Random number generator is not present.
		 */
		bus_space_unmap(sc->sc_st, sc->sc_sh, I82802_IOSIZE);
		return;
	}

	/* Enable the RNG. */
	bus_space_write_1(sc->sc_st, sc->sc_sh,
	    I82802_RNG_HWST, reg8 | I82802_RNG_HWST_ENABLE);
	reg8 = bus_space_read_1(sc->sc_st, sc->sc_sh, I82802_RNG_HWST);
	if ((reg8 & I82802_RNG_HWST_ENABLE) == 0) {
		/*
		 * Couldn't enable the RNG.
		 */
		bus_space_unmap(sc->sc_st, sc->sc_sh, I82802_IOSIZE);
		return;
	}

	/* Check to see if we can read data from the RNG. */
	count_ff = 0;
	for (j = 0; j < PCHB_RNG_MIN_SAMPLES; ++j) {
		for (i = 0; i < PCHB_RNG_RETRIES; i++) {
			reg8 = bus_space_read_1(sc->sc_st, sc->sc_sh,
			    I82802_RNG_RNGST);
			if (!(reg8 & I82802_RNG_RNGST_DATAV)) {
				delay(10);
				continue;
			}
			reg8 = bus_space_read_1(sc->sc_st, sc->sc_sh,
			    I82802_RNG_DATA);
			break;
		}
		if (i == PCHB_RNG_RETRIES) {
			bus_space_unmap(sc->sc_st, sc->sc_sh, I82802_IOSIZE);
			aprint_verbose_dev(&sc->sc_dev,
			    "timeout reading test samples, RNG disabled.\n");
			return;
		}
		if (reg8 == 0xff)
			++count_ff;
	}

	if (count_ff == PCHB_RNG_MIN_SAMPLES) {
		bus_space_unmap(sc->sc_st, sc->sc_sh, I82802_IOSIZE);
		aprint_verbose_dev(&sc->sc_dev,
		    "returns constant 0xff stream, RNG disabled.\n");
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

	aprint_normal_dev(&sc->sc_dev, "random number generator enabled\n");

	callout_init(&sc->sc_rnd_ch, 0);
	/* pch is polled for entropy, so no estimate is available. */
	rnd_attach_source(&sc->sc_rnd_source, sc->sc_dev.dv_xname,
	    RND_TYPE_RNG, RND_FLAG_NO_ESTIMATE);
	sc->sc_rnd_i = sizeof(sc->sc_rnd_ax);
	pchb_rnd_callout(sc);
	sc->sc_rnd_attached = true;
}

static void
pchb_rnd_callout(void *v)
{
	struct pchb_softc *sc = v;

	if ((bus_space_read_1(sc->sc_st, sc->sc_sh, I82802_RNG_RNGST) &
	     I82802_RNG_RNGST_DATAV) != 0) {
		sc->sc_rnd_ax = (sc->sc_rnd_ax << NBBY) |
		    bus_space_read_1(sc->sc_st, sc->sc_sh, I82802_RNG_DATA);
		if (--sc->sc_rnd_i == 0) {
			sc->sc_rnd_i = sizeof(sc->sc_rnd_ax);
			rnd_add_data(&sc->sc_rnd_source, &sc->sc_rnd_ax,
			    sizeof(sc->sc_rnd_ax),
			    sizeof(sc->sc_rnd_ax) * NBBY);
		}
	}
	callout_reset(&sc->sc_rnd_ch, 1, pchb_rnd_callout, sc);
}
