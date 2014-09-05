/*	$NetBSD: exynos_io.c,v 1.8 2014/09/05 08:01:05 skrll Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
 * 
 * This code is derived from software contributed to The NetBSD Foundation
 * by Reinoud Zandijk
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

#include "locators.h"
#include "opt_exynos.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: exynos_io.c,v 1.8 2014/09/05 08:01:05 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <arm/locore.h>
#include <arm/mainbus/mainbus.h>

#include <arm/samsung/exynos_io.h>

#if 0
#include <arm/samsung/exynos_reg.h>
#endif
#include <arm/samsung/exynos_var.h>


static int  exyo_match(device_t, cfdata_t, void *);
static void exyo_attach(device_t, device_t, void *);

static struct exyo_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_tag_t		sc_a4x_bst;
	bus_space_handle_t	sc_bsh;
	bus_dma_tag_t		sc_dmat;
	bus_dma_tag_t		sc_coherent_dmat;
} exyo_sc;

CFATTACH_DECL_NEW(exyo_io, 0, exyo_match, exyo_attach, NULL, NULL);

/* there can only be one */
static int
exyo_match(device_t parent, cfdata_t cf, void *aux)
{
	if (exyo_sc.sc_dev != NULL)
		return 0;

	return 1;
}


static int
exyo_print(void *aux, const char *pnp)
{
	const struct exyo_attach_args * const exyo = aux;

	if (exyo->exyo_loc.loc_port != EXYOCF_PORT_DEFAULT)
		aprint_normal(" port %d", exyo->exyo_loc.loc_port);

	return QUIET;
}


void
exyo_device_register(device_t self, void *aux)
{
}


void
exyo_device_register_post_config(device_t self, void *aux)
{
}

static int
exyo_find(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	const struct exyo_attach_args * const exyo = aux;
	const struct exyo_locators * const loc = &exyo->exyo_loc;
	const int port = cf->cf_loc[EXYOCF_PORT];

	if (strcmp(cf->cf_name, loc->loc_name)
	    || (port != EXYOCF_PORT_DEFAULT && port != loc->loc_port))
		return 0;

	return config_match(parent, cf, aux);
}


#if !defined(EXYNOS4) && !defined(EXYNOS5)
#error Must define a SoC
#endif
static void
exyo_attach(device_t parent, device_t self, void *aux)
{
	const struct exyo_locators *l = NULL;
	struct exyo_softc * const sc = &exyo_sc;
	prop_dictionary_t dict = device_properties(self);
	size_t nl = 0;

	sc->sc_dev = self;
	sc->sc_bst = &exynos_bs_tag;
	sc->sc_a4x_bst = &exynos_a4x_bs_tag;
	sc->sc_bsh = exynos_core_bsh;
	sc->sc_dmat = &exynos_bus_dma_tag;
	sc->sc_coherent_dmat = &exynos_coherent_bus_dma_tag;

	const uint16_t product_id = EXYNOS_PRODUCT_ID(exynos_soc_id);
	aprint_naive(": Exynos %x\n", product_id);
	aprint_normal(": Exynos %x\n", product_id);

	/* add sysctl nodes */
	exynos_sysctl_cpufreq_init();

	/* add all children */
#if defined(EXYNOS4)
	if (IS_EXYNOS4_P()) {
		l = exynos4_locinfo.locators;
		nl = exynos4_locinfo.nlocators;
	}
#endif
#if defined(EXYNOS5)
	if (IS_EXYNOS5_P()) {
		l = exynos5_locinfo.locators;
		nl = exynos5_locinfo.nlocators;
	}	
#endif 
	KASSERT(l != NULL);
	KASSERT(nl > 0);

	for (const struct exyo_locators *loc = l; loc < l + nl; loc++) {
		char prop_name[31];
		bool skip;

		if (loc->loc_port == EXYOCF_PORT_DEFAULT) {
			snprintf(prop_name, sizeof(prop_name),
			    "no-%s", loc->loc_name);
		} else {
			snprintf(prop_name, sizeof(prop_name),
			    "no-%s-%d", loc->loc_name, loc->loc_port);
		}
		if (prop_dictionary_get_bool(dict, prop_name, &skip) && skip)
			continue;

		struct exyo_attach_args exyo = {
			.exyo_loc = *loc,
			.exyo_core_bst = sc->sc_bst,
			.exyo_core_a4x_bst = sc->sc_a4x_bst,
			.exyo_core_bsh = sc->sc_bsh,
			.exyo_dmat = sc->sc_dmat,
			.exyo_coherent_dmat = sc->sc_coherent_dmat,
		};
		cfdata_t cf = config_search_ia(exyo_find,
		    sc->sc_dev, "exyo", &exyo);
		if (cf == NULL) {
#ifdef EXYO_REQUIRED
			if (loc->loc_flags & EXYO_REQUIRED)
				panic("%s: failed to find %s!", __func__,
				    loc->loc_name);
#endif
			if (loc->loc_port == EXYOCF_PORT_DEFAULT) {
				aprint_verbose_dev(self, "%s not found\n",
				    loc->loc_name);
			} else {
				aprint_verbose_dev(self, "%s%d not found\n",
				    loc->loc_name, loc->loc_port);
			}
			continue;
		}
		config_attach(sc->sc_dev, cf, &exyo, exyo_print);
	}
}
