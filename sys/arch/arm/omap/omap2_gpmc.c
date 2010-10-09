/*	$Id: omap2_gpmc.c,v 1.1.22.4 2010/10/09 03:31:40 yamt Exp $	*/

/* adapted from: */
/*	$NetBSD: omap2_gpmc.c,v 1.1.22.4 2010/10/09 03:31:40 yamt Exp $ */


/*
 * Autoconfiguration support for the Texas Instruments OMAP GPMC bus.
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

#include "opt_omap.h"
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: omap2_gpmc.c,v 1.1.22.4 2010/10/09 03:31:40 yamt Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/reboot.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <arm/cpufunc.h>
#include <arm/mainbus/mainbus.h>
#include <arm/omap/omap_var.h>

#include <arm/omap/omap2_gpmcreg.h>
#include <arm/omap/omap2_gpmcvar.h>

typedef struct {
	boolean_t	cs_valid;
	ulong		cs_addr;
	ulong		cs_size;
} gpmc_csconfig_t;

struct gpmc_softc {
	device_t		sc_dev;
	bus_dma_tag_t		sc_dmac;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	gpmc_csconfig_t		sc_csconfig[GPMC_NCS];
};


static bus_size_t csreg7[GPMC_NCS] = {
	GPMC_CONFIG7_0,
	GPMC_CONFIG7_1,
	GPMC_CONFIG7_2,
	GPMC_CONFIG7_3,
	GPMC_CONFIG7_4,
	GPMC_CONFIG7_5,
	GPMC_CONFIG7_6,
	GPMC_CONFIG7_7,
};


/* prototypes */
static int	gpmc_match(device_t, cfdata_t, void *);
static void	gpmc_attach(device_t, device_t, void *);
static void	gpmc_csconfig_init(struct gpmc_softc *);
static int 	gpmc_search(device_t, cfdata_t, const int *, void *);
static int	gpmc_print(void *, const char *);

/* attach structures */
CFATTACH_DECL_NEW(gpmc, sizeof(struct gpmc_softc),
	gpmc_match, gpmc_attach, NULL, NULL);

static int gpmc_attached;	/* XXX assumes only 1 instance */

static int
gpmc_match(device_t parent, cfdata_t match, void *aux)
{
	struct mainbus_attach_args *mb = aux;

	if (gpmc_attached != 0)
		return 0;

#if defined(OMAP2)
	if (mb->mb_iobase == GPMC_BASE)
		return 1;
#endif

	return 0;
}

static void
gpmc_attach(device_t parent, device_t self, void *aux)
{
	struct gpmc_softc *sc = device_private(self);
	struct mainbus_attach_args *mb = aux;
	bus_space_handle_t ioh;
	uint32_t rev;
	int err;

	sc->sc_dev = self;
	sc->sc_iot = &omap_bs_tag;

	err = bus_space_map(sc->sc_iot, mb->mb_iobase, GPMC_SIZE, 0, &ioh);
	if (err != 0)
		panic("%s: Cannot map registers, error %d",
			device_xname(self), err);

	aprint_normal(": General Purpose Memory Controller");

	rev = bus_space_read_4(sc->sc_iot, ioh, GPMC_REVISION);

	aprint_normal(", rev %d.%d\n",
		GPMC_REVISION_REV_MAJ(rev),
		GPMC_REVISION_REV_MIN(rev));

	sc->sc_ioh = ioh;
	sc->sc_dmac = NULL;

	gpmc_csconfig_init(sc);

	gpmc_attached = 1;

	/*
	 * Attach all our devices
	 */
	config_search_ia(gpmc_search, self, "gpmc", NULL);
}

static void
gpmc_csconfig_init(struct gpmc_softc *sc)
{
	gpmc_csconfig_t *cs;
	uint32_t r;
	int i;

	cs = &sc->sc_csconfig[0];
	for (i=0; i < GPMC_NCS; i++) {
		memset(cs, 0, sizeof(gpmc_csconfig_t));
		r = bus_space_read_4(sc->sc_iot, sc->sc_ioh, csreg7[i]);
		if ((r & GPMC_CONFIG7_CSVALID) != 0) {
			cs->cs_valid = TRUE;
			cs->cs_addr = omap_gpmc_config7_addr(r);
			cs->cs_size = omap_gpmc_config7_size(r);
			aprint_normal("%s: CS#%d valid, "
				"addr 0x%08lx, size %3ldMB\n",
				device_xname(sc->sc_dev), i,
				cs->cs_addr, (cs->cs_size >> 20));
		}
		cs++;
	}
}

static int
gpmc_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct gpmc_softc *sc = device_private(parent);
	struct gpmc_attach_args aa;
	gpmc_csconfig_t *cs;
	int i;

	/* Set up the attach args. */
	if (cf->cf_loc[GPMCCF_NOBYTEACC] == 1) {
		if (cf->cf_loc[GPMCCF_MULT] == 1)
			aa.gpmc_iot = &nobyteacc_bs_tag;
		else
			panic("nobyteacc specified for device with "
				"non-byte multiplier\n");
	} else {
		switch (cf->cf_loc[GPMCCF_MULT]) {
		case 1:
			aa.gpmc_iot = &omap_bs_tag;
			break;
		case 2:
			aa.gpmc_iot = &omap_a2x_bs_tag;
			break;
		case 4:
			aa.gpmc_iot = &omap_a4x_bs_tag;
			break;
		default:
			panic("Unsupported EMIFS multiplier.");
			break;
		}
	}

	aa.gpmc_dmac = sc->sc_dmac;
	aa.gpmc_addr = cf->cf_loc[GPMCCF_ADDR];
	aa.gpmc_size = cf->cf_loc[GPMCCF_SIZE];
	aa.gpmc_intr = cf->cf_loc[GPMCCF_INTR];

	cs = &sc->sc_csconfig[0];
	for (i=0; i < GPMC_NCS; i++) {
		if ((aa.gpmc_addr >= cs->cs_addr)
		&&  (aa.gpmc_addr < (cs->cs_addr + cs->cs_size))) {
			/* XXX
			 * if size was specified, then check it too
			 * otherwise just assume it is OK
			 */
			if ((aa.gpmc_size != GPMCCF_SIZE_DEFAULT)
			&&  ((aa.gpmc_addr + aa.gpmc_size)
				>= (cs->cs_addr + cs->cs_size)))
					continue;	/* NG */
			aa.gpmc_cs = i;
			if (config_match(parent, cf, &aa)) {
				config_attach(parent, cf, &aa, gpmc_print);
				return 0;		/* love it */
			}
		}
		cs++;
	}
	return 1;	/* NG */
}

static int
gpmc_print(void *aux, const char *name)
{
	struct gpmc_attach_args *sa = (struct gpmc_attach_args*)aux;

	if (sa->gpmc_addr != GPMCCF_ADDR_DEFAULT) {
		aprint_normal(" addr 0x%08lx", sa->gpmc_addr);
		if (sa->gpmc_size != GPMCCF_SIZE_DEFAULT)
			aprint_normal("-0x%08lx",
				sa->gpmc_addr + sa->gpmc_size-1);
	}
	if (sa->gpmc_intr != GPMCCF_INTR_DEFAULT)
		aprint_normal(" intr %d", sa->gpmc_intr);

	return UNCONF;
}

uint32_t
gpmc_register_read(struct gpmc_softc *sc, bus_size_t reg)
{
	return bus_space_read_4(sc->sc_iot, sc->sc_ioh, reg);
}

void
gpmc_register_write(struct gpmc_softc *sc, bus_size_t reg, const uint32_t data)
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, reg, data);
}
