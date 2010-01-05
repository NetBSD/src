/*	$NetBSD: gemini_obio.c,v 1.8 2010/01/05 13:14:56 mbalmer Exp $	*/

/* adapted from:
 *      NetBSD: omap2_obio.c,v 1.5 2008/10/21 18:50:25 matt Exp
 */

/*
 * Autoconfiguration support for the Gemini "On Board" I/O.
 *
 * Based on arm/omap/omap2_obio.c which in turn was derived
 * Based on arm/omap/omap_emifs.c which in turn was derived
 * Based on arm/xscale/pxa2x0.c which in turn was derived
 * from arm/sa11x0/sa11x0.c
 *
 * Copyright (c) 2002, 2005  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
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
 *	This product includes software developed for the NetBSD Project by
 *	Genetec Corporation.
 * 4. The name of Genetec Corporation may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (c) 1997,1998, 2001, The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro, Ichiro FUKUHARA and Paul Kranenburg.
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
 *
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
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
 */

#include "opt_gemini.h"
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gemini_obio.c,v 1.8 2010/01/05 13:14:56 mbalmer Exp $");

#include "locators.h"
#include "obio.h"
#include "geminiicu.h"
#include "pci.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/reboot.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <arm/cpufunc.h>
#include <arm/mainbus/mainbus.h>
#include <arm/gemini/gemini_var.h>
#include <arm/gemini/gemini_reg.h>

#include <arm/gemini/gemini_obiovar.h>

typedef struct {
	boolean_t	cs_valid;
	ulong		cs_addr;
	ulong		cs_size;
} obio_csconfig_t;

/* prototypes */
static int	obio_match(device_t, cfdata_t, void *);
static void	obio_attach(device_t, device_t, void *);
static int 	obio_search(device_t, cfdata_t, const int *, void *);
static int	obio_find(device_t, cfdata_t, const int *, void *);
static int	obio_print(void *, const char *);
static void	obio_attach_critical(struct obio_softc *);

/* attach structures */
CFATTACH_DECL_NEW(obio, sizeof(struct obio_softc),
	obio_match, obio_attach, NULL, NULL);


static int
obio_match(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

static void
obio_attach(device_t parent, device_t self, void *aux)
{
	struct obio_softc *sc = device_private(self);
	struct mainbus_attach_args *mb = (struct mainbus_attach_args *)aux;
#if NPCI > 0
	struct pcibus_attach_args pba;
#endif

	sc->sc_dev = self;

	sc->sc_iot = &gemini_bs_tag;

	aprint_normal(": On-Board IO\n");

#if (GEMINI_BUSBASE != 0)
	sc->sc_dmarange.dr_sysbase = 0;
	sc->sc_dmarange.dr_busbase = (GEMINI_BUSBASE * 1024 * 1024);
	sc->sc_dmarange.dr_len = MEMSIZE * 1024 * 1024;
	gemini_bus_dma_tag._ranges = &sc->sc_dmarange;
	gemini_bus_dma_tag._nranges = 1;
#endif

	sc->sc_ioh = 0;
	sc->sc_dmat = &gemini_bus_dma_tag;
	sc->sc_base = mb->mb_iobase;
	sc->sc_size = mb->mb_iosize;

	/*
	 * Attach critical devices first.
	 */
	obio_attach_critical(sc);

#if NPCI > 0
	/*
	 * map PCI controller registers
	 */
	if (bus_space_map(sc->sc_iot, GEMINI_PCICFG_BASE,
		GEMINI_PCI_CFG_DATA+4, 0, &sc->sc_pcicfg_ioh)) {
			aprint_error("cannot map PCI controller at %#x\n",
				GEMINI_PCICFG_BASE);
			return;
	}
	/*
	 * initialize the PCI chipset tag
	 */
	gemini_pci_init(&sc->sc_pci_chipset, sc);
#endif	/* NPCI */

	/*
	 * attach the rest of our devices
	 */
	config_search_ia(obio_search, self, "obio", NULL);

#if NPCI > 0
	/*
	 * attach the PCI bus
	 */
	pba.pba_memt = sc->sc_iot;
	pba.pba_iot =  sc->sc_iot;
	pba.pba_dmat = sc->sc_dmat;
	pba.pba_dmat64 = NULL;
	pba.pba_pc = &sc->sc_pci_chipset;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_intrswiz = 0;
	pba.pba_intrtag = 0;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY | PCI_FLAGS_MWI_OKAY;

	(void) config_found_ia(sc->sc_dev, "pcibus", &pba, pcibusprint);
#endif	/* NPCI */
	
}

static int
obio_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct obio_softc * const sc = device_private(parent);
	struct obio_attach_args oa;

	/* Set up the attach args. */
	if (cf->cf_loc[OBIOCF_NOBYTEACC] == 1) {
#if 0
		if (cf->cf_loc[OBIOCF_MULT] == 1)
			oa.obio_iot = &nobyteacc_bs_tag;
		else
#endif
			panic("nobyteacc specified for device with "
				"non-byte multiplier\n");
	} else {
		switch (cf->cf_loc[OBIOCF_MULT]) {
		case 1:
			oa.obio_iot = &gemini_bs_tag;
			break;
#if 0
		case 2:
			oa.obio_iot = &gemini_a2x_bs_tag;
			break;
#endif
		case 4:
			oa.obio_iot = &gemini_a4x_bs_tag;
			break;
		default:
			panic("Unsupported obio bus multiplier.");
			break;
		}
	}

	oa.obio_dmat = sc->sc_dmat;
	oa.obio_addr = cf->cf_loc[OBIOCF_ADDR];
	oa.obio_size = cf->cf_loc[OBIOCF_SIZE];
	oa.obio_intr = cf->cf_loc[OBIOCF_INTR];
	oa.obio_intrbase = cf->cf_loc[OBIOCF_INTRBASE];

	if (config_match(parent, cf, &oa)) {
		config_attach(parent, cf, &oa, obio_print);
		return 0;			/* love it */
	}

	return UNCONF;	/* NG */
}

static int
obio_find(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct obio_attach_args * const oa = aux;

	if (oa->obio_addr != OBIOCF_ADDR_DEFAULT
	    && oa->obio_addr != cf->cf_loc[OBIOCF_ADDR])
		return 0;
	if (oa->obio_size != OBIOCF_SIZE_DEFAULT
	    && oa->obio_size != cf->cf_loc[OBIOCF_SIZE])
		return 0;
	if (oa->obio_intr != OBIOCF_INTR_DEFAULT
	    && oa->obio_intr != cf->cf_loc[OBIOCF_INTR])
		return 0;
	if (oa->obio_intrbase != OBIOCF_INTRBASE_DEFAULT
	    && oa->obio_intrbase != cf->cf_loc[OBIOCF_INTRBASE])
		return 0;

	/* Set up the attach args. */
	if (cf->cf_loc[OBIOCF_NOBYTEACC] == 1) {
#if 0
		if (cf->cf_loc[OBIOCF_MULT] == 1)
			oa->obio_iot = &nobyteacc_bs_tag;
		else
#endif
			panic("nobyteacc specified for device with "
				"non-byte multiplier\n");
	} else {
		switch (cf->cf_loc[OBIOCF_MULT]) {
		case 1:
			oa->obio_iot = &gemini_bs_tag;
			break;
#if 0
		case 2:
			oa->obio_iot = &gemini_a2x_bs_tag;
			break;
#endif
		case 4:
			oa->obio_iot = &gemini_a4x_bs_tag;
			break;
		default:
			panic("Unsupported obio bus multiplier.");
			break;
		}
	}
	oa->obio_addr = cf->cf_loc[OBIOCF_ADDR];
	oa->obio_size = cf->cf_loc[OBIOCF_SIZE];
	oa->obio_intr = cf->cf_loc[OBIOCF_INTR];
	oa->obio_intrbase = cf->cf_loc[OBIOCF_INTRBASE];

	return config_match(parent, cf, oa);
}

#if NGEMINIICU == 0
#error no geminiicu present in config file
#endif

static const struct {
	const char *name;
	bus_addr_t addr;
	bool required;
} critical_devs[] = {
#if defined(GEMINI_MASTER) || defined(GEMINI_SINGLE)
	{ .name = "geminiicu0",   .addr = 0x48000000, .required = true },
	{ .name = "geminiwdt0",   .addr = 0x41000000, .required = true },
	{ .name = "geminitmr0",   .addr = 0x43000000, .required = true },
	{ .name = "geminitmr2",   .addr = 0x43000000, .required = true },
	{ .name = "com0",         .addr = 0x42000000, .required = true },
#elif defined(GEMINI_SLAVE)
	{ .name = "geminiicu1",   .addr = 0x49000000, .required = true },
	{ .name = "geminitmr1",   .addr = 0x43000000, .required = true },
	{ .name = "geminitmr2",   .addr = 0x43000000, .required = true },
	{ .name = "geminilpchc0", .addr = 0x47000000, .required = true },
#endif
};

static void
obio_attach_critical(struct obio_softc *sc)
{
	struct obio_attach_args oa;
	cfdata_t cf;
	size_t i;

	for (i = 0; i < __arraycount(critical_devs); i++) {
		oa.obio_iot = sc->sc_iot;
		oa.obio_dmat = sc->sc_dmat;

		oa.obio_addr = critical_devs[i].addr;
		oa.obio_size = OBIOCF_SIZE_DEFAULT;
		oa.obio_intr = OBIOCF_INTR_DEFAULT;
		oa.obio_intrbase = OBIOCF_INTRBASE_DEFAULT;

#if 0
		if (oa.obio_addr != OBIOCF_ADDR_DEFAULT
		    && (oa.obio_addr < sc->sc_base 
		        || oa.obio_addr >= sc->sc_base + sc->sc_size))
			continue;
#endif

		cf = config_search_ia(obio_find, sc->sc_dev, "obio", &oa);
		if (cf == NULL && critical_devs[i].required)
			panic("obio_attach_critical: failed to find %s!",
			    critical_devs[i].name);

		oa.obio_addr = cf->cf_loc[OBIOCF_ADDR];
		oa.obio_size = cf->cf_loc[OBIOCF_SIZE];
		oa.obio_intr = cf->cf_loc[OBIOCF_INTR];
		oa.obio_intrbase = cf->cf_loc[OBIOCF_INTRBASE];
		config_attach(sc->sc_dev, cf, &oa, obio_print);
	}
}

static int
obio_print(void *aux, const char *name)
{
	struct obio_attach_args *oa = (struct obio_attach_args*)aux;

	if (oa->obio_addr != OBIOCF_ADDR_DEFAULT) {
		aprint_normal(" addr 0x%08lx", oa->obio_addr);
		if (oa->obio_size != OBIOCF_SIZE_DEFAULT)
			aprint_normal("-0x%08lx",
				oa->obio_addr + oa->obio_size-1);
	}
	if (oa->obio_intr != OBIOCF_INTR_DEFAULT)
		aprint_normal(" intr %d", oa->obio_intr);
	if (oa->obio_intrbase != OBIOCF_INTRBASE_DEFAULT)
		aprint_normal(" intrbase %d", oa->obio_intrbase);

	return UNCONF;
}
